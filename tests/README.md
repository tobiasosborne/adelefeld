# tests: how to add a test

1. Name the file `tests/test_<module>.c`, one test program per file; the Makefile picks it up.
2. Include `"test_runner.h"` last: it supplies `main`, and it must come after `<adelefeld.h>`.
3. Write the tests as `ADF_TEST(name) { ... }`; names must be unique within the file.
4. Assert with `ADF_CHECK(cond)`; add a message with `ADF_CHECK_MSG(cond, "fmt", ...)`.
5. Each assertion states the claim, not the function that made it: `ADF_CHECK(a == b)`, not a boolean.
6. A test is not a test if a wrong implementation would pass it (`docs/PLAN.md` section 7).
7. Use FLINT directly for the exact cases; state the precision when the claim needs one.
8. Run `make check`; it builds every test, runs it, and exits 1 if any check failed.
9. Run `make clean && make check SAN=1` before committing anything that touches memory.
10. Reference data goes to `tests/golden/`, the Python reference to `tests/ref/`; neither is compiled.
