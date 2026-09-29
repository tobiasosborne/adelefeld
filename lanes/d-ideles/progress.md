# Lane d-ideles: running notes (for an agent that continues this lane)

Files of the lane: `docs/api-2.md`, `proto/ideles_checks.py`, `lanes/d-ideles/`.

## State

- [x] Read: brief, CLAUDE.md, COMMON.md, workflow.md, SPEC 4, 5, 15, PLAN 4, 5, 6, 7, ideles.md (all),
      precision.md, conventions 2 to 5.7, 7 to 12, api-s.md, api-m1.md, adele.h, review of s-design (start),
      catalogue P12, P13, golden vectors of ucoset, idele, dump.
- [ ] `proto/ideles_checks.py` part 2 (reference of the interface)
- [ ] `docs/api-2.md` 2.1
- [ ] `docs/api-2.md` 2.2
- [ ] `docs/api-2.md` 2.3
- [ ] `docs/api-2.md` 2.4
- [ ] decisions, slices, findings
- [ ] `lanes/d-ideles/result.md`

## Facts found

1. `proto/ideles_checks.py` EXISTED before this lane (464 lines, the checks of `docs/proofs/ideles.md`, lane
   m0-proofs-ideles). The brief calls it new. It is kept: `ideles.md` cites its functions by name. The
   reference of the interface is added to it as part 2. It ran 99 s on this machine (check_power 57 s,
   check_products 33 s).
2. python-flint 0.8.0 is importable, but `arb(mid, rad)` rounds the radius up (radius 1 becomes
   `1 + 2^-29`), so exact balls cannot be made with it. FLINT is probed by a C program instead
   (`lanes/d-ideles/probe_arb.c`).
3. The ball product `[m1 m2 +/- ...]` of `x = y = 1 +/- (1 - 2^-30)` contains 0 at every precision (the
   midpoint of the ball product is 1, the midpoint of the product set is about 2). So the kernel of SPEC 5
   "sign preservation" cannot be `arb_mul` followed by a sign test.

## Design choices taken so far (see api-2.md section 5 for the decisions asked of TJO)

- D2-1 results of operations are stored in normal form (constructors keep the modulus as supplied, CV-17).
- D2-2 real kernel by end points; `NOT_DETERMINED` by the exponent gap `> prec`.
- D2-3 limit of the bits of `r^k` in `adf_idele_pow`.
- D2-4 the simple hull is formed from the normal form.
- D2-5 the unsuffixed idele-to-adele map is the smallest hull.
- D2-6 `adf_idele_set_adele` belongs to 2.3; no `adf_adele_div`.
- D2-7 text and dump of the three types are the last slice.
