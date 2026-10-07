# Red and green runs, lane q-slice7

A. Vectors: `timeout 300 python3 lanes/q-slice7/gen_vectors.py` ->
   79 records (64 add, 15 neg), 391468 bytes, raw counts 1..427, 9 with duplicates before dedup,
   1 with duplicates after Q1. The generator asserts, per record, that the construction equals
   (oracle `compare`) the pairwise sums or negations of the inputs' exact integer-modulus pieces
   (the method of `check_arithmetic`), and that each of the 40 sum/negation points lies in the exact
   union (`direct_member` after the diagonal translation by -w). Two generator bugs were found by
   these assertions and fixed before any C was written: rational w passed to `direct_member` (which
   expects an integral finite coordinate), and `raw == len(reduce(...))` (reduce already deduplicates).

B. Red: `make -j2 BUILD=lanes/q-slice7/build lanes/q-slice7/build/test_qclass_arith` with the
   declarations appended to qclass.h and no implementation: link error, undefined references to
   `adf_qclass_add` and `adf_qclass_neg` (first test of the file; log red-build.log, removed at the end).

C. Green: src/qclass_arith.c written; same build exit 0; `timeout 300 lanes/q-slice7/build/test_qclass_arith`
   first run failed at the test's own exact-work case (line 340): a centre 1/(2^1048579+1) alone is
   refused by Q1's midpoint exponent bound (|e| <= 2^20), not admitted as the test assumed. The test was
   wrong, not the code (D3-2 bounds the midpoint exponent): the case now uses centres near 1/2 with
   2^20+5-bit denominators, which pass alone and fail only in the centre sum. Second run: exit 0,
   1205134 checks, Q2 containment 28 decided, 2 over the 4000000 budget.

D. Driver red: `timeout 900 sh tests/test_driver.sh` with the fixture and no commands: exit 1, qclass-arith
   differs (every line `error: PARSE`, the verbs unknown). Green after the two commands: exit 0, 76 cases,
   101433 lines; all 21 hand-derived lines matched on the first run.
   Julia: 23 of 23 after `sh tests/test_exports.sh` (518 of 518 declared functions exported).

F. Faults: `timeout 1500 python3 lanes/q-slice7/plant_faults.py <scratch>`: first run 8 of 9 rejected and one
   not compiled (the fault "N alone" left qa_gcd unused under -Werror); replanted as gcd(N,N): 9 of 9 rejected.

Mutation, first attempt: baseline failed under INV=1: the test's `sentinel()` wrote a fractional finite radius
into a PIECES value (non-canonical; INV abort in adf_qclass_set). Test bug, fixed: the sentinel is now a fresh
LIFT, checked canonical. Second attempt: 60 mutants, 41 killed, 10 survived, 7 not compiled, 2 timed out.
Survivor tests added (component boundaries, normalised mag, 2^62 byte product), red against each survivor
in scratch copies (`check_survivors.py`): 7 of 10 killed; 3 equivalent (see report).
