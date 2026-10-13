# Red and green runs, lane t5-slice5 (slice 5e)

1. Test of the side evaluation, `tests/test_tate_fe.c`. The test was written in full (vectors, goldens, statuses,
   poles, hand values, independence, widths). Red: a scratch copy with `fe_sides` reduced to `return
   ADF_UNSUPPORTED` (`lanes/t5-slice5/scratch/test_tate_fe_stub.c`, removed afterwards), built against
   `lanes/t5-slice5/build/libadelefeld.a`, failed as an assertion:
   `test_tate_fe_stub.c:362: fe_sides(a.z, ..., one, s, -1, 64, 0) == ADF_DOMAIN && which == 'A'` (exit 1).
   Green: the recipe of design section 5 in `fe_sides`. The first green run failed at the count check
   `fe_want == 248`: the raw golden (10,3) lowers to conductor 5, like (8,7) to 4, so 8 rows (not 7) get the
   extra bits 20 and 80: 264 cases. The constant and its comment were corrected (no value check had failed).
   Then `tate_fe: 18412 checks` (6.1 s).
2. Driver command `tate_fe`. The fixture `tests/driver/tate-fe.cmd` / `.out` (15 lines) was derived by
   `lanes/t5-slice5/derive_fixture.py` before the command existed. Red: `sh tests/test_driver.sh` stopped at
   `tate-fe` with 15 lines `error: PARSE` (unknown operation word). Green: `adf_drv_tate_fe` in `tools/adf/adf.c`;
   `test_driver: 100 cases, 101691 expected lines, all equal (SAN=0)` on the first run after the code.
3. Julia `tests/julia/tate_fe.jl` calls existing library functions only (no new library code in this slice), so
   it has no red stage: it passed 20 of 20 on its first run. Its expected texts were derived before the run by
   `derive_fixture.py`.
4. Later additions to the test, each run once and passing: the bits 20 and 80 cases for C = 15 (the oracle's pair
   (15,2); 280 cases), and the section `guard` (B alone failing). `tools/memcheck/check_uninit.py` then found an
   uninitialised `mag_t` in the test's helper `ratio` (use and clear before init; ASan had not reported it); fixed
   with `mag_init`. Final: `tate_fe: 19126 checks`.
