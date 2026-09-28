# Lane m1-rat: exact rationals, status strings, places (part of work package 1.2)

Read `lanes/COMMON-C.md`. Then `include/adelefeld/common.h`, `status.h`, `place.h`, `rat.h`, `docs/api-m1.md`,
`docs/conventions.md` sections 2 to 5.1 and 7, `docs/SPEC.md` 4.1, `tests/ref/adfref/rat.py`.

**You own:** `src/status.c`, `src/place.c`, `src/rat.c`, `src/inlines.c` (the file that defines
`ADF_INLINES_C` and includes `adelefeld.h`, so that every header-inline function is also an exported symbol),
`tests/test_status.c`, `tests/test_place.c`, `tests/test_rat.c`, `tests/ref/vectors/m1-rat/`.

Implement every function these three headers declare. For places: the primality requirement of the header (what
is validated, what is a precondition) exactly as the conventions say. After your work
`nm -D --defined-only` on a shared build, or `nm` on `build/libadelefeld.a`, must show every function of the
three headers as a defined symbol; write this check as a test script in your lane directory and report its
output.
