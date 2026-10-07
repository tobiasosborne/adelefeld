# Red-green log of lane t5-slice2 (slice 5c)

Build: `timeout 600 make -s -j2 BUILD=lanes/t5-slice2/build lanes/t5-slice2/build/test_tate`;
run: `timeout 300 lanes/t5-slice2/build/test_tate`.

1. RED (link, first test of the file): tests/test_tate.c written whole before src/tate.c; the build fails with
   "undefined reference to `adf_tate_vector'" and "`adf_tate_integral'" (tests/test_tate.c:172 and on).
2. GREEN (adf_tate_vector) / RED (adf_tate_integral, assertion): src/tate.c with adf_tate_vector and a stub
   adf_tate_integral returning NOT_DETERMINED; vector_statuses passes (LIMIT order, sentinel bytes, prec 2 and
   the cap), then `tests/test_tate.c:246: adf_tate_integral(z, chi, s, 80, 128) == ADF_OK && narrow(z, 80)`
   (zeta at s = 2), exit 1.
3. GREEN (adf_tate_integral): the full src/tate.c. Fixes of the TEST on the way (each a test error, not a code
   change): the closed-endpoint ball [1 - 2^-9, 1] built exactly (a mag rounding had moved its end above 1); the
   input balls of radius 2^-60 / 2^-70 cannot meet 2^-80 (N-D23): the test now requires NOT_DETERMINED there, with
   the corner references proving that no OK is possible, and OK at bits 48; a phase reference such as
   cos(2 pi/3) = -1/2 is stored exactly by adf_phase_get_acb, so overlap with the 512-bit reference replaces
   containment. Then: `tate: 18445 checks`, 2.9 s (vectors: 18 vectors, 144 value records, 256 OK calls,
   1024 reference points inside, 1 witness).
4. RED/GREEN (T3 against an independent integral; the width check after the error): piece records added to the
   generator (mpmath incomplete Gamma, the oracle's numeric_piece) and two tests, piece_record and boundary; the
   hidden helper adf_tate_taylor_piece (src/roots.c:49-53 precedent) first as a stub: RED
   `tests/test_tate.c:250: adf_tate_taylor_piece(...) == ADF_OK`; then the body (a call of tt_side): GREEN after
   three test repairs (the boundary radii were scanned too high: the enclosure radius is about 1.26 r; R = 1 gives
   the exact 0, which cannot contain the 1e-50 reference ball; a wrong self-comparison arb_le(qerr, qerr)).
5. After the mutation run (20 survivors of 60): the hidden hook adf_tate_cutoffs (N, R, panel degrees, work) and
   the tests cutoff_record (57 records of the oracle's cutoffs and degrees) and two work-cap cases (C = 1024;
   zeta at bits 1400). The hook was written before its test, so this step has no red run of its own; instead the
   new tests were run against the surviving mutants (lanes/t5-slice2/resurvivors.py, resurvivors.out): 8 of 11
   gap mutants are now killed (122, 387, 231, 232, 218, 374, 356, 336); 383, 560, 551 survive.
   Final: `tate: 22904 checks` (INV 22914), plain, INV, SAN with detect_leaks=1, clang.
