# Lane m1-memcheck: uninitialised FLINT / adelefeld values in tests and library

Issue adf-tgx. Read: `CLAUDE.md`, `lanes/COMMON-C.md`, `tests/README.md`, the `Makefile`,
`lanes/m1-cap-chain/report.md` (last sections).

## What was done

1. Checked what is installed. `valgrind` is absent. `clang` 18.1.3 is present, but
   MemorySanitizer is not usable on this host: a trivial program with a plain uninitialised
   `int` read aborts with `MemorySanitizer: stack-overflow` (a known clang/ASLR fault), also
   under `setarch $(uname -m) -R`. Even if it ran, FLINT and GMP are not instrumented, so every
   value that comes back from FLINT would be reported as uninitialised. No useful
   MemorySanitizer run is possible, and none was faked.
2. Because neither runtime tool is usable, wrote the static checker
   `tools/memcheck/check_uninit.py` (as the brief requires in any case) and made
   `tools/memcheck/run.sh` dispatch to it, while still containing the full valgrind path for a
   host that has valgrind.
3. Red-green: the checker reports the five unimplemented `fmpq` values in the old
   `tests/test_recon.c` and reports nothing for the new one.
4. Ran the checker over all of `src/*.c` and `tests/*.c`. It parsed 595 functions and 484
   target-type declaration tokens; the current tree has no finding in any of the three
   categories.

## Files written

- `tools/memcheck/check_uninit.py` -- the static checker.
- `tools/memcheck/run.sh` -- the runner (valgrind path, MemorySanitizer probe, checker
  fallback).
- `tools/memcheck/README.md` -- what the tool does, what this host has, how to run it, its
  limits and its results.
- `lanes/m1-memcheck/report.md` -- this file.

No file outside `tools/memcheck/` and `lanes/m1-memcheck/` was changed.

## Checks run

Environment:

    command -v valgrind            -> no output (valgrind: absent)
    clang --version | head -1      -> Ubuntu clang version 18.1.3 (1ubuntu1)
    clang -fsanitize=memory on a trivial uninitialised-read program
                                   -> MemorySanitizer:DEADLYSIGNAL,
                                      ERROR: MemorySanitizer: stack-overflow

Red-green for the tool. Old file is `git show cb268ba^:tests/test_recon.c` (sha256
`b74419c6d1...`), written to `/tmp/test_recon_old.c`.

    python3 tools/memcheck/check_uninit.py --category use-before-init /tmp/test_recon_old.c
    -> 5 findings, exit 1:
       1248: fa: first use `fa` (declared line 1191)
       1249: fN: first use `fN` (declared line 1191)
       1252: glo: first use `glo` (declared line 1191)
       1252: ghi: first use `ghi` (declared line 1191)
       1253: out: first use `out` (declared line 1191)

    python3 tools/memcheck/check_uninit.py --category use-before-init tests/test_recon.c
    -> no output, exit 0

    python3 tools/memcheck/check_uninit.py /tmp/test_recon_old.c
    -> use-before-init: 5, clear-before-init: 5, init-without-clear: 0

    python3 tools/memcheck/check_uninit.py tests/test_recon.c
    -> use-before-init: 0, clear-before-init: 0, init-without-clear: 0

The only difference between the two revisions of the file is the five inserted `fmpq_init`
calls:

    git diff --stat cb268ba^ -- tests/test_recon.c
    -> tests/test_recon.c | 5 +++++, 1 file changed, 5 insertions(+)

Whole tree:

    tools/memcheck/run.sh --checker          # = check_uninit.py src/*.c tests/*.c
    -> # use-before-init: 0
       # clear-before-init: 0
       # init-without-clear: 0
       exit 0

Category self-test on a synthetic file with one deliberate defect of each kind
(`/tmp/synth.c`): `use-before-init: 1`, `clear-before-init: 1`, `init-without-clear: 1`,
each on the intended variable; a correct function and a function fed by a helper that calls
`fmpq_init` produce no finding. This checks that the clean whole-tree result is not a silent
failure.

`bash -n tools/memcheck/run.sh` -> ok. `python3 -m py_compile` on the checker -> ok.

The valgrind branch of `run.sh` (build into `build-memcheck/` at `-O1 -g`, run each of the
test binaries under `valgrind --error-exitcode=9 --track-origins=yes --leak-check=full -q`,
2 at a time, 120 s limit each, one line per program) is present but was not executed, because
valgrind is not installed on this host.

## Findings

The current tree is clean: no `use-before-init`, no `clear-before-init`, no
`init-without-clear`. The defects in the old revision were confirmed by reading the code:

- `tests/test_recon.c`, old revision, test `an_adele_with_operands_of_4096_bits`: `fmpq_t fa,
  fN, glo, ghi, out;` at line 1191; used at 1248 (`fmpq_set(fa, a->q)`), 1249
  (`fmpq_set(fN, N->q)`), 1252 (`real_interval(glo, ghi, ...)`) and 1253
  (`expected_status(out, ...)`) with no `fmpq_init`; cleared at 1284-1288 with no init.
  Confirmed.
- Current `tests/test_recon.c` initialises all five at lines 1205-1209 and clears them at
  the end. Confirmed.

Notes on the checker, for honesty:

- It first reported 25 `use-before-init` findings over the tree. All 25 were locals of
  `adf_place_t` (for example `src/place.c:44`, `tests/test_place.c:44`) and
  `adf_text_limits_t` (`src/text.c:432`, the `lim` locals of the text tests). Reading the
  headers, both are plain by-value structs with no init function
  (`include/adelefeld/place.h:20-24`, `include/adelefeld/text.h:67-74`). They are excluded
  from the target types; the 25 were false positives of an over-broad `adf_*_t` rule.
- The checker also needed interprocedural helper inference: `adf_adele_t x` initialised by
  `adele_build_small(x, ...)` is not a `*_init*` call, so a purely local rule first reported
  `tests/test_recon.c:971: x`. The checker now computes which arguments each helper
  initialises to a fixpoint; `x` is correctly recognised as initialised.
- The checker is syntactic and per translation unit. It can miss a use behind a macro or
  through a pointer, and it does not track a cross-file initialiser that is not named
  `*_init*`. In this tree it parsed 595 functions and 484 target declarations.

## What is not done

- No memory run of the tests happened: valgrind is absent and MemorySanitizer is unusable
  here. The valgrind branch of `run.sh` is written but unexecuted.
- The checker was not run under a second implementation and has no unit-test suite of its
  own beyond the synthetic file above and the old/new `test_recon.c` pair.

## Sources pending

None. The statements used were read on disk: the FLINT type and init facts from the FLINT
headers as they are named at the places of use, and the adelefeld type facts from
`include/adelefeld/place.h` and `include/adelefeld/text.h`.

## Findings against the specification

None. Nothing in `docs/SPEC.md` was found to be wrong or unprovable in the part this lane
covers.

## Note on the brief (not against the specification)

The brief lists the target types as `fmpz_t`, `fmpq_t`, `arb_t`, `acb_t`, `arf_t`, `mag_t` and
`adf_*_t`. Two of the adelefeld types, `adf_place_t` and `adf_text_limits_t`, are plain
by-value structs with no init or clear; a strict `adf_*_t` rule reports every use of them and
is wrong. The checker targets the adelefeld types that own memory
(`adf_adele_t`, `adf_cadele_t`, `adf_fball_t`, `adf_rat_t`, `adf_scaled_t`,
`adf_ctx_desc_t`), and this is written in the README.
