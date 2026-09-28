# Lane m1-testgaps: report

Work tree: `/home/tobias/Projects/adelefeld-wt/m1-testgaps`. All times are UTC.

## 1. What was done

1. `tests/test_fball_overwrite.c` (new, 400 lines, 11 tests): every constructor and setter of
   `include/adelefeld/fball.h` is called on a value that already holds other data, and afterwards every
   field of the result is compared with the expected one and `adf_fball_is_canonical` is required to hold.
2. `tests/test_adele_prec.c` (new, 837 lines, 17 tests): the precision argument of `adele.h`, the finite
   half of the two swaps, and the status of the projection to the real place.
3. Red-green for both files: 38 mutations applied by hand in a scratch copy of the tree under `build/`,
   log in `lanes/m1-testgaps/redgreen.log`.
4. `tools/mutate/equivalent.txt`: 34 lines appended (the 24 of the report of lane tools-mutate, plus 10
   more that the complete run of 347 mutants of `src/fball.c` showed; the brief's list came from a run
   limited to 200 mutants).
5. The five mutation runs of the brief, one after the other, and three more of `src/recon.c`.
6. `make -j2 check`, `make clean && make -j2 check SAN=1` and `make clean && make -j2 check CC=clang`.

## 2. Files written

| file | what |
|---|---|
| `tests/test_fball_overwrite.c` | new test, 11 tests, 44 checks |
| `tests/test_adele_prec.c` | new test, 17 tests, 148 checks |
| `tools/mutate/equivalent.txt` | 34 lines appended; 43 entries in all, 9 of them from before this lane |
| `lanes/m1-testgaps/redgreen.sh` | applies one mutation in a scratch copy and runs one test |
| `lanes/m1-testgaps/run_redgreen.sh` | the 38 red-green runs |
| `lanes/m1-testgaps/redgreen.log` | their output |
| `lanes/m1-testgaps/run_mutate.sh` | the mutation runs of the brief, one after the other |
| `lanes/m1-testgaps/mutate-<file>.log` and `mutate-recon-1.log` to `-3.log` | the whole output of each run |
| `lanes/m1-testgaps/mutate-runs.txt` | the order, the wall-clock times and the summary line of each run |
| `lanes/m1-testgaps/report.md` | this file |

No file outside those paths was changed. `src/` and the existing tests were not touched.

## 3. The two test files

### 3.1 `tests/test_fball_overwrite.c`

The claim: a constructor or setter of `fball.h` writes every field of its output (A, H, d, backend,
mctx, res) and leaves a value that satisfies predicate G of `docs/conventions.md` 5.2.

"Already holds other data" is the ball `(2^200 + 2^210 Zhat)/3^40` (A large, H > 0, d > 1, already
canonical, since 2^200 < 2^210 and gcd(2^200, 2^210, 3^40) = 1). Every expected result of the file
differs from that ball in all three integers, so a statement that is not executed leaves the old value
in the field and the comparison fails. The failure message prints the whole state of the value.

Called on such a value: `adf_fball_zero`, `adf_fball_one`, `adf_fball_set_si`, `adf_fball_set_fmpz`,
`adf_fball_set_rat`, `adf_fball_set_fmpz3`, `adf_fball_set_center_radius`, `adf_fball_set`,
`adf_fball_canonicalise` (on a raw non-canonical triple written over the loaded ball), and
`adf_fball_swap` (the other way round, as the last test of the file). `adf_fball_init` is excepted by
the brief: it has no earlier value. `adf_fball_clear` is not a constructor and is not tested here.

### 3.2 `tests/test_adele_prec.c`

The claim, for each of `adf_adele_add`, `adf_adele_sub`, `adf_adele_mul`, `adf_adele_add_rat`,
`adf_adele_mul_rat`, `adf_adele_div_rat`, `adf_adele_set_rat` and the seven `adf_cadele` counterparts:
with operands that are exact at 200 bits and whose exact result is not representable at 2 bits,

* the result at `prec = 200` has a radius below 2^-150 times the midpoint, and
* the result at `prec = 2` still contains the exact value.

The operands are the real parts `X = 1 + 2^-100` (101 significant bits) and `Y = 2 + 2^-90` (92), each
with a nonzero imaginary part `1/4` and `-1/2` in the complex case, and finite parts `1/3` and
`(5 + 18 Zhat)/1`. The exact values of the results are computed in the test with `fmpq_add`, `fmpq_sub`,
`fmpq_mul` and `fmpq_div` on the same exact rationals. The relative radius is decided with
`arf_set_mag(r, arb_radref(x)) * 2^150 < |arb_midref(x)|`; the containment with `arb_contains_fmpq` on the
exact rational. For a complex result the exact value `er + ei i` lies in the ball exactly when `er` lies
in the real interval and `ei` in the imaginary interval, and both are decided that way.

`adf_adele_set_rat` and `adf_cadele_set_rat` take the exact rational `3/7`, which no finite precision
represents: the radius at 200 bits is below 2^-150 times the midpoint, the ball at 2 bits still contains
`3/7`, and the imaginary part of the complex result is the exact 0.

The two swaps: two values with equal real (complex) parts and different finite parts; after the swap
each finite half is the one the other value held (`adf_fball_identical` against a copy taken before),
and the real parts are still equal. The status: `adf_adele_get_arb_at(r, x, adf_place_inf())` returns
`ADF_OK` and writes the real part.

## 4. Red-green (`lanes/m1-testgaps/redgreen.log`)

`lanes/m1-testgaps/redgreen.sh` copies `Makefile`, `include/`, `src/`, `tests/support/` and the one test
file into `build/redgreen/`, applies one mutation there, builds that one test program with `make -j2` and
runs it from the repository root. `src/` is never written to. 38 runs, of which

* 33 are red (the test fails, as it must),
* 5 do not build,
* 0 survive.

| file:line | mutation | result |
|---|---|---|
| fball.c 259, 260, 261 | `drop_call` in `adf_fball_zero` (A, H, d) | red |
| fball.c 271, 272, 273 | `drop_call` in `adf_fball_one` (A, H, d) | red |
| fball.c 283 | `drop_call` of `fmpz_set_si(x->A, n)` | not compiled: `n` unused, -Werror |
| fball.c 284, 285 | `drop_call` in `adf_fball_set_si` (H, d) | red |
| fball.c 295 | `drop_call` of `fmpz_set(x->A, n)` | not compiled: `n` unused |
| fball.c 296, 297 | `drop_call` in `adf_fball_set_fmpz` (H, d) | red |
| fball.c 307, 308, 309 | `drop_call` in `adf_fball_set_rat` (A, H, d) | red |
| fball.c 170, 171, 172 | `drop_call` in `adf_fball_set` (A, H, d) | red |
| fball.c 77, 78, 79 | `drop_call` in `fb_store` (A, H, d) | not compiled: the parameter is unused |
| adele.c 256, 259 | `prec` -> 2 in `adf_adele_add_rat` | red |
| adele.c 283, 285 | `prec` -> 2 in `adf_adele_mul_rat` | red |
| adele.c 318, 319 | `prec` -> 2 in `adf_adele_div_rat` | red |
| adele.c 489, 492 | `prec` -> 2 in `adf_cadele_add_rat` | red |
| adele.c 512, 514 | `prec` -> 2 in `adf_cadele_mul_rat` | red |
| adele.c 543, 544 | `prec` -> 2 in `adf_cadele_div_rat` | red |
| adele.c 123, 392 | `prec` -> 2 in `adf_adele_set_rat`, `adf_cadele_set_rat` | red |
| adele.c 82, 360 | `drop_call` of `adf_fball_swap` in the two swaps | red |
| adele.c 179 | `status`: `ADF_OK` -> `ADF_DOMAIN` in `adf_adele_get_arb_at` | red |

The five that do not build are mutants that the tool also reports as "not compiled" (the deleted line was
the only use of a parameter, and the Makefile builds with `-Werror`); they were never survivors.

Every red run in the log names the test and the check that failed, for example
`adf_adele_add_rat at prec 200: the radius is not below 2^-150 times the midpoint (-2 bits of relative
error)`.

## 5. Equivalent mutants

`python3 tools/mutate/mutate.py --root . --files src/rat.c src/cap.c src/recon.c src/adele.c
src/fball.c --limit 100000 --list` was used to check that every line number of the report still names
the mutant it names, and that every key appended below is a mutant the tool generates. All 24 of the
report do; so do the 10 further ones.

The aliasing question the brief asks about: in all 24 the exchange is between arguments 1 and 2 of a
call, and argument 0 (the output) is never moved, so the exchange does not change which arguments the
output aliases. The aliasing of the output with an input is what the code relies on and what our own
rules give:

* `fball.h:23` (conventions 4.1, CV-05): an output of `adf_fball_add`, `adf_fball_mul` may be either input;
* `fmpq.rst:410`: "Aliasing between any combination of the variables is allowed" for `fmpq_add`,
  `fmpq_mul`, `fmpq_div`;
* `fmpz.rst:52-54`: "Unless otherwise specified, all functions in this section permit aliasing between
  their input arguments and between their input and output arguments" (`fmpz_gcd` is in that section,
  `fmpz.rst:1040`);
* `fmpq.rst:28-33` on the non-underscore `fmpq` functions; `fmpq_gcd` is `fmpq.rst:519`.

The commutativity of each operation is either from FLINT's definition (a sum, a product and a gcd of two
numbers do not depend on their order) or from our own proofs: `docs/proofs/precision.md` Proposition 1
(line 27) for the set sum and Proposition 2 (line 34) for the tight set product, whose radius
`gcd(a M, b N, N M)` is symmetric in `(a, N)` and `(b, M)`.

The 10 lines added after the brief's list, all of `src/fball.c`, from the complete run of 347 mutants:

* 141, 142: `drop_call` of the two `fmpz_zero` in `adf_fball_init`. `fmpz_init` sets the value to zero
  (`refs/src/flint-3.0.1/fmpz.rst:181-184`), and the three `fmpz_init` calls of lines 138 to 140 are
  the only writes of A and H before these lines, so the two `fmpz_zero` change nothing for every input.
* 468, 469, 497, 551, 553, 554, 582, 610: `swap_args` of two input arguments of `fmpq_add`, `fmpq_mul`
  or `fmpq_gcd`, in every case with the output in a temporary of its own.

Note for the reader of the log: the tool prints nothing for an excused mutant, so a key that is excused
appears only in the count at the end of the run. The five keys of `fball.c` that were in
`equivalent.txt` before the run of 347 mutants are inside the "8 excused" of that run, not among its 21
survivors; this was checked by applying the five swaps by hand to a scratch copy and running
`make -s -j2 check` there: all five pass, as they must.

## 6. The mutation runs

Command, one after the other, never two at once (`lanes/m1-testgaps/run_mutate.sh`, times and summaries
in `lanes/m1-testgaps/mutate-runs.txt`):

    make mutate FILES=src/<f>.c LIMIT=400 JOBS=2

| file | mutants | killed | survived | not compiled | timed out | excused | wall clock | exit |
|---|---|---|---|---|---|---|---|---|
| rat.c | 36 | 20 | 0 | 14 | 0 | 2 | 134.0 s | 0 |
| cap.c | 48 | 39 | 0 | 6 | 0 | 3 | 285.5 s | 0 |
| recon.c | 63 | 48 | 3 | 7 | 0 | 5 | 357.5 s | 2 |
| adele.c | 144 | 105 | 0 | 25 | 0 | 14 | 693.2 s | 0 |
| fball.c | 347 | 280 | 21 | 38 | 0 | 8 | 1361.3 s | 2 |

The run of `src/adele.c` is the one the brief asks about: before this lane it was 90 killed and 29
survived, now 105 killed and 0 survived. The 15 mutants that changed are the 12 `prec` mutants of lines
256, 259, 283, 285, 318, 319, 489, 492, 512, 514, 543, 544, the two `drop_call` of `adf_fball_swap` at
lines 82 and 360, and the `status` mutant of line 179.

The run of `src/fball.c` was 1361.3 s, under the 25 minutes of the brief. Its 21 survivors are the 5
`drop_call` of `flint_free(x->res)` that the brief names, 3 more of the same kind, the 2 `fmpz_zero` of
`adf_fball_init` and 8 commutative swaps (all 10 of these are now in `equivalent.txt`), 2 `status`
mutants and 1 `drop_call` in `adf_fball_contains`; they are in section 8.

The run of `src/fball.c` was repeated after the last ten excuses were appended, to have the counts of
the file as it stands. It was stopped by this lane after 33 minutes without a result: two other lanes
were running mutation runs of their own on the same machine (load average 14) and the first run had
already taken 22.7 minutes. The counts of section 6 for `fball.c` are therefore those of the first run,
with 8 excuses; with the ten later lines they would read 280 killed, 11 survived, 38 not compiled,
18 excused. Nothing else depends on that number.

### 6.1 The three runs of `src/recon.c`

Run 1: 63 mutants, 48 killed, 3 survived, 7 not compiled, 5 excused, 357.5 s.
Run 2: 63 mutants, 48 killed, 3 survived, 7 not compiled, 5 excused, 253.3 s.
Run 3: 63 mutants, 48 killed, 3 survived, 7 not compiled, 5 excused, 335.4 s.

The three verdict lists are identical, and so is the whole output: `diff` of the three log files gives
only the two timing lines of each (the baseline time and the total time). The three survivors are the
same three mutants in the same three lines. The hypothesis of the orchestrator, that the different runs
of lane m1-recon came from uninitialised `fmpq` in `tests/test_recon.c`, does not hold on this tree: the
five runs (the one in the table above and the three repeats) agree line for line. The seed is fixed
(`SEED = 20260928`), and `LIMIT = 400` is above the number of mutants, so every run draws the same
sample; the order of the printed lines follows the completion order of the two workers and did not
differ either.

## 7. Checks run, with the result

| command | result |
|---|---|
| `make -j2 check` | `check passed: all 24 test programs` (22 before this lane), 11.4 s |
| `make clean && make -j2 check SAN=1` | `check passed: all 24 test programs`, 28.4 s, no ASAN or UBSAN report |
| `make clean && make -j2 check CC=clang` | `check passed: all 24 test programs`, 7.3 s |
| `./build/test_fball_overwrite` | 11 tests, 44 checks, 0 failed |
| `./build/test_adele_prec` | 17 tests, 148 checks, 0 failed |
| the 38 red-green runs of section 4 | 33 red, 5 not compiled, 0 survived |
| the five mutation runs and the three repeats | section 6 |

Two checks are too long for the table above and are written out here.

* `python3 tools/mutate/mutate.py --root . --files src/rat.c src/cap.c src/recon.c src/adele.c
  src/fball.c --limit 100000 --list`: 36, 48, 63, 144 and 347 mutants. Every key of
  `tools/mutate/equivalent.txt` is one of them, except the pre-existing `src/common.c:96`, whose file is
  not in the list.
* The five `swap_args` of `src/fball.c` of lines 54, 214, 552, 555, 684, each applied by hand in a
  scratch copy of the tree, then `make -s -j2 check` there: all five pass with the mutant, as they must
  (section 5).

## 8. Survivors that remain, and the test each one needs

### 8.1 `src/fball.c`: the drops of `flint_free(x->res)` (8)

Lines 75 (in `fb_store`), 155 (`adf_fball_clear`), 168 (`adf_fball_set`), 264 (`adf_fball_zero`), 276
(`adf_fball_one`), 288 (`adf_fball_set_si`), 300 (`adf_fball_set_fmpz`), 312 (`adf_fball_set_rat`).
The five that the brief names are 155, 264, 276, 300, 312; the other three are the same statement in
`fb_store` and `adf_fball_set`, and line 288.

Open until work package 1.8, as the brief says. A drop of `flint_free(x->res)` leaks the residue array of
a local value, and a value with a residue array can only be built by the local backend, which does not
exist in this tree. `tests/test_fball.c` builds local-shaped values by hand for the structural parts of
predicate L; the brief forbids it here and this file does not do it. The test that is missing is a test
of work package 1.8: build a local value, call each of these eight functions on it, and run under
`SAN=1` (the leak is only visible to a leak checker; `make mutate` does not use one). Until then these
eight are excused in no way and are counted as survivors.

### 8.2 `src/fball.c`: the two `status` mutants of the weak place references (2)

Lines 420 and 422 of `adf_fball_prec_at`:

    if (adf_place_is_archimedean == NULL) return ADF_UNSUPPORTED;
    if (adf_place_prime_get == NULL) return ADF_UNSUPPORTED;

`status: 'ADF_UNSUPPORTED' -> 'ADF_OK'`. The two branches are unreachable in every build the Makefile
makes, because `src/place.c` is always compiled and linked and defines both functions; the check exists
only for a worktree in which the place functions are absent (the comment at lines 29 to 31 of `fball.c`
says so). No test of the public interface can reach them, so this is not a missing test. The owner of
`src/fball.c` should either delete the two guards, or list the two mutants in `equivalent.txt` with the
reason "the branch is not reachable in a build that links `src/place.c`". This lane did not list them:
the reason is a statement about the link, not about the two programs computing the same thing for every
input, and the choice is the owner's.

### 8.3 `src/fball.c`: the `drop_call` of line 734 (1)

Line 734 is the second `fmpq_sub(diff, a, b);` in `adf_fball_contains`, in the branch `N > 0 and M > 0`.
The first one is line 726, before the branch; between them only `fmpq_div(t, N, M)` writes, and `t` and
`diff` are two different `fmpq_t`, so the value of `diff` at line 734 is the one line 726 put there.
The mutant is therefore equivalent for every input. The owner may excuse it or delete the redundant
statement. I did not add it to `equivalent.txt`: the brief gave item 4 the 24 swaps of the report, and
`src/fball.c` is not mine. (A test with `N/M` an integer and `(a - b)/M` not an integer, for instance
`(0 + 2 Zhat)/1` against `(1 + 2 Zhat)/4`, was written and applied against the mutant in a scratch copy:
it passes with the mutant, which is the expected outcome for a redundant statement.)

### 8.4 `src/recon.c`: the three survivors (3)

Lines and mutants, the same three in all four runs:

* **97, `drop_call` of `fmpz_one(fmpq_denref(q));`** in `fmpq_set_dyadic`, the branch `exp >= 0`. The
  only two calls of that static function are lines 217 and 218, on `lo->q` and `hi->q`, both of which
  `adf_rat_init` has just set to 0/1 (`rat.h`, "adf_rat_init(x): x = 0, stored 0/1"). For every input of
  `adf_adele_reconstruct` the denominator is therefore already 1 and the statement changes nothing. The
  missing thing is a written precondition of the static helper, not a test; the owner of `src/recon.c`
  should either state it or drop the line.
* **175, `drop_call` of `fmpz_one(fmpq_denref(c->q));`** in `adf_fball_reconstruct`. `c` is
  `adf_rat_init`ed at the top of the function and is not written on the path that reaches line 175
  (the `N = 0` branch, which writes `c`, is the other branch of the same `if`), so the same argument
  applies.
* **111, `drop_call` of `fmpq_canonicalise(q);`** in `fmpq_set_dyadic`. This one is not a no-op: the
  normalising of `arb_get_interval_fmpz_2exp` removes only the *common* trailing zeros of `a` and `b`
  (`refs/src/flint-3.0.1/arb.rst:461-466`), so `a` alone may be even; for the ball `[5/16 +- 1/16]` the
  interval is `[1/4, 3/8] = [2, 3] * 2^-3`, which was measured here, so `lo` is `2/8` and needs
  `fmpq_canonicalise`. Every use of `lo` and `hi` further down is invariant under a common factor of
  numerator and denominator (`fmpq_cmp`, and `fmpz_cdiv_q` / `fmpz_fdiv_q`, which are homogeneous), so
  no output of the public function changes. That was also measured: a scratch program ran 10584 balls
  (6 finite parts, 6 x 6 midpoints, 6 x 6 exponents and radii) through `adf_adele_reconstruct` and
  printed a hash of every status and every returned rational; the hash is the same with the statement
  removed (`cases=10584 hash=3374918993701445113` in both). The owner should either excuse the mutant
  with that reason, keeping in mind that FLINT documents `fmpq_add` and the rest as assuming canonical
  inputs (`fmpq.rst:400-410`), or make the invariant explicit in a comment, or keep a test that pins the
  canonical form of the bounds. This lane did none of the three: `src/recon.c` and `tests/test_recon.c`
  are not mine.

## 9. Not done

* The five (in fact eight) `flint_free(x->res)` survivors, work package 1.8, see 8.1.
* The three survivors of `src/recon.c` and the three of `src/fball.c` of sections 8.2 to 8.4: the
  reasons are given, the decision and the code are the owners'.
* The complete run of `src/fball.c` with the ten late excuses, section 6.
* Nothing of the finite coordinate of the two new tests: the finite half is not what
  `test_adele_prec.c` is about, and `tests/test_adele.c` and `tests/test_cadele.c` cover it.
* `make check CC=clang` was run for the whole suite once (7.3 s, 24 programs), not after every change.

## 10. Sources pending

* `[source pending: a statement of FLINT 3.0.1, in refs/src/flint-3.0.1/arb.rst or acb.rst, that the
  output of arb_add, arb_mul, acb_add or acb_mul may be one of the inputs.]` The comment block of
  `src/adele.c` asserts it and cites `arb.rst:767-805`; that range documents the functions and their
  rounding, not the aliasing. The excuse of the seven real and complex swaps does not rest on it (the
  output is argument 0 and is not moved by the exchange), but the statement in `src/adele.c` and in
  `include/adelefeld/adele.h` ("Aliasing (conventions 4.1)") is ours, not FLINT's, and should be
  marked the same way as the `[unverified: ...]` of the same header.
* `[source pending: the FLINT 3.0.1 source of arb_add, arb_mul, acb_add and acb_mul, to read whether
  the ball written is bit-identical when the two operands are exchanged.]` The excuses use the
  documented meaning, "z = x + y, rounded to prec bits" (`arb.rst:767-775`, `acb.rst:429-431`), which
  makes the value a function of the exact sum and of prec alone.

## 11. Findings against the specification

None. `docs/SPEC.md` 4.1 and 4.3 agree with what the two new test files claim, and the commutativity
used in section 5 is a property of the set operations of `SPEC.md` 4.3 as proved in
`docs/proofs/precision.md` Propositions 1 and 2.

Two findings that are not against the specification but that the orchestrator should know:

* **The 24 commutative swaps of the report are not 24 survivors any more.** Five of them
  (`fball.c` 54, 214, 552, 555, 684) and the two `fmpz_zero` of `adf_fball_init` are equivalent for
  every input, and the eight further commutative swaps of `fball.c` are of the same kind. All fifteen are
  now in `equivalent.txt` with a reason each.
* **`src/fball.c` lines 420 and 422 are dead code** in every build the Makefile produces (section 8.2).
  They are a leftover of the worktree split and should go, or be excused.
