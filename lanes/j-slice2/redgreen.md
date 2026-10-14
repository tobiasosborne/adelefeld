# Red and green runs of lane j-slice2

Gate: `sh lanes/j-slice2/gate.sh` (the command of `docs/api-j.md:409-410`, with the lane's depot, the trailing
colon on `JULIA_DEPOT_PATH`, `JULIA_PKG_OFFLINE=true`, TMPDIR in the lane, one thread, one precompile task, under
`timeout 180`). Library: `build/libadelefeld.so` from `timeout 600 sh tests/test_exports.sh` (exports.log:
625 of 625 declared functions exported).

0. Before any change (before.log): exit 0, `Adelefeld step J-a.1 | 904 904`, "tests passed".
1. Red (red.log). The testset "Adelefeld step J-a.2" appended to `test/runtests.jl` (18 testsets) and two
   fixture faults added to `test/fixtures/fault.c` (FAULT_FBALL_SIZE, FAULT_FBALL_ALIGN); `src/` unchanged.
   Gate -> exit 1: J-a.1 904 of 904 pass; `Adelefeld step J-a.2 | Pass 5, Fail 20, Error 45, Total 70`.
   Every testset that uses FBall errors (`FBall not defined`); the two fixture cases fail (the library loads,
   since no fball row is gated); the launcher session fails. The 5 passes are checks that hold without the
   step (for instance `detect_ambiguities` empty and the fixture subprocesses' exit code 0).
2. Green (green1.log). `src/fball.jl` written; `src/status.jl` (PointComparison, to_point_comparison, to_bool),
   `src/load.jl` (row fball 48/8, 23 symbols), `src/Adelefeld.jl` (include, 10 exports). Gate -> exit 0,
   `Adelefeld step J-a.1 | 904 904`, `Adelefeld step J-a.2 | 1015 1015`, "tests passed". The first green attempt
   passed; no test was changed after the red run.
3. Final (final.log), after the documentation edits: exit 0, 904 of 904 and 1015 of 1015, "tests passed".

## Faults (lanes/j-slice2/faults.py, faults.log, fault-F*.log)

Each fault was planted in a scratch copy of the package (lanes/j-slice2/scratch, deleted after each run), the gate
was run on the copy with `ADELEFELD_LIB=build/libadelefeld.so`; `julia/Adelefeld` was never edited by the script.
Two batches: F1-F7 in 4 min 45 s, F8-F14 in 7 min 8 s. 12 of 14 caught; F6 (equal_set by identical) is an
equivalent mutant for global values; F7 (GC.@preserve omitted) cannot be forced by a test. Numbers in the
result and in faults.log.
