#!/usr/bin/env python3
"""Insert the milestone S rows and the new table of docs/sources.md.

The table rows themselves come from mk_table.py, so every quote is verbatim.
"""
import subprocess
import os

DOC = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "docs", "sources.md")
HERE = os.path.dirname(os.path.abspath(__file__))

TABLE1 = """| shoup-ntb | V. Shoup, A Computational Introduction to Number Theory and Algebra, version 2 (Cambridge University Press 2008; author's copy, free at the author's page) | https://shoup.net/ntb/ntb-v2.pdf | shoup-ntb/ntb-v2.pdf | 8e1abc54f4510c3f274dfbed07ea602a6a439ee24b2c916e61abe829b402ec06 | 2026-09-28 | PDF + txt | SPEC 9.2; milestone S.1, S.3 |
| storjohann-thesis | A. Storjohann, Algorithms for Matrix Canonical Forms, Diss. ETH No. 13922 (2013); author's copy on his university page | https://cs.uwaterloo.ca/~astorjoh/diss2up.pdf | storjohann-thesis/diss2up.pdf | ebe79b8c7c1c306d25426ef76b6972efe2c8143cf1924a0630b5e4c888bf18e8 | 2026-09-28 | PDF + txt | SPEC 9.1; milestone S.1 |
| conrad-hensel | K. Conrad, Hensel's lemma (expository note) | https://kconrad.math.uconn.edu/blurbs/gradnumthy/hensel.pdf | conrad-hensel/hensel.pdf | 3243fe15855aa549d42ed96f476bfd9f5241cdeb8d33f72beee33fd2e6bfdebf | 2026-09-28 | PDF + txt | SPEC 9.3.3; milestone S.2 |
| flint-3.0.1 | FLINT 3.0.1 documentation, `fmpz_mat.rst` (Hermite form with transformation, Smith form, Howell form modulo `mod`, kernel) | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/fmpz_mat.rst | flint-3.0.1/fmpz_mat.rst | 882f69f7c17f0048854847e05248ac449b94eb1af41cd200fe48edc4a627d801 | 2026-09-28 | reST | SPEC 9.1; milestone S.1 |
| flint-3.0.1 | FLINT 3.0.1 documentation, `nmod_mat.rst` (Howell form, strong echelon form, solving, nullspace) | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/nmod_mat.rst | flint-3.0.1/nmod_mat.rst | 468e1693bb86d4df199b3ba99c30b7f134f83d793c24a7f937b102478064fc6f | 2026-09-28 | reST | SPEC 9.1; milestone S.1 |
| flint-3.0.1 | FLINT 3.0.1 documentation, `fmpz_mod_mat.rst` (Howell form, strong echelon form, solving) | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/fmpz_mod_mat.rst | flint-3.0.1/fmpz_mod_mat.rst | e277db5fe1d29e02e152faaf091937f77c964b35df3384d706940644b758ddbf | 2026-09-28 | reST | SPEC 9.1; milestone S.1 |
| flint-3.0.1 | FLINT 3.0.1 documentation, `fmpz_poly.rst` (Hensel lifting of factors, counting real roots) | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/fmpz_poly.rst | flint-3.0.1/fmpz_poly.rst | f72dc44920d7f69d213ceba3ee1c164d47b5399e7736f8d0439fd76209731c65 | 2026-09-28 | reST | SPEC 9.1, 9.3.3; milestone S.2 |
| flint-3.0.1 | FLINT 3.0.1 documentation, `padic_poly.rst` (p-adic polynomials: integrality, evaluation) | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/padic_poly.rst | flint-3.0.1/padic_poly.rst | 475dcbce062a98ba55b94be65d7620b9b821d5c9d8ab04ff48a22d09792afa56 | 2026-09-28 | reST | SPEC 9.3.3; milestone S.2 |
| flint-3.0.1 | FLINT 3.0.1 documentation, `arb_calc.rst` (subdivision-based certified root isolation) | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/arb_calc.rst | flint-3.0.1/arb_calc.rst | eb11bd5a91210629a3f972d0f66d4fa562834dc234e3303d4010e7d0e08807f4 | 2026-09-28 | reST | SPEC 9.1; milestone S.2 |
| flint-3.0.1 | FLINT 3.0.1 documentation, `arb_fmpz_poly.rst` (all roots of an integer polynomial, isolated) | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/arb_fmpz_poly.rst | flint-3.0.1/arb_fmpz_poly.rst | dc023312d774353403a44f52dc4894d277e2f67dec3fda52f7154ccde6ed5d5e | 2026-09-28 | reST | SPEC 9.1; milestone S.2 |
| flint-3.0.1 | FLINT 3.0.1 documentation, `arb_poly.rst` (polynomials over balls) | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/arb_poly.rst | flint-3.0.1/arb_poly.rst | 84149b60538df3709aa75f7c4283a2ebfd212e79cf3a5b2e785ce61248d26a5d | 2026-09-28 | reST | SPEC 9.3.3; milestone S.2 |
| flint-src-3.0.1 | FLINT 3.0.1 C sources at the tag v3.0.1, the rational reconstruction of `fmpq` (`reconstruct_fmpz.c`, `reconstruct_fmpz_2.c`, `reconstruct_fmpz_2_naive.c`, `get_cfrac.c`, `get_cfrac_helpers.c`, `cfrac_bound.c`) | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/src/fmpq/reconstruct_fmpz_2.c | flint-src-3.0.1/fmpq/ | see refs/manifest.sha256 | 2026-09-28 | C | milestone S.3 (what FLINT does) |
| flint-src-3.0.1 | FLINT 3.0.1 C sources at the tag v3.0.1, the matrix modules: Hermite form and its transformation, Smith form, Howell form and strong echelon form modulo `mod`, nullspace, solving (`fmpz_mat/*.c`, `nmod_mat/*.c`, `fmpz_mod_mat/*.c`) | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/src/fmpz_mat/howell_form_mod.c | flint-src-3.0.1/fmpz_mat/, flint-src-3.0.1/nmod_mat/, flint-src-3.0.1/fmpz_mod_mat/ | see refs/manifest.sha256 | 2026-09-28 | C | milestone S.1 (what FLINT does) |
| flint-src-3.0.1 | FLINT 3.0.1 C sources at the tag v3.0.1, Hensel lifting of factors, counting real roots, p-adic polynomial evaluation (`fmpz_poly/hensel*.c`, `fmpz_poly/num_real_roots*.c`, `padic_poly/*.c`, `arb_fmpz_poly/complex_roots.c`) | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/src/fmpz_poly/hensel_lift.c | flint-src-3.0.1/fmpz_poly/, flint-src-3.0.1/padic_poly/, flint-src-3.0.1/arb_fmpz_poly/ | see refs/manifest.sha256 | 2026-09-28 | C | milestone S.2 (what FLINT does) |
"""

HEAD = """## Table 3: milestone S, which source settles which statement

Added 2026-09-28 (lane `s-sources`). The rows are the statements the design of `PLAN.md` section 6, milestone S,
needs. Every quote is verbatim from the extraction named in the third column, and every line number was produced
by `lanes/s-sources/mk_table.py`, which slices the quote out of the file; `lanes/s-sources/check_quotes.py` reads
the table back and checks every quote against the file on disk. Rows of the three sub-tables are independent:
a row states what one source says, nothing else. Where two sources differ, the difference is written in the row
and repeated in the notes after the tables. A pipe inside a cell is written `\\|`, a break between two extraction
lines is written " / ".

Sources that are not on disk are listed at the end of this section; nothing in the tables is cited from memory.

### S.3 partial rational reconstruction

"""

MID = """
### S.1 linear systems modulo `N`

"""

TAIL = """
### S.2 roots

"""

NOTES = """
### Notes for the design of milestone S

Where two sources state the same theorem with different hypotheses or different constants, both are in the tables
above. The differences that matter:

1. **The denominator sign (S.3).** Shoup states the bounded problem with `0 < |t| <= t*` (row of
   `shoup-ntb:ntb-v2.txt:4161`), so `t` may be negative; `docs/SPEC.md` 9.2 and
   `proofs/quotient.md` Proposition 13 ask for `0 < d <= B`, and FLINT asserts `fmpz_sgn(d) > 0`. The EEA row of
   Shoup's Theorem 4.9 can have `t0 < 0`; FLINT fixes the sign with the determinant of the accumulated 2x2 matrix
   (`fmpq/reconstruct_fmpz_2.c:1031`). The design must state which sign convention it uses and must not quote
   Shoup's `|t|` as if it were `d`.
2. **The constant is the same (S.3).** `2 A B < m` in the SPEC is Shoup's `n > 2 r* t*`
   (`ntb-v2.txt:4159`) and FLINT's `2ND < m` (`fmpq.rst:563`). No source on disk gives a different constant for
   uniqueness. `2 A B = m` with two solutions (Proposition 13.2) is not contradicted by Shoup: his remark at
   `ntb-v2.txt:4152-4153` says the pair is determined only up to a non-zero multiple, which is exactly what the
   example `1/1` and `-1/1` for `m = 2` exhibits. The sharpness example itself is a project calculation, not a
   quote.
3. **What FLINT guarantees and what it does not (S.3).** The documentation says the answer is unique if it exists
   (`fmpq.rst:565`) and that 0 means no solution. The code, however, accepts the EEA row only when `d <= D` and
   `gcd(n, d) = 1` (`fmpq/reconstruct_fmpz_2.c:1037, 1040`). Under `2 N D < m` these two conditions cannot lose a
   solution: by Shoup's Theorem 4.9(i) the row of the EEA has `|t0| <= t*` whenever a solution with `|t| <= t*`
   exists, and the reduced form of a solution is again a solution inside the same bounds (own calculation). So the
   return value 0 does mean "no solution" under the documented hypotheses, and a caller outside those hypotheses
   (in particular with `2 N D >= m`) gets neither uniqueness nor the meaning of 0. The SPEC's fourth result,
   "uniqueness not certified", is not a value FLINT can return; the design must produce it itself.
4. **Existence is not the SPEC's hypothesis (S.3).** `proofs/quotient.md` Proposition 13 claims at most one
   solution when `2 A B < m`; it does not claim that a solution exists. Existence is Thue's lemma
   (`ntb-v2.txt:2184`) and needs `m > A B` with the strict bounds of that statement. A solver must therefore be
   prepared to return "none" for `2 A B < m`.
5. **The prime-field assumption (S.1).** `nmod_mat_can_solve` and `fmpz_mod_mat_can_solve` are documented for a
   prime modulus (`nmod_mat.rst:562`, `fmpz_mod_mat.rst:388`), and `fmpz_mod_mat_solve` says "The modulus is
   assumed to be prime". For a general `N` the entry point of FLINT 3.0.1 is `fmpz_mat_howell_form_mod(A, mod)`,
   whose source computes gcds against `N` (`fmpz_mat/strong_echelon_form_mod.c:78, 80`), so no primality is used
   there. The design of S.1 for a composite `N` cannot be built on `fmpz_mod_mat`.
6. **The certificate is not where FLINT puts it (S.1).** `fmpz_mat_hnf_transform` returns `U` with `U A = H`
   (`fmpz_mat.rst:1239`); `fmpz_mat_snf` has no such argument (`fmpz_mat.rst:1314`); and
   `nmod_mat_howell_form` works in place and returns only the number of nonzero rows
   (`nmod_mat/howell_form.c:15, 44`). The transformation for the Howell form is, in Storjohann's terms, the tuple
   `(Q, U, C, W, r)` (`diss2up.txt:2078`), and the kernel is the product `W Q U C` (`diss2up.txt:2120`). The
   design of the certificate must either track the row operations itself or take them from the HNF over the
   integers, which is the only transformation matrix FLINT 3.0.1 hands out.
7. **The orientation of the echelon form (S.1).** FLINT returns an upper right normal form and says so
   (`nmod_mat.rst:711`); the strong echelon form of [FieHof2014] is a lower left normal form. Storjohann's
   conditions (r1) to (r4) (`diss2up.txt:2058, 2068, 2070`) are the ones FLINT attributes to [StoMul1998]. A
   design that writes its own row reduction must fix the orientation before it states a normal form.
8. **Two shapes of Hensel's lemma (S.2).** Conrad's Theorem 2.1 (`hensel.txt:31`) is the simple-root form with
   the unique lift; his Theorem 4.1 (`hensel.txt:315`) is the strong form `|f(a)| < |f'(a)|^2`, which also applies
   when `a mod p` is a multiple root (`hensel.txt:310-311`), and it adds `|alpha - a| = |f(a)/f'(a)|`. Thorne's
   Lemma 3.6 (`jackthornenotes.txt:567`) has different hypotheses: `f` monic, `|f(x)| < 1`, `|f'(x)| = 1`, and the
   weaker conclusion `|y - x| <= |f(x)|`, not `|f(x)/f'(x)|`. Baker's Theorem 1.37 (`padicnotes.txt:662`) has a
   third shape, `f(a) ≡ 0 mod p^(2r-1)` and `f'(a) ≢ 0 mod p^r`, and states no uniqueness. The SPEC's square root
   at 2 ("unit part 1 modulo 8") and the loss of one digit can be read off Theorem 4.1; the digit-by-digit proofs
   are in `hensel.txt:37` and `jackthornenotes.txt:580`.
9. **What FLINT does not have (S.2).** `fmpz_poly` lifts factors, not roots (`fmpz_poly.rst:2997`), and
   `padic_poly` has no root finding at all; the greps behind this statement are in the report of the lane. Simple
   roots by Hensel lifting have to be written in the project. For real roots, FLINT 3.0.1 offers two things:
   `fmpz_poly_num_real_roots_sturm` counts the real roots of a squarefree polynomial and returns no intervals
   (`fmpz_poly.rst:3253`), while `arb_fmpz_poly_complex_roots` isolates all the roots of a squarefree integer
   polynomial, real ones first in ascending order (`arb_fmpz_poly.rst:68, 70, 73`), and `arb_calc_isolate_roots`
   isolates the roots of a real analytic function with an explicit completeness rule read off the flags
   (`arb_calc.rst:127, 131`). A "completeness status" for the SPEC is therefore: complete when no flag other than
   1 occurs, and incomplete otherwise, with the two cases that the algorithm cannot isolate at all (multiplicity
   above one, roots at the end points) excluded by the input, not by the status.

### Not on disk, and what it would settle

Nothing below was fetched; no row of the tables above depends on any of it.

1. **P. S. Wang, "Continued fraction expansions of rational numbers" (or the 1981 paper on rational
   reconstruction), and P. S. Wang, R. K. Guy, H. Davenport, "On the determination of a rational number from
   its modular residue" (1982).** Not offered lawfully free; both are cited by Monagan. They would settle the
   attribution and the date of the EEA reconstruction method, and the sharper uniqueness statement with a
   condition on `gcd(d, m)`. Shoup 4.6 already gives the theorem and its proof in full, so nothing in the design
   depends on them. `[source pending: Wang 1981 and Wang-Guy-Davenport 1982, for the attribution only]`
2. **M. Monagan, "Maximal quotient rational reconstruction", ISSAC 2004.** No copy on the author's page or on
   arXiv was found. It would settle the *maximal quotient* variant, that is the largest bound pair for which a
   solution is certified to exist, and the treatment of a bound `B` that is not reached by the EEA. Not needed for
   the bounded problem of `SPEC.md` 9.2. `[source pending: Monagan, maximal quotient rational reconstruction]`
3. **G. E. Collins and M. J. Encarnacion (1995), "Extending thecontinued fraction algorithm for computing
   rational reconstructions".** Not lawfully free. It would settle the incremental (single precision) version of
   the algorithm, which matters only for performance. `[source pending: Collins and Encarnacion 1995]`
4. **J. von zur Gathen and J. Gerhard, "Modern Computer Algebra", Theorem 5.26.** A book, not lawfully free; TJO
   may have a copy. It states the rational reconstruction theorem; Shoup 4.6 replaces it, with the same constant.
   `[source pending: von zur Gathen and Gerhard 5.26, if the project's citation should name it]`
5. **A. Storjohann and T. Mulders, "Fast algorithms for linear algebra modulo N", ESA 1998.** Not lawfully free
   on the author's page (only the later dissertation was found). It is the paper FLINT cites as [StoMul1998] for
   the definition of the Howell form (`nmod_mat.rst:719`). Storjohann's dissertation restates the definition and
   the conditions (r1) to (r4), so the definition is settled; the complexity bounds of the ESA paper are not.
   `[source pending: Storjohann and Mulders 1998, for the complexity bounds]`
6. **M. Fiedler and T. Hofmann, the paper behind [FieHof2014].** The bibliography entry is not in the fetched
   `.rst` files and no copy was found by the searches of this lane. It is the source of FLINT's strong echelon
   form and of `fmpz_mat_hnf_modular_eldiv`. The definition FLINT uses is stated in `nmod_mat.rst:710-712` and the
   algorithm is in `nmod_mat/strong_echelon_form.c`. `[source pending: Fiedler and Hofmann, [FieHof2014]]`
7. **J. A. Howell (1986).** No open copy found. Existence and uniqueness of the Howell form over `Z/(N)` is
   quoted from Storjohann's dissertation (`diss2up.txt:2119-2120`), which cites Howell for it. `[source pending:
   Howell 1986, for the original proof]`
8. **Keith Conrad, the note on the Smith normal form.** It is not in the current index of his expository notes
   (`https://kconrad.math.uconn.edu/blurbs`, checked 2026-09-28: 265 entries, no entry on the Smith or Hermite
   form); the old path `blurb/papers/smithnormalform.pdf` returns 404. Storjohann's dissertation and the FLINT
   documentation are used instead. `[source pending: nothing; the note is not needed]`

### Sources pending for milestone S

1. A statement of the *maximal quotient* rational reconstruction, if the design of S.3 wants to certify existence
   with a bound pair larger than `A B < m`: `[source pending: Monagan 2004, item 2 above]`.
2. The complexity bounds of the modular linear algebra algorithms of [StoMul1998] and [FieHof2014], if the design
   of S.1 needs them for `PERF.md`: `[source pending: Storjohann and Mulders 1998; Fiedler and Hofmann]`.
3. Nothing is pending for S.2: Hensel's lemma in both forms is on disk with proofs, and the FLINT contract for
   real root isolation is on disk.

"""


def main():
    rows = subprocess.run(["python3", os.path.join(HERE, "mk_table.py")],
                          capture_output=True, text=True, check=True).stdout.strip().split("\n")
    s3 = [r for r in rows if r.startswith("| S.3")]
    s1 = [r for r in rows if r.startswith("| S.1")]
    s2 = [r for r in rows if r.startswith("| S.2")]
    assert len(s3) + len(s1) + len(s2) == len(rows), "a row has no S.x prefix"

    def table(rs):
        head = ("| Statement needed | Source key | File and line | Quote, verbatim |\n"
                "|---|---|---|---|\n")
        return head + "\n".join(rs) + "\n"

    section = (HEAD + table(s3) + MID + table(s1) + TAIL + table(s2) + NOTES)

    text = open(DOC, encoding="utf-8").read()
    anchor = "| hvh-mult |"
    i = text.index(anchor)
    j = text.index("\n", i) + 1
    assert TABLE1 not in text
    text = text[:j] + TABLE1 + text[j:]

    k = text.index("\n## Sources pending\n")
    text = text[:k + 1] + section + text[k + 1:]
    open(DOC, "w", encoding="utf-8").write(text)
    print(f"inserted {len(TABLE1.splitlines())} table-1 rows and {len(rows)} quotes")


if __name__ == "__main__":
    main()
