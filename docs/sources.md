# Sources on disk (work package 0.2)

Date: 2026-09-27. Every source the specification leans on is on disk under `refs/src/<key>/` (gitignored:
other people's texts are not redistributed), re-fetchable by `refs/fetch_sources.sh`, with byte hashes in
`refs/manifest.sha256` and text-extraction hashes in `refs/manifest-extra.sha256`. Quotes are cited as
`<key>:<file>:<line>` against these files; for PDFs the quote target is the `pdftotext -layout` extraction
next to the PDF. Format preference: TeX, then HTML or reStructuredText, then PDF plus extraction. Only
lawful, publicly offered copies were fetched.

Tate's thesis is not lawfully available as an open copy. `tate-poonen`, `tate-kudla` and `tate-warwick` are
open expositions of the same theory and are used as substitutes; table 2 says which statement each settles.

The lines of the tables below carry 64-hex hashes and URLs and run longer than the 116-character prose
limit; the prose of this document keeps to the limit. Quotes: leading indentation of the extraction line is
dropped; the control characters that pdftotext uses as symbol placeholders are written \xNN; a quote spanning
two extraction lines shows the break with " / "; a pipe inside a table cell is written \|. A citation with its verbatim quote is data like a table row and
may run past the limit. All 57 quotes were machine-checked against the files on disk.

## Table 1: sources

| Key | Full reference | URL | Path under `refs/src/` | sha256 | Fetched | Format | Used for |
|---|---|---|---|---|---|---|---|
| tate-poonen | B. Poonen, Tate's thesis, notes for MIT 18.786 (2015) | https://math.mit.edu/~poonen/786/notes.pdf | tate-poonen/notes.pdf | 4775c02ca6c2c005445b750fbe2858042061fc1b5f15ef383fe5ddfee165a66b | 2026-09-27 | PDF + txt | SPEC 6, 8, 9.3.7 |
| tate-kudla | S. S. Kudla, Tate's thesis (chapter; copy on a university course page) | https://u.cs.biu.ac.il/~reznikov/courses/kudla-1.pdf | tate-kudla/kudla-1.pdf | 4932f3f8b8795dce6a7736a1d2a47feaf892979e48dddf8cad139ea81d961ae7 | 2026-09-27 | PDF + txt | SPEC 6, 9.3.7 |
| tate-warwick | Warwick Number Theory Study Group, Tate's Thesis, 2023-05-01 | https://warwick.ac.uk/fac/sci/maths/people/staff/sheth/tatesthesis_notes.pdf | tate-warwick/tatesthesis_notes.pdf | ca4ce381aaa7ea60e474f2d26d7ff5197b50f5a0a5af1c6883c500701b9c42cf | 2026-09-27 | PDF + txt | SPEC 8 |
| milne-cft | J. S. Milne, Class Field Theory, v4.03 (2020) | https://www.jmilne.org/math/CourseNotes/CFT.pdf | milne-cft/CFT.pdf | 50d79af78250a9f1117ad9d337e0b231704a533fc707966ed1bfa52e13d498f5 | 2026-09-27 | PDF + txt | SPEC 2, 5, 9.3.7 |
| milne-ant | J. S. Milne, Algebraic Number Theory (course notes) | https://www.jmilne.org/math/CourseNotes/ANT.pdf | milne-ant/ANT.pdf | 24b83c789a89f25aebffb3cbe4ae5ca29edd0075acc072de093d140770630847 | 2026-09-27 | PDF + txt | SPEC 2, 3 (completions, valuations) |
| hertogh-thesis | M. Hertogh, Computing with adèles and idèles, MSc thesis, Leiden 2021 (author's copy; canonical record hdl.handle.net/1887/3249353) | https://raw.githubusercontent.com/mathehertogh/adeles/1acd6362bbedccb35aac7a343eeead17d443b0d1/Computing_with_adeles_and_ideles.pdf | hertogh-thesis/thesis.pdf | 63ddd5ca8fe6b53826f6c1be4763f6636fec695137753607a1c977419acd7e57 | 2026-09-27 | PDF + txt | SPEC 5, 14 |
| adeles-pkg | SageMath package `adeles` (Computing with adèles and idèles), pinned commit 1acd6362bbedccb35aac7a343eeead17d443b0d1 | https://github.com/mathehertogh/adeles | adeles-pkg/COMMIT, adeles-pkg/snapshot/ | c6f67ceca0d1b00f1cb2f3958f4d559e9f099313aa618080241cf4afa370dbf5 (COMMIT) | 2026-09-27 | Python + rst | SPEC 14 |
| granville-ntr | A. Granville, Number Theory Revealed, Appendix 16: p-adic logarithm and dilogarithm | https://dms.umontreal.ca/~revealed/Appendix16.pdf | granville-ntr/Appendix16.pdf | c13202c40b1d18816fcba1f711024ca86286ff8c2efd9f86243e0c929980add9 | 2026-09-27 | PDF + txt | SPEC 9.3.2 (p-adic log, exp) |
| baker-padic | A. J. Baker, An Introduction to p-adic Numbers and p-adic Analysis (course copy) | https://people.willamette.edu/~cstarr/math356/Notes/padicnotes.pdf | baker-padic/padicnotes.pdf | 4c627764406a275c2b30c4ff80a0ddea8b39cfd77d1b4c9d949be570629a42df | 2026-09-27 | PDF + txt | SPEC 9.3.2, 9.3.3 |
| evertse-padic | J. H. Evertse, p-adic numbers (course notes, UConn) | https://kconrad.math.uconn.edu/math5020f11/evertsepadicnotes.pdf | evertse-padic/evertsepadicnotes.pdf | 8f740f273d119f11f1f626a6497be51ce7388d8afe4a0db5ec6c4c5375c40b74 | 2026-09-27 | PDF + txt | SPEC 9.3.2 (exp, log domains) |
| thorne-padic | J. Thorne, p-adic analysis, p-adic arithmetic (course notes, UConn) | https://kconrad.math.uconn.edu/math5020f11/jackthornenotes.pdf | thorne-padic/jackthornenotes.pdf | 82bba78d434f328d0d665a3fbd583b078a491e04a612d002c16c8ef2af6a5edd | 2026-09-27 | PDF + txt | SPEC 9.3.2 (exp, log) |
| hilbert-mit | MIT 18.786, Lecture 2: Hilbert Symbols (2016) | https://ocw.mit.edu/courses/18-786-number-theory-ii-class-field-theory-spring-2016/27f59437d11cbc1a6039b250e4c325ca_MIT18_786S16_lec2.pdf | hilbert-mit/lec2.pdf | 5e853722e03c4c035f366eacbed64db99eaf319a02a3a96ac70b101229de15ac | 2026-09-27 | PDF + txt | SPEC 9.3.7 (Hilbert symbol) |
| hilbert-bristol | The Hilbert symbol, Bristol lecture notes (formulas at odd p, at 2 and at R, with proof) | https://people.maths.bris.ac.uk/~malab/PDFs/Algae_are_more_numb_19.pdf | hilbert-bristol/lecture19.pdf | 9694f749690d252ac22d9c3e69ec838a1894b02ca0c5cb852f9ba3cc3124f252 | 2026-09-27 | PDF + txt | SPEC 9.3.7 (Hilbert symbol) |
| hilbert-msp | S. V. Vostokov, Explicit formulas for the Hilbert symbol, Geom. Topol. Monogr. 3 (2000), 81-89 | https://msp.org/gtm/2000/03/gtm-2000-03-008p.pdf | hilbert-msp/explicit-hilbert.pdf | 9ceea707cfeaf0d77ca529c646971cbc43aee75f1746b5429cecedc402703de2 | 2026-09-27 | PDF + txt | SPEC 9.3.7 (higher norm-residue symbols; later tiers) |
| flint-3.0.1 | FLINT 3.0.1 documentation, `padic.rst` (repository tag v3.0.1) | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/padic.rst | flint-3.0.1/padic.rst | a78c6e4398cbd89aaa687e3c6f61c6a0cf4b5339e69f7d178396855a0fcdc5a0 | 2026-09-27 | reST | SPEC 9.3.2, 4.4 |
| flint-3.0.1 | FLINT 3.0.1 documentation, `fmpz_mod.rst` | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/fmpz_mod.rst | flint-3.0.1/fmpz_mod.rst | dd75c63b7daa022f781b0ccbe2cf5ec6568a134622a65258b86bd08b9641ef8b | 2026-09-27 | reST | SPEC 4.1 (local backend) |
| flint-3.0.1 | FLINT 3.0.1 documentation, `nmod.rst` | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/nmod.rst | flint-3.0.1/nmod.rst | 081846a79539cf282a7bae04f269746678e63d9b60f3687cee536ca3d36f2e41 | 2026-09-27 | reST | SPEC 4.1, PERF 1 |
| flint-3.0.1 | FLINT 3.0.1 documentation, `arb.rst` | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/arb.rst | flint-3.0.1/arb.rst | 2296237bf12759a183eb7e73e5d5e478c13a70ca465c16134193fbfe7cbaf973 | 2026-09-27 | reST | SPEC 4.1, 9.3.6 |
| flint-3.0.1 | FLINT 3.0.1 documentation, `acb.rst` | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/acb.rst | flint-3.0.1/acb.rst | a6d48dde52a8a9d7ddc43f0b1bfe7f45c9ef5655c1412f2828cbf6948d5db987 | 2026-09-27 | reST | SPEC 4.1, 9.3.6 |
| flint-3.0.1 | FLINT 3.0.1 documentation, `acb_dirichlet.rst` | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/acb_dirichlet.rst | flint-3.0.1/acb_dirichlet.rst | 549186a1a53b61bc5f8c668703526a30e2a08e2335e9d0b4daa8b242d173bce3 | 2026-09-27 | reST | SPEC 8, 9.3.7 |
| flint-3.0.1 | FLINT 3.0.1 documentation, `fmpq.rst` | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/fmpq.rst | flint-3.0.1/fmpq.rst | 6b95a74c9535b4aab59a914095952157ccb11e1879640b76d14f3e01755b7a9d | 2026-09-27 | reST | SPEC 4.1 |
| flint-3.0.1 | FLINT 3.0.1 documentation, `ulong_extras.rst` | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/ulong_extras.rst | flint-3.0.1/ulong_extras.rst | 65d6805bd89f8d66bf87f459d4f604b19499e5c9129896a07c72e11d4f773468 | 2026-09-27 | reST | SPEC 9.3.7 (Jacobi) |
| pari-doc | PARI/GP 2.17.4 manual source, `usersch3.tex` (functions; bnfcertify, hilbert, kronecker, bnrinit) | https://pari.math.u-bordeaux.fr/pub/pari/unix/pari-2.17.4.tar.gz | pari-doc/usersch3.tex (tarball: pari-doc/pari.tar.gz) | 5fda898f532e87189eea0f2b7cab9f4e88dbde3a92d025256ffe6d7d7e11737f | 2026-09-27 | TeX | SPEC 9.3.7, 13 |
| pari-doc | PARI/GP 2.17.4 manual source, `usersch1.tex` to `usersch8.tex`, `appb.tex`, `appd.tex` | same tarball | pari-doc/*.tex | see refs/manifest.sha256 | 2026-09-27 | TeX | SPEC 9.3.7, 13 |
| uops-zen2 | uops.info instruction page `ADD_R64_M64` (with AMD Zen 2 measurements) | https://uops.info/html-instr/ADD_R64_M64.html | uops-zen2/ADD_R64_M64.html | 98f4740aee0c1cac404f7c0045ff6f68bc6eddad9e96551fcf34299365fa430a | 2026-09-27 | HTML | PERF 1 |
| uops-zen2 | uops.info instruction page `ADC_R64_M64` (Zen 2) | https://uops.info/html-instr/ADC_R64_M64.html | uops-zen2/ADC_R64_M64.html | 08bf7bb6a884fcefb6e5dcf26b04ae0ae00f7411af17e7b273a06f622fb13bc2 | 2026-09-27 | HTML | PERF 1 |
| uops-zen2 | uops.info instruction page `MUL_R64` (Zen 2) | https://uops.info/html-instr/MUL_R64.html | uops-zen2/MUL_R64.html | c9d8adccabbccb56359a9dfd7e22f1a8c17054fc5ca9b9d229ce37efd7b488f8 | 2026-09-27 | HTML | PERF 1 |
| uops-zen2 | uops.info instruction page `IMUL_R64_R64` (Zen 2) | https://uops.info/html-instr/IMUL_R64_R64.html | uops-zen2/IMUL_R64_R64.html | 919891e67d3289323715ff4e1d2ac482609bd5431f33eca7dd37939e880ee4a7 | 2026-09-27 | HTML | PERF 1 |
| uops-zen2 | uops.info instruction page `IMUL_R64_R64_I8` (Zen 2) | https://uops.info/html-instr/IMUL_R64_R64_I8.html | uops-zen2/IMUL_R64_R64_I8.html | 19269b14d60d35e3b848397bd5ea455e24c557acf0136cc26b9849fa4c86d200 | 2026-09-27 | HTML | PERF 1 |
| uops-zen2 | uops.info instruction page `IMUL_R64_R64_I32` (Zen 2) | https://uops.info/html-instr/IMUL_R64_R64_I32.html | uops-zen2/IMUL_R64_R64_I32.html | 0f10dbe562039609819dd62e446127cd6b5bd3eb9be1f5d8aab5d5a1dd856137 | 2026-09-27 | HTML | PERF 1 |
| uops-zen2 | uops.info instruction page `VPMULUDQ_YMM_YMM_YMM` (Zen 2) | https://uops.info/html-instr/VPMULUDQ_YMM_YMM_YMM.html | uops-zen2/VPMULUDQ_YMM_YMM_YMM.html | dafbbacebe29e23e00db9b8fa1b050ab3e288535ba290157b4db85009a632db2 | 2026-09-27 | HTML | PERF 1 |
| hvh-mult | D. Harvey, J. van der Hoeven, Integer multiplication in time O(n log n), Ann. of Math. 193 (2021), 563-617 | https://hal.science/hal-02070778/file/nlogn.pdf | hvh-mult/nlogn.pdf | 23706afbca829df933be41754743bf760ed01ffe73d499ddb410774c4ee0bf1d | 2026-09-27 | PDF + txt | PERF 3 |

Not fetched, recorded as local reference: the FLINT 3.0.1 headers matching these docs are on this machine
under `/usr/include/flint` (161 files, e.g. `padic.h`, `arb.h`, `acb.h`).

The Sage package snapshot is the whole pinned tree (`adeles-pkg/snapshot/`, 2202 files, including its
committed documentation); every file is hashed in `refs/manifest.sha256`. The clone at
`adeles-pkg/adeles/` keeps the history; `adeles-pkg/COMMIT` records the pin. The fetch script uses
`git clone` plus `git archive` of the pinned commit (the lane rules forbid `git checkout`).

## Table 2: labels `[unverified]` and `[standard]` in `docs/SPEC.md`

The definitions of the labels themselves at SPEC lines 18 and 20 are not statements and get no row. Quotes
are verbatim from the extraction files; `pdftotext` renders some symbols badly (e.g. Greek letters as
blanks). "kept" means the label stays and the reason is given.

| SPEC | Label | First words of the statement | Verdict |
|---|---|---|---|
| 2 (line 47) | [standard] | "A profinite integer is an integer known modulo every N consistently; ... the adeles A = R x A_f." | quoted, see 2.1 |
| 6 (line 259) | [unverified] | "The additive character. Convention of Tate's thesis, section 2.2 ... Fourier transform: hat f(y) = ..." | quoted, see 2.2 |
| 8 (line 317) | [standard] | "Outside Re(s) > 1 the defining integral is not absolutely convergent. ..." | quoted in part, kept in part, see 2.3 |
| 9.3.7 (line 488) | [standard] | "(R5) [standard] throughout, from memory; each formula is to be quoted from a source on disk ..." | quoted in part, kept in part, see 2.4 |
| 9.3.7 (line 508) | [unverified] | "Two conventions: arithmetic, z -> z^(1/u'), and geometric, z -> z^(u') (both in J. S. Milne's notes ...)" | quoted, see 2.5 |
| 13 (line 574) | [unverified] | "Fast class group computations are conditional on the generalised Riemann hypothesis by default; ..." | quoted, see 2.6 |
| 14 (line 581) | [unverified] | "Its documented rules for sum and product of profinite numbers agree with section 4.3, ..." | quoted, see 2.7 |

### 2.1 SPEC 2, line 47: profinite integers, finite adeles, A = R x A_f

- hertogh-thesis:thesis.txt:231: "The ring of profinite K-integers, denoted O, b is the projective limit lim O/I"
  (233: "where I runs over all non-zero ideals of O."; for K = Q this is Zhat = lim Z/N.)
- hertogh-thesis:thesis.txt:245: "extends naturally to an isomorphism K −   b   → AK of K-algebras and topological"
  (K-hat is the finite adele ring A'_K.)
- milne-cft:CFT.txt:9373: "restricted product of the family, .Vv ; Uv /, is the subset of Vv consistingQof the families .av /"
- milne-cft:CFT.txt:9375: "the product topology. Sometimes the restricted product is written 0 Vv . NowQ   AK D v .Kv ; Uv /,"
- milne-cft:CFT.txt:9376: "where Uv D Ov if v is finite and Uv D Kv otherwise."
  For K = Q this is A = R x A_f.

The name `Zhat` is the project's notation and needs no source. Verdict: quoted.

### 2.2 SPEC 6, line 259: the additive character and the Fourier transform

- tate-poonen:notes.txt:693: "• If F = R, let ψ(x) := e−2πix . (The minus sign is there so that a global product"
  (694: "formula later on will hold.)")
- tate-poonen:notes.txt:700: "which is characterized by ψ|Zp = 1 and ψ(1/pn ) = e2πi/p for all n ≥ 1."
- tate-poonen:notes.txt:740: "(Tate originally took the complex conjugate of the additive character, but many references since then have not done so.)"
- tate-kudla:kudla-1.txt:902: "Fourier transforms and the global functional equation. We fix a global additive"
  (903: "character ψ of A, trivial on k.")

Verdict and three side notes:

1. The additive character of SPEC section 6 is exactly the standard psi of tate-poonen: e^(2 pi i (-x_inf +
   sum {x_p}_p)). The minus sign at R and the plus sign of the p-primary fractional parts are both quoted
   above. Triviality on Q is the "global product formula" of tate-poonen:693 and the "trivial on k" of
   tate-kudla:903.
2. The Fourier transform of SPEC section 6 uses conj(psi(x y)), which tate-poonen:740 identifies as Tate's
   original convention. Both fetched expositions define the transform WITHOUT the conjugate
   (tate-poonen:736-737, tate-kudla:706). This is a documented difference of convention, not a
   contradiction: the SPEC follows Tate's original. The kernel signs of the SPEC (real exp(+2 pi i x y),
   finite exp(-2 pi i x y)) follow from its own psi by taking the conjugate; this is an own calculation.
3. The section number "2.2" cannot be checked: no lawful copy of Tate's thesis is on disk.
   [source pending: a lawful copy of Tate's thesis, to pin the section number and Tate's own psi.]

### 2.3 SPEC 8, line 317: continuation outside Re(s) > 1, functional equation

- tate-poonen:notes.txt:1720: "(a) The integral Z(f, χ) converges for idèle-class characters χ of exponent σ > 1."
- tate-poonen:notes.txt:1725: "(c) We have Z(f, χ) = Z(fb, χ∨ ) as meromorphic functions of χ ∈ X ."
- tate-warwick:tatesthesis_notes.txt:520: "ζ(f, χ, s) = ζ(fb, χ−1 , 1 − s)."

Verdict: the analytic content is quoted. The clause "is a proof obligation of work package 0.3" is kept,
because it assigns proof work inside this project and is not a claim about a source. The value of the zeta
case of SPEC 8 is quoted in tate-warwick:tatesthesis_notes.txt:512: "=π              Γ(s/2) ·       (1 − p       )"
(that is pi^(-s/2) Gamma(s/2) times the Euler product), from the example starting at line 493.

### 2.4 SPEC 9.3.7, line 488 (R5): the catalogue of formulas

One sub-row per formula of the catalogue. "kept" entries name the reason.

| Formula | Verdict |
|---|---|
| Legendre (a/p), Jacobi (a/b), "FLINT has them" | flint-3.0.1:ulong_extras.rst:456: ".. function:: int n_jacobi(mp_limb_signed_t x, ulong y)" (458: "Computes the Jacobi symbol `\left(\frac{x}{y}\right)` for any `x` and odd `y`.") |
| Kronecker (a/b), value of (a/2) | pari-doc:usersch3.tex:9270: "\item $(x\|2) = 0$ if $x$ is even and $1$ if $x = 1,-1 \mod 8$ and $-1$" (9271: "if $x=3,-3 \mod 8$."). This is (-1)^((a^2-1)/8) for odd a; an own calculation. Negative and zero b: pari-doc:usersch3.tex:9266 and 9268 |
| Hilbert symbol: definition, formulas at odd p, at 2, at R, product formula | hilbert-bristol:lecture19.txt:9: "+1 if z 2 − ax − by 2 = 0 has a nontrivial solution in k 3 ,"; 46: "If k = Qp and if we write a = pα u, b = pβ w, with u, w ∈ Z∗p , we have"; 56: "(a, b)2 = (−1) 2 2          8        8             ." (with proof from line 58); 89: "Theorem 3. If a, b ∈ Q∗ , we have (a, b)p = 1 for almost all primes p and" |
| local zeta factor (trivial character) | tate-poonen:notes.txt:1733: "For all but finitely many v, the function fv is 1Ov and Z(fv , \| \|σv ) = (1 − qv−σ )−1 , so we need" (non-archimedean) and tate-poonen:notes.txt:1016: "= π −s/2 Γ(s/2)" (real place). The pole locations follow from these factors; own calculation |
| Gauss sums | tate-poonen:notes.txt:1055: "g(ω, ψ) :=        ω(x) ψ(x) d× x." (over O times, with the additive character of section 6) |
| local constants (local functional equation) | tate-poonen:notes.txt:935-937: "Z(fb, χ∨ )               Z(f, χ)" / "= ϵ(χ, ψ, dx)                                     (5)" / "L(χ∨ )                   L(χ)". The SPEC's gamma form is this equation rearranged; own calculation |
| Haar volume of a finite ball, 1/N | kept, because it is this project's measure convention (volume 1 for Zhat) and an immediate consequence of additivity; no external formula is used |
| profinite power a^x | kept, because the criterion (c^M = 1 mod N) and the coarser coset are proved in this project (review R3) |
| binomial coefficient binom(x, k) | kept, because [proved] in this project (review R3, section on binomials) |
| content of an idele | kept as definitional (SPEC 5); the uniqueness of the decomposition is supported by milne-cft:CFT.txt:9853: "P ROOF. Any idèle a D .a1 ; a2 ; : : : ; ap ; : : :/ can be written" and 9858: "—take a D . sign.a1 // p ordp .ap / , t D a1 =a, up D ap =a. Moreover, the expression" |
| theta series of a test function | kept, because Poisson summation with certified tails is proved in work packages 0.3 and 0.4 of this project |
| cyclotomic action | see 2.5 |

### 2.5 SPEC 9.3.7, line 508: cyclotomic action, two conventions

- milne-cft:CFT.txt:9883: "In this case, the global reciprocity map is the reciprocal of"
- milne-cft:CFT.txt:9884: "\x1EW IQ ! ZO \x02 ! Gal.Qcyc =Q/;"
- milne-cft:CFT.txt:9885: "where IQ ! ZO \x02 is the above projection map, and ZO \x02 ! Gal.Q cyc =Q/ is the canonical"
- milne-cft:CFT.txt:9886: "isomorphism (see I A.5c)."
- milne-cft:CFT.txt:6015: "(a) for every prime element \x19 of K, \x1EK .\x19/jK un D FrobK I" (normalisation of the
  local Artin map)
- milne-cft:CFT.txt:1307: "such that \x1B ˛ \x11 ˛ q mod mL for all ˛ 2 OL . This \x1B is called the Frobenius element of"
  (Frobenius is x -> x^q, the arithmetic one)
- milne-cft:CFT.txt:9904: "(b) For every prime v of K unramified in L, the idèle"
- milne-cft:CFT.txt:9905: "˛ D .1; : : : ; 1; \x19 ; 1; : : :/;     \x19 a prime element of Ov ;"
- milne-cft:CFT.txt:9908: "maps to the Frobenius element .pv ; L=K/ in Gal.L=K/:"

Verdict: quoted. Both formulas of the SPEC appear in Milne's notes: the canonical isomorphism sends u' to
z -> z^(u') (the SPEC's "geometric" name), and the global reciprocity map is its reciprocal, z -> z^(1/u')
(the SPEC's "arithmetic" name, matching Milne's normalisation). The test vector of the SPEC is confirmed by
milne-cft:CFT.txt:9908 together with 1307: the idele with p at the place p and 1 elsewhere acts on roots of
order prime to p as z -> z^p, which is z^(1/u') for u' = 1/p away from p. Two side notes: Milne's notes do
not use the names "arithmetic" and "geometric" for the two maps [source pending: a source for these names
as applied to the cyclotomic action]; and the extraction drops the Greek letters, so the quotes show blanks
for zeta and psi.

### 2.6 SPEC 13, line 574: GRH and bnfcertify

- pari-doc:usersch3.tex:19478: "\misctitle{Warning} Make sure you understand the above! By default, most of"
  (19479: "the \kbd{bnf} routines depend on the correctness of the GRH. In particular,")
- pari-doc:usersch3.tex:19484: "group. You must use \kbd{bnfcertify} to certify the computations"
  (19485: "unconditionally.")
- pari-doc:usersch3.tex:19556: "\kbd{bnfinit}, checks whether the result is correct, i.e.~whether it is"
- pari-doc:usersch3.tex:19557: "possible to remove the assumption of the Generalized Riemann"
- pari-doc:usersch3.tex:19558: "Hypothesis\sidx{GRH}. It is correct if and only if the answer is 1. If it is"

Verdict: quoted.

### 2.7 SPEC 14, line 581: Hertogh's rules and his equality

- hertogh-thesis:thesis.txt:374: "be the unique representations of K-integers with smallest represented subsets"
- hertogh-thesis:thesis.txt:375: "(with respect to inclusion) satisfying"
- hertogh-thesis:thesis.txt:377: "R(a) + R(b) ⊆ R(a + b),     R(a) − R(b) ⊆ R(a − b),      R(a)R(b) ⊆ R(ab)."
- hertogh-thesis:thesis.txt:386: "a + b = (v(a) + v(b)) mod gcd(m(a), m(b)),"
- hertogh-thesis:thesis.txt:394: "ab = v(a)v(b) mod gcd(v(a)m(b), v(b)m(a), m(a)m(b))."
- hertogh-thesis:thesis.txt:1525: "We call a and b loosely equivalent if R(a) ∩ R(b) 6= ∅, i.e. there exists an"
- hertogh-thesis:thesis.txt:1529: "define equivalence relations, while loose equivalence fails to be transitive in gen-"
- hertogh-thesis:thesis.txt:1538: "we ended up implementing loose equivalence for == comparison of our own rep-"
- hertogh-thesis:thesis.txt:849: "We define a representation of multiplicative p-adics to be a pair a = (x, n) ∈"
  (853: "the precision of a, denoted p(a)."; 866: "(c(a)c(b), min(p(a), p(b))).")

Verdict: quoted, and all three sub-claims of SPEC 14 hold. (i) The sum rule of the thesis is exactly SPEC
4.3 for sums; the product rule is exactly the SPEC's radius gcd(a M, b N, N M); the phrase "smallest
represented subsets ... with respect to inclusion" is the SPEC's tightness. (ii) The multiplicative p-adic
representations carry their own precision n per prime and multiply with min of precisions; this is the
"separate multiplicative precision for ideles" of the SPEC. (iii) The equality of the package is loose
equivalence, i.e. the overlap test R(a) ∩ R(b) != empty, and the thesis itself says it fails to be
transitive. The SPEC does not adopt it; the thesis chose it deliberately (thesis.txt:1534-1538).

## Sources pending

1. Tate's thesis itself, for "section 2.2" and Tate's own signs (SPEC 6).
2. A source for the names "arithmetic" and "geometric" convention of the cyclotomic action (SPEC 9.3.7);
   the two formulas themselves are quoted from milne-cft.
3. An open text with proofs of the domains of convergence of the p-adic sine, cosine, sinh and cosh series
   (SPEC 9.3.2). The fetched texts cover exp and log with proofs (granville-ntr, evertse-padic, baker-padic)
   but not the trigonometric series.
4. A uops.info page for the register-register form `add r64, r64` on Zen 2; the site serves the memory
   form `ADD_R64_M64` under that name (see the report for what this means for PERF.md section 1).
