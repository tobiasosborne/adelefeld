# Milestone 1 local review: closure check
CLOSED 2; CLOSED WITH EDIT 1; OPEN 0; SETTLED BY DECISION 1. NO BLOCKER OPEN.

| Finding | Review severity | Verdict | Evidence |
|---|---|---|---|
| R1 | MAJOR | CLOSED | Old statuses are now five zeros; 1,440 cap cases had 0 mismatches in each build. |
| R2 | BLOCKER | SETTLED BY DECISION | M1-D2 excludes the one-byte context; 7 live-pointer cases passed. |
| R3 | MAJOR | CLOSED | Old call exits 134 with the required line; 1,432 of 1,432 debug break runs abort. |
| R4 | MINOR | CLOSED WITH EDIT | The replacement satisfies L, but assigns `mctx` by hand against proposed M1-D10. |

## R2: M1-D2

The unchanged `checks/probe.c bad-context` still exits 1 under AddressSanitizer with a heap-buffer-overflow
at `src/modctx.c:472`, reached from `src/fball.c:361`. Its `mctx` points to a one-byte allocation, not a context.
M1-D2 at `docs/SPEC.md:861` makes that input undefined. `include/adelefeld/fball.h:92-98` and
`docs/conventions.md:146` state the same pointer precondition. I found no disagreement for M1-D2.

`closure-checks/probe_closure.c` uses a context made by `adf_modctx_new_blocks` and a full residue array.
It checks seven valid and invalid field patterns, including `d = 0`, wrong `H`, a residue at the block bound,
NULL `mctx`, and an invalid backend tag. Each of release, invariant, and ASan/UBSan builds returned 7 checks
and 0 failures. The broader `closure-checks/invalid_fixture_closure.c` returned 16 checks and 0 failures in
each build. The predicates returned 0 for the malformed fields without aborting.

## R4: edit to the repaired test

`tests/test_fball.c:1020-1031` now uses a live context and local data `(d; r) = (2; 2)` in context `(4)`.
This satisfies L. The unchanged old reproducer, `checks/invalid_fixture.c`, still reports `L = 0` for its
old fake-context fixture; it does not describe the current test. The current test reached
`ok canonicalise_raw_and_domains` in the invariant build. A scratch copy with the old fixture restored failed
2 of 412 checks, both in that test. The expected raw fields follow from `docs/conventions.md:427-430` and
the exact conversion rule at `:444-450`.

The current test writes `x->mctx` directly. Proposed M1-D10 at `docs/SPEC.md:869` puts a local value made
that way outside the lifetime contract. The debug borrow count cannot see the new reference. Replace
`tests/test_fball.c:1021-1027` exactly as follows. Leave the surrounding assertions in place.

Old:

```c
    fmpz_set_si(x->A, 0);
    fmpz_set_si(x->H, 4);
    fmpz_set_si(x->d, 2);
    x->backend = ADF_LOCAL;
    x->mctx = ctx4;
    x->res = (ulong *) flint_malloc(sizeof(ulong));
    x->res[0] = 2;
```

New:

```c
    fmpz_set_si(x->A, 2);
    fmpz_set_si(x->H, 4);
    fmpz_set_si(x->d, 2);
    ADF_CHECK(adf_fball_canonicalise(x) == ADF_OK);
    ADF_CHECK(adf_fball_set_local(x, x, ctx4) == ADF_OK);
```

The first call makes the raw global triple canonical. The second constructs the same raw local `(2; 2)`
through the library and records its borrow. `closure-checks/invalid_fixture_closure.c` exercised this path
under the invariant build and freed the context with 0 errors.

## Checks of the repair tests and decisions

`tests/test_cap_local.c` passed 6 tests and 253 checks. A scratch copy of `src/cap.c` with the former local
guard restored in `adf_fball_cap` made it fail 89 checks in 3 tests. Its local/global equality checks come
from the set contract. An independent rational oracle checked 1,440 local, mixed, and aliased cap calls with
0 mismatches in release and invariant builds. It forms sums and products from four rational witnesses and
takes their rational gcd for the tight radius, then the gcd with `C` for the cap.

`tests/test_invariants.c` passed 9 tests and 5,376 checks; its 1,432 break runs all ended by SIGABRT.
`tests/test_invariants_lifetime.c` passed 13 tests and 116 checks. A scratch copy with the entry check of
`adf_fball_neg` removed made `test_invariants` fail 43 checks in 2 tests, including both R3 assertions.
The expected abort and function name come from `docs/conventions.md:288`. The old R3 reproducer exited 134
and named `adf_fball_neg` and argument `x`.

The unchanged oracle checked 40,208 requests with 0 mismatches in each of release and invariant builds.
For the cap oracle, a product of members is `(a + u N)(b + v M)` with `u, v` in `Zhat`.
Its difference from `a b` is `u b N + v a M + u v N M`. These three coefficients generate every
difference. The four choices `u, v` in `{0, 1}` recover the three coefficients by subtraction.
Their rational gcd is therefore the tight radius; taking its gcd with `C` gives the capped radius.
The full release suite passed 42 programs. The full invariant suite ran 42 programs: 37 passed and the five
already identified older programs failed. It found no additional failing program. M1-D10 and M1-D11 are
still proposed. I found no input against either decision in this scope. M1-D10 needs the R4 test edit above;
its library-only construction rule is not yet stated in `docs/conventions.md:324-328` or the value headers.

## New findings

None.

## What was not examined

I did not review performance, allocation failure, every exported function independently, or all thread
interleavings. The five known invariant-build test failures were not repaired here. I did not review the
other milestone 1 closure lanes or the proposed decisions outside the local review's scope.
