# S.1 slice 1 adversarial review

## Findings

No BLOCKER, MAJOR or MINOR finding was established.

## Work and files written

- `probe.c` exposes the solver's status, two predicates and five matrices to an independent Python oracle.
- `oracle.py` enumerates `(Z/N)^c`, the solution set, the kernel and the span of `G`. It tests (E1) to (E4)
  directly against the enumerated kernel and checks `A V_i = E_i`.
- `big_cases.py` checks one-equation systems with moduli of about 1,000 bits and signed entries up to 2,048
  bits. For this oracle, put `d = gcd(a, N)`. Bezout gives `u a + v N = d`. Thus `a x = b` is solvable
  exactly when `d` divides `b`, and its kernel consists of multiples of `N/d` modulo `N`.
- `verify_probe.c`, `hand_cases.py` and `verify_sweep.py` build certificate fields directly and test the public
  verifier. They do not use the solver to construct those fields.
- `alias_probe.c` checks reuse of `sol`, `A = b`, `N = sol->N`, and output preservation on DOMAIN and LIMIT.
- The executables `probe`, `probe_asan`, `verify_probe` and `alias_probe` were built in this directory.

## Checks run

- `make -j2`: exit 0; 16 library objects and `build/libadelefeld.a` built.
- The following three commands each exited 0:

  ```sh
  cc -Iinclude -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror \
    lanes/s1-review/probe.c build/libadelefeld.a -lflint -lgmp -lm -o lanes/s1-review/probe
  cc -Iinclude -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror \
    lanes/s1-review/verify_probe.c build/libadelefeld.a -lflint -lgmp -lm \
    -o lanes/s1-review/verify_probe
  cc -Iinclude -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror \
    lanes/s1-review/alias_probe.c build/libadelefeld.a -lflint -lgmp -lm \
    -o lanes/s1-review/alias_probe
  ```

- `python3 lanes/s1-review/oracle.py --cases 1000`: 1,000 cases; OK 555, NO_SOLUTION 445;
  0 disagreements.
- `python3 lanes/s1-review/oracle.py --cases 12000 --seed 371`: run four times during development;
  each returned 12,000 cases, OK 6,871, NO_SOLUTION 5,129, 0 disagreements.
- `python3 lanes/s1-review/hand_cases.py`: 8 certificates; 2 true accepted, 5 false refused,
  1 noncanonical refused.
- `python3 lanes/s1-review/big_cases.py`: 150 cases; OK 95, NO_SOLUTION 55; 0 disagreements.
- `lanes/s1-review/alias_probe`: 5 checks, 0 failures.
- The sanitizer build command exited 0:

  ```sh
  cc -Iinclude -std=c11 -O1 -g -Wall -Wextra -Wpedantic -Werror \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    lanes/s1-review/probe.c src/linsolve.c build/libadelefeld.a \
    -lflint -lgmp -lm -o lanes/s1-review/probe_asan
  ```

- `ASAN_OPTIONS=detect_leaks=1 python3 lanes/s1-review/oracle.py --cases 12000 --seed 371`
  `--probe lanes/s1-review/probe_asan`: exit 1 after the cases; LeakSanitizer failed under ptrace.
  It supplied no leak result.
- `ASAN_OPTIONS=detect_leaks=0 python3 lanes/s1-review/oracle.py --cases 12000 --seed 371`
  `--probe lanes/s1-review/probe_asan`: 12,000 cases; OK 6,871, NO_SOLUTION 5,129;
  0 disagreements and 0 ASan or UBSan reports.
- `python3 lanes/s1-review/verify_sweep.py`: 84,038 canonical certificates; 1,713 accepted,
  82,325 refused, 0 false accepted.
- `make -j2 build/test_linsolve`: exit 0.
- `./build/test_linsolve`: 12 tests, 14,415 checks; 0 failed checks and 0 failed tests.

The independent 12,000-case run included 924 cases with `N = 1`, 4,563 with `r > c`, 4,500 with `r < c`,
3,006 with `r = 0`, 2,930 with `c = 0`, and 725 with both zero. It included 2,303 matrices whose entries
were all nonunits, 600 whose entries were all nonzero zero divisors, 1,489 kernels whose cardinality is not
a power of `N`, 1,091 planted solutions, 4,861 matrices with a negative entry, and 281 one-variable cases
where the pivot did not divide `b`. A case failed if the status, coset, complete kernel, Howell conditions,
certificate equations, or returned predicates disagreed with enumeration.

The 150 large cases failed if the status disagreed with the gcd condition, `G` disagreed with the one-row
kernel form, `x0` failed the equation, or `y` failed to separate `b`. The 84,038 verifier cases varied `A`,
`b`, kind, `E`, `V`, `G`, `x0` and `y` by hand over `N = 4, 6, 8, 9`. An accepted false solution set,
incomplete kernel or invalid separator would have failed the sweep. The 5 alias checks failed on a changed
status, false certificate, wrong particular solution or output changed on DOMAIN or LIMIT.

The 12 author tests were inspected for assertions that could not fail; none was found. The review also
inspected the guard order in `adf_linsolve_mod` and `adf_linsol_verify`; no later matrix read before an
earlier failed guard, and no allocation before the solver's DOMAIN and LIMIT checks, was found.

## Algorithm H third-case pending pair

The pair `((N/g) w', j + 1)` in `docs/proofs/solvers.md` lines 635 to 637 is redundant under its stated
invariant (I3), lines 658 to 662. Let `U` be the span of the later rows and pending vectors before the
third case. Then `(N/h) w` is in `U`. After the replacement, `v'` is also a later vector. The identities

    (N/g) v = (N/h) v' + (a/g) (N/h) w
    (N/g) w = (h/g) (N/h) w

show that both `(N/g) v` and `(N/g) w` lie in `U + <v'>`. Since `w' = s v + t w`, so does `(N/g) w'`.
Thus the extra pair is a simplification opportunity, not an error in the output or specification.

## Not done

- No exhaustive enumeration for `c > 3` or large `N`; the large-modulus check uses the one-equation proof.
- Leak detection did not run: LeakSanitizer stopped under ptrace. Address and undefined behavior checks ran.
- No external source attribution was checked. `refs/src` contained 0 files during this review.

## Sources pending

- `[source pending: local Storjohann dissertation under refs/src/storjohann-thesis]` to audit the external
  attribution of Definition 2.1. The mathematical checks and the third-case proof above do not use it.

## Findings against the specification

None. The third-case pair is redundant but does not make a stated result false.
