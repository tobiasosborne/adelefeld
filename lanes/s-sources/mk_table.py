#!/usr/bin/env python3
"""Build the "Milestone S" table of docs/sources.md from the files under refs/src/.

Every quote is sliced out of the file at the cited line, so it is verbatim by construction.
A part is (line, start, end): the text from the first occurrence of `start` to the first
occurrence of `end` after it (inclusive); None means "from the first non-space character to
the last non-space character", that is, the whole line without leading indentation.
Parts are joined with " / ". A pipe inside a cell is written \\|.

Usage:  python3 lanes/s-sources/mk_table.py > /tmp/table.md
"""
import os
import sys

REFS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "refs", "src")

# (statement, key, file, [parts])
ROWS = [
    # ------------------------------------------------------------------ S.3
    ("S.3: existence. For `m`, `c` and bounds with `m > A B` there is at least one `n/d` with "
     "`n = c d mod m`, `|n| < A`, `0 < |d| < B` (Thue's lemma)", "shoup-ntb", "ntb-v2.txt",
     [(2184, None, None), (2185, None, None)]),
    ("S.3: the object of the problem is the ratio `n/d`, not the pair `(n, d)`: from one solution "
     "one gets all the others by a non-zero multiple", "shoup-ntb", "ntb-v2.txt",
     [(4152, "if r ≡ bt (mod n)", "(mod n),"), (4153, "and so we can only hope", "ratio r/t is unique.")]),
    ("S.3: uniqueness. If `2 A B < m` then the ratio is unique: `2 A B < m` is Shoup's "
     "`n > 2 r* t*`", "shoup-ntb", "ntb-v2.txt", [(4159, None, None), (4164, None, None)]),
    ("S.3: the key inequality of that proof: `|n1 d2 - n2 d1| <= 2 A B < m`", "shoup-ntb",
     "ntb-v2.txt", [(4170, "However, we also have", "However, we also have"),
                    (4171, None, None)]),
    ("S.3: the bound is stated with `0 < |t| <= t*` for the denominator (a sign is allowed), while "
     "the SPEC asks for `0 < d <= B`", "shoup-ntb", "ntb-v2.txt", [(4161, None, None)]),
    ("S.3: which row of the remainder sequence is the answer: the smallest index `j` with "
     "`rj <= r*`, and the answer is that row", "shoup-ntb", "ntb-v2.txt",
     [(4192, "smallest index", "and set"), (4196, "r 0 := rj", "t0 := tj .")]),
    ("S.3: what that row gives: `0 < |t0| <= t*` in every case, and under `2 A B < m` the pair "
     "is the given one up to a common factor", "shoup-ntb", "ntb-v2.txt",
     [(4202, None, None), (4203, None, None), (4204, None, None)]),
    ("S.3: the denominator bound for the chosen row is the hard part of the theorem", "shoup-ntb",
     "ntb-v2.txt", [(4215, "This is the hardest part", "let")]),
    ("S.3: the same stopping rule for the non-reconstructive case: the first remainder below `r*` "
     "(existence)", "shoup-ntb", "ntb-v2.txt",
     [(4045, "smallest index", "and set".replace("and set", "Then, setting r := rj and")),
      (4048, "t := tj", "t := tj")]),
    ("S.3: FLINT 3.0.1 `fmpq_reconstruct_fmpz_2` promises exactly the bounded problem with the "
     "condition `2 N D < m` and the reducedness of the answer", "flint-3.0.1", "fmpq.rst",
     [(560, None, None), (562, None, None), (563, None, None), (564, None, None), (565, None, None),
      (566, None, None), (567, None, None)]),
    ("S.3: FLINT 3.0.1 `fmpq_reconstruct_fmpz` has no bounds argument; the bounds are fixed at "
     "`N = D = floor(sqrt((m-1)/2))`", "flint-3.0.1", "fmpq.rst", [(574, None, None)]),
    ("S.3: what the FLINT code does: the extended Euclidean algorithm on `(m, a)`, the quotients "
     "accumulated into a 2x2 matrix, stopped at the first remainder `<= N`", "flint-src-3.0.1",
     "fmpq/reconstruct_fmpz_2.c", [(980, None, None)]),
    ("S.3: the answer of FLINT is the last remainder `B` with the denominator taken from the "
     "accumulated matrix, the sign fixed by the determinant", "flint-src-3.0.1",
     "fmpq/reconstruct_fmpz_2.c",
     [(1024, None, None), (1026, None, None), (1027, None, None), (1031, None, None)]),
    ("S.3: FLINT accepts the row only if the denominator is within `D` and the fraction is "
     "reduced; otherwise it returns 0", "flint-src-3.0.1", "fmpq/reconstruct_fmpz_2.c",
     [(1036, None, None), (1037, None, None), (1039, None, None), (1040, None, None)]),
    ("S.3: FLINT tries the integers `a` and `a - m` with denominator 1 before the algorithm",
     "flint-src-3.0.1", "fmpq/reconstruct_fmpz_2.c", [(940, None, None), (941, None, None),
                                                     (949, None, None), (950, None, None)]),
    ("S.3: the balanced bound of FLINT in the source: `N` is `m >> 1`, lowered by 1 when `m` is "
     "even, then `N = D = sqrt(N)`", "flint-src-3.0.1", "fmpq/reconstruct_fmpz.c",
     [(22, None, None), (25, None, None), (26, None, None)]),
    # ------------------------------------------------------------------ S.1
    ("S.1: the ring of the problem: `Z/(N)` is handled as a principal ideal ring, and a residue "
     "class ring of a PID is a stable PIR", "storjohann-thesis", "diss2up.txt",
     [(224, "For the other forms", "ring we"), (225, "work over is a principal ideal ring",
      "with identity"), (226, "in which every ideal is principal", "is principal.")]),
    ("S.1: the use of the form: it is the canonical form for solving systems of linear equations "
     "over the ring of entries", "storjohann-thesis", "diss2up.txt",
     [(221, None, None), (223, None, None)]),
    ("S.1: the transforming matrix is unimodular, that is invertible over the ring",
     "storjohann-thesis", "diss2up.txt", [(209, "The transforming matrix U is unimodular", "that U"),
                                          (210, "is invertible over R", "over R.")]),
    ("S.1: the Hermite form is not canonical for left equivalence over a PIR (it is canonical "
     "only over a PID)", "storjohann-thesis", "diss2up.txt",
     [(887, "we may conclude", "canon-"), (888, "ical form for left equivalence", "a PIR.")]),
    ("S.1: the four conditions an echelon form over a PIR may satisfy; the Howell form is an "
     "echelon form with a maximal number of rows", "storjohann-thesis", "diss2up.txt",
     [(417, "The Howell form of A", "maximal number"), (418, "of rows.", "of rows.")]),
    ("S.1: the conditions of the Howell form itself: (r1) echelon position of the first nonzero "
     "entry of each row", "storjohann-thesis", "diss2up.txt",
     [(2058, "(r1)", "first r rows"), (2065, "of H are nonzero", "nonzero."), (2066, "entry in row i",
      "j1 < j2 < . . . < jr .")]),
    ("S.1: (r2) each pivot generates an ideal, every entry above it is in the ring of multipliers "
     "of that ideal", "storjohann-thesis", "diss2up.txt", [(2068, "(r2)", "1 ≤ k < i ≤ r.")]),
    ("S.1: (r4) the Howell property: rows `i+1, ..., r` generate the rows of `A` whose first `j_i` "
     "entries are zero", "storjohann-thesis", "diss2up.txt", [(2070, None, None)]),
    ("S.1: the name and the consequence: the first `r` rows of `H` are a canonical generating set "
     "for the row module `S(A)`", "storjohann-thesis", "diss2up.txt",
     [(2075, None, None), (2077, "basis of A", "S(A).")]),
    ("S.1: the certificate: a Howell transform is the 5-tuple `(Q, U, C, W, r)`, with `U` "
     "unimodular and `W` a kernel of the remaining block `T`", "storjohann-thesis", "diss2up.txt",
     [(2078, "A Howell transform for A", "(Q, U, C, W, r)"), (2084, "which satisfies and can be",
      "block decomposition"), (2094, None, None), (2099, "that is W ∈", "vA = 0}.")]),
    ("S.1: a kernel of `A` is the product `W Q U C` (it may be dense; a factorisation is also "
     "given)", "storjohann-thesis", "diss2up.txt", [(2120, "W QU C is a kernel for A", "kernel for A")]),
    ("S.1: solving with the transform: `b` is in the row module exactly when the Howell form of the "
     "right-hand side has zero last block, and then `x` is read off the transform", "storjohann-thesis",
     "diss2up.txt", [(2112, "with the right hand side in Howell form", "if and only if"),
                     (2114, "b0 = 0. If b ∈ S(A)", "U C.")]),
    ("S.1: existence and uniqueness of the Howell form over `Z/(N)`: Howell (1986); over an "
     "arbitrary PIR: Buchmann and Neis (1996)", "storjohann-thesis", "diss2up.txt",
     [(2119, "Existence and uniqueness", "by Howell"), (2120, "(1986) for matrices", "constructive"),
      (2121, "and leads to an O(n3 )", "Neis"), (2122, "(1996) give a proof", "uniqueness")]),
    ("S.1: the one-equation case over `Z/(n)`: solvable iff `gcd(a, n)` divides `b`, and the "
     "solution is unique modulo `n / gcd(a, n)`", "shoup-ntb", "ntb-v2.txt",
     [(1221, None, None), (1222, None, None), (1223, None, None), (1224, None, None)]),
    ("S.1: FLINT 3.0.1 `fmpz_mat_hnf_transform` gives the Hermite form together with `U` such that "
     "`U A = H`: the certificate for the integral case", "flint-3.0.1", "fmpz_mat.rst",
     [(1235, None, None), (1237, None, None), (1238, None, None), (1239, None, None)]),
    ("S.1: FLINT 3.0.1 `fmpz_mat_snf` takes and returns only `S` and `A`: the Smith form comes "
     "without a transformation matrix", "flint-3.0.1", "fmpz_mat.rst", [(1314, None, None)]),
    ("S.1: FLINT 3.0.1 `fmpz_mat_howell_form_mod` computes the Howell form modulo a given `mod`, "
     "in place, and returns the number of nonzero rows", "flint-3.0.1", "fmpz_mat.rst",
     [(1168, None, None), (1170, None, None), (1171, None, None), (1172, None, None),
      (1176, None, None)]),
    ("S.1: the same function for a prime modulus is `nmod_mat_howell_form`; the definition used is "
     "the one of [StoMul1998]", "flint-3.0.1", "nmod_mat.rst",
     [(716, None, None), (718, None, None), (719, None, None), (720, None, None)]),
    ("S.1: the strong echelon form of FLINT is the Howell form up to a permutation of the rows, but "
     "with the opposite orientation: upper right against lower left", "flint-3.0.1", "nmod_mat.rst",
     [(707, None, None), (710, None, None), (711, None, None), (712, None, None)]),
    ("S.1: the source of `nmod_mat_howell_form` works in place and returns only the count of the "
     "nonzero rows: no transformation matrix is produced", "flint-src-3.0.1", "nmod_mat/howell_form.c",
     [(15, None, None), (26, None, None), (44, None, None)]),
    ("S.1: the modular Howell form of `fmpz_mat` is computed with gcds against `N`, not with "
     "inverses modulo a prime, so no primality of the modulus is assumed", "flint-src-3.0.1",
     "fmpz_mat/strong_echelon_form_mod.c", [(67, "_fmpz_stab(fmpz_t t", "const fmpz_t N)"),
                                             (78, None, None), (80, None, None)]),
    ("S.1: the solving functions of FLINT 3.0.1 for a prime field: a solution is returned if one "
     "exists, `0` and a zeroed `X` otherwise; `A` may be singular", "flint-3.0.1", "nmod_mat.rst",
     # line 561 contains " / " inside the ring Z / pZ, so the quote of that line stops before it
     [(561, "Solves the matrix-matrix equation", "over"), (562, None, None), (565, None, None),
      (566, None, None), (569, None, None)]),
    ("S.1: `fmpz_mod_mat` is a prime-field module: its solving functions say so", "flint-3.0.1",
     "fmpz_mod_mat.rst", [(376, None, None), (380, None, None), (388, None, None)]),
    ("S.1: the kernel over a prime field: `nmod_mat_nullspace` returns a maximal rank matrix with "
     "`A X = 0`", "flint-3.0.1", "nmod_mat.rst", [(647, None, None), (649, None, None),
                                                  (650, None, None), (651, None, None)]),
    ("S.1: the kernel over `Q`: `fmpz_mat_nullspace` returns a basis of the right nullspace, "
     "entries not minimal", "flint-3.0.1", "fmpz_mat.rst", [(1185, None, None), (1186, None, None),
                                                           (1187, None, None)]),
    # ------------------------------------------------------------------ S.2
    ("S.2: Hensel's lemma for a simple root, with the uniqueness of the lifted root",
     "conrad-hensel", "hensel.txt", [(31, None, None), (32, None, None), (33, None, None)]),
    ("S.2: its proof, by induction on the exponent, digit by digit", "conrad-hensel", "hensel.txt",
     [(37, None, None), (38, None, None), (39, None, None)]),
    ("S.2: the stronger form `|f(a)| < |f'(a)|^2`, with the distance of the root and the "
     "uniqueness", "conrad-hensel", "hensel.txt",
     [(315, None, None), (316, None, None), (317, None, None), (318, None, None), (319, None, None)]),
    ("S.2: the strong form is not a special case of the simple-root form: it also applies when "
     "`a mod p` is a multiple root", "conrad-hensel", "hensel.txt",
     [(310, "It can be applied", "multiple"), (311, "root of f (X) mod p", "mod p.")]),
    ("S.2: Hensel's lemma for `Z[X]` with the explicit lift formula (Baker, Theorem 1.33)",
     "baker-padic", "padicnotes.txt", [(572, None, None), (573, None, None), (574, None, None)]),
    ("S.2: a general version with the hypotheses `f (a) ≡ 0 mod p^(2r-1)` and "
     "`f' (a) ≢ 0 mod p^r` (Baker, Theorem 1.37); no uniqueness is stated there",
     "baker-padic", "padicnotes.txt",
     [(662, None, None), (670, None, None), (671, None, None), (672, None, None)]),
    ("S.2: a different hypothesis shape over a local field: `|f (x)| < 1` and `|f' (x)| = 1` for a "
     "monic polynomial, with the weaker conclusion `|y - x| <= |f (x)|` (Thorne, Lemma 3.6)",
     "thorne-padic", "jackthornenotes.txt",
     [(567, None, None), (569, None, None), (570, None, None), (571, None, None), (572, None, None)]),
    ("S.2: the same note states the simple-root case over `Z_p` and proves it digit by digit "
     "(Thorne, Lemma 3.7)", "thorne-padic", "jackthornenotes.txt",
     [(574, None, None), (576, None, None), (577, None, None), (578, None, None), (579, None, None),
      (580, None, None)]),
    ("S.2: FLINT 3.0.1 `fmpz_poly` lifts factors, not roots: `fmpz_poly_hensel_lift_once` lifts a "
     "squarefree product of local factors to `p^N`", "flint-3.0.1", "fmpz_poly.rst",
     [(2993, None, None), (2995, None, None), (2997, None, None), (2998, None, None),
      (3000, None, None)]),
    ("S.2: FLINT 3.0.1 counts real roots exactly by a Sturm sequence, for a squarefree input, and "
     "returns no isolating intervals", "flint-3.0.1", "fmpz_poly.rst",
     [(3251, None, None), (3253, None, None), (3254, None, None), (3256, None, None)]),
    ("S.2: the certified isolation of real roots of a real analytic function: a flag of 1 means "
     "exactly one root, any other flag is undetermined, and completeness is read off the flags",
     "flint-3.0.1", "arb_calc.rst", [(127, None, None), (129, None, None), (131, None, None),
                                      (132, None, None), (133, None, None)]),
    ("S.2: what the isolation cannot do: roots of multiplicity above one, and roots at the end "
     "points, are not isolated", "flint-3.0.1", "arb_calc.rst",
     [(136, None, None), (137, None, None), (138, None, None)]),
    ("S.2: FLINT 3.0.1 isolates all the roots of an integer polynomial at once: the enclosures are "
     "disjoint, so all roots are isolated, but the input must be squarefree", "flint-3.0.1",
     "arb_fmpz_poly.rst", [(66, None, None), (68, None, None), (70, None, None), (71, None, None),
                            (79, None, None)]),
    ("S.2: FLINT's own remark on that function: adequate, but not competitive with the state of the "
     "art for real roots alone", "flint-3.0.1", "arb_fmpz_poly.rst",
     [(103, None, None), (104, None, None), (105, None, None)]),
    ("S.2: the `p`-adic polynomial of FLINT 3.0.1 has no root finding; it offers the integrality "
     "test that a Hensel step needs", "flint-3.0.1", "padic_poly.rst",
     [(198, None, None), (200, None, None), (201, None, None), (202, None, None)]),
    ("S.2: the precision of an evaluation at a `p`-adic point, as FLINT states it", "flint-3.0.1",
     "padic_poly.rst", [(438, None, None), (446, None, None), (447, None, None)]),
]


def slice_part(lines, line, start, end):
    text = lines[line - 1]
    if start is None:
        return text.strip()
    i = text.index(start)
    j = text.index(end, i) + len(end)
    return text[i:j]


def main():
    for statement, key, rel, parts in ROWS:
        path = os.path.join(REFS, key, rel)
        with open(path, encoding="utf-8") as fh:
            lines = fh.read().split("\n")
        try:
            quote = " / ".join(slice_part(lines, *part) for part in parts)
        except ValueError as exc:
            raise SystemExit(f"row {ROWS.index((statement, key, rel, parts)) + 1} of {key}:{rel}: {exc}")
        quote = quote.replace("|", "\\|")
        where = ", ".join(str(p[0]) for p in parts)
        statement = statement.replace("|", "\\|")
        print(f"| {statement} | {key} | {rel}:{where} | \"{quote}\" |")
    print(f"# {len(ROWS)} rows", file=sys.stderr)


if __name__ == "__main__":
    main()
