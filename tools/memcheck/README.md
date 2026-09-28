# tools/memcheck: finding reads of uninitialised FLINT / adelefeld values

A read of an uninitialised value of a FLINT type is invisible to the compiler and to the
address sanitizer. `fmpq_t` is `__mpq_struct[1]`, so `fmpq_t q;` without `fmpq_init(q)` is a
valid pointer to uninitialised bytes; the program only fails later, when GMP follows a garbage
limb pointer, and it fails in an unrelated-looking place. The undefined-behaviour sanitizer
does not flag the read either, because the type is a struct and the read is not, from the
language's point of view, of an uninitialised scalar.

This directory holds the two tools of lane m1-memcheck, and the self-test of both:

- `run.sh` -- the entry point. It picks the strongest check that is possible on the host.
- `check_uninit.py` -- a static checker that reads the C source and reports the three
  lifetimes a FLINT local can get wrong.
- `selftest.py` and `snippets/` -- the test of the two, see "Self-test" below.

## What this host has

Checked on this machine (2026-09-28):

- `valgrind` is installed, at `~/.local/bin/valgrind` (not on the default `PATH` of every shell).
  `run.sh` looks on `PATH` first and then at that path; `run.sh --valgrind-path` prints the one it
  would use. An earlier version of this file said valgrind was not installed; that was true when
  it was written and is not now. The static checker is therefore a fast first look, and valgrind
  is the check that sees a read of an uninitialised value on the path that actually runs.
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

`run.sh` probes for this: when there is no valgrind it runs a tiny program with a plain
uninitialised read under `clang -fsanitize=memory` and only calls MemorySanitizer usable when
that read is reported.

## Running

    tools/memcheck/run.sh                  # valgrind when present, else the static checker
    tools/memcheck/run.sh --checker        # force the static checker
    tools/memcheck/run.sh --valgrind-path  # which valgrind would be used
    python3 tools/memcheck/check_uninit.py src/*.c tests/*.c tools/adf/adf.c
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
  initialiser, at a place in the text that stands before the first init call with it, or with no
  init call at all. This is the crash the tool exists for. Only the first such use of a variable is
  reported. (Until 2026-09-28 the tool reported this only when there was no init at all, so
  `fmpz_add_ui(a, a, k); fmpz_init(a);` passed; surface review R1.) A `memset(x, ...)` is not a use
  and not an init: the tests zero a value before its init so that its padding is defined for
  `memcmp`.
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

The analysis is syntactic and per translation unit, and it reads the text in order. It is blind to
the following, each of which is pinned by a snippet in `snippets/` and a check in `selftest.py`
(surface review R4), and each of which valgrind sees when the path is run:

- **Branches.** An init anywhere in the scope counts for the whole scope, wherever it stands.
  `if (flag) fmpq_init(q); fmpq_set_si(q, 7, 3);` gives no finding
  (`snippets/s2_init_on_one_branch.c`).
- **Array elements.** `v[0]` and `v[1]` are the one variable `v` (`argument_base`); the init of
  one element counts for all. `fmpz_init(v[0]); fmpz_add(v[0], v[0], v[1]);` gives no finding
  (`snippets/s3_array_element.c`).
- **Jumps.** A `goto` over an init is not seen: the text order says the init comes first.
  `goto done;` over `arb_init(x)` and then `arb_clear(x)` gives no finding, also no
  `clear-before-init` (`snippets/s5_goto_skips_init.c`).
- **Macros, pointers, struct fields.** A use hidden behind a macro or reached through a pointer is
  not seen, and the tool can be fooled by a function whose name contains `_init` but which does
  not initialise its first argument. It does not look at struct fields or pointers.
- **Other files.** It does not see a cross-file initialiser that is not named `*_init*`, because it
  does not track the helper relation across files.

Text order, jumps and the branches are the price of not building a control-flow graph. If one of
them is ever handled, the README, the snippet and the pinning check change together.

## Self-test

    python3 tools/memcheck/selftest.py [--no-valgrind]

It needs no build of the library. It checks that the three snippets of the reproducers that the
checker must report (`s0_control.c`, `s1_use_then_init.c`, `s4_adf_use_then_init.c`) are reported,
at the line of the use; that correct code, also with a `memset` before the init, gives nothing; that
the three snippets of the limits give nothing, so that the list above is the behaviour of the code;
and that `src/*.c tests/*.c tools/adf/adf.c` give no finding. With valgrind it also checks that
`run.sh` finds it although it is not on `PATH`, and that valgrind reports an error for each of the
five snippets that build against FLINT alone (so that `s2`, `s3` and `s5` are real defects the
checker misses, not clean code). `--no-valgrind` skips that part.

## Results

- Old `tests/test_recon.c` (`git show cb268ba^:tests/test_recon.c`): the checker reports the
  five missing initialisations (`fa`, `fN`, `glo`, `ghi`, `out`, at uses 1248, 1249, 1252,
  1252, 1253) and their five `clear-before-init` calls. See the lane report.
- The current tree, `src/*.c tests/*.c tools/adf/adf.c`: 0 `use-before-init`, 0 `clear-before-init`,
  0 `init-without-clear` (`selftest.py` checks this). The one-method variant of the review
  (`docs/reviews/m1/surface/checks/memcheck/ordered_check.py`) found 0 uses before the first init in
  the tree as well: the tree has no defect of the shape of `s1` today, and the tool now could tell.
