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
