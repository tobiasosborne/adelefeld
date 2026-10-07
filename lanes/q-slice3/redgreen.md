# Red-green record

Commands run from the repository root. Each test executable is under timeout.
Build command B:

    timeout 180 make -s -j2 BUILD=lanes/q-slice3/build lanes/q-slice3/build/test_psi

Initial B: exit 2, undefined references to all five public functions.
Two preliminary builds found the wrong target name and an acb_contains_int signature error.
Those are test setup errors, not a red contract result.

After each code step, B returned 0.

| Call | Red run | Result | Green run | Result |
|---|---|---|---|---|
| fball phase | initial B | exit 2, missing symbol | timeout 30 lanes/q-slice3/build/test_psi finite | exit 0, 4 |
| adele phase | timeout 30 lanes/q-slice3/build/test_psi adele | exit 1, OK assertion | same | exit 0, 6 |
| phase_get_acb | timeout 30 lanes/q-slice3/build/test_psi phase | exit 1, OK assertion | same | exit 0, 416 |
| default | timeout 30 lanes/q-slice3/build/test_psi default | exit 1, enclosure assertion | same | exit 0, 6 |
| strict | timeout 30 lanes/q-slice3/build/test_psi strict | exit 1, OK assertion | same | exit 0, 5 |

The unimplemented calls initially returned NOT_DETERMINED without writes, allowing assertion reds
after the first link red. No such stub remains in the final source.

## Resumed verification, 2026-10-07

The table above was inherited from the stopped run. Its historical reds were not rerun as original builds.
The resumed baseline B and `timeout 150 lanes/q-slice3/build/test_psi` both returned 0: 172069 checks.
The strict stage above was rerun and returned 0: 5 checks.

Added independent input membership checks, phase endpoint bounds at p=2,20,53,128, deep failure snapshots,
resource boundaries, and numerical certificate failure at the cap. The first input-membership test failed
at line 205: a 512-bit conversion cannot be contained in a 2000-bit exact real point. This was a test
setup defect. The test now compares exact rational endpoints instead; no production code was changed.

B plus `timeout 150 lanes/q-slice3/build/test_psi`: exit 0 after the successive additions, 186717,
186744, then 186772 checks. The named faults record later assertion reds against the complete test,
followed by the pristine normal/SAN/INV/clang greens. See fault-results.tsv and the final report.

First INV build: exit 2 for ignored freopen return under -Werror. The child checks freopen now.

The automatic sweep ran 60 mutants with the test frozen: 38 killed, 14 survived, 7 compile failures,
1 timeout. The source remained unchanged. Rollover and inclusive-bit-boundary assertions were added.
The public test then returned 0 with 186799 checks under SAN+INV+clang.

`ASAN_OPTIONS=detect_leaks=0 timeout 150 python3 -B lanes/q-slice3/recheck_survivors.py \
  /tmp/adf-q-slice3-pristine`: exit 0; 14 original survivors rechecked, 7 assertion kills, 7 equivalent.
Each pristine run: 186799 public checks, 37 instrumented checks, 0 unmatched q/arf temporaries.
The lifecycle fault has qlive=1 and arflive=1; the instrumented certificate faults fail assertions.

The timeout-only recheck used the same script with `timed-out` and timeout 40: exit 0, 1 assertion kill.
The pristine instrumented and lifecycle tests returned 0 before and after that recheck.
The named-fault script's final run returned 0: 11 faults, each test exited 1 on an assertion.
Six are the slice's design faults, four are the brief's faults, and one extra returns the universal square.
All scratch copies were removed by the scripts. Final whole-tree results are in report.md.
