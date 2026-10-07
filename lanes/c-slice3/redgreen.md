# Red/green record

All programs and scripts below ran under timeout. Builds used at most two jobs.

- `timeout 120 python3 -B lanes/c-slice3/gen_vectors.py`: first exit 1 at the size guard,
  422853 bytes. Integer endpoints on the 10^-60 grid remove duplicated denominators.
  Final exit 0: 277546 bytes, 3113 records across six files. No numerical data was dropped.
- Initial test compile failed: one missing parenthesis and a nonexistent acb_contains_si call.
  Those test compile defects were corrected before the first link red.
- `timeout 60 make -s -j2 BUILD=lanes/c-slice3/plain lanes/c-slice3/plain/test_char_class`:
  first link red, exit 2, all four new functions undefined (red-link.log).
- Default class implementation, with deliberately wrong strict and idele stubs:
  `timeout 60 lanes/c-slice3/plain/test_char_class class`: exit 0, 693680 checks.
  The `strict` invocation aborted (134), rejecting ambiguous default acceptance at line 118.
  The `idele` invocation aborted (134), rejecting omitted real sign at line 161.
- Correct strict class and default idele:
  `timeout 60 lanes/c-slice3/plain/test_char_class strict`: exit 0, 283325 checks.
  The `idele` invocation aborted (134), rejecting default behavior in the strict wrapper, line 164.
- Correct strict idele wrapper: the subsequent full test result is recorded below.
  `timeout 60 lanes/c-slice3/plain/test_char_class`: exit 0, initially 1000323 checks.
- Added conversion failures, allocation observations, injected certificates and INV member aliases.
  Two test compile defects were fixed: arb_set_si_si was a helper from test_idclass rather than
  a public FLINT function; the INV conditional mixed acb and arb pointer types before conversion.
  The final plain test has 1001857 checks; INV has 1001953. All 32 INV children abort.
- The final class wrapper has 1001951 plain checks and 1002039 INV checks, all passing.
- `timeout 180 sh tests/test_driver.sh`: exit 0, 90 cases, 101567 expected lines, 0 differences.
  The 21 new expected lines were written before running. The command implementation preceded
  that first execution; no separate driver assertion red is claimed.
- `JULIA_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 timeout 180 sh tests/test_julia.sh`:
  exit 0; new class test 17/17, exports 595/595. The existing system-GMP preload retry is used.
- `timeout 180 python3 -B lanes/c-slice3/plant_faults.py`: exit 0; six object builds and six
  links exit 0; all six wrapped test programs abort (-6), no timeout.
- `timeout 180 python3 -B lanes/c-slice3/run_checks.py CONFIG`, CONFIG=plain,san,inv,clang:
  all final configurations exit 0. The first INV compile exit 2 is retained in checks.json.
  test_char/test_char_eval are unchanged; every new status failure keeps sentinel bytes unchanged.
- Leak detection attempt: LeakSanitizer reports that it cannot run under ptrace.
  ASan/UBSan runs use detect_leaks=0; the FLINT allocation balance observation is separate evidence.

## Final extensions and results

- family_hull.py adds a finite-candidate proof for continuous t/s families. The final generator
  exits 0, with 360805 bytes and 3113 records. There are 40 family images and 160 certified extrema.
  Every family extremum interval is at most two units wide on the 10^-60 grid.
- A new assertion initially required containment of an entire independently rounded reference box.
  It aborted at value id 508, coordinate 2. The singleton-i family has true imaginary minimum
  about -1.4627698786, while the reference box extends to -1.5779842754. That extra bound is not
  an attained value. The corrected test checks every point witness and all certified hull extrema.
  Whole-family reference enclosures remain in the image records; the rectangle width bound remains.
- The first class-only printed-radius check did not kill removal of the finite radius successor:
  acb multiplication added another rounding ulp. A direct finite-hull print check rejects it.
- Mutation gaps: principal constructor setup, heap Gauss accumulator cleanup and finite printed radius.
  New checks reject all three. The implementation source was not changed by those repairs.
- Final class-only rechecks of plain, INV, SAN and Clang all exit 0. Counts are 1208883 in
  plain/SAN/Clang, 1208979 in INV; wrappers have 1208981 and 1209069 respectively.
- The final six scratch faults again compile/link, then abort (-6), with no timeout.
- Final driver rerun: 90 cases, 101567 expected lines, 0 differences, exit 0.
- Mutation batches 1/2 complete in 137.9/137.7 seconds; batch 3 stops at 180.1 seconds.
  All ten batch-3 candidates are then judged individually. The first replay script itself
  reaches timeout 180 after those ten; the remaining two survivors are rerun separately.
  Fifteen targeted slice c candidates all fail. The radius-only replay, timeout 30, fails too.
  Final distinct totals: 45 candidates, 37 killed, 7 not compiled, 1 survivor, 0 unjudged.
- Final LeakSanitizer attempt exits 1 with the ptrace limitation. No leak-detector pass is claimed.
- Final style/cleanup script exits 0: 13 full authored files and appended char.c checked,
  0 overlong lines, 0 missing newlines, 0 remaining lane build/fault trees,
  0 logs above 100000 bytes. Machine JSONL/log records retain their required single-line form.
