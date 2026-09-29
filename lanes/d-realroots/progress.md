# Lane d-realroots: progress (running notes; the report is result.md)

## Done
- 2026-09-29 16:35. Sources fetched with `lanes/d-realroots/fetch_only_mine.sh` (the same commands as the
  additions to `refs/fetch_sources.sh`; the whole script was not run). 28 files; hashes merged into
  `refs/manifest.sha256`; list in `lanes/d-realroots/manifest-additions.sha256`.
  Keys: `flint-src-3.0.1` (19 more C files), `flint-3.0.1` (`acb_poly.rst`), `sagraloff-mehlhorn`
  (arXiv 1308.4088v2, TeX), `kerber-sagraloff` (arXiv 1104.1362v3, TeX).
- FLINT 3.0.1 has no Descartes or real isolation routine for `fmpz_poly` (directory listing of the tag,
  no file name with descartes/isolate; the installed header agrees).

## Next
1. bench/bench_roots_real.c and the measurements (where the time goes).
2. proto/real_isolation.py, proto/test_real_isolation.py.
3. docs/design/real-roots.md.
4. docs/sources.md rows; result.md.

## Not available
- Sturm's theorem, a text: Yap's book (cs.nyu.edu, 403) and Basu-Pollack-Roy (rennes, 404) could not be
  fetched. Still source pending.
- `perf`: installed, but `perf_event_paranoid` = 4, closed to users.

## 2026-09-29 16:36: measurements done (bench/bench_roots_real.c; outputs in lanes/d-realroots/runs/)
- The machine was loaded (load average 5 to 6, other lanes): times are about 2 times those of the review.
- Cause found. `trace pair e`: FLINT's loop doubles the working precision (complex_roots.c:91) after at most
  `4 deg + 64` Durand-Kerner iterations (complex_roots.c:94). On a pair of roots of relative distance 2^-e
  the iteration gains 100 bits of "correction" for each 72 iterations (linear convergence) until the
  correction is below 2^-e. So the last precision is 2^(8 + e/100): 2^14 (e = 600), 2^17 (900), 2^20 (1200).
  Needed: about e bits. e = 3000 would need 2^38 bits.
- `close` (roots 1 and 1 + 2^-e) behaves the same as `pair`; `far` (2^e and 3 2^e) takes 1 ms at e = 100000.
  So the cause is the relative distance, not the size.
- `deg` (d roots 2^e + i, e = 300): d = 2 0.007 s, d = 4 0.44 s, d = 8 and 16 not finished in 100 s.
- count (fmpz_poly_num_real_roots) and one exact sign: microseconds for the pair at e = 3000.

## Design decisions so far (to be written in docs/design/real-roots.md)
- Recommended: bisection with Descartes' rule on the squarefree part, exact integers (Collins-Akritas form:
  q(X) = g(lo + w X) scaled; test var of (1+X)^d q(1/(1+X)); halves by q(X/2) 2^d and the shift X+1).
  Exact dyadic roots found at the midpoints are recorded as exact balls and divided out.
- Refinement: bisection first (simple), QIR of Abbott / Kerber-Sagraloff as the later slice.
- Certificate unchanged: exact signs of P3.8 at the end points, and n = FLINT's count.
- Alternatives: Sturm (FLINT's _fmpz_poly_num_real_roots_sturm on the shifted polynomial gives the count
  right of a point); Durand-Kerner with our own precision schedule (no bound proved).
