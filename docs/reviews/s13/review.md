# s13-review

## Findings

### BLOCKER: the result verifier accepts a status the function did not return

The claim in `include/adelefeld/resid.h:230` and the row of `docs/api-s.md` section 2 is that
`adf_resid_verify_result` checks what `adf_resid_reconstruct` returned for the given `limit`.
For three inputs with `limit = 0`, reconstruction returns `NOT_DETERMINED`, while the verifier
accepts a different claimed status. The certificates below were built by hand and pass C1 to C4.

| `(m, c, A, B)` | Claimed status | Hand certificate `(R', T', R, T)` | Verifier |
|---|---|---|---|
| `(10, 1, 2, 5)` | `OK`, `q = 1/1` | `(10, 0, 1, 1)` | 1 |
| `(2, 1, 1, 1)` | `NOT_UNIQUE` | `(2, 0, 1, 1)` | 1 |
| `(4, 2, 1, 2)` | `NO_SOLUTION` | `(2, 1, 0, -2)` | 1 |

For the first input, the only reduced solution is `1/1`: `2/2` is not reduced and denominators
3 to 5 give no numerator in `[-2, 2]`. For the second, both `-1/1` and `1/1` solve the problem.
For the third, the only congruent pair in the box is `0/2`, which is not reduced. In every case
`A < m <= 2 A B`, `abs(T) <= B`, and `X > 0`. A limit of zero visits no round, so S-D3 requires
`NOT_DETERMINED`. The verifier instead checks the full solution set for the first three statuses
(`src/resid.c:728-792`). The mathematical cardinality it establishes is true, but the stated
returned-status claim is false. The author's test oracle in `tests/test_resid_rest.c:1840-1850`
also treats those three statuses as cardinality claims regardless of `limit`.

Reproduce with `timeout 120 lanes/s13-review/probe_resid` after the compile command below. It prints
`actual=NOT_DETERMINED` and `accepted=1` for all three claimed statuses.

### MAJOR: a valid residue claim can occupy the verifier for more than two minutes

Input: `m = 2`, `c = 0`, `A = 1`, `B = 5000000000`, `limit = 0`, status claim `OK`, `q = 0/1`,
hand certificate `(2, 0, 0, 1)`. The code did not return within 125 seconds; `timeout` returned
124. The claim that the full solution set is `{0/1}` is true: congruence modulo 2 leaves only
numerator 0 in `[-1, 1]`, and `0/d` is reduced only for `d = 1`. The verifier visits every one
of `B` rounds before accepting (`src/resid.c:790-792`, `src/resid.c:377-414`). With `B = 10000000`
the same call returned 1 in 0.43 seconds. This is a resource finding, not a wrong mathematical
answer; the header's cost paragraph admits the full search.

Reproduce with `timeout 125 /usr/bin/time -f 'elapsed=%e seconds' \
lanes/s13-review/probe_verify_time 5000000000` after the compile command below.

### MAJOR: the ball solver allocates before its size-limit refusal

Input: a `4097` by `0` integer matrix and 4097 finite balls `0 + Zhat`, with `r = 4097`.
`adf_linsolve_fball` returns `LIMIT`, leaves `sol` untouched, and makes 2 FLINT malloc calls and
4 calloc calls during that call. In contrast, `adf_linsolve_mod` on a `4097` by `0` matrix and a
`4097` by `1` right-hand side returns `LIMIT` with zero allocations. S-D8 in
`docs/SPEC.md:893` requires the dimension limit to be decided from sizes before allocation.
The ball path allocates three vectors, computes all canonical triples, builds the transformed
matrices and only then calls the size guard (`src/linsolve.c:914-958`). The same input with its
first ball exact returns `UNSUPPORTED` after 3 calloc calls, contrary to the internal comment
at `src/linsolve.c:900-901` that nothing is built on a status other than `OK`.

Reproduce with `timeout 120 lanes/s13-review/probe_linsolve` after the compile command below.
The probe uses FLINT's allocation hooks around each call and checks that `sol` is untouched.

### MINOR: one branch of the author's residue vector test has no assertion

At `tests/test_resid_rest.c:2048-2060`, the `NOT_UNIQUE` branch allocates a temporary rational
and two integers, writes zero to one integer, then clears them. It never reads `q`, checks an
outcome, or changes later test state. This branch cannot detect a `NOT_UNIQUE` defect. The
surrounding vector test still checks the status and verifier; the finding is confined to this
branch. Reproduce with `timeout 10 python3 lanes/s13-review/check_dead_test.py`: it reports
line 2048, zero `ADF_CHECK` calls and zero `adf_resid_` calls in the 13-line branch.

## Attacks without a further finding

- A: `probe_resid` compared the current strict reconstruction with the source of commit
  `29845cc` for 643440 grid calls and 720 calls with moduli from `2^128 + 13` through
  `2^2048 + 13`. It compared status, `q` including preservation on non-`OK`, and all certificate
  fields. A difference would have stopped the program. The grid covered `m = 1..24`, `c = -2..m+2`,
  `A = -1..m+1`, `B = -1..12`, and limits `-2, 0, 1, 2, 5, 20`. It compared `reconstruct_first`
  status, count and first fraction with a separate enumeration of reduced pairs and their
  coordinates in the certificate basis. All 643440 calls matched. The grid had 70200 `OK`,
  187429 `NOT_UNIQUE`, 174037 `NO_SOLUTION`, and 211774 `NOT_DETERMINED` strict statuses.
- A: The verifier accepted all 643440 library results in that grid. Setting a required
  certificate kind to zero was rejected in 333851 cases. A rejection of a library result or
  acceptance of one of these absent certificates would have stopped the program.
- A: `probe_sets` checked 18000 `set_rat` calls and 279000 memberships by divisibility of
  exact integers. It checked 22448 raw finite or exact ball triples, including negative raw
  denominators and triples that the ball constructor canonicalizes; every kept ball contributed
  11 rational members. It compared 1032 local conversions with global conversions, 48 fractions
  with operands up to about 2048 bits, and 1000 lifecycle cases. It also observed 70 manually
  negative denominator rational structs against their canonical equivalents. These last 70
  are outside the `adf_rat` input precondition and support no contract claim. A wrong status,
  residue, membership, untouched output, or copy/swap/identity result would have stopped it.
- B: `probe_linsolve` used exact integer congruences on 2060 two-equation, two-variable ball
  systems with canonical lcm at most 12. It checked 141672 candidate vectors, 3292 coordinate
  residues, kernel orders, image-certificate copies, set/swap/identity, and 2060 changed-modulus
  false certificates. It observed 299 `OK` and 1761 `NO_SOLUTION` systems. Any disagreement with
  the enumerated solution set or projected coordinate sets would have stopped it.
- The four points in `lanes/s3-slice3/report.md` were assessed. The missing list of two pairs
  underlies finding 1. Returning 0 when `X` exceeds a word is expressly allowed by the header.
  The `limit <= 0` example in note 5 of `docs/api-s.md` names the strict function's status; the
  `reconstruct_first` table gives its separate result. For `A >= m`, the header explicitly names
  `c/1` as the first result. Neither last point yielded a code finding.

## Checks run

- `timeout 120 make -j2`: exit 0; 17 objects and `build/libadelefeld.a` built.
- `timeout 10 python3 lanes/s13-review/extract_baseline.py lanes/s13-review/resid_29845cc.c`:
  exit 0; loose commit tree `336178aeb8e619756c24a5c7b9fe3447df4605df`, residue blob
  `198cbdf7f1195a120c4d4db74a820386f0313d1c`, 11553 bytes. No Git command was run.
- `timeout 10 python3 lanes/s13-review/find_empty.py`: exit 0; first case `4 2 1 2`.
- `cc -std=c11 -O2 -Wall -Wextra -Werror -Iinclude lanes/s13-review/probe_resid.c \
  lanes/s13-review/baseline.c build/libadelefeld.a -lflint -lgmp -lm \
  -o lanes/s13-review/probe_resid`: exit 0. `timeout 120 lanes/s13-review/probe_resid`:
  exit 0; final counts are above, with three false returned-status claims accepted. Earlier
  revisions ran twice with 643440 grid calls and zero grid disagreements.
- `cc -std=c11 -O2 -Wall -Wextra -Werror -Iinclude lanes/s13-review/probe_sets.c \
  build/libadelefeld.a -lflint -lgmp -lm -o lanes/s13-review/probe_sets`: exit 0.
  `timeout 120 lanes/s13-review/probe_sets`: exit 0; 18000 setters, 279000 memberships,
  22448 forget calls, 1032 local cases, 48 large cases, 1000 lifecycle cases, 70 raw negative
  denominators. Two earlier revisions also exited 0 with their then smaller case sets.
- `cc -std=c11 -O2 -Wall -Wextra -Werror -Iinclude lanes/s13-review/probe_linsolve.c \
  build/libadelefeld.a -lflint -lgmp -lm -o lanes/s13-review/probe_linsolve`: exit 0.
  `timeout 120 lanes/s13-review/probe_linsolve`: exit 0; 2060 systems, 141672 points,
  3292 coordinate checks, 2060 false certificates refused. An earlier revision also exited 0.
- `cc -std=c11 -O2 -Wall -Wextra -Werror -Iinclude lanes/s13-review/probe_verify_time.c \
  build/libadelefeld.a -lflint -lgmp -lm -o lanes/s13-review/probe_verify_time`: exit 0.
  `timeout 30 /usr/bin/time -f 'elapsed=%e seconds' lanes/s13-review/probe_verify_time 10000000`:
  exit 0, accepted 1, elapsed 0.43 seconds. The same command with `timeout 125` and
  `5000000000`: exit 124 after 125 seconds, with no result.
- `timeout 10 python3 lanes/s13-review/check_dead_test.py`: exit 0; line 2048,
  zero checks, zero residue calls, 13 lines.
- `cc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
  -fno-omit-frame-pointer -Iinclude lanes/s13-review/probe_sets.c src/resid.c \
  build/libadelefeld.a -lflint -lgmp -lm -o lanes/s13-review/probe_sets_san`: exit 0.
  `timeout 120 env ASAN_OPTIONS=detect_leaks=0 lanes/s13-review/probe_sets_san`: exit 0,
  same final counts, zero sanitizer diagnostics.
- The analogous sanitizer compile with `probe_linsolve.c`, `src/linsolve.c`, and output
  `lanes/s13-review/probe_linsolve_san`: exit 0. Its `timeout 120 env ASAN_OPTIONS=detect_leaks=0`
  run exited 0 with the 2060-system counts and zero sanitizer diagnostics.
- `timeout 120 env ASAN_OPTIONS=detect_leaks=1 lanes/s13-review/probe_sets_san`: exit 1;
  LeakSanitizer stopped with a ptrace incompatibility, so it supplied no leak result.
- `wc -L lanes/s13-review/*.c lanes/s13-review/*.py`: maximum 111 characters.

## Files written

`extract_baseline.py`, `resid_29845cc.c`, `baseline.c`, `probe_resid.c`, `probe_sets.c`,
`probe_linsolve.c`, `probe_verify_time.c`, `find_empty.py`, `check_dead_test.py`, six generated
binaries (`probe_resid`, `probe_sets`, `probe_linsolve`, `probe_verify_time`, and two sanitizer
binaries), and this report, all under `lanes/s13-review/`.

## Not done

No leak result was available. No full run of the 5-billion-round verifier was made; the
125-second timeout establishes the delay. No exhaustive enumeration was attempted for large
linear systems or large ball moduli. Manually noncanonical `adf_rat` input is outside the input
contract; its 70 negative-denominator observations were not treated as findings.

## Sources pending

None. The numerical claims above follow from direct integer enumeration or the shown inputs;
no external formula or convention is quoted.

## Findings against the specification

None against the mathematical statements in `docs/SPEC.md`. Finding 1 is a mismatch between
the API's returned-status sentence and the implementation's cardinality check. S-D3 itself
predicts `NOT_DETERMINED` for all three inputs. Finding 3 is an implementation violation of
the allocation order required by S-D8.
