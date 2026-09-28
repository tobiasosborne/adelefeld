# Lane m1-cap-chain: report

## What was done

The three chain tests of the absolute cap, as asked in the brief, in one new test program
`tests/test_cap_chain.c` (three tests, 320975 checks). No source file of the library was changed in
the final state; `git diff` is empty for the whole tree.

1. `chain_invariant`: the invariant of Proposition 15 along a chain of 50 random capped
   operations. 4 caps (1, 1/6, 12 and 2^120 * 3^50, a cap of 200 bits) times 3 seeds, so 12
   chains of 50 steps; the operation of each step is drawn at random from add, sub, mul, mul_rat,
   and the test asserts that each of the four kinds occurred in every chain. The start value and
   every fresh operand are passed through `adf_fball_cap` first, so that all values of the chain
   are capped values. After every step: the status is ADF_OK, the result satisfies predicate G,
   the radius divides C (checked as `C/R` a positive integer, Proposition 15.1), the cap is
   idempotent on the result (Proposition 15.2), a sum or a difference of two capped values is
   identical to the tight result of the same step, since `gcd(R1, R2) | C` (Proposition 15.3), and
   when the tight product or the tight product with an exact scalar has a radius that does not
   divide C, the cap really changed it (Proposition 15.4). 5163 checks.
2. `chain_contains_tight`: the same random expression of 50 steps, once with the tight operations
   of fball.h and once with the capped operations (same 4 caps, same 3 seeds, same draws, 600
   steps). After every step: `adf_fball_contains(tight, capped)` for the tight result of the
   capped inputs and for the value of the all-tight chain, and the radius of the capped value is
   the rational gcd of the radius of the tight result of the capped inputs with C, 0 when that
   tight result is exact, in which case the capped value must be that exact result unchanged. The
   gcd is not taken from `fmpq_gcd` (the call of src/cap.c) but recomputed in the test with fmpz
   alone, as the reference `tests/ref/adfref/rat.py:32 qgcd` does: L = lcm of the two denominators,
   gcd of the two numerators over L. 4292 checks.
3. `enumeration_small_radii`: 15 small balls (A, H, d) with radius H/d up to 4, denominator up to
   3, three of them exact, paired in all 225 ways, for the 4 caps, and for the bare inputs and for
   the capped inputs (1800 runs). For every run, 8 members of each input in a window (the stored
   centre and the next 7 members: the set is N-periodic, so 8 consecutive members lie in every
   window of 8 periods), and then every sum of two members, every product of two members and the
   product of a member with each of the four exact scalars 3/2, -5/4, 0, 7 must lie in the
   corresponding capped result, checked with `adf_fball_contains_rat`. 311520 checks, of which
   288000 are membership checks of a sum, a product or a scalar product.

Randomness is FLINT's `flint_rand_t` with a fixed seed per (cap, seed) pair
(`flint_randseed(st, 1000 + seed, 17 + cap)`), so a failing run is reproducible.

### Red-green by planted errors

`lanes/m1-cap-chain/redgreen.log` has, for each planted error, the diff, the failing run (its
first lines, the per-test count of failed checks, the summary) and the passing run after the error
was removed. Summary:

- A: the cap skipped in `adf_fball_mul_cap` (`adf_fball_set(z, t)` in place of
  `adf_fball_cap(z, t, C)`). Failing tests: chain_invariant (12 failed checks),
  chain_contains_tight (22). Total 34. Caught by the test it was planted for, test 1.
- B: the exact case capped, the early return of Proposition 14.2 removed from `adf_fball_cap`.
  Failing tests: chain_contains_tight (42). Total 42. Caught by test 2.
- C: the radius taken from the centre A/d instead of the radius H/d in `adf_fball_cap`. Failing
  tests: chain_invariant (146), chain_contains_tight (1127), enumeration_small_radii (88577).
  Total 89850. Caught by test 3 (and by the other two).
- D (extra, the example named in the brief): `fmpq_gcd` replaced by `fmpq_mul` in
  `adf_fball_cap`. Failing tests: chain_invariant (1005), chain_contains_tight (996),
  enumeration_small_radii (110452). Total 112453.

A sample of the failing output of each:

```
A: FAIL tests/test_cap_chain.c:332: chain_invariant: check failed: rat_divides(caps[ci], R)
   | cap 0 seed 2 step 28 op mul: radius 7/15552 does not divide C 1
B: FAIL tests/test_cap_chain.c:470: chain_contains_tight: check failed: adf_fball_is_exact(tc)
   | cap 0 seed 0 step 35 op mul_rat: the exact value 0 was widened
C: FAIL tests/test_cap_chain.c:662: enumeration_small_radii: check failed:
     adf_fball_contains_rat(zmul, p)
   | cap 3 v1 x=(5,0,2) y=(7,4,3): the product 15 is not in the capped result
D: FAIL tests/test_cap_chain.c:363: chain_invariant: check failed: !adf_fball_identical(t2, t)
   | cap 0 seed 0 step 5 op mul_rat: the cap did not act on radius 5/24
```

After the last planting `src/cap.c` was restored from the copy taken before the first planting;
its md5 is `da44dbd56ac5ece6c7650bc9a5415349`, the same as the file as the lane received it, and
`git diff src/cap.c` prints nothing.

## Files written

- `tests/test_cap_chain.c` (692 lines): the three tests and their helpers.
- `lanes/m1-cap-chain/redgreen.log` (193 lines): the planted errors, the failing and passing runs.
- `lanes/m1-cap-chain/report.md`: this file.

Nothing else in the tree was created or changed.

## Checks run

- `./build/test_cap_chain`: 3 tests, 320975 checks, 0 failed checks, 0 failed tests; wall time
  0.04 s. Per test, counted with a separate binary in /tmp that calls the three test bodies one
  after the other: `chain_invariant` 5163, `chain_contains_tight` 4292, `enumeration_small_radii`
  311520 checks, 0 failed each.
- `make clean && make -j2 check`: exit 0, "check passed: all 22 test programs"; the new program
  reports 3 tests, 320975 checks, 0 failed. 12.1 s. Repeated: 5 runs, 4 passed, 1 failed, the
  failure being a segfault in `build/test_recon`, a test of another lane, see "A defect found in
  another lane's test" below.
- `make clean && make -j2 check SAN=1`: exit 0, "check passed: all 22 test programs"; the new
  program reports 3 tests, 320975 checks, 0 failed. 22.8 s. Repeated: 3 runs, 3 passed.
- `make clean && make -j2 check CC=clang` (not asked for, run once as tests/README item 9 says):
  exit 0, "check passed: all 22 test programs". 9.2 s.
- `git diff` (whole tree): empty; `git status --porcelain`: only the new untracked files
  `tests/test_cap_chain.c`, `lanes/m1-cap-chain/redgreen.log`, `lanes/m1-cap-chain/report.md`
  and the lane log written by the harness.
- `md5sum src/cap.c`: da44dbd56ac5ece6c7650bc9a5415349, unchanged.

## What is not done

- No mutation run (`make mutate FILES=src/cap.c`): the brief of this lane asks for the red-green
  with planted errors, not for a mutation run, and a mutation run of src/cap.c belongs to the lane
  m1-cap. The four planted errors above cover the same ground by hand.
- No benchmark and no fuzz target: none asked for, and the cap has no parser.
- Test 1 and test 2 do not test the cap with operands of thousands of bits; lane m1-cap tests that
  on single operations. In test 2 the operands of the 200-bit cap reach about 400 bits (the
  radius of a product of two wide balls is the square of the wide radius before the cap), and the
  tight chain of that cap stays below 2000 bits over 50 steps.
- The three tests do not check the statuses other than ADF_OK of the capped operations on a chain
  (C <= 0, a local-backend input). Those are single-operation cases and are tested in
  `tests/test_cap.c`; a chain would only repeat them.
- The count of the steps in which the cap acted is a property of the random draws, not of the
  specification, so it is required to be positive per cap and summed over the three seeds, not per
  seed: with cap 1/6 and seed 0 the cap acts in none of the 50 steps, and a per-seed requirement
  would be a claim about the draws. The check says that item 4 of Proposition 15 was exercised.

## Sources pending

None. Every statement used was read on disk and is cited in the header of the test file with file
and line: docs/proofs/policies.md (Definition 13 line 253, Proposition 14 line 258, Proposition 15
line 283, Theorem 3 line 75, Lemma 1 line 43), docs/SPEC.md (4.2 line 223, 4.4 item 3 line 277),
docs/conventions.md (5.4 "The absolute cap" line 520), tests/ref/adfref/policies.py:150
(`absolute_cap`), tests/ref/adfref/rat.py:32 (`qgcd`).

## Findings against the specification

None. Against the unchanged `src/cap.c` all three tests pass; nothing in docs/SPEC.md or in
docs/proofs/policies.md section 3 was found to be wrong or unprovable as stated in the part this
lane covers.

## One finding against the brief (not against the specification)

The brief asks for `adf_fball_contains(capped, tight)` after every step. With the meaning the
library gives the verb, that call is false, and it is the other order that is the enclosure claim.
`include/adelefeld/fball.h` says of `adf_fball_contains(x, y)`: "1 if the set x is inside the set
y ("first inside second", SPEC 4.2)", and docs/SPEC.md line 223 gives the condition "`N/M` is an
integer and `(a - b)/M` is an integer", with N the radius of x and M the radius of y, that is
exactly `x` inside `y`. The cap replaces the radius R by `gcd(R, C)`, which divides R, so the
capped ball is the larger one (docs/proofs/policies.md Proposition 14.1, "c + R Zhat is inside
c + gcd(R, C) Zhat").

Smallest case, run against the unchanged source: `x = 0 + 1 Zhat`, `q = 2`, `C = 1`,
`adf_fball_mul_rat_cap`:

```
x = 0 + 1 Zhat, q = 2, C = 1
tight  = (0 + 2 Zhat)/1
capped = (0 + 1 Zhat)/1
adf_fball_contains(capped, tight) = 0
adf_fball_contains(tight, capped) = 1
```

So the tests check `adf_fball_contains(t, tc)` and `adf_fball_contains(t2, tc)`, that is the tight
value inside the capped one, and this is written in the header of the test file and at the place of
the check. If the orchestrator wants the literal call of the brief, the verb of the library would
have to read the other way round, which would contradict fball.h and docs/SPEC.md line 223.

## A defect found in another lane's test (not mine, not changed)

`make -j2 check` is flaky in this tree, and the cause is not this lane. `build/test_recon`
(of lane m1-recon) terminates with SIGSEGV in about 3 to 5 runs out of 20, with no output at all,
because stdout is buffered and lost. Numbers measured here:

- `./build/test_recon` 20 times: 5 segfaults.
- The same 20 times with `tests/test_cap_chain.c` moved out of the tree: 3 segfaults, so the new
  test program is not involved.
- 15 further runs: 3 segfaults, at a random point of the run.
- Under `SAN=1`: 5 runs, no segfault (the sanitizers do not see uninitialised reads).

Localised with gdb (`gdb -batch -ex run -ex "bt 6" ./build/test_recon`), every time in the same
place:

```
Program received signal SIGSEGV, Segmentation fault.
#0  __gmpz_set () from /lib/x86_64-linux-gnu/libgmp.so.10
#1  fmpq_set (src=0x7fffffffd380, dest=0x7fffffffd3a0) at /usr/include/flint/fmpq.h:81
#2  adf_test_fn_an_adele_with_operands_of_4096_bits () at tests/test_recon.c:1249
```

The cause is in `tests/test_recon.c`, test `an_adele_with_operands_of_4096_bits`: the five
`fmpq_t` values `fa, fN, glo, ghi, out` are declared at line 1191 and cleared at the end of the
test, but never initialised; `fmpq_set(fa, a->q)` (line 1248) and `fmpq_set(fN, N->q)`
(line 1249) then write through whatever the stack held. Five `fmpq_init` calls are missing. Not
changed: the file belongs to lane m1-recon. Until it is fixed, `make -j2 check` can fail for a
reason that has nothing to do with the change under test.

## An observation on cost, for the lane m1-cap (no change made)

`adf_fball_cap` reads the centre with `adf_fball_get_center` and writes with
`adf_fball_set_center_radius`, which re-canonicalises the triple and so does a second gcd of
A, H, d on a value that is already canonical (it comes from a tight operation of fball.h). The
radius only has to be divided by the factor `gcd(N, C)`: from the canonical triple of x, writing
N = H/d, g = gcd(N, C) and N = g N', one gets A, H, d from the triple of the centre and g N'. That
would save one gcd of the size of the operands per capped operation. Not changed: src/cap.c is not
mine outside the planted errors.
