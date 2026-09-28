# tools/memcheck: finding reads of uninitialised FLINT / adelefeld values

A read of an uninitialised value of a FLINT type is invisible to the compiler and to the
address sanitizer. `fmpq_t` is `__mpq_struct[1]`, so `fmpq_t q;` without `fmpq_init(q)` is a
valid pointer to uninitialised bytes; the program only fails later, when GMP follows a garbage
limb pointer, and it fails in an unrelated-looking place. The undefined-behaviour sanitizer
does not flag the read either, because the type is a struct and the read is not, from the
language's point of view, of an uninitialised scalar.

This directory holds the two tools of lane m1-memcheck:

- `run.sh` -- the entry point. It picks the strongest check that is possible on the host.
- `check_uninit.py` -- a static checker that reads the C source and reports the three
  lifetimes a FLINT local can get wrong.

## What this host has

Checked on this machine:

- `valgrind` is not installed (`command -v valgrind` prints nothing).
- `clang` 18.1.3 is installed, so MemorySanitizer can be built.
- MemorySanitizer is not usable here: a trivial program with a plain uninitialised `int` read
  aborts with

      MemorySanitizer:DEADLYSIGNAL
      ERROR: MemorySanitizer: stack-overflow

  even with `setarch $(uname -m) -R`, so the run is not an instrumented run.
- Even if it ran, FLINT and GMP are not instrumented. Every value that comes out of FLINT
  would be reported as uninitialised, so the output would not separate our bugs from the
  library's untracked writes. A useful MemorySanitizer run is therefore not possible, and
  `run.sh` does not fake one.

`run.sh` probes for this: it runs a tiny program with a plain uninitialised read under
`clang -fsanitize=memory` and only calls MemorySanitizer usable when that read is reported.

## Running

    tools/memcheck/run.sh             # valgrind when present, else the static checker
    tools/memcheck/run.sh --checker   # force the static checker
    python3 tools/memcheck/check_uninit.py src/*.c tests/*.c
    python3 tools/memcheck/check_uninit.py --category use-before-init tests/test_recon.c

The exit status is 0 when nothing was found and 1 when a finding was printed.

When valgrind is present, `run.sh` builds every `tests/test_*.c` at `-O1 -g` into
`build-memcheck/` (the Makefile's `BUILD` variable is set on the command line) and runs each
binary under

    valgrind --error-exitcode=9 --track-origins=yes --leak-check=full -q

at most 2 at a time, with a 120 s limit per program. It prints one line per program, `PASS`,
`ERROR` (with the first error line) or `TIMEOUT`, and exits 1 on any error or timeout.

## What the static checker does

`check_uninit.py` reads each `.c` file, blanks comments, strings and preprocessor lines, and
walks every top-level function body that it can find. It reports one line per problem:

    tests/test_recon.c:1248: fa: use-before-init: first use `fa` (declared line 1191)

The three categories are:

- `use-before-init`: a local of a target type is read, or passed to a function other than an
  initialiser, before any init call with it. This is the crash the tool exists for.
- `clear-before-init`: a clear call with the variable before any init call. GMP would free
  uninitialised pointers.
- `init-without-clear`: an init with no clear before the variable leaves its scope -- a leak of
  the object the initialiser owns. Only variables that were actually initialised are reported.

The target types are `fmpz_t`, `fmpq_t`, `arb_t`, `acb_t`, `arf_t`, `mag_t` and the adelefeld
types that own memory: `adf_adele_t`, `adf_cadele_t`, `adf_fball_t`, `adf_rat_t`,
`adf_scaled_t`, `adf_ctx_desc_t`. `adf_place_t` (one `ulong`) and `adf_text_limits_t` (four
scalars) are plain by-value structs with no init function (include/adelefeld/place.h:20-24,
include/adelefeld/text.h:67-74); flagging them would be a false positive, so they are not
targets.

A call initialises a variable when the callee name contains `_init` and the variable is the
base identifier of the first argument, or when the callee is a project helper whose
corresponding parameter is itself initialised in its body. That helper relation is computed to
a fixpoint, which is what recognises `adele_init_fields`, `adele_build`, `adele_build_small`,
`adele_init`, `cadele_init`, `make_caps`, `tx_buf_init` and friends. A call whose callee name
contains `_clear` clears the first argument. A declaration that is never used and never
initialised is not reported: the compiler already reports it as unused.

### Limits

The analysis is syntactic and per translation unit. It can miss a use hidden behind a macro or
reached through a pointer, and it can be fooled by a function whose name contains `_init` but
which does not initialise its first argument. It does not look at struct fields or pointers.
It does not see a cross-file initialiser that is not named `*_init*`, because it does not track
the helper relation across files. For this tree, over `src/*.c tests/*.c`, it parsed 595
functions and 484 target-type declaration tokens.

## Results

- Old `tests/test_recon.c` (`git show cb268ba^:tests/test_recon.c`): the checker reports the
  five missing initialisations (`fa`, `fN`, `glo`, `ghi`, `out`, at uses 1248, 1249, 1252,
  1252, 1253) and their five `clear-before-init` calls. See the lane report.
- The current tree, `src/*.c tests/*.c`: 0 `use-before-init`, 0 `clear-before-init`,
  0 `init-without-clear`.
