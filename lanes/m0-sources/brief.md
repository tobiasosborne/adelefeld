# Lane m0-sources: work package 0.2 (sources on disk with hashes)

Read `CLAUDE.md` rules 3 and 4, `docs/PLAN.md` row 0.2, and in `docs/SPEC.md` every place marked **[unverified]**
or **[standard]** (grep for them). As a model for the mechanics read
`/home/tobias/Projects/riemann-channel/refs/fetch_sources.sh` and its `README.md` (read only).

**You own:** `refs/` (all of it), `docs/sources.md`. You have network access; use `curl`.

Goal: every source the specification leans on is on disk under `refs/src/` (this directory is gitignored: other
people's texts are not redistributed), re-fetchable by `refs/fetch_sources.sh`, with `refs/manifest.sha256`.
Preference order: TeX source, then HTML or reStructuredText, then PDF (with a text extraction by `pdftotext -layout`
next to it, if `pdftotext` exists). Only lawful, publicly offered copies: arXiv, authors' and university pages,
institutional repositories, official project repositories. No shadow libraries. If a source is not lawfully
available (Tate's thesis in Cassels-Froehlich is probably not), say so and fetch the best open substitutes that
state the same conventions, naming them as substitutes.

Wanted:

1. Tate's thesis, "Fourier analysis in number fields and Hecke's zeta-functions", or open expositions that state
   its conventions precisely (additive character, self-dual measures, local and global zeta integrals, functional
   equation): for example lecture notes by B. Poonen, K. Conrad, S. Kudla, J. Milne, P. Garrett, or an arXiv text.
   At least two independent expositions.
2. M. Hertogh, "Computing with adeles and ideles", master's thesis, Leiden 2021, and the Sage package `adeles`
   (git clone at a pinned commit; record the commit hash).
3. A text on adeles and ideles with proofs (restricted products, `A/Q` compact, ideles, class group of `Q`):
   for example J. Milne's "Class Field Theory" and "Algebraic Number Theory" notes (needed also for the two
   conventions of the reciprocity map, SPEC 9.3.7, cyclotomic action).
4. p-adic `exp`, `log`, `sin`, `cos`, roots of unity, n-th roots: an open text with proofs of the domains of
   convergence (for example K. Conrad's expository notes on the p-adic exponential and logarithm, or a book
   offered openly by its author).
5. Hilbert symbol formulas, Kronecker symbol: an open source (Serre's Course in Arithmetic is not open; find
   lecture notes that state the formulas at odd `p`, at 2 and at the real place, with proof).
6. FLINT 3.0.1 documentation sources for `padic`, `fmpz_mod`, `nmod`, `arb`, `acb`, `acb_dirichlet`, `fmpq`,
   `ulong_extras` (the `.rst` files from the FLINT repository at tag `v3.0.1`), and the matching headers are in
   `/usr/include/flint`. PARI/GP documentation for `bnfcertify`, `hilbert`, `kronecker`, `bnrinit` (from the PARI
   sources or the official web documentation).
7. For `docs/PERF.md`: the uops.info pages or data it cites (read PERF.md section 1 to see which instructions and
   which microarchitecture), and the Harvey-van der Hoeven paper (HAL or arXiv).

`docs/sources.md`: one table row per source: short key, full reference, URL, file path under `refs/src/`, sha256,
date fetched, format, what it is used for (SPEC section). Then a second table: every **[unverified]** and
**[standard]** label in `docs/SPEC.md` (by section and the first words of the statement), with either the exact
quotation that settles it (`key:file:line`, quoted verbatim, at most three lines) or "kept, because ...".
Quote only what you have read in the file on disk. If the source says something different from the
specification, that is a finding: report it, do not smooth it over. In particular check: the sign convention of
the additive character and of the Fourier transform (SPEC section 6); Hertogh's rules for sum and product of
profinite numbers and his notion of equality (SPEC section 14); the two conventions for the reciprocity map.

Verify at the end: run `refs/fetch_sources.sh` in a way that re-checks the hashes (`sha256sum -c`), and report the
result.
