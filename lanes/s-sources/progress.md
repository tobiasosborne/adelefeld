# Lane s-sources: progress

## Done

- Read `CLAUDE.md`, `lanes/COMMON.md`, `docs/sources.md`, `refs/README.md`, `refs/fetch_sources.sh`,
  `lanes/m0-sources/report.md`, `docs/PLAN.md` section 6 (Milestone S), `docs/SPEC.md` 9.1 and 9.2,
  `docs/proofs/quotient.md` Propositions 11 to 13.
- Local reading (no network): `flint-3.0.1/fmpq.rst` (rational reconstruction, lines 543-575); Hensel
  statements in `baker-padic` (Theorems 1.33 at 572, 1.37 at 662) and `thorne-padic` (Lemmas 3.6 at 567,
  3.7 at 574); `evertse-padic` has no Hensel statement (`grep -i hensel` gives nothing).
- Searched for open copies. Found and fetched: Shoup (free at the author's page; section 4.6 has
  Theorem 4.8 and Theorem 4.9, the uniqueness bound and the EEA algorithm), Storjohann's dissertation
  (free on his university page; the Howell form and its transform), Conrad's Hensel note.
  Not found and not lawful to fetch: Wang 1981/1982, Collins-Encarnacion 1995, Monagan ISSAC 2004,
  von zur Gathen and Gerhard 5.26, Storjohann-Mulders 1998, Howell 1986, Fiedler-Hofmann.
- `refs/fetch_sources.sh`: new block "Milestone S (the solvers)" at the end of the fetching part; a bug of
  the script was fixed (on this worktree `refs/src` is a symlink and `find src` does not descend into it, so
  the manifests came out with 1 and 0 lines; `find src/` with the trailing slash fixes it).
- Fetch run: 3 PDFs, 8 `.rst` files, 54 C files; 7.5 MB in all. `manifest.sha256` 2912 files,
  `manifest-extra.sha256` 70 files; `--check`: 2982 OK, 0 FAILED, exit 0.
- `docs/sources.md`: 14 new rows in table 1, and the new "Table 3: milestone S, which source settles which
  statement" with 57 rows (16 for S.3, 25 for S.1, 16 for S.2), nine notes for the design, eight items
  "not on disk", and the list of URLs that failed.
- `lanes/s-sources/mk_table.py` builds the rows by slicing the quotes out of the files (so they are verbatim
  by construction); `lanes/s-sources/insert_table.py` puts them into `docs/sources.md`;
  `lanes/s-sources/check_quotes.py` reads the table back and checks every quote: 57 rows, 9259 characters,
  0 failures. A negative control (one character changed in a quote) is reported as a failure, exit 1.
- `refs/README.md`: four new keys and a "Milestone S" paragraph.

## Not done

- The books and papers of the "not on disk" list (see the report); TJO may supply them.
- Nothing is pending for S.2.
