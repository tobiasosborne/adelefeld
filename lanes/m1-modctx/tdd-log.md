# Red-green log, lane m1-modctx

Runs from the repository root. `make -j2 build/test_modctx` builds the one test program;
`make check` builds and runs every test.

| # | What | Command | Result |
|---|---|---|---|
| 1 | reference unit tests (before any C) | `python3 lanes/m1-modctx/test_modctx_ref.py` | red: 2 of 23 failed (two wrong hand cases in the test: the blocks of 5! are 8, 3, 5 not 8, 9, 5; the number 112272535095293 is prime, the intended composite was not composite) |
| 2 | reference unit tests after fixing the two cases | `python3 lanes/m1-modctx/test_modctx_ref.py` | green: 23 tests, 0 failures (42.6 s, mostly the primality run over 1048577 primes) |
| 3 | the C test (first test of the file: a link error is the allowed red) | `make -j2 build/test_modctx` | red: compile errors first (typo `jsonl_ulong`, one unused variable), then, after fixing those, `undefined reference to adf_modctx_*` (link error) |
| 4 | after writing src/modctx.c | `make -j2 check` | see the report |
