# Report of lane m1-common

Work package 1.9, part: the three functions of `include/adelefeld/common.h`, the export check and
the `dlopen` check. Repository `adelefeld`, worktree `lanes/m1-common`, date 2026-09-28.

## 1. What was done

### 1.1 `src/common.c` (new, the whole file)

Three public functions and one static comparison, nothing else:

- `adf_str_free(char *)` calls `flint_free` and nothing more (conventions 4.2:249-252, 12.8:1515-1518,
  CV-43). `s = NULL` reaches `flint_free(NULL)`, which the installed FLINT passes to the free
  function; `free(NULL)` does nothing, as the header of the function says.
- `adf_flint_version_compiled(void)` returns `FLINT_VERSION`, the string literal of the FLINT
  header this file was compiled against (flint.h:97, "3.0.1"). It is a literal, so the pointer is
  the same at every call and the caller must not free it, as the header says.
- `adf_version_check(void)` returns `ADF_OK` when the major and the minor of `flint_version`
  (flint.h:105) and of `adf_flint_version_compiled()` agree, and `ADF_UNSUPPORTED` otherwise
  (conventions 12.11:1541-1543, CV-44). With the installed FLINT 3.0.1 it returns `ADF_OK`, which
  `tests/test_common.c` checks.
- `adf_version_major_minor_equal(a, b)`, static. It reads a run of digits, then a '.', then a run
  of digits from each string, drops the leading zeros of each run, and compares the two pairs of
  runs. The comparison reads the digits from the right, which is the same as writing the shorter
  run with `'0'` in front of it; no number is computed, so nothing can overflow, and "3.10" is not
  "3.1".

Four readings of the contract that the header leaves open, all of them stated in a comment at the
function and all of them tested (the header is not mine, COMMON-C rule 6):

1. A string that is not a version (the empty string, a string that does not begin with a digit, a
   string with no '.', a string with no digit after the '.') compares unequal to every string,
   **also to itself**. `ADF_OK` is a promise that the two versions agree, and nothing proves it for
   a string that is not a version. The run-time string of FLINT always has the right shape, so a
   caller never sees this.
2. The text after the minor number is not read: "3.0.1-rc1" has the major and the minor of "3.0".
3. Leading zeros are not part of a number: "03.00.1" and "3.0.1" agree.
4. `adf_flint_version_compiled` returns the `FLINT_VERSION` macro of the header, not a literal
   written in this file. The string is the same; the macro cannot fall out of step with the header.

The test-only entry `adf_test_version_major_minor_equal` is inside `#ifdef ADF_COMMON_TEST` at the
end of the file, with a declaration and a definition, as the brief asks. The library is compiled
without the macro, so the symbol is in no library: `nm --defined-only build/libadelefeld.a` lists
`adf_str_free`, `adf_flint_version_compiled` and `adf_version_check`, and no `adf_test_` symbol.

### 1.2 `tests/test_common.c` (new)

Seven tests, 78 checks. It defines `ADF_COMMON_TEST` and includes `../src/common.c` (the dependency
file of the test then lists `tests/../src/common.c`, so the test is rebuilt when the source
changes; the linker takes the definitions of the test object and does not pull `common.o` out of
the archive, so no symbol is defined twice).

- the compiled version string is `FLINT_VERSION`, is `flint_version`, begins with "3.0", and is the
  same pointer at two calls;
- `adf_version_check()` is `ADF_OK`, is the same at two calls, and its status name is "OK";
- `adf_str_free` reaches `flint_free`: the test installs counting memory functions
  (`__flint_set_memory_functions`, flint.h:211-217), allocates with `flint_malloc`, and counts
  exactly one call for one free and two for two frees, then puts the functions back. This is a
  claim without a sanitizer;
- `adf_str_free(NULL)` returns (the header: "does nothing"; whether the free function of FLINT is
  entered for a null pointer is not part of the contract, so the test claims no count);
- a table of 31 cases for the comparison, each in both orders: the nine cases of the brief, and the
  ones a wrong reader gets wrong (the digit 0, the digit 9, a two-digit major and minor against a
  one-digit one in both orders, a major 10 against a major 0, leading zeros, a trailing `.`, a
  missing digit, a string without a dot, the empty string in both orders and with itself, trailing
  text, a string that is not a version with itself);
- the comparison of the installed FLINT with the compiled string says the two agree.

### 1.3 `tests/test_exports.sh` (new)

Builds the whole of `src/` as a shared object with `cc -shared -fPIC` in `build/exports/`, copies it
to `build/libadelefeld.so`, reads the declared function names with `gcc -aux-info` (the tool
`lanes/m1-headers/gen_api_table.py` uses) filtered to `include/adelefeld/*.h`, lists the defined
dynamic symbols with `nm -D --defined-only`, and prints three lists. It fails if a declared
function is variadic, or if a symbol is exported that no header declares. The second list
(declared and not exported) is printed with its count and does not fail while the library is
incomplete, as the brief says. A defined name that begins with `_` is printed apart and not judged:
such a name is reserved to the implementation (C11 7.1.3) and belongs to the linker, and the
public names of this library all begin with `adf_`. On this system there are none, so the list is
empty and the point is untested here.

Numbers of the run of 2026-09-28: 193 declared names, 81 exported, 81 declared and exported, 112
declared and not exported, 0 exported and not declared, 0 variadic declarations. Exit 0.

### 1.4 `tests/test_dlopen.c` (new)

Two tests, 18 checks. If `build/libadelefeld.so` is there, it opens it with `RTLD_NOW | RTLD_LOCAL`,
finds `adf_version_check`, `adf_rat_init`, `adf_rat_clear`, `adf_sizeof_rat` and
`adf_flint_version_compiled` with `dlsym`, calls them, and checks with `dladdr` that the code
behind the pointer is the code of that file and not the static library this program links (all
five names exist in both). The claims: the version check is `ADF_OK`, the compiled version string
is the FLINT of the headers and begins with "3.0", `adf_sizeof_rat` is `sizeof(adf_rat_struct)`
and 16 bytes (conventions 5.1, 12.4), and `adf_rat_init` leaves the exact zero as 0/1 (conventions
5.1) which `adf_rat_clear` then releases. If the file is not there, each test prints one line and
passes with 0 checks.

Linking: `make build/test_dlopen` links with the `LDLIBS` of the Makefile, `-lflint -lgmp -lm`, and
needs no `-ldl` on this system (Ubuntu glibc 2.39: `dlopen`, `dlsym` and `dladdr` are in libc
since 2.34). An older system needs `LDLIBS='-lflint -lgmp -lm -ldl'`.

## 2. Files written

| Path | What |
|---|---|
| `src/common.c` | the three functions, the static comparison, the test-only entry |
| `tests/test_common.c` | 7 tests, 78 checks |
| `tests/test_exports.sh` | the shared object, the three lists, the variadic check |
| `tests/test_dlopen.c` | 2 tests, 18 checks |
| `lanes/m1-common/red-green.md` | the red and green runs, with the two faults of the first version |
| `lanes/m1-common/report.md` | this file |

No other file was created or changed.

## 3. Checks, with command and result

Numbers as printed; the locale writes a comma for the decimal point.

- `make build/test_common`, before `src/common.c` existed:
  red, `fatal error: ../src/common.c: No such file or directory`.
- `./build/test_common`, the first version of the source:
  red, `7 tests, 64 checks, 6 failed checks, 1 failed tests`, cases 10, 12 and 18.
- `./build/test_common`, after the fix: green, `7 tests, 74 checks, 0 failed checks`.
- `./build/test_common`, final, four cases added for the mutants:
  `7 tests, 78 checks, 0 failed checks, 0 failed tests`.
- `sh tests/test_exports.sh`: exit 0 and 204 lines of output, ending
  `81 of 193 declared functions are exported, 112 are not implemented yet, no exported name is
  undeclared, no variadic function`.
- `make -j2 check` with gcc, after `sh tests/test_exports.sh`:
  `check passed: all 11 test programs`.
- `make clean && sh tests/test_exports.sh && make -j2 check SAN=1`:
  `check passed: all 11 test programs`, no report of the sanitizers.
- `make clean && sh tests/test_exports.sh && make -j2 check CC=clang`:
  `check passed: all 11 test programs`.
- `./build/test_dlopen` with the shared object there:
  `2 tests, 18 checks, 0 failed checks`; the line "dladdr does not name a file" was not printed, so
  the check of the origin of the code ran.
- `mv build/libadelefeld.so /tmp; ./build/test_dlopen`:
  `2 tests, 0 checks, 0 failed checks`, exit 0, with the two lines "is not there".
- `make mutate FILES=src/common.c JOBS=2`, run 1:
  `76 mutants in 269.1 s: 48 killed, 4 survived, 22 not compiled, 2 timed out`.
- The same command, run 2, after the first two changes:
  `76 mutants in 275.0 s: 50 killed, 2 survived, 22 not compiled, 2 timed out`.
- The same command, run 3, final:
  `76 mutants in 355.4 s: 51 killed, 1 survived, 22 not compiled, 2 timed out`.
- `awk 'length > 116' src/common.c tests/test_common.c tests/test_dlopen.c tests/test_exports.sh`
  and the same for the two files of the lane: no line longer than 116 characters.
- `sh -n tests/test_exports.sh`: no output.
- `nm --defined-only build/libadelefeld.a`: `adf_str_free`, `adf_flint_version_compiled` and
  `adf_version_check` are there, and no symbol of the test-only entry.

### 3.1 The two tests were shown not to be vacuous

- `tests/test_exports.sh`, on a copy of `include/` and `src/` in `/tmp/exp-check` with
  `int adf_probe_variadic(const char *, ...);` added to `include/adelefeld/common.h` and
  `int adf_probe_undeclared(int x)` added to `src/place.c`: exit 1, the variadic declaration printed
  with its place (`include/adelefeld/common.h:91:NC`), and `adf_probe_undeclared` printed under
  "exported and not declared". Both failure paths work.
- `tests/test_dlopen.c`, on a copy of the tree in `/tmp/neg` with `src/rat.c` removed: exit 1, the
  two failures `dlsym(adf_rat_init): build/libadelefeld.so: undefined symbol: adf_rat_init` and the
  same for `adf_rat_clear`. The dlsym claims are real.

### 3.2 The mutation run in detail

76 mutants of `src/common.c`, 2 jobs, limit 200, seed 20260928, timeout 120 s per mutant.

- **51 killed.**
- **22 not compiled**, and none of them is a claim the tests do not check: 20 are the star of a
  pointer in a parameter list or in a field (`char *s` becomes `char /s`), which is a syntax error,
  and 2 are the removal of one of the two statements that read the minor number, which `-Werror`
  catches as `may be used uninitialized` at `-O2`.
- **2 timed out**, both of them loops that never end and can therefore not be killed by a test that
  finishes: `src/common.c:93:26` (`&&` to `||` in the loop that measures the run of digits: a
  character is never both below `'0'` and above `'9'`, so the loop condition is always true) and
  `src/common.c:96:5` (the negation of the loop that drops the leading zeros). They cost 2 x 120 s
  of the 355 s. A run with `--timeout 25` would finish in about 135 s and reach the same verdict.
- **1 survived**, and it is equivalent; the line for `tools/mutate/equivalent.txt` is in section 5.

The two earlier runs are the red-green of the tests, and nothing was weakened to reach the end:

- run 1 left 4 survivors: `r.n = 0` made `r.n = 1` (the scan then starts one digit late), and the
  three tokens of the loop that drops the leading zeros (`k + 1 < r.n` as `k + 0 < r.n`, as
  `k - 1 < r.n` and as `k + 1 <= r.n`).
- The emptiness checks now test `n` instead of `end`, which is the same condition (the strip keeps
  the last digit of a run, so a run is empty exactly when its stripped length is 0), and the case
  "10.1" against "0.1" was added. Run 2 left 2 survivors: the scan that starts one digit late, and
  `k - 1 < r.n`.
- The case ".0.1" against itself was added, which is the one a string that is not a version has to
  fail. Run 3 left one survivor, `k - 1 < r.n`, which is equivalent (section 5).

## 4. What is not done

- `tests/test_exports.sh` is not run by `make check`: the Makefile takes `tests/test_*.c` and the
  Makefile is not mine. The order is `sh tests/test_exports.sh && make -j2 check`; without the
  script, `tests/test_dlopen.c` prints its two lines and passes. Whoever owns the Makefile may want
  to add the script to `check`.
- The list "declared and not exported" does not make the script fail, as the brief says. It will
  have to when the milestone closes.
- There are no vectors for these functions. `tests/ref/vectors/` holds no record of a version string
  or of a string free, `tests/ref/adfref/` has no module for them, and nothing in the Python
  reference can produce a version mismatch or observe a free, so no vector file was generated.
- The mutant that survived is not in `tools/mutate/equivalent.txt`: that file is not mine (section
  5 gives the line).
- No fuzz target: nothing here reads untrusted input. The reader of a version string is the only
  thing that parses, and it is covered by 30 cases and by the mutation run.

## 5. Sources pending

- `[source pending: FLINT 3.0.1 src/flint.h (or src/flint.h.in) under refs/src/flint-3.0.1/]`.
  `refs/src/flint-3.0.1/` holds eight documentation files (`padic.rst`, `acb.rst`, `arb.rst`,
  `fmpq.rst`, `fmpz_mod.rst`, `nmod.rst`, `ulong_extras.rst`, `acb_dirichlet.rst`) and no header,
  so the brief's source is not on disk. The statements that need it were read in the installed
  header `/usr/include/flint/flint.h` of FLINT 3.0.1 and are marked in `src/common.c`:
  - `flint.h:97  #define FLINT_VERSION "3.0.1"` (used by `adf_flint_version_compiled`);
  - `flint.h:105  FLINT_DLL extern char flint_version[];` (used by `adf_version_check`);
  - `flint.h:204  void flint_free(void * ptr);` (used by `adf_str_free`);
  - `flint.h:211-217  __flint_set_memory_functions`, `__flint_get_memory_functions` (used by
    `tests/test_common.c` only);
  - `flint.h:94-96  __FLINT_VERSION 3, __FLINT_VERSION_MINOR 0, __FLINT_VERSION_PATCHLEVEL 1`.
  The line numbers of the installed file agree with the ones `include/adelefeld/common.h` cites
  (flint.h:94-97, 105, 193, 201-204), so the citations of the header hold for it. The four
  citations of `docs/conventions.md` (4.2, 12.8, 12.11, the table of section 13) are files of this
  repository and need no source.
- The line needed by `tools/mutate/equivalent.txt` (that file is not mine to change). The three
  lines below are one line in the file:

      src/common.c:96:op | the mutant never drops a leading zero (k = 0 gives the size_t 0 - 1,
      which is not < r.n), so its run keeps the zeros: the same number, and end is computed before
      and is untouched, and the comparison pads the shorter run with '0', so every input agrees

## 6. Findings against the specification

- `docs/conventions.md:1491` (rule 12.1) says "the header defines `ADF_INLINE` as `static
  __inline__`", but `include/adelefeld/common.h:36-44` defines it as `static inline` and its own
  comment says that this is deliberate ("written with the C99/C++ keyword `inline` instead of the
  GNU spelling `__inline__`, so that no compiler extension is used"). The header follows the second
  reading, and this lane does too; conventions 12.1 is out of date on this point. Nothing in the
  specification depends on the GNU spelling: `tests/test_exports.sh` shows that the functions marked
  `ADF_INLINE` are exported symbols of the library all the same.
- No other contradiction was found. `docs/SPEC.md` and `docs/PLAN.md` 1.9 ask for exactly what
  `adf_version_check`, `adf_str_free`, the export list and the `dlopen` program do; the version
  comparison of conventions 12.11 (major and minor) and the two statuses of conventions 3.1 are
  implemented as written.

## 7. Notes on the cost

Nothing avoidable was found. `adf_str_free` is one call, `adf_flint_version_compiled` returns a
constant, and the comparison walks the two strings once, over at most as many characters as the
longer version number; it allocates nothing and keeps no state. The length of a version number is
not bounded, and the scan stops at the first character that is not a digit, so a long string costs
only the length of its first two numbers.
