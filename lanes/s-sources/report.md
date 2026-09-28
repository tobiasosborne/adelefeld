# Report, lane s-sources: the sources of milestone S on disk (work package 0.2, for the solvers)

Date: 2026-09-28. Everything below was done in this working tree. No git command that changes state was run and
`bd` was not run; the orchestrator commits. The only git commands inside `fetch_sources.sh` are the ones that
were already there for the package pin (`git clone`, `git rev-parse`, `git archive`), and they were not changed.

## What was done

The sources that milestone S of `docs/PLAN.md` section 6 leans on are on disk, hashed, and indexed. For every
statement of the brief a row of the new table 3 of `docs/sources.md` quotes the source, with file and line of the
text extraction, and every quote is checked by a script against the file on disk (57 rows, 9259 quoted
characters, 0 failures; a negative control is reported in the checks below).

Four keys were added and the key `flint-3.0.1` was extended:

| Key | What it settles |
|---|---|
| `shoup-ntb` | S.3: the existence lemma (Thue), the uniqueness theorem with the constant `2 A B < m` and its proof, and the algorithm: which row of the remainder sequence of the extended Euclidean algorithm is the answer, with a proof that the denominator of that row is within the bound |
| `storjohann-thesis` | S.1: the Hermite form and its conditions over a PID, the Howell form over a principal ideal ring with the conditions (r1), (r2), (r4), the Howell transform as the certificate, the kernel as a product of the transform, and the existence and uniqueness results |
| `conrad-hensel` | S.2: Hensel's lemma, the simple-root form with the unique lift, and the strong form `\|f(a)\| < \|f'(a)\|^2` with the distance of the root |
| `flint-src-3.0.1` | What FLINT 3.0.1 actually does: the code of `fmpq_reconstruct_fmpz_2`, of the Hermite form with its transformation, of the Smith form, of the Howell form modulo a general `mod`, of the nullspaces and of the Hensel lifting of factors |
| `flint-3.0.1` (8 new `.rst`) | What FLINT 3.0.1 promises: `fmpz_mat`, `fmpz_mod_mat`, `nmod_mat`, `fmpz_poly`, `padic_poly`, `arb_poly`, `arb_calc`, `arb_fmpz_poly` |

The rows already on disk were used as well, and are quoted in the new table: `baker-padic` (Hensel Theorems 1.33
and 1.37) and `thorne-padic` (Lemmas 3.6 and 3.7). `evertse-padic` was checked and states no Hensel lemma.

## Files written (all inside the lane's ownership)

- `refs/fetch_sources.sh`: a new block "Milestone S (the solvers)" at the end of the fetching part, and a fix of
  a bug in the manifest step (see the checks, item 3).
- `refs/manifest.sha256` (2912 files), `refs/manifest-extra.sha256` (70 files), rewritten by the script.
- `refs/README.md`: four new keys in the key table and a new section "Milestone S".
- `docs/sources.md`: 14 new rows at the end of table 1 (no present row was changed) and the new section
  "Table 3: milestone S, which source settles which statement" with three sub-tables (16 rows for S.3, 25 for
  S.1, 16 for S.2), nine notes for the design, eight items "not on disk" and the list of URLs that failed.
- `lanes/s-sources/mk_table.py` (builds the rows by slicing the quotes out of the files, so a quote cannot be
  mistyped), `lanes/s-sources/insert_table.py` (puts the rows into `docs/sources.md`),
  `lanes/s-sources/check_quotes.py` (reads the table back and checks every quote),
  `lanes/s-sources/progress.md`, `lanes/s-sources/report.md`.
- `refs/src/` (gitignored, not in git): `shoup-ntb/`, `storjohann-thesis/`, `conrad-hensel/`,
  `flint-src-3.0.1/` and eight new `.rst` files under `flint-3.0.1/`.

## Checks run, with results

1. `refs/fetch_sources.sh` (run twice; the first run downloaded, the second reported every file as present).
   Downloads of this lane: 3 PDFs (`shoup-ntb/ntb-v2.pdf` 3647756 bytes, `storjohann-thesis/diss2up.pdf`
   980479 bytes, `conrad-hensel/hensel.pdf` 459665 bytes), 8 `.rst` files, 54 C files. Volume of the new
   material: 7.5 MB (`du -sh`: 5.0M, 1.6M, 556K, 348K), far below the limit of 200 MB. Manifest lines:
   `manifest.sha256` 2912 files, `manifest-extra.sha256` 70 files. New keys in the manifests: `shoup-ntb` 1+1,
   `storjohann-thesis` 1+1, `conrad-hensel` 1+1, `flint-src-3.0.1` 54+0; `flint-3.0.1` grew from 15 to 22 files.
2. `refs/fetch_sources.sh --check`: exit status 0; 2982 lines "OK"; 0 lines "FAILED".
3. Bug found and fixed while doing item 1. On this worktree `refs/src` is a symbolic link to the main tree, and
   `find src -type f` does not descend into a symbolic link given as the starting point, so the first run wrote
   a manifest with 1 line (the hash of the empty string) and an extra manifest with 0 lines. The three `find`
   calls of the script now carry a trailing slash (`find src/`), which does descend. After the fix the
   manifests hold 2912 and 70 lines and the check passes. No file under `refs/src` was deleted or moved.
4. `python3 lanes/s-sources/check_quotes.py`: `rows: 57, quoted characters: 9259, failures: 0`, exit 0. With
   `--verbose` every quote part is listed with its file, line and number of characters matched.
5. Negative control of the checker: one character of one quote was changed in a copy of `docs/sources.md`
   (`Further,` to `FurtherX,`), the checker printed
   `FAIL shoup-ntb:ntb-v2.txt:4159: not in the line:` with both strings and ended with `failures: 1`, exit 1;
   the original file was then restored and the check gave 0 failures again.
6. Identity of the three new documents, read from the extraction:
   - `shoup-ntb/ntb-v2.txt:1` region: pdfinfo gives Title "A Computational Introduction to Number Theory and
     Algebra (Version 2)", Author "Victor Shoup", 598 pages; the file is a PDF 1.4.
   - `storjohann-thesis/diss2up.txt:1`: "Diss. ETH No. 13922", "Algorithms for / Matrix Canonical Forms",
     "presented by / ARNE STORJOHANN", "SWISS FEDERAL INSTITUTE OF TECHNOLOGY / ZURICH", 2013. The thesis was
     announced in 2000 and the printed copy on the author's page is the 2013 version; the content used
     (definitions of chapter 4) is the same matter.
   - `conrad-hensel/hensel.txt:1`: "HENSEL'S LEMMA", "KEITH CONRAD".
7. Line numbers of the anchors, all confirmed by printing the line before it was written into the table:
   Shoup Theorem 2.33 at 2184, Theorem 4.7 at 4042, Theorem 4.8 at 4159, Theorem 4.9 at 4189, Theorem 2.5 at
   1221; Storjohann 209, 221, 417, 887, 2058, 2068, 2070, 2075, 2078, 2094, 2119, 2120; Conrad 31, 37, 310, 315;
   Baker 572, 662; Thorne 567, 574; FLINT `fmpq.rst` 560-574, `fmpz_mat.rst` 1159-1176, 1185, 1235, 1314,
   `nmod_mat.rst` 561-723, `fmpz_mod_mat.rst` 376-388, `fmpz_poly.rst` 2993-3256, `arb_calc.rst` 112-138,
   `arb_fmpz_poly.rst` 66-105, `padic_poly.rst` 198-447; C sources `fmpq/reconstruct_fmpz_2.c` 940-1041,
   `fmpq/reconstruct_fmpz.c` 22-26, `nmod_mat/howell_form.c` 15-44,
   `fmpz_mat/strong_echelon_form_mod.c` 67-80.
8. Negative findings, each a `grep` on the file on disk (the numbers are the counts of matching lines):
   - `grep -c -i -E "root|hensel" refs/src/flint-3.0.1/padic_poly.rst` gives 0: the module `padic_poly` of
     FLINT 3.0.1 has no root finding.
   - `grep -c -i isolate refs/src/flint-3.0.1/arb_poly.rst` gives 0.
   - `grep -n "function::" refs/src/flint-3.0.1/fmpz_poly.rst | grep -i -E "root|hensel"` gives 24 lines; every
     `hensel` entry lifts factors (fmpz_poly.rst:2840-3000), the `root` entries are `div_root`, `product_roots`,
     `bound_roots` and `num_real_roots`. There is no function of `fmpz_poly` that isolates a real root.
   - `grep -n transform refs/src/flint-3.0.1/{fmpz_mat,nmod_mat,fmpz_mod_mat}.rst`: the only documented
     output that is a transformation matrix is `fmpz_mat_hnf_transform` (fmpz_mat.rst:1235); the other hits
     are similarity transforms (fmpz_mat.rst:809, nmod_mat.rst:666, fmpz_mod_mat.rst:397) and conversions to
     dense form.
   - `grep -i hensel refs/src/evertse-padic/evertsepadicnotes.txt` gives nothing.
9. Searches that found nothing (the "not on disk" list of `docs/sources.md` and the URL list below): the arXiv
   API query `all:"Howell form"` returned 0 entries; the index of Conrad's notes
   (`https://kconrad.math.uconn.edu/blurbs`, 265 entries) has no entry on the Smith or the Hermite form; the
   addresses tried and not obtained are listed in `docs/sources.md` under "URLs tried on 2026-09-28 that did not
   give a source" (10 addresses, with the HTTP status of each).
10. Line lengths: in the new part of `docs/sources.md` (from line 252 on) no line of prose is longer than 116
    characters; the table rows are data, as the paragraph at the top of the file says. The seven lines of
    `docs/sources.md` above line 252 that are longer than 116 are the pre-existing ones of the lane m0-sources
    (checked with a character count, not a byte count).

## What is not done

1. The eight items of the "not on disk" list are not on disk; TJO may supply copies. Their content is named
   there with what each would settle. None of them is needed for the three sub-tables.
2. `mk_table.py` and `insert_table.py` were run once; re-running `insert_table.py` on the present
   `docs/sources.md` would fail on purpose (it asserts that its block is not already there). To change a row,
   edit the row in `mk_table.py`, delete the section "Table 3" of `docs/sources.md` by hand, and run
   `insert_table.py` again. A working copy of the probe downloads and of `docs/sources.md` as it was before the
   lane was kept while the lane ran and has been deleted; nothing in the report depends on it.
3. The `--force` path of `fetch_sources.sh` (a full re-download) was not exercised; the fetch path was
   exercised twice and the hash check four times.
4. The complexity bounds of the modular algorithms are not on disk (items 5 and 6 of the "not on disk" list),
   so `PERF.md` cannot yet cite a figure for the Howell form of FLINT 3.0.1.

## Sources pending

1. Wang 1981 and Wang-Guy-Davenport 1982: the attribution of the extended Euclidean algorithm reconstruction
   method. `[source pending: Wang 1981, Wang-Guy-Davenport 1982]`
2. Monagan, "Maximal quotient rational reconstruction", ISSAC 2004: the maximal quotient variant, that is the
   largest bound pair for which existence is certified. `[source pending: Monagan ISSAC 2004]`
3. Collins and Encarnacion 1995: the incremental version of the reconstruction algorithm. `[source pending:
   Collins-Encarnacion 1995]`
4. von zur Gathen and Gerhard, Theorem 5.26: the same theorem as Shoup 4.8/4.9, in the book the project would
   rather cite. `[source pending: von zur Gathen and Gerhard 5.26]`
5. Storjohann and Mulders, ESA 1998 ([StoMul1998] in FLINT): the complexity bounds of the modular algorithms.
   `[source pending: Storjohann-Mulders 1998]`
6. Fiedler and Hofmann ([FieHof2014] in FLINT): the strong echelon form and `fmpz_mat_hnf_modular_eldiv`.
   `[source pending: Fiedler-Hofmann]`
7. Howell 1986: the original existence and uniqueness proof over `Z/(N)`, quoted at second hand in
   `diss2up.txt:2119-2120`. `[source pending: Howell 1986]`
8. Nothing is pending for S.2.

## Findings against the specification

No statement of `docs/SPEC.md` was found to be wrong. Five points of the specification of milestone S were
checked against the sources; all of them hold, and three of them need a sentence in the design. They are written
out in the notes of table 3 of `docs/sources.md`; the short form is:

1. **S.3, the constant.** `docs/SPEC.md` 9.2 and `proofs/quotient.md` Proposition 13 use `2 A B < m`. Shoup's
   Theorem 4.8 (`ntb-v2.txt:4159`) uses `n > 2 r* t*` and FLINT's documentation uses `2ND < m`
   (`fmpq.rst:563`). The same constant in all three; no source on disk gives a different one.
2. **S.3, the sign of the denominator.** Shoup states the bounded problem with `0 < |t| <= t*`
   (`ntb-v2.txt:4161`), so the denominator of the EEA row may be negative; the SPEC and FLINT ask for
   `d > 0`. This is a difference of convention, not a contradiction: FLINT fixes the sign with the determinant
   of the accumulated matrix (`fmpq/reconstruct_fmpz_2.c:1031`). The design must say which convention it uses
   and must not cite Shoup's `|t|` as `d`.
3. **S.3, what the fourth result means.** `docs/SPEC.md` 9.2 lists "uniqueness not certified" among the
   results. FLINT cannot return it: `fmpq_reconstruct_fmpz_2` returns 1 or 0 only (`fmpq.rst:566-567`), and 0
   means "no solution" only under the documented hypothesis `2ND < m`. The status has to be produced by the
   project. (Under `2ND < m` the code cannot lose a solution: by Shoup's Theorem 4.9(i) the EEA row has
   `|t0| <= t*` whenever a solution with `|t| <= t*` exists, and the reduced form of a solution is a solution
   inside the same bounds. The second half of this sentence is an own calculation, the first is
   `ntb-v2.txt:4202`.)
4. **S.3, existence is not claimed.** Proposition 13 claims at most one solution under `2 A B < m`; it does not
   claim that a solution exists. Existence is Thue's lemma (`ntb-v2.txt:2184`) and needs `m > A B` with the
   bounds of that statement. A solver must be ready to return "none" in the range `A B < m <= 2 A B`.
5. **S.1, the prime-field assumption.** `docs/SPEC.md` 9.1 asks for systems modulo `N`, with `N` arbitrary. The
   solving functions of FLINT 3.0.1 in `nmod_mat` and `fmpz_mod_mat` are documented for a prime modulus
   (`nmod_mat.rst:562`, `fmpz_mod_mat.rst:388`). For a general `N` the entry point is
   `fmpz_mat_howell_form_mod`, and its source works with gcds against `N`, not with inverses modulo a prime
   (`fmpz_mat/strong_echelon_form_mod.c:78, 80`). The design of S.1 must not be built on `fmpz_mod_mat` for a
   composite `N`. This is a constraint on the design, not an error in the specification.
6. **S.2, the two shapes of Hensel's lemma.** The SPEC's existence conditions for roots at odd `p` and at 2
   (`docs/SPEC.md` 9.3.3) are instances of Conrad's Theorem 4.1 (`hensel.txt:315`), including the square root at
   2 with the unit part 1 modulo 8, which is the case `f = X^2 − u`, `a = 1`, `p = 2`; that last step is an own
   calculation, since `|f(1)|_2 <= 1/8 <= 1/4 = |f'(1)|^2_2`. Thorne's Lemma 3.6 gives a weaker distance
   (`|y - x| <= |f(x)|`, not `|f(x)/f'(x)|`) under different hypotheses (monic, `|f'(x)| = 1`), so the design
   must take the distance from Conrad, not from Thorne. Nothing in the SPEC depends on the weaker bound.
