# q-review1 progress

Adversarial review of `docs/api-3.md` (lane d-quotient). Only `lanes/q-review1/` was written.

## What was read

`CLAUDE.md`, `lanes/COMMON.md`, `lanes/PROOFSTYLE.md`, `docs/api-3.md` (all 877 lines),
`docs/proofs/quotient.md` (all), `docs/proofs/analysis.md` Lemma 2 with line numbers,
`docs/proofs/precision.md` lines 20-35, `docs/SPEC.md` 4.1 and 6 (386-444) and M1-D6, M1-D9,
`docs/PLAN.md` rows 3.1 and 3.2, `docs/conventions.md` 2, 3.1-3.3, 3.2 table, 4.6, 5.2, 5.5, 5.10,
6.1, 6.4, 7, 9.2-9.5, 10.1-10.2, `refs/src/tate-poonen/notes.txt:685-745`,
`include/adelefeld/adele.h`, `proto/quotient3_checks.py` (all 700 lines), `lanes/d-quotient/report.md`,
`docs/reviews/s-design/review.md` (form only).

## Facts fixed by the reading, used later

- conventions:844 is the psi formula, quoted exactly by the design. conventions:735-739 is the CV-45
  reason; F5's dyadic ball refutes its wording. conventions:745 states only the sufficient direction,
  so F1's quotation of it as "exactly when" is wrong; SPEC:402-403 has the "exactly when".
- conventions:753-755 (`ADF_CMP_UNDECIDED` for union equality) against conventions:200 (set predicates
  return 0 or 1): the conflict behind F2 is real.
- conventions:756-757 = translation; the same sentence is on SPEC:413-**414**, not 414-415 (F4 slip).
- conventions 3.2 has no qclass-constructor row and no row containing `NOT_DETERMINED` for the quotient,
  so the design reports two status-row exceptions (D3-1, D3-2) and misses two more (class psi, `add_rat`).
- conventions 5.5 allows `arch = 0`, so `adf psi_at (x, inf)` can be a place that `x` does not have; the
  design's `_at` declarations have no `where` and no `DOMAIN`.
- tate-poonen:693-694 (`e^{-2 pi i x}` at R) and 693-700 (`psi(1/p^n) = e^{2 pi i/p^n}`, trivial on `Z_p`)
  say what the design says. :733-740 is the transform without the conjugate plus the remark on Tate.
- analysis Lemma 2 step 2 (lines 87-88) is triviality on `Q`; the descent to `A/Q` is step 4 (92-94).
  The design cites 87-88 twice for the descent.
- precision.md Proposition 1 is at line 27 and does give `(a+N Zhat)+(b+M Zhat) = (a+b)+gcd(N,M) Zhat`,
  including a zero radius through Lemma 2 at line 22. Q3's citation is correct.
- M1-D9 exists and is about the binary exponent of a dumped real ball in a `qclass` piece.
- A finite ball with `d > 1`, for instance `(* ; 0 mod 1/2)`, is a coset in the full product `prod Q_p`
  and is not inside `A_f = Q + Zhat` (quotient.md Lemma 1). This is pre-existing (SPEC 4.1, conventions
  5.2) and the golden files implement it; the design only repeats SPEC 6. Recorded as an observation,
  not counted.

## Model used for the independent checks

A quotient value is a list of pieces `(lo, hi, m, N)`. Membership of `(s,w)` with `s` rational and `w`
integer: `s+q in [lo,hi]` and `w+q-a in N Zhat` for some rational `q`. Writing `N = A/B`, the second
condition says `B(w+q-a)/A` is an integer, so `q = a + Ak/B - w` and one enumerates `k` in an interval.
My first two versions of that test were wrong (they assumed `q in Z`, or that `N Zhat` is inside
`Zhat`); the version above is exact for every rational centre and radius and was checked against the
design's own algorithm on every case.

## Work done

1. `review_checks.py`, fifteen sections, own brute force in `fractions.Fraction`. Final run 72.8 s, exit 0.
2. `mutate_oracle.py` with fifteen mutants of `proto/quotient3_checks.py` on scratch copies in
   `mutants/`; `mutants-final.txt`.
3. `abi/sizes.c` compiled against the real headers; the layout claims of section 1 are correct.
4. `review.md` with 14 VALID, 9 MINOR, 0 INVALID, and MAY BE IMPLEMENTED AFTER THE REPAIRS.

## Open points carried into the report

- No statement Q1 to Q5 is false. The repairs are local.
- The author's oracle does not test the boundary glue: deleting it leaves all fifteen groups green.
- The rounding direction of Q1 is not pinned by the oracle: nearest-plus-successor passes everything.
- The oracle's `fault_witnesses` group is the first to notice none of the fifteen mutants.