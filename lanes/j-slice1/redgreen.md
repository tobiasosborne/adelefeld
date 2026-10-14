# Red and green runs of lane j-slice1

## Part 1, J-status (C)

1. Red. `tests/test_status.c` test `the_status_count_accessor_returns_the_macro` appended (4 checks), header
   unchanged. `timeout 600 make -s -j2 BUILD=lanes/j-slice1/b-plain lanes/j-slice1/b-plain/test_status` ->
   `error: implicit declaration of function 'adf_status_count'` (-Werror), build failed. A missing symbol is the
   permitted red for the first test of a new function (lanes/COMMON-C.md rule 1).
2. Green. `adf_status_count` added to `include/adelefeld/status.h` after `adf_status_str`. Same command, then
   `timeout 60 lanes/j-slice1/b-plain/test_status` -> `8 tests, 126 checks, 0 failed checks, 0 failed tests`.
3. Fault (return 10 instead of ADF_STATUS_COUNT), planted and removed by sed: `8 tests, 126 checks, 4 failed
   checks, 1 failed tests` (all four new checks fail). Restored; green again.
4. Checks of the header addition: `test_status` built into lanes/j-slice1/b-{plain,san,inv,clang} with
   `make -s -j2 [SAN=1|INV=1|CC=clang] BUILD=...`: each `8 tests, 126 checks, 0 failed checks, 0 failed tests`.
   `timeout 600 sh tests/test_exports.sh` -> `passed: 625 of 625 declared functions are exported`.

## Part 2, the Julia package (gate: lanes/j-slice1/gate.sh, the command of docs/api-j.md section 9)

1. First attempt of the red run: `JULIA_DEPOT_PATH=lanes/j-slice1/depot:$HOME/.julia` (no trailing colon)
   -> exit 124 after 180 s while precompiling Pkg and 23 standard libraries into the fresh depot. Depot deleted;
   the gate now uses `...:$HOME/.julia:` (the trailing colon appends Julia's bundled depots).
2. Red. Tests, fixture and subprocess script written; `src/Adelefeld.jl` an empty module. Gate -> exit 1,
   `Adelefeld step J-a.1 | Error 43 Total 43` (every testset errors: `diagnostics`, `Rat`, ... not defined).
   Log: red.log.
3. First green attempt: `852 passed, 17 failed`: one test bug (a tuple compared with a vector) and 16 failures
   of the fixture cases: the fixtures had no DT_NEEDED on the library (`--as-needed` dropped it), so every case
   reported `adf_version_check` missing, and the subprocess printed the error in an old world age (repr instead
   of the package's showerror). Fixed in the tests (`-Wl,--no-as-needed`, `invokelatest`). Log: green1.log.
4. Green: `869 / 869` (green2.log); with the launcher test `871 / 871` (green3.log); with the aligned
   allocation tests `904 / 904` (green4.log).

## Faults planted in the Julia code (lanes/j-slice1/faults.py; one at a time, gate run, file restored)

See faults.log. 9 of 10 caught; F3 (free the adjusted pointer instead of the base) survives: glibc malloc
returns 16-aligned addresses and the rat alignment is 8, so the adjusted pointer equals the base on this
machine and no test can tell them apart; the proof in `aligned_alloc` covers it.
