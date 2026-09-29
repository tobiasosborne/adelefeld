# Lane s2-sources: FLINT 3.0.1 documentation for roots modulo a prime of any size

Decision S-D10 (`docs/SPEC.md` 15.3): the root finder accepts every prime; for large `p` the roots modulo
`p` are found by a routine of FLINT, each is tested by evaluation, and completeness is certified by
`deg gcd(g, X^p - X)` (`docs/proofs/solvers.md` Proposition 3.7(2)). The documentation of the FLINT
functions that this needs is not on disk. No code and no build in this lane.

Read first: `refs/README.md`, `refs/fetch_sources.sh`, `refs/manifest.sha256`, `docs/sources.md` (the head
and table 3, the rows of FLINT for milestone S), `docs/proofs/solvers.md` lines 1284 to 1310.

**You own:** `refs/fetch_sources.sh` and `refs/manifest.sha256` (additions only), `docs/sources.md`
(new rows only), `lanes/s2-sources/`. `refs/src/` is not in git; you may add files under
`refs/src/flint-3.0.1/` through the fetch script. Everything else is read-only.

Do NOT create `lanes/s2-sources/report.md` before the work is done: the runner takes the existence of that
file as the end of the lane. Keep notes in `lanes/s2-sources/notes.md` while you work.

1. Find out how the 22 files of `refs/src/flint-3.0.1/` were fetched (the script, the URL, the tag). Add
   `fmpz_mod_poly.rst`, `fmpz_mod_poly_factor.rst`, `fmpz_mod.rst`, `nmod_poly.rst`,
   `nmod_poly_factor.rst` of the SAME version (tag v3.0.1, directory `doc/source/`) to the script in the
   same way, run it, and add their checksums to the manifest in the form that is there. If the network does
   not answer, try three times with a minute between, then report it; do not take the files from another
   place on this machine, and do not write anything from memory.
2. From the files on disk, with file and line and the quoted words, answer in rows of `docs/sources.md`
   (the form of the rows of table 3):
   - Which function computes `X^p` modulo `g` over `F_p` for a prime `p` that is an `fmpz`
     (`fmpz_mod_poly_powmod_...`)? What does the documentation require of its arguments (degree of the
     base below the degree of the modulus, a precomputed inverse, a nonzero modulus)?
   - Which function computes the gcd of two polynomials over `F_p`, and is the result monic? What is said
     for a modulus that is not prime?
   - Which function finds the roots of a polynomial over `F_p` (`fmpz_mod_poly_roots`,
     `fmpz_mod_poly_factor_...`)? What does it require (squarefree, monic, prime modulus), what does it
     promise (all roots, multiplicities), is it randomised, and what happens if the modulus is not prime?
   - The same three questions for `nmod_poly` (one-word `p`).
   - Does any of these functions test that `p` is prime? If the documentation does not say, write that it
     does not say.
3. A statement that the documentation does not make is not written as if it did. Write
   `[source pending: ...]` for what is still missing (for example the C source of a function whose
   documentation is silent).
4. The report: the files fetched with sizes and checksums; the rows added; every question of item 2 with its
   answer or "not said"; what is pending.
