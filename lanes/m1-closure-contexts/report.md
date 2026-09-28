# Lane m1-closure-contexts report

4 CLOSED; 1 CLOSED WITH EDIT; 1 OPEN; 0 SETTLED BY DECISION. NO BLOCKER OPEN.

## Work done

Rebuilt the library, reran every unchanged contexts reproducer, checked the landed code and
accepted M1-D5, attacked neighbouring inputs, and tested four repaired conditions against
scratch reversions. The closure judgment is `docs/reviews/m1/contexts/closure.md`. R5 stays
OPEN because m1-repair-tools has not landed. R2 is CLOSED WITH EDIT because the status row in
`docs/conventions.md:188` omits the M1-D5 block cap; the exact edit is in the closure.
No production file or original reproducer was edited.

## Files written and retained

- `docs/reviews/m1/contexts/closure.md`
- `docs/reviews/m1/contexts/closure-checks/context_dump_cases.py`
- `docs/reviews/m1/contexts/closure-checks/red_green.py`
- `docs/reviews/m1/contexts/closure-checks/primorial_time.sh` (unchanged copy of the old script,
  placed beside its binary so its relative path resolves)
- `docs/reviews/m1/contexts/closure-checks/valgrind_get_fball.log`
- `lanes/m1-closure-contexts/report.md`

Temporary binaries and generated oracle output under `closure-checks/` were removed after the checks.
Scratch reversions and their objects were under `/tmp` and were removed by the harness.

## Commands and results

Read-only `cat`, `rg`, `sed`, `nl`, `ls`, `du`, `find`, and `awk` inspections of the named rules,
review, reports, headers, sources, tests, references, and output files exited 0. `free -g`
showed 23 GB available before the first build and before focused test compilation.

In this section, `checks/` abbreviates `docs/reviews/m1/contexts/checks/` and
`closure-checks/` abbreviates `docs/reviews/m1/contexts/closure-checks/`.
The seven-program compilation used this command:

```sh
for n in ctor_cases dump_run get_fball_into_local primorial_time reduce_run scaled_ops set_context_alias; do
    cc -std=c11 -O2 -g -Iinclude docs/reviews/m1/contexts/checks/$n.c \
        build/libadelefeld.a -lflint -lgmp -lm -lpthread \
        -o docs/reviews/m1/contexts/closure-checks/$n || exit
done
```

The baseline compilation used `cc -std=c11 -O1 -g` with the original baseline source,
`-lflint -lgmp -lpthread`, and an output in `closure-checks/`.

- `make clean && make -j2`
  Result: Exit 0; 14 source objects and `build/libadelefeld.a` built.
- `cc -std=c11 -O2 -g -Iinclude checks/NAME.c ...` for seven original C reproducers
  Result: Seven compiled; exit 0.
- `cc -std=c11 -O1 -g checks/flint_threads_baseline.c ...`
  Result: Compiled; exit 0.
- `bash closure-checks/primorial_time.sh`
  Result: Eight calls: exit 0, all UNSUPPORTED, 0.00 s, maxrss 2-3 MB.
- `closure-checks/set_context_alias`
  Result: 7,200 cases; wrong plain 0, wrong alias 0; exit 0.
- `python3 checks/excuse_lines.py`
  Result: 28 excuses; 1 right line, 27 stale; exit 0.
- `bash checks/ground_truth.sh`
  Result: Exit 0; fixed line labels stale; claims checked by hand.
- `closure-checks/ctor_cases > ctor.out`
  Result: Exit 0; 8,115 case records; untouched_bad 0.
- `python3 checks/ctor_oracle.py ctor.out`
  Result: 8,115 calls, 0 mismatches; exit 0.
- `closure-checks/reduce_run > reduce.out`
  Result: Exit 0; 8,800 records.
- `python3 checks/reduce_oracle.py reduce.out`
  Result: 8,800 calls, 0 wrong; exit 0.
- `closure-checks/reduce_run threads 3000`
  Result: 11 contexts, 4 threads x 3,000 round trips, 0 wrong.
- `closure-checks/scaled_ops SEED 3000 > scaled_SEED.out`, SEED 1, 2, 3
  Result: 49,600 records each; exit 0.
- `python3 checks/scaled_oracle.py scaled_SEED.out`, SEED 1, 2, 3
  Result: 49,600 checked per seed; 0 failures.
- `python3 checks/dump_diff.py closure-checks/dump_run 20000`
  Result: 20,086 inputs; 0 C failures, 0 reference differences, 49 old-oracle differences.
- Four nested-body dumps piped to `closure-checks/dump_run` at occurrences 0..2
  Result: 12 cases; 0 C failures.
- `python3 closure-checks/context_dump_cases.py closure-checks/dump_run`
  Result: 95 cases, 0 wrong, 0 C failures.
- Six calls of `closure-checks/primorial_time N E` near overflow and block cap
  Result: 4 OK, 2 UNSUPPORTED; times 0.00-0.63 s.
- `make -j2 build/test_scaled_alias build/test_modctx_limits build/test_dump_ctx build/test_modctx`
  Result: Exit 0; four tests built.
- `build/test_scaled_alias`
  Result: 9 tests, 142,938 checks, 0 failed.
- `build/test_modctx_limits`
  Result: 7 tests, 239 checks, 0 failed.
- `build/test_dump_ctx`
  Result: 14 tests, 28,587 checks, 0 failed.
- `build/test_modctx`
  Result: 11 tests, 10,197 checks, 0 failed.
- `closure-checks/get_fball_into_local`
  Result: Two cases, 0 wrong; exit 0.
- `closure-checks/flint_threads_baseline`
  Result: Exit 0, printed `done`.
- `bash checks/mutants_698_710.sh`
  Result: Four builds/runs; no mutation due stale lines; 0 failed of 10,197 checks each.
- `valgrind --error-exitcode=1 --leak-check=full --errors-for-leak-kinds=definite,indirect
  closure-checks/get_fball_into_local`
  Result: Exit 0; 0 errors, 0 bytes live.
- `python3 closure-checks/red_green.py`
  Result: Exit 0; R1 old 4,865 wrong; R2 old signal 11; R3 old 61 wrong; R4 old 20 wrong; R5 dropped multiply
  killed by oracle.
- `cc -std=c11 -O0 -g -fsanitize=address,undefined ... checks/dump_run.c src/*.c ... -o closure-checks/dump_run_san`
  Result: Exit 0.
- `ASAN_OPTIONS=detect_leaks=0 python3 closure-checks/context_dump_cases.py closure-checks/dump_run_san`
  Result: 95 cases, 0 wrong, 0 sanitizer reports.
- `find closure-checks -maxdepth 1 -type f -name '*.out' -delete`
  Result: Six generated oracle output files removed.
- `find closure-checks -maxdepth 1 -type f -executable ! -name '*.sh' ! -name '*.py' -delete`
  Result: Nine generated binaries removed; the unchanged shell copy was kept.
- `awk 'length($0)>116 ...'` on retained Markdown and Python
  Result: 0 lines above 116 characters.
- `python3` with `ast.parse` on the two new scripts and line checks on closure and report
  Result: 2 scripts parsed; 0 lines above 116 characters.

The first `red_green.py` run exited 1 because its harness used `check=True` for a deliberately
failing old-code run. The harness was corrected. The next two runs exited 0; the final run
includes the R5 drop-call check. The unchanged `mutants_698_710.sh` cannot test its intended
mutants at the current line numbers, and no result was inferred from those four passing runs.

## Test quality

R1's repaired test is red on the old loss order (4,865 failures) and its expected loss follows
integrality of `u K'/K`, not the implementation. R2's size test exits on signal 11 without
the `k` guards. Its large-n expected status comes from M1-D5, but its overflow-threshold
helper shares FLINT `n_root` with the production code. An independent `e = 4` boundary run
covered `n = 65536` and `65537`. The R3 and R4 repaired conditions were reverted in scratch
objects: the independent 95-case check then failed 61 and 20 cases, respectively. Its
expected contexts and statuses were read from `docs/conventions.md` 10.1 and 10.2.
The R6 range pin is green on this FLINT even without the new modulo reduction, so it does
not prove that line necessary; the range follows directly from `fmpz_fdiv_r`.

## Not done

The 65,536-block admitted construction was not rerun; the repair lane reported over five
minutes. No full mutation run or coverage-guided fuzzing was run. m1-repair-tools and
m1-invariants have not landed; their memory checker and `ADF_CHECK_INVARIANTS` were not examined.
The contexts review did not examine the later-stage dump bodies beyond the 95 focused cases.

## Sources pending

- [source pending: FLINT 3.0.1 documentation of `n_root`]
- [source pending: FLINT 3.0.1 documentation of `fmpz_multi_mod_precomp`]
- [source pending: definition of the `sign` argument of `fmpz_multi_CRT_precomp`]
- [source pending: FLINT 3.0.1 range and alias rule for `fmpz_fdiv_r`]

The last argument is no longer needed to prove `[0,K)` because the repaired code reduces
modulo `K` after the FLINT call.

## Findings against the specification

None found. The remaining R2 edit concerns the older status summary in `docs/conventions.md`;
M1-D5 in `docs/SPEC.md` states the block cap.
