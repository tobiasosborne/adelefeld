# Reviewer `surface`: driver, common functions, places, statuses, tools

Read `lanes/m1-review/COMMON.md`; it binds you. Authors: pi models (space-bunny-alpha, deepseek-flash) and,
for the mutation tool's repair, Claude sonnet (review it all the same; say so in the review).
Files under review: `tools/adf/adf.c` with `tests/test_driver.sh` and `tests/driver/`, `src/common.c`,
`src/place.c`, `src/status.c`, `src/inlines.c`, `tests/test_exports.sh`, `tests/test_dlopen.c`,
`tools/mutate/mutate.py` with `selftest.py`, `tools/memcheck/`. `docs/SPEC.md` sections 4 and 15 (M1-D1).

Look in particular at: the expected lines of `tests/driver/*.out`: the lane wrote them by hand from the
specification and then corrected seven of them where its arithmetic was wrong; recompute every expected
line of the scripts 01 to 05 with your own exact arithmetic from `docs/SPEC.md` and report each line that
agrees with the program but not with the specification; the driver with hostile input (a line of 65536
bytes and one more, NUL bytes, no final newline, the separator ` with ` inside and at the ends of a line,
operands that are only whitespace, 10^6 operands' worth of nesting is not possible but long digit strings
are); the guard on the binary exponent of a real ball before printing; the mutation tool: a mutant that it
reports as killed but that was not built or not run (a false kill), a mutant reported as not compiled whose
test in fact failed (the lane m1-text found one such pattern, `error:` in a test name), the scratch copy
leaving files behind; `tools/memcheck/check_uninit.py`: a use before init that it misses (write three C
snippets with such defects in different shapes and run it).
