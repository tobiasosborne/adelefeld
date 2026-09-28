# Lane m1-cap-chain: the missing chain tests of the absolute cap (issue adf-o9e)

Read `lanes/COMMON-C.md`, `lanes/m1-cap/brief.md`, `lanes/m1-cap/report.md`, `include/adelefeld/scaled.h`
(the cap block), `fball.h`, `docs/proofs/policies.md` section 3, `src/cap.c`, `tests/test_cap.c`.

**You own:** `tests/test_cap_chain.c`, `lanes/m1-cap-chain/`. `src/cap.c` is read-only, except for the
planted errors below, which you remove again.

Write three tests:
1. the invariant of Proposition 15 along a chain of 50 random capped operations (add, sub, mul, mul_rat,
   mixed; several seeds; caps `1`, `1/6`, `12`, a 200-bit cap);
2. the same random expression of 50 steps evaluated tight (`fball.h`) and capped, with
   `adf_fball_contains(capped, tight)` after every step, and the radius of the capped value equal to
   `gcd(radius of the tight result of the capped inputs, C)` (rational gcd);
3. enumeration for small radii: all members of the capped inputs in a window, their sums and products lie in
   the capped result.

Red-green by planted errors: for each test plant one error in `src/cap.c` that the test must catch (for
example `fmpq_gcd` replaced by `fmpq_mul`; the cap skipped in `adf_fball_mul_cap`; the exact case capped),
show the failing output, remove the error, show the passing output. Log in `lanes/m1-cap-chain/redgreen.log`.
At the end `git diff src/cap.c` must be empty; `make -j2 check` and `make clean && make -j2 check SAN=1`
pass. If a test fails against the unchanged `src/cap.c`, that is a finding: do not change the test or the
source, report it with the smallest failing case.
