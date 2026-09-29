# lanes/m1-leftovers: progress

One section per task. Each says what was done, the commands, and the results with numbers.
`lanes/m1-leftovers/redgreen.log` holds the red runs of the tests of task 2 and 3.

## Task 1 (adf-4lj): the target `check-all`

Done.

### What was written

`Makefile`, four places and nothing else:

* a comment line in the header comment, after the line of `make check`;
* `.PHONY` (one name added);
* the target `check-all` itself, between `check` and `clean`;
* two lines in `help` (the list of targets, and a line under it).

The target is

```make
check-all:
	@set -e; \
	echo "== make check CC=$(CC) SAN=$(SAN) INV=$(INV)"; \
	$(MAKE) check CC="$(CC)" SAN="$(SAN)" INV="$(INV)"; \
	echo "== sh tests/test_driver.sh"; \
	SAN="$(SAN)" sh tests/test_driver.sh; \
	echo "== sh tests/test_exports.sh"; \
	CC="$(CC)" sh tests/test_exports.sh; \
	echo "== sh tests/test_julia.sh"; \
	sh tests/test_julia.sh; \
	echo "== python3 tools/mutate/selftest.py"; \
	python3 tools/mutate/selftest.py; \
	echo "== python3 tools/memcheck/selftest.py"; \
	python3 tools/memcheck/selftest.py; \
	echo "check-all passed: ..."
```

`set -e` in the one shell of the recipe stops at the first step that fails. `CC`, `SAN` and `INV`
are passed to `make check` on the command line of the sub-make, not only through the environment.
`SAN` is also given to `tests/test_driver.sh` and `CC` to `tests/test_exports.sh`, because those
two scripts read them (`tests/test_driver.sh:25` and `tests/test_exports.sh:42`). The target of
the sub-make is `check`, which is unchanged.

`make check` is unchanged: no line of it was touched.

### Checks

Baseline before the change (not part of the target, to know the tree was clean):

```
$ make -j2 check
real 0m22,037s
check passed: all 42 test programs
```

The run on the clean tree, all six steps, in order:

```
$ make check-all
$ echo $?
0
$ grep -n '^== ' /tmp/l1_ok.log | grep -v test_ | head -3
1:== make check CC=cc SAN=0 INV=0
533:== building the shared object            (this is test_exports.sh, step 3)
738:== build/libadelefeld.so is up to date ... (this is test_julia.sh, step 4)
$ tail -1 /tmp/l1_ok.log
check-all passed: make check, driver, exports, julia, mutate-selftest, memcheck-selftest
```

The two tool self-tests both ran and both passed: `tools/memcheck/selftest.py` printed
`selftest: passed` (10 `ok` lines, of which 5 valgrind cases), and the mutate self-test printed
`selftest: passed` after its 11 checks. Julia is installed here (1.12.5) and its step ran, so the
step was not skipped.

The failing run. Nothing in the tree was changed for it: a scratch copy of `tests/test_exports.sh`
was written under `build/scratchcheck/`, with one line added that prints a message and exits 1,
and a scratch copy of the Makefile (`build/scratchcheck/Makefile.red`) that differs from the real
one in the path of that script alone (two lines; `diff` shown in the session). The real
`tests/test_exports.sh` and the real `Makefile` were not written to.

```
$ sed -i 's|^set -u$|set -u\necho "scratch copy: forced failure"; exit 1|' build/scratchcheck/fake_exports.sh
$ sed 's|sh tests/test_exports.sh|sh build/scratchcheck/fake_exports.sh|' Makefile > build/scratchcheck/Makefile.red
$ make -f build/scratchcheck/Makefile.red check-all
$ echo $?
2
```

The steps that ran in that run, in order:

```
1:== make check CC=cc SAN=0 INV=0
532:== sh build/scratchcheck/fake_exports.sh
```

and its last two lines:

```
scratch copy: forced failure
make: *** [build/scratchcheck/Makefile.red:123: check-all] Error 1
```

So the failing step stops the target: the lines of `tests/test_julia.sh`,
`tools/mutate/selftest.py` and `tools/memcheck/selftest.py` are nowhere in the log, and neither
`check-all passed` nor the step banners of steps 4, 5 and 6.

The scratch files live under `build/`, which `make clean` removes; they are not part of the tree.

## Task 2 (adf-whv): tests for the known gaps

Done. Three new test files and one line of checking; no source file of the tree was changed. The
red runs are in `lanes/m1-leftovers/redgreen.log`, in a scratch copy of the tree under
`build/scratch/`.

### 2.1 `tests/test_common_unsupported.c`: the status ADF_UNSUPPORTED of src/common.c:161

The line is the second return of `adf_version_check` (src/common.c:157-162). It is reached when the
run-time string `flint_version` of FLINT (flint.h:105) and the compiled string
`adf_flint_version_compiled()` (= `FLINT_VERSION`, flint.h:97) do not have the same major and minor
number. On this machine they agree, so the function always returns at src/common.c:159.

The test reaches it by giving the program its own definition of the array `flint_version`:

    char flint_version[32] = "3.0.1";

`flint.h:105` declares it `FLINT_DLL extern char flint_version[];` and `FLINT_DLL` is empty on this
platform (flint.h:43-45), so the definition is legal C. The static linker resolves the reference of
the library to it, and at load time the definition of the executable comes before the one of
libflint.so. Nothing of the library is recompiled. The array is writable, so a test puts any
version string in it and calls the real `adf_version_check`. That the override is in force is
checked, not assumed: the first test puts "3.0.1" in the array and requires ADF_OK, and the
remaining tests would all see the same answer if the library read FLINT's own array.

What the file checks: the same major and minor gives ADF_OK whatever the patch number, the suffix
and the leading zeros; a different minor, a different major, a two-digit minor and a string that is
not a version give ADF_UNSUPPORTED; the code of ADF_UNSUPPORTED is 8 and its name is "UNSUPPORTED"
(status.h:26 and 63); the function writes nothing, so two calls on the same input agree.

    green: 6 tests, 48 checks, 0 failed, exit 0
    red (src/common.c:161 changed to `return ADF_OK;` in build/scratch/common):
          6 tests, 48 checks, 16 failed checks, 5 failed tests, exit 1

### 2.2 `tests/test_modctx_pin.c`: the bound [0, K) of adf_modctx_recombine

The bound is carried by src/modctx.c:603, `fmpz_fdiv_r(out, out, ctx->K);` in
`adf_modctx_recombine`; the promise is in src/modctx_internal.h (the unique integer of [0, K) with
the given residues). The old pin, tests/test_modctx_limits.c:353, takes K from
`adf_modctx_get_modulus` and, as the closure judge of the contexts review found
(lanes/m1-closure-contexts/report.md:133-134), is green even when the line is deleted: on FLINT
3.0.1 `fmpz_multi_CRT_precomp(out, ..., 0)` returns a value of [0, K) by itself.

The new pin differs in three ways: K is the product of the blocks, computed by the test; the value
is compared with an oracle of the test that walks [0, K) and keeps the one integer with the given
residues (the definition of docs/proofs/policies.md Lemma 17.1, not the implementation); and the
two ends of the range are pinned by value (the residues 0 give 0, the residues q[i] - 1 give K - 1,
and with one block the representative is the residue itself). Contexts: K = 35, 105, 2310 (every
residue), two word-sized primes, one block over 2, 65537 and 4294967291, and the round trip of -1
and -(K - 1).

    green: 7 tests, 5716 checks, 0 failed, exit 0, real 0m0,008s
    red (the bound moved by one, `fmpz_add_ui(out, out, 1);` added after src/modctx.c:603, in
         build/scratch/pin): 7 tests, 5716 checks, 2847 failed checks, 7 failed tests, exit 1
         (the old pin on the same build: 239 checks, 76 failed, 1 failed test)
    and the case the judge named, the line deleted and not moved: both the new pin and the old
    one are green (0 failed checks each). No test of the value can see that difference on this
    FLINT; this is stated in the test and in the report.

### 2.3 `tests/test_text_r3r9.c`: R3 and R9 of the text review

R3. The rule pinned is the one of include/adelefeld/text.h:32-42 with decision M1-D6: at the
default limits a value that the reader admitted either prints nothing (NULL with *len = 0) or
prints text that the default reader reads again into a ball that contains the source with the same
finite part. The first test is the reviewer's input `(9.99e100000 ; 0)` at prec = 128, digits = 1,
and the same value with a negative exponent; the second test is the rule over a table of seven
texts; the third is the cadele case, which the review did not give; the fourth states the
qualification at a small max_exp10 (green on the old printer as well, and it says so).

    green: 8 tests, 144 checks, 0 failed, exit 0
    red (old printer: tx_arb_printable of src/text.c:1111 made to return 1): 12 failed checks in
         3 tests, exit 1. The old printer prints "(1e100001 +/- 1.1e99998 ; 0)", which is the text
         of the review, and the default reader gives ADF_LIMIT (10), the status of the review.

R9. The finding is a coverage gap: the old target took prec from the first byte of the text and
digits from the last, so an accepted adele or cadele reached prec in {11, 12, 15, 34, 42} and
digits in {3, 10, 11, 12, 14} and never prec = 2 or digits = 1. Three tests of the file:

  * prec = 2 and digits = 1 on the adele and cadele value forms, on texts with leading and
    trailing whitespace, asserting the contract of conventions 9.6;
  * prec = 2 against prec = 64 on the same text, so that a test that ignores prec cannot pass:
    the ball at prec = 2 is strictly wider and both contain 1/10;
  * the bare adf_rat and adf_fball value forms, which the corpus of the target (adele and cadele
    lines) does not cover, with the round trip of conventions 9.6;
  * the coverage evidence itself as an assertion: the corpus of tests/fuzz/corpus/text/ is read as
    the target reads it (byte 0 gives prec = 2 + byte mod 190, byte 1 gives digits = 1 + byte mod
    30, tests/fuzz/fuzz_text.c:287-288) and an accepted adele or cadele text must be reached at
    digits = 1 and over a large part of both ranges. The same corpus is then read the way the old
    target read it, and the test asserts that the old scheme reaches neither prec = 2 nor
    digits = 1 on it. Measured on the clean tree: the old scheme reaches 1 prec value and 1
    digits value (42 and 12) over 90 accepted texts, the present scheme 65 prec values and all
    30 digits values.

    red (old target and old corpus, i.e. the two control bytes removed from all 283 files and the
         derivation of the target put back): 5 failed checks in 1 test, exit 1.

  One thing is measured and not asserted, and it is a finding: no accepted adele or cadele text of
  the committed corpus carries the control byte for prec = 2 (the test prints "prec = 2 reached: 0").
  So the digits half of R9 is closed for the round trips of the target and the prec half is not.
  The value of prec is used by the adele and cadele round trips only; the rat and fball paths of
  the target do not take it. An assertion `precs[2]` would be red on the clean tree; the fix is
  one control byte in one file of tests/fuzz/corpus/text/, which this lane does not own. It is in
  the report under "Findings".

### 2.4 src/recon.c:272 (ADF_DOMAIN for an infinite real ball)

Checked, nothing changed. The line is the first check of `adf_adele_reconstruct` (src/recon.c:271-272)
and the status is the courtesy of the release build of decision M1-D11. The test
`an_adele_with_an_infinite_real_ball_is_rejected` is at tests/test_recon.c:1362 (the brief names
1313; the file has moved since). Its release expectation is the block under `#ifndef
ADF_CHECK_INVARIANTS` at tests/test_recon.c:1377-1379, and the comment at tests/test_recon.c:1365-1371
says in words that the answer is "a courtesy of the release build and no promise (M1-D11: recon.h
does not exempt a non-canonical adele); the test of it is compiled only without
ADF_CHECK_INVARIANTS". The test function itself is compiled in both builds, as every test of
tests/test_recon.c is; under `INV=1` the same function requires the abort of
tests/test_recon.c:1380-1382 instead. That is what the brief asks for; one line, no edit.

## Task 3 (adf-xrt): the six statements that mutation testing showed to be without effect

The line numbers of `lanes/m1-testgaps/report.md` section 8 are of an older tree. Each statement was
found by its text; the present line is given below.

### The six statements, and the reason for each, before anything was deleted

**1 and 2. `src/fball.c` 420 and 422 of the report, the two dead guards of `adf_fball_prec_at`.**

    if (adf_place_is_archimedean == NULL) return ADF_UNSUPPORTED;
    if (adf_place_prime_get == NULL) return ADF_UNSUPPORTED;

Both statements are already gone from the tree; nothing to delete. `git show 2b804c5 -- src/fball.c`
(commit "WIP m1-repair-adele") removed them together with the two `#pragma weak` declarations above
them, and the function now calls the two place functions directly (src/fball.c:599 and 606). The
file `src/place.c` is compiled by the Makefile in every build, so the two guards were false in every
build that exists. Two of the six are therefore already done by another lane; this lane changes
nothing here.

**3. `src/fball.c` 734 of the report: the second `fmpq_sub(diff, a, b);` of `fb_contains_g`, which
is at src/fball.c:1057 today.**

    1049        fmpq_sub(diff, a, b);      <- the first one, before the branch
    1050        fmpq_div(t, diff, M);
    1051        if (fmpq_is_zero(N))
    1052            res = fb_is_integer(t);
    1053        else
    1054        {
    1055            fmpq_div(t, N, M);
    1056            res = fb_is_integer(t);
    1057            fmpq_sub(diff, a, b);   <- the line in question
    1058            fmpq_div(t, diff, M);
    1059            res = res && fb_is_integer(t);

Reason it cannot change any result: `diff` is a local `fmpq_t` (src/fball.c:1032) that is written
only by the call of line 1049 on the path that reaches line 1057; the three statements between
(1050, 1055, and the test of 1051) write and read `t`, a different `fmpq_t`, and `a`, `b`, `M` are
not written at all. `fmpq_sub` writes its first argument and reads the other two, so the call of
line 1057 stores exactly the value that line 1049 stored, and line 1058 divides that same value as
before. The result `res` is therefore the same for every input. Deleted.

**4. `src/recon.c` 97 of the report: `fmpz_one(fmpq_denref(q));` in `fmpq_set_dyadic`, the branch
`exp >= 0`. Present line src/recon.c:135.**

    131        if (fmpz_sgn(exp) >= 0)
    132        {
    133            fmpz_mul_2exp(t, mn, sh);
    134            fmpz_set(fmpq_numref(q), t);
    135            fmpz_one(fmpq_denref(q));
    136        }

Reason it cannot change any result: the statement writes the value 1 into the denominator of `q`,
and the denominator of `q` is 1 on every call that exists. The two calls of the static function are
src/recon.c:295 and 296, on `lo->q` and `hi->q`; both objects are `adf_rat_init`ed at
src/recon.c:282-283 and nothing between those two lines and the calls writes them: the only
statements in between are `arb_get_interval_fmpz_2exp` (src/recon.c:290), which writes its three
`fmpz` outputs `a`, `b`, `exp` and no `fmpq`, and the two calls themselves.
`adf_rat_init` is `fmpq_init(x->q)` (src/rat.c:49-53), and `fmpq_init` sets the numerator to 0 and
the denominator to 1 (/usr/include/flint/fmpq.h:28-32: `x->num = WORD(0); x->den = WORD(1);`).
So the denominator of `q` is 1 when line 135 runs, and the statement stores what is already there.
Note that line 135 is the only statement of the branch that writes the denominator, so after the
deletion the branch writes the numerator only and relies on that fact; the fact is a statement about
the two callers, not about `fmpq`. Deleted, as TJO decided, and the test of task 3
(tests/test_recon_canon.c) pins the value of the helper, not its entry state.

**5. `src/recon.c` 175 of the report: `fmpz_one(fmpq_denref(c->q));` in `adf_fball_reconstruct`,
the branch `kmin == kmax` with `N != 0`. Present line src/recon.c:234.**

    230        else
    231        {
    232            /* fmpz_set of kmin into a one-denominator fraction, then a + N k. */
    233            fmpz_set(fmpq_numref(c->q), kmin);
    234            fmpz_one(fmpq_denref(c->q));
    235            adf_rat_mul(c, c, N);
    236            adf_rat_add(c, c, a);
    237        }

Reason it cannot change any result: the same one. `c` is `adf_rat_init`ed at src/recon.c:188, so
its stored value is 0/1 and its denominator is 1; the branch that writes `c` before line 234 is the
other branch of the same `if` (`adf_rat_set(c, a)` at src/recon.c:210, in the `N = 0` branch), which
cannot be taken together with line 234; the statements of the taken branch before line 234
(src/recon.c:215-229) write `clo`, `chi`, `kmin` and `kmax` and not `c`. So the denominator of
`c->q` is 1 at line 234 and the statement stores the value that is there. Deleted, as TJO decided.

**6. `src/recon.c` 111 of the report: `fmpq_canonicalise(q);` in `fmpq_set_dyadic`. Present line
src/recon.c:144. KEPT, as TJO decided, and pinned by a new test.**

It is not a no-op on the value of the stored form: for `exp < 0` the branch writes
`num = mn` and `den = 2^sh` (src/recon.c:141-142), and when `mn` is even the two have a common
factor, so the stored rational is not in canonical form without the call. Every function of the
`fmpq` module assumes canonical input and produces canonical output
(refs/src/flint-3.0.1/fmpq.rst:21-26), which is what the comment block of the function
(src/recon.c:104-115) says. The report of the test-gap lane measured that the *public* result of
`adf_adele_reconstruct` does not change when the line is removed (10584 balls, the same hash), and
the reason is in the code: the bounds are read by `fmpq_cmp`, by `adf_rat_sub`, `adf_rat_div` and by
`fmpq_ceil_fmpz`/`fmpq_floor_fmpz`, which use `fmpz_cdiv_q` and `fmpz_fdiv_q` and are homogeneous,
and the value that is finally written is built from `a`, `N` and `kmin`, which are canonical. So
the line is kept for the form of the bounds and the test has to look at the bounds: the new test
tests/test_recon_canon.c includes src/recon.c, as tests/test_common.c:22 includes src/common.c, and
calls the static `fmpq_set_dyadic` itself.

### What was deleted, and the run after each

| Step | Statement deleted | Command | Result |
|---|---|---|---|
| a | `src/fball.c:1057`, `fmpq_sub(diff, a, b);` in `fb_contains_g` | `make -j2 check` | exit 0 |
| b | `src/recon.c:135`, `fmpz_one(fmpq_denref(q));` in `fmpq_set_dyadic` | `make -j2 check` | exit 0 |
| c | `src/recon.c:234`, `fmpz_one(fmpq_denref(c->q));` in `adf_fball_reconstruct` | `make -j2 check` | exit 0 |

Each of the three runs ended with `check passed: all 45 test programs`; the count was 45 because
the fourth test file of this lane was not yet written.

Three lines in two files, nothing else: `git diff --stat src/` says `2 files changed, 3 deletions(-)`.

Kept: the `fmpq_canonicalise(q);` of `fmpq_set_dyadic`, and the two dead guards that another lane
had already deleted. No statement was deleted without a reason written down above.

### `tests/test_recon_canon.c`

The public result of `adf_adele_reconstruct` does not depend on the form of the bounds, so a test
of the public result is green with and without the line; the new test includes `src/recon.c`, as
`tests/test_common.c:22` includes `src/common.c`, and calls the static `fmpq_set_dyadic` itself.
The archive member `src/recon.o` is not pulled by the linker, because every symbol it exports is
defined in the test file. What is checked:

1. the value of the bound is `mn * 2^exp`, against a value the test computes with `fmpz`, over 15
   pairs including `exp = 0`, a zero `mn` and a 64-bit exponent;
2. the stored form is canonical (`gcd(num, den) = 1` and `den > 0`, computed by the test with
   `fmpz_gcd`, not with the function under test), over 10 pairs of which 7 have a common factor
   without the line: 6/2, 12/8, 0/2, 1024/1024, 2/2, 18/4 and -12/8;
3. a bound that is not canonical on entry (4/2, -8/4) comes out canonical, so the helper does not
   rely on the entry state that the two callers happen to give it;
4. the refusal: `|exp| > max_exp` returns 0 and writes nothing, compared field by field, and the
   bound itself is `|exp| = max_exp`;
5. the public `adf_adele_reconstruct` on six intervals with one candidate each: the status is
   ADF_OK, the value is the candidate of docs/proofs/quotient.md Proposition 11, it lies in the
   ball and it is canonical. This test is green with and without the line, and says so.

    green: 5 tests, 139 checks, 0 failed, exit 0
    red (the line deleted in build/scratch/canon): 5 tests, 139 checks, 25 failed checks in
         3 tests, exit 1

### The four runs at the end of the brief

All four were run from the repository root, each after a `make clean`. The last line of each:

```
$ make clean && make -j2 check              real 0m25,124s
check passed: all 46 test programs

$ make clean && make -j2 check SAN=1        real 0m47,539s
check passed: all 46 test programs

$ make clean && make -j2 check INV=1        real 0m29,477s
check passed: all 46 test programs

$ make clean && make check-all              real 2m0,297s
check-all passed: make check, driver, exports, julia, mutate-selftest, memcheck-selftest
```

The first `make check-all` of this list failed, and the reason is worth writing down: the
self-test of the memory checker runs the checker over `tests/*.c` and requires no finding, and
`tests/test_modctx_pin.c` gave five of them (`K: use-before-init`, `K: clear-before-init`), because
its helper `modulus_of_blocks` initialised its parameter with `fmpz_one` and not with a call whose
name contains `_init`, which is the rule of `tools/memcheck/check_uninit.py` (its file comment,
"the analysis is syntactic"). The three tests of `test_modctx_pin.c` that had no other `fmpz_init`
were changed to call `fmpz_init(K)` themselves, and the helper now writes with `fmpz_set_ui(K, 1)`
and says in its comment that `K` must be initialised by the caller. After that
`python3 tools/memcheck/check_uninit.py tests/test_modctx_pin.c tests/test_recon_canon.c
tests/test_common_unsupported.c tests/test_text_r3r9.c` prints three zeros and exits 0, and
`make check-all` passes. The three other new test files gave no finding either.
