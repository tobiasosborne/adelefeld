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
| strict | timeout 30 lanes/q-slice3/build/test_psi strict | exit 1, OK assertion | same | recorded below |

The unimplemented calls initially returned NOT_DETERMINED without writes, allowing assertion reds
after the first link red. No such stub remains in the final source.
