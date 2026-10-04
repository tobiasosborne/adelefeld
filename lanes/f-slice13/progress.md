# f-slice13 running notes

Read CLAUDE.md, workflow and lane rules, SPEC 5, 9.3.7 and N-D16 to N-D18,
catalogue Propositions 10 to 15, conventions and the symbol, power, driver and Julia patterns.
Read FLINT fmpz.rst:923-929, 997-1008, 1040-1048 and 1279-1303.
Proposed N-D19 in catalogue.h first. Slice A is in progress.

Slice A finished: C library (3 tests, 25527 checks), 822 oracle rows, 17 driver lines,
Julia (8 assertions), and four compiled planted faults detected. Oracle: 36423 period samples.
Initial red: link exit 2, absent binomial functions. Green exit 0. Driver's first expected values
omitted the finite-ball wrapper; corrected by hand from text.h:162-164, then diff exit 0.
Initial Julia helper used the wrong get_str signature; corrected before its first run.

Slice B finished: 1362 oracle rows (7080 image residues), library 5 tests/74478 checks,
24 driver lines, Julia binomial 8 + power 10 assertions, four compiled planted faults detected.
Red stubs: 4654 failed checks. Green: 0 failed checks. The finest algorithm uses gcd blocks and
the existing tight integer power of U(1), so no N factorisation or N bit cap is needed.
Only g<=256 is required by fine. The temporary Julia test placement was fixed before its run.

Slice C finished: reuse of existing Haar volume and content, 1001 volume triples, huge radius,
local backend, 6 tests/76487 checks; 8 driver lines; Julia 5 assertions; four scratch fball faults
compiled and detected. Initial conflicting proposed declaration was removed after locating fball.h:188.
No Haar implementation was added. Driver red: 6 mismatched lines before command implementation;
green diff exit 0. The C tests already passed the existing library accessor, so no library red is claimed.

Slice D finished: 2308 oracle rows, library 9 tests/131636 checks, 22 driver lines,
Julia 7 assertions, four compiled faults detected (one aborts after precision assertions fail).
Red stubs: 1988 failed checks. Green: 0. Both names and the original-order odd lift are implemented.
Added 120 volume oracle rows after the Slice C manual grid; all are consumed by volume_vectors.
The oracle generated 120 rows (5*6*4), independent of the 1001 manual grid triples.

Mutation: 60 compiled, 47 killed, 13 survived, 0 not compiled, 0 tool timeouts, 329.2 s.
Strengthened the INV tests to capture the public diagnostic and check binomial entry before LIMIT.
Replay of the same 13: 5 killed, 8 arithmetic-equivalent survivors. Notes in survivor-notes.md.
No second sweep. All tests still pass: release/SAN 9 tests/131636 checks; INV 10 tests/131701 checks.
SAN disables LeakSanitizer as requested. memcheck selftest passed before this strengthening.
prepare_mutation.py's first line-wrapping edit had a SyntaxError; corrected, py_compile exit 0.
Final variant builds use one job each, in two independent sessions, for at most two compiler cores.

Final full run: timeout 900 make -j2 check-all -> exit 2. All 79 C programs pass.
It stops at driver: new fixtures lacked #!exit 1 and therefore expected exit 0.
Added the intentional-error exit metadata to all four new fixtures. No expected result was changed.
The full run is not repeated; the driver and remaining acceptance steps are run individually below.

Final remaining steps pass: driver 61 cases/101124 expected lines; exports 462/462;
Julia including catalogue 30 assertions; mutation selftest and memcheck selftest pass.
Header C11 and C++17 standalone syntax checks exit 0. All final build variant tests exit 0.
Final source audit: 0 missing newlines, 0 new lines above 116 columns, vectors 274864 bytes.
Only the existing harness brief has an overlong title; it was not changed.
The report is written once after all checks, with the failed broad run retained.
