# Review `contexts` of milestone 1: `src/modctx.c`, `src/modctx_internal.h`, `src/scaled.c`

BLOCKER FOUND: 1 BLOCKER, 2 MAJOR, 3 MINOR.

Reviewer: `contexts` (adversarial, refute mode). Worktree:
`/home/tobias/Projects/adelefeld/.claude/worktrees/agent-ad240f33abddb0e89`, at master `a19cd71`.
Checks: `docs/reviews/m1/contexts/checks/`. Library built with `make -j2`; sanitizer builds compile `src/*.c`
directly with `-O0 -fsanitize=address,undefined` (or `-fsanitize=thread`). Oracles: Python `fractions` and Python
integers, my own prime test and my own reading of the dump grammar; the project reference
`proto/text_grammar.py` is used only as a second opinion.

## Findings

### R1 (BLOCKER) `adf_scaled_set_context` with `y = x` reports the wrong `lost`

- File: `src/scaled.c:269-277`. Line 270 writes `y->u = u'`; line 276 then reads `x->u` for the loss test
  `u K'/K`. With `y = x` it reads `u'`, not `u`.
- Requirement: `include/adelefeld/scaled.h:104` and `docs/conventions.md:509-510` (closure C5): "write *lost=1
  exactly when the set changes ... y may alias x". `docs/conventions.md` 4.1 item 1 permits the aliasing.
- Input: any `x = s (u + K Zhat)` converted in place. Example: `K = 2`, `x = 1 + 2 Zhat` (`s = 1, u = 1`),
  `ctx` of modulus 1: the result is `Zhat` (the set grew), but `*lost = 0`. The non-aliased call gives 1.
- Behaviour: in an exhaustive run over `K, K'` in 1..24 and every `u` (7200 cases) the aliased call gives the
  wrong flag in 4865 cases: 4579 times `lost = 0` although the set changed (a false claim of an exact
  conversion), 286 times `lost = 1` although it did not. The non-aliased call is right in all 7200. In the random
  run (below) 2328, 2336 and 2332 of 10000 aliased calls per seed are wrong, with moduli up to 4096 bits.
  The value itself stays the best enclosure; only the reported loss is wrong. I grade it BLOCKER because `lost`
  is the documented result by which the conversion "says so" (SPEC 4.4, closure C5); a caller that meets two
  contexts and trusts `lost = 0` believes it kept the set.
- Why the tests did not see it: every aliased call in `tests/test_scaled.c:1337-1342` passes `lost = NULL` and
  checks containment only; no vector row runs the aliased call. The lane report (`lanes/m1-scaled/report.md:44-45`)
  claims "aliasing of every permitted combination".
- Reproducer: `checks/set_context_alias.c`. Output:

      K=2 u=1 K'=1: set changed=1, lost(plain)=1, lost(y=x)=0
      K=2 u=1 K'=3: set changed=1, lost(plain)=1, lost(y=x)=0
      ...
      cases 7200, wrong lost (plain) 0, wrong lost (y = x) 4865
        of these: set changed but lost = 0: 4579; set unchanged but lost = 1: 286

  Also `checks/scaled_ops.c` + `checks/scaled_oracle.py` (record `set_context_alias`).
- Fix direction (not applied): compute `*lost` from `x->u` before `y` is written, or into a local first.

### R2 (MAJOR) `adf_modctx_new_primorial_pow` aborts the process where the header requires `ADF_UNSUPPORTED`

- File: `src/modctx.c:359-377`. For `e` in 2..63 the code first calls `n_prime_pi(n)` (FLINT extends its cached
  prime table up to `n`, `refs/src/flint-3.0.1/ulong_extras.rst:694-701`), then allocates `pi(n)` words, then walks
  every prime up to `n` before it meets the first `p^e >= 2^64`.
- Requirement: `include/adelefeld/modctx.h:66-69`: "ADF_UNSUPPORTED if some p^e >= 2^64";
  `include/adelefeld/modctx.h:16` and `docs/conventions.md:313-314`: on another status "no allocation is
  retained"; `docs/conventions.md:188`: raw constructors return `OK`, `DOMAIN`, `UNSUPPORTED`.
- Input: `n = 2^64 - 1, e = 2` (also `e = 3`; also `n = 2^33` or `2^40, e = 2` under a 4 GB address limit). The
  answer is decided by `n` and `e` alone: the largest prime `p <= n` has `p^e >= 2^64`.
- Behaviour: FLINT prints `Unable to allocate memory (4611686018427387904)` and the process gets `SIGABRT`
  (exit 134) in 0.1 s. For `n = 10^8, e = 3` the call returns `UNSUPPORTED` after 0.3 s, but the process keeps
  131 MB resident after the return (FLINT's thread-local prime cache, filled by the call). For `e = 1` there is no
  admissible answer for a huge `n` (the hole the lane reported, `lanes/m1-modctx-b/report.md:122-127`); for
  `e >= 2` there is one, and the code does not give it.
- Reproducer: `checks/primorial_time.c`, `checks/primorial_time.sh` (60 s timeout, `ulimit -v 4000000`). Output:

      args 100000000 3              exit=0    ... status=UNSUPPORTED ... maxrss=132MB   resident after return: 131MB
      args 8589934592 2             exit=134  ... Unable to allocate memory (4294967296).
      args 1099511627776 2          exit=134  ... Unable to allocate memory (549755813888).
      args 18446744073709551615 2   exit=134  ... Unable to allocate memory (4611686018427387904).
      args 18446744073709551615 3   exit=134  ... Unable to allocate memory (4611686018427387904).
      args 18446744073709551615 1   exit=134  ... Unable to allocate memory (4611686018427387904).

- Fix direction: for `e >= 2` compare `n` with the largest prime whose `e`-th power fits a word, before any
  table; for `e = 1` the specification needs a bound or a status.

### R3 (MAJOR) `adf_modctx_new_from_dump` returns `ADF_PARSE` for valid dumps of every body but `modctx`

- File: `src/modctx.c:713-715` (only the keyword `modctx` is accepted); `src/modctx.c:648-653` says so
  (HEADER-FINDING of the lane).
- Requirement: `include/adelefeld/modctx.h:72-78` and `docs/conventions.md` 10.2: the context "recorded at the
  occurrence-th context occurrence ... of the dump text s"; an index out of range is `ADF_DOMAIN`.
- Input: `adf1 Q fball l 1 6 1 6 0` (one occurrence, `K = 6`, block 6), `adf1 Q fball l 1 6 2 2 3 0 0`,
  `adf1 Q scaled x 1 1 6 1 6`, `adf1 Q scaled s 1 1 0 6 2 2 3`; and `adf1 Q rat 1 1` (no occurrence).
- Behaviour: `ADF_PARSE` for all. Expected: the context `(6; 6)`, `(6; 2, 3)`, ...; `ADF_DOMAIN` for the
  occurrence index 1 of a one-occurrence dump and for index 0 of `rat`. Known to the lane and, per the brief, being
  extended by a running lane; recorded because the header on master promises it.
- Reproducer: `checks/dump_run.c` + `checks/dump_diff.py` (12 inputs of this kind; C differs from the reference in
  all 12).

### R4 (MINOR) Version followed by a non-space byte: `ADF_UNSUPPORTED` instead of `ADF_PARSE`

- File: `src/modctx.c:688-699`. The version digits are read and a version other than `1` returns
  `ADF_UNSUPPORTED` (line 696) before the separator after the version is checked (line 698).
- Requirement: `docs/conventions.md` 10.1 (`dump = "adf" , version , " " , ...`) and 8.5 item 3. The project
  reference reads the version as `adf([0-9]+)( |\Z)` (`proto/text_grammar.py:1023`) and gives `PARSE`; my reading
  agrees. The version `1` path already gives `PARSE` for `adf1x ...` (line 698), so the two paths disagree.
  The golden row `adf2 anything at all -> UNSUPPORTED` (`tests/golden/dump.tsv:22`) is not affected.
- Input: `adf2x Q modctx 1 0`, `adf10x ...`. Behaviour: `ADF_UNSUPPORTED`.
- Reproducer: `checks/dump_diff.py`: 441 of 20086 inputs differ from my reading, all 441 of this form.
  The grammar leaves "version" undefined, so this is an ambiguity to settle, not a proven error.

### R5 (MINOR) The 28 excused mutants of `src/scaled.c` name lines that no longer hold them

- File: `tools/mutate/equivalent.txt:83-110` (commit `ce1fb15`, today). The excuses carry the line numbers of
  `src/scaled.c` before that commit shrank the file by 6 to 8 lines. `mutate.py` matches an excuse by
  `(path, line, kind)` only (`tools/mutate/mutate.py:170`, `:638-656`); the file header says "LINE is the line of
  the file as it stands now".
- Behaviour: 0 of 28 excuses name the line with the quoted code. One now covers a different mutant:
  `src/scaled.c:466:drop_call` ("fmpq_zero(t.s) ... already stores the exact 0") now matches the call
  `fmpq_clear(aq)` at line 466, so a surviving "drop the clear" mutant (a leak) would be excused with an unrelated
  reason. The other 27 excuse nothing, so the next run on `src/scaled.c` reports their mutants as survivors.
  This bears on the lane's claim "254 killed, 0 survived, 28 excused" only for the file before `ce1fb15`.
- Reproducer: `checks/excuse_lines.py`. Output (last line): `scaled.c excuses: 28, naming the right line: 0,
  stale: 28`.

### R6 (MINOR) FLINT citations of `src/modctx.c` do not say what the code claims

- `src/modctx.c:14-15`: "[source pending: refs/src/flint-3.0.1/fmpz.rst ...]". The file is on disk; it documents
  `fmpz_multi_CRT_precompute`/`_precomp` (`fmpz.rst:1350-1372`) and not `fmpz_multi_mod_precomp`.
- `src/modctx_internal.h:10-11` promises `[0, K)` from `adf_modctx_recombine`, which calls
  `fmpz_multi_CRT_precomp(out, ..., 0)` (`src/modctx.c:536`). The documentation says: "Set output to an integer
  of smallest absolute value that is congruent ..." (`fmpz.rst:1364-1365`) and does not define `sign`. The promise
  rests on undocumented behaviour. It holds in every case I ran (4400 recombinations, all in `[0, K)`).
- `src/modctx.c:147-148` cites `fmpz.h:656 and :691` for the `const` program of the precomp calls; those lines
  declare the underscore functions (`:691` takes a writable `tmp`); the functions called are at `:657` and `:692`.
- `src/modctx.c:19-23` says the context holds "the fmpz_comb tables" and that the kernels use
  `fmpz_comb_temp_t`; the code uses no `fmpz_comb` at all (the lane report, lines 26-28, says it was dropped).
- Reproducer: `checks/ground_truth.sh` (prints each cited line next to the claim).

## Examined and found correct

Scaled policy (`src/scaled.c`), oracle `checks/scaled_oracle.py` (exact `Fraction` arithmetic; tight sum
`(c1 + c2, gcd(R1, R2))`, tight product `(c1 c2, gcd(c1 R2, c2 R1, R1 R2))`, containment by integrality in `Z`):
- `checks/scaled_ops.c`, seeds 1, 2, 3 with 10000 rounds each: 154600 records per seed (add, sub, mul, mul_tight
  20000 each; neg, mul_rat, add_rat, get_fball, set_context 10000 each; set_fball 14600 including 4600 local
  inputs). Moduli 1, 2, 3, 4, 6, 12, 30, 36, 60, 64, 210, 360, 1000, 7, 97, 65536, 2^64 - 1, 2^64, 2^64 + 1, 2^128,
  2^200 and 2^4000 * 3^60 (4096 bits; the last five without blocks); scales up to 300-bit numerator and
  denominator; residues 0, K - 1 and random; exact values including 0 and negatives. Found: every result contains
  the tight result (0 failures); sums and `mul_tight` equal the tight result; one exact operand gives the best
  enclosure (Proposition 8); the default product equals `s t (u v + K Zhat)` and loses exactly `h = gcd(u, v, K)`;
  neg and mul_rat are exact as sets; exact inputs keep the tag and the value; `set_fball` and non-aliased
  `set_context` give the best enclosure (Propositions 7, 11) with `lost` exactly when the set changes; results are
  canonical. The only failures are R1.
- Aliasing, checked in C against the non-aliased call on copies in every round: `(x,x,y)`, `(y,x,y)`, `(x,x,x)` of
  add, sub, mul, mul_tight; `y = x` of neg, mul_rat, add_rat (value and `lost`): 0 differences in 3 x 10000 rounds.
- Shared-pointer rule: two contexts of equal modulus at different pointers, output = x, output = y, output = a
  third value: always `ADF_DOMAIN`, output fields bitwise unchanged, 0 failures in 3 x 10000 rounds x 4 ops x 3.
- `adf_scaled_set_fball` with a local input (today's change): 2000 local values `(A0 + K Zhat)/d`, `K = 4*9*25*7`,
  `d <= 50`, converted into 23 target contexts each: 46000 comparisons with the global copy, identical value and
  `lost` in all; 4600 of them also checked by the oracle.
- `adf_scaled_get_fball` into an output that holds a local value: global, canonical, right set, no leak
  (`checks/get_fball_into_local.c`, valgrind clean).
- Sanitizers: `scaled_ops` under ASan+UBSan (3000 rounds) and valgrind memcheck (300 rounds): no error, no leak.
- Citations in `src/scaled.c` and `scaled.h` to `docs/proofs/policies.md` (lines 75, 107, 113, 124, 139, 159, 184,
  193, 199, 200, 206, 218, 241), `precision.md` (9-11, 94, 99, 100, 106) and `fmpq.rst:510-528`: each names the
  statement it claims.

Contexts (`src/modctx.c`, `src/modctx_internal.h`):
- Constructors, `checks/ctor_cases.c` + `checks/ctor_oracle.py` (own Miller-Rabin with 12 bases, Legendre's
  formula, own primorials): 8115 calls, 0 mismatches: `new_blocks` with k = -1, 0, blocks 0, 1, 2, 2^64 - 1,
  (2^64 - 1, 2^64 - 2), non-coprime pairs, NULL; `new_prime_powers` with p = 0, 1, 4, e = 0, 2^63, 2^64,
  e = 2^64 - 1, 3^40, 3^41, the largest 64-bit prime to the powers 1 and 2, repeated primes, DOMAIN before
  UNSUPPORTED in both orders; `new_fmpz` with -5, 0, 1, 2, 2^64 - 1, 2^64, 2^65; `new_factorial` for every
  n in 0..80, 1000, 2^63 - 1, 2^64 - 1 (UNSUPPORTED from 66 on, K = n! checked); `new_primorial_pow` for n in
  0..120 x e in 0..65, (1000, 1), (100000, 1) (9592 blocks), (2^64 - 1, 2^64 - 1), (2^64 - 1, 64). `*out` is the
  sentinel after every failure; `out = NULL` gives `DOMAIN`. ASan+UBSan and valgrind: no error, no leak.
- `adf_modctx_new_from_dump`, `checks/dump_run.c` + `checks/dump_diff.py`: 20086 inputs (valid dumps of random
  contexts with 0..6 blocks up to 2^64 - 1 and K up to 200 bits without blocks; 1 to 3 random mutations each:
  byte changes including NUL, TAB, 0x7f, 0xff, insertions, deletions, truncations, token swaps, trailing tokens,
  version and field variants; `max_items` in {-1, 0, 1, 2, 3, k, k - 1}; occurrence 0, 1, 2, 2^63). Each input is
  run twice: from a heap block of exactly `len` bytes under ASan+UBSan at `-O0`, and at the end of a mapped page
  followed by a `PROT_NONE` page, at `-O0` and at `-O2` without sanitizer. No read past `len`, no fault, no leak,
  the two placements agree, `*out` untouched on every failure, and `adf_modctx_dump_str` of every loaded context is
  the canonical body. Apart from R3 and R4 the status and the context agree with my reading in all inputs.
- The two mutants the lane excused (`||` -> `&&` at `src/modctx.c:698` and `:710`): the code as it stands has no
  such read (above). Applied by hand (`checks/mutants_698_710.sh`), both are killed at `-O0` by the page test
  of `tests/test_modctx.c` (segmentation fault) and survive at `-O2` (all 11 tests pass), as the lane reported.
  On master `tools/mutate/equivalent.txt` has no `src/modctx.c` entry (they were in `501e0d4` and are gone).
- `adf_modctx_reduce`, `adf_modctx_recombine` (`checks/reduce_run.c` + `checks/reduce_oracle.py`): 11 contexts
  (one block 2, 3, 2^64 - 1, 2^64 - 59, 2^63; two blocks 2^64 - 1, 2^64 - 2; five mixed; 64 and 200 primes;
  65!; primorial 1000 to the 5th): 4400 reductions of 0, +-1, +-(K - 1), +-K, +-(K + 1), K^3 + 5, -(K^3 + 7) and
  random up to 3 times the bits of K, both signs, and 4400 recombinations of residues 0, q - 1 and random: all
  right. Threads: 4 threads x 3000 round trips on each of the 11 shared contexts, 0 wrong, no ThreadSanitizer
  report (`setarch -R`). Under helgrind (20 round trips) all 28 reports are on addresses in FLINT's BSS inside
  `_fmpz_new_mpz`/`_fmpz_clear_mpz`; the FLINT-only program `checks/flint_threads_baseline.c` (no adelefeld code)
  gives reports on the same addresses, so none concerns the context.
- The project's own tests on this worktree: `test_scaled` 23 tests, 88410 checks; `test_scaled_vectors` 11 tests,
  62707 checks; `test_modctx` 11 tests, 10197 checks; 0 failed.

## Not examined

- `adf_modctx_new_from_dump` for the nested bodies beyond the 12 inputs of R3 (the lane that extends it owns them).
- The five `adf_fball` conversions declared in `modctx.h` (`set_local`, `set_local_enclose`, `set_global`,
  `is_local`, `context`) beyond their use as inputs above; they are `src/fball_local.c`.
- The cap functions of `scaled.h` (`src/cap.c`), the dump and text functions of scaled values.
- `adf_modctx_matches_desc` and the descriptor functions only by reading (no hostile descriptors run).
- `new_blocks` with very many blocks (cost k^2 gcds, as documented); `new_primorial_pow` with `e = 1` and
  `n` between 10^5 and 10^9 (legitimately huge contexts).
- No full mutation run (time); no fuzzing with libFuzzer (the differential run above is random, not
  coverage-guided).
- Whether the 28 stale excuses of R5 hide a real survivor: the mutation run was not repeated.
