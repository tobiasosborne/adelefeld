# Lane i-slice3: progress

- 2026-09-30 01:23: started; worktree at edfa658; refs/src symlinked; read CLAUDE.md, COMMON, COMMON-C,
  workflow, brief.
- 2026-09-30 01:24: read SPEC 4.5, 5, 15; PLAN milestone 2; ideles.md (all); api-2.md; conventions 2 to 5.7, 6,
  12; results of i-slice1 and i-slice2; headers and sources of both slices; the unreviewed d-ideles reference
  (part 2: ref_uc_pow_tight "E3", ref_real_pow, ref_idele_pow, hulls, set_adele, division).
- Plan: headers idpow.h, idmap.h -> reference part 4 -> vectors -> tests red -> code green -> Julia -> faults
  -> api-2.md section 3 -> check suites -> result.
- Decisions so far: no set predicates of ideles and classes (conventions ask them only of finite balls, SPEC
  4.2; adf_adele has none); exponent slong; pow_tight void (size of M_k bounded by k); ADF_IDELE_POW_BITS_MAX
  2^26; div real part by arb_div, NOT_DETERMINED on a non-finite ball (conventions 4.4, CV-08).
- 01:30-01:40: headers idpow.h, idmap.h; reference part 4 (check_api_powers, _hulls, _division pass, 12 s);
  gen_vectors.py -> tests/ref/vectors/i-slice3/ (idpow.jsonl 553596 bytes, idmap.jsonl 147115 bytes).
- 01:42-01:45: test_idpow.c red (link, then stub: 5582 failed checks), green (10122 checks).
- 01:48-01:49: test_idmap.c red (link, then stub: 25574 failed checks), green (54197 checks).
- 01:51: tests/julia/idmap.jl, block 2d in tests/test_julia.sh: 35 of 35 pass; test_julia passed.
- 01:52-01:56: docs/api-2.md section 3 (Statements J to O, decisions i3-1 to i3-12); long lines wrapped; the
  division test body moved into a helper (same 54197 checks); vectors regenerated, sha256 identical.
- 01:57: run_faults.sh: six faults H1-H6, each caught. 01:58-02:00: proto/ideles_checks.py all 19 checks pass
  (2 min 8 s); ref_faults.py: three faults of the reference, each caught.
- 02:01-02:22: check-all, SAN=1, CC=clang, INV=1 all passed. Then the INV entry checks of idele/class powers were
  moved from the static helpers into the public functions (the abort message named "idele_pow"); so the four
  suites run again below. M_k cost probe: k = 897612484786617600 (103680 divisors): 556490 bits, 0.2 s.
- 02:23-02:38: check-all, SAN=1, CC=clang, INV=1 passed again; check_headers passed; INV probe aborts with the
  public names.
- 02:39: result.md written. Done.
