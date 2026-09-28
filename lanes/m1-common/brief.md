# Lane m1-common: the functions of common.h (part of work package 1.9)

Read `lanes/COMMON-C.md`. Then `include/adelefeld/common.h`, `status.h`, `docs/api-m1.md`;
`docs/conventions.md` 4.2, 12.8, 12.11; `refs/src/flint-3.0.1/src/flint.h` (or `flint.h.in`) for
`flint_version`, `flint_free`, the version macros; cite file and line.

**You own:** `src/common.c`, `tests/test_common.c`, `tests/test_exports.sh`, `tests/test_dlopen.c`.

1. Implement `adf_str_free`, `adf_flint_version_compiled`, `adf_version_check`. The comparison of versions
   takes the major and the minor number from both strings; write it as a static function over two strings
   so that the test can reach the mismatch cases through a test-only entry that you declare in
   `tests/test_common.c` by including `src/common.c` directly, or document another way; the public behaviour
   with the installed FLINT is `ADF_OK`. Cases: equal; patch differs; minor differs; major differs; a
   string without a dot; empty string; `3.0` against `3.0.1`; `3.10.0` against `3.1.0`.
2. `tests/test_exports.sh`: builds the library as a shared object in a scratch directory under `build/`
   (`cc -shared -fPIC`), lists its dynamic symbols with `nm -D --defined-only`, and compares them with the
   function names declared in `include/adelefeld/*.h`. It prints three lists: declared and exported;
   declared and not exported (expected now, the library is not complete: print the count, do not fail);
   exported and not declared (must be empty: fail). It fails if any declared function is variadic.
3. `tests/test_dlopen.c`: if `build/libadelefeld.so` exists (built by the script), load it with `dlopen`,
   find `adf_version_check`, `adf_rat_init`, `adf_rat_clear`, `adf_sizeof_rat` through `dlsym` and call
   them; if the file does not exist, print a line and pass. Link needs `-ldl` only on old systems; check
   that the plain link of the Makefile works, and say so.

Mutation run: `make mutate FILES=src/common.c JOBS=2`.
