# Lane s-sources: the sources of milestone S on disk (work package 0.2 again, for the solvers)

Read `lanes/COMMON.md`, `CLAUDE.md` (rules 3 and 4), `docs/sources.md` (all of it: it is your pattern),
`refs/README.md`, `refs/fetch_sources.sh`, `lanes/m0-sources/report.md`, `docs/PLAN.md` section 6
"Milestone S", `docs/SPEC.md` sections 9.1 and 9.2, `docs/proofs/quotient.md` Propositions 11 to 13.

**You own:** `refs/fetch_sources.sh` (additions at the end, in a new block "Milestone S"),
`refs/manifest.sha256`, `refs/manifest-extra.sha256`, `refs/README.md`, `docs/sources.md` (new rows of
table 1 and a new table "Milestone S: which source settles which statement"; change no present row),
`lanes/s-sources/`. The directory `refs/src/` is not in git; you write the fetched files there.

You design nothing and you implement nothing. You find, fetch and index. No statement about the content of
a source is written from memory: every row of your table quotes the source, with file and line of the text
extraction, and the quote is checked by a script against the file on disk (the pattern of
`lanes/m0-sources/`: all 57 quotes there were machine-checked).

## What the design of milestone S will need

For each item: the best lawful, publicly offered copy (the author's page, arXiv, a university repository,
the publisher if open). Format preference: TeX (arXiv source), then HTML or reStructuredText, then PDF with
`pdftotext -layout`. A book or paper that is not lawfully free is NOT fetched: list it under "not on disk"
with what it would settle, so that TJO can supply a copy.

1. **S.3 Partial rational reconstruction** (given `m`, `c`, bounds `A`, `B`: `n/d` with `n = c d mod m`,
   `|n| <= A`, `0 < d <= B`).
   - The algorithm by the extended Euclidean algorithm and its proof of correctness: the statement of which
     row of the remainder sequence is the answer, and the statement of uniqueness (`2 A B < m`).
     Candidates to look for: P. S. Wang (1981) and Wang, Guy, Davenport (1982); M. Monagan, "Maximal quotient
     rational reconstruction" (ISSAC 2004); G. E. Collins and M. J. Encarnacion (1995); lecture notes that
     prove the theorem in full (the statement is Theorem 5.26 in von zur Gathen and Gerhard, which is not
     free).
   - FLINT 3.0.1: `fmpq.rst` is on disk; find in it the functions `fmpq_reconstruct_fmpz` and
     `fmpq_reconstruct_fmpz_2` and quote what they promise and what they do not (the return value, the
     conditions on the bounds). Fetch the C source of these functions at the tag v3.0.1
     (`src/fmpq/reconstruct_fmpz_2.c` and what it calls), as the ground truth of what FLINT does.
2. **S.1 Linear systems modulo `N`** (particular solution, generators of the kernel, certificate).
   - Hermite normal form with the transformation matrix, and the Howell form over `Z/N` (the canonical form
     of a module over a ring with zero divisors). Candidates: A. Storjohann, "Algorithms for matrix canonical
     forms" (thesis, ETH Zurich, 2000); A. Storjohann and T. Mulders, "Fast algorithms for linear algebra
     modulo N" (ESA 1998); J. A. Howell (1986) if an open copy exists.
   - FLINT 3.0.1 documentation at the tag v3.0.1: `fmpz_mat.rst` (Hermite form, `fmpz_mat_hnf_transform`,
     Smith form, kernel), `fmpz_mod_mat.rst` and `nmod_mat.rst` (Howell form, `strong echelon form`), and
     the C sources of the Howell form. Quote what each function promises.
3. **S.2 Roots.**
   - Hensel's lemma with proof, in the form for simple roots AND in the stronger form
     (`|f(a)| < |f'(a)|^2`), with the statement of uniqueness of the lifted root. Candidates: K. Conrad,
     "Hensel's lemma" (expository paper); the notes already on disk (`baker-padic`, `evertse-padic`,
     `thorne-padic`): say where they state it, by file and line.
   - FLINT 3.0.1 documentation: `padic_poly.rst`, `fmpz_poly.rst` (Hensel lifting, real root counting by
     Sturm or Descartes: quote what exists), `arb_poly.rst`, `arb_calc.rst`, `arb_fmpz_poly.rst`
     (isolation of real roots: what is certified, and what "complete" can mean).

## Work

- Extend `refs/fetch_sources.sh` so that a fresh machine gets every file; run it; write the hashes.
  `./fetch_sources.sh --check` must pass at the end.
- Rows of table 1 of `docs/sources.md` in the form of the present rows (key, full reference, URL, path,
  sha256, date, format, used for).
- The new table: statement needed (from the list above) | source key | file and line | the quote, verbatim.
  Where two sources state a theorem with different hypotheses or different constants, give both rows and say
  so: the design must know.
- A script `lanes/s-sources/check_quotes.py` that checks every quote of your table against the file on
  disk; its output in the report.
- Network: use `curl` with `--max-time 180`. A URL that fails is tried once more and then listed as failed.
  Do not fetch more than 200 MB in total.

## The report

Keep notes of progress in `lanes/s-sources/progress.md`. Do NOT create `report.md` before everything is
done: the runner stops the lane as soon as that file exists. `report.md` at the end, as `lanes/COMMON.md`
rule 8 says, with the list "not on disk" and the failed URLs.
