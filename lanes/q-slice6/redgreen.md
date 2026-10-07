# Red and green runs

All commands run from the repository root. Build commands use at most two jobs.

1. Vectors: `timeout 180 python3 lanes/q-slice6/gen_vectors.py` initially failed its grid assertion
   at samples-not-coset-3. The grid included all coset integers it sampled as point exceptions.
   Enlarging it beyond every exception plus two common moduli gave 279 vectors, 151816 bytes,
   414620 direct membership pairs, 558 independent containment checks, and one huge-B limit row.
   No oracle was changed.
2. Initial red: `timeout 180 make -j2 BUILD=lanes/q-slice6/plain
   lanes/q-slice6/plain/test_qclass_sets` exited 2: three undefined query symbols.
3. Equality green: after implementation, `timeout 180 lanes/q-slice6/plain/test_qclass_sets 1`
   exited 0: 51930 checks, 2795 calls. Compilation fixes: a label typo, initialized K/L,
   and separate finite/real reader pointers to avoid GCC's member-array warning.
   A test exponent was corrected to exceed the actual ARF exponent cap rather than its shift.
4. Containment red: `timeout 180 lanes/q-slice6/plain/test_qclass_sets 2` exited 134:
   glue-2-0 containment returned the temporary LIMIT stub at budget 28; expected OK, truth 0.
5. Containment green: the same command exited 0: 59475 checks, 5590 calls.
6. Overlap red: `timeout 180 lanes/q-slice6/plain/test_qclass_sets 3` exited 134:
   glue-2-0 overlap returned the temporary LIMIT stub at budget 28; expected OK, truth 1.

Each implementation change was rebuilt with the same make target under timeout 180.
Further green runs, integration reds, and faults are recorded below as they finish.

7. Overlap green: `timeout 180 lanes/q-slice6/plain/test_qclass_sets` exited 0:
   67020 checks, 8385 calls before added component checks.
8. Driver red: `timeout 180 make -j2 -C tools/adf`, then
   `timeout 30 build/adf < tests/driver/qclass-sets.cmd` produced 22 PARSE lines.
   The first 21 differed from the hand-written expected output; the missing-limit case already expected PARSE.
9. Driver green: the same build and fixture run after adding commands matched all 22 lines by `diff -u`.
   `timeout 180 sh tests/test_driver.sh` exited 0: 73 cases, 101370 expected lines.
10. Added preflight-allocation observation, exact-gap versus Q1, local-backend and saturation checks.
    Sorted finite point candidates once globally, retaining O(K L) fibre work at each cell.
    The final C suite gives 69558 checks and 8393 calls in plain, SAN and clang builds.
    INV and clang SAN+INV give 69576 checks, 8393 calls and six invalid-input aborts.
    Build: `timeout 180 make -s -j2 BUILD=lanes/q-slice6/<config> <flags>
    lanes/q-slice6/<config>/test_qclass_sets lanes/q-slice6/<config>/test_qclass_reduce`.
    Run: `timeout 180 lanes/q-slice6/<config>/test_qclass_sets`.
    SAN runs use ASAN_OPTIONS=detect_leaks=0 after the default LeakSanitizer failed under ptrace.
    The initial INV compilation caught an unchecked freopen return in the test; it was checked and rebuilt.
11. `timeout 180 python3 lanes/q-slice6/plant_faults.py` compiled and ran ten separate SAN+INV faults.
    Nine aborted on assertions; the word-overflow fault failed UBSan. Zero survived or failed compilation.
12. Julia first failed to load FLINT because Julia's bundled GMP lacks __gmpn_modexact_1_odd.
    `LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 timeout 60 julia --startup-file=no
    tests/julia/qclass_sets.jl lanes/q-slice6/libadelefeld.so` passed 32 tests.
    The registration includes the same conditional retry as the existing Julia tests.

13. Differential run: `timeout 180 python3 lanes/q-slice6/differential.py
    lanes/q-slice6/libadelefeld.so 120` exited 0: 109214 pairs, 1310568 calls,
    1093 brute grids, 343144 membership pairs, zero disagreements in 120.000 s.
14. Mutation sweep, seed 310606, limit 40: 30 killed, eight survived, two not compiled.
    Surviving initialization, cleanup, denominator-bound and status mutations led to component checks.
    Byte-product refusal now precedes endpoint streaming; exterior empty cells are skipped.
    `timeout 180 python3 lanes/q-slice6/check_survivors.py` repeats the eight changes:
    five killed, three equivalent survivors, zero compile failures. See report.md for each survivor.
15. Final sweep, seed 310608, limit 12: ten killed, one survived, one not compiled.
    Added numerator/denominator cap checks; `timeout 180 python3 lanes/q-slice6/check_size_survivor.py`
    killed the size-guard survivor by assertion. No source guard was weakened.
16. Final suites: 86357 checks, 8394 calls in plain, SAN and clang; 86375 checks in INV and clang SAN+INV.
    All five builds used the same make and timeout commands above. The reducer gives 223472 checks,
    or 223475 with INV. Initial component input 2^58 fit this file's smaller piece struct;
    the refusal test was corrected to 2^59, which exceeds its actual byte capacity.
17. The literal point-comparison fault first failed compilation because yx became unused.
    Reading it through a void cast makes that deliberately wrong implementation compile.
    Final planted-fault run: eleven compiled, eleven killed, zero survivors, zero compile failures.
18. Final driver: 73 cases, 101370 lines. Final Julia: 32 passes.
    Post-change differential: 47917 pairs, 575004 calls, 480 brute grids,
    152536 membership pairs, zero disagreements in 30.001 s.
