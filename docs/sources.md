# Sources on disk (work package 0.2)

Date: 2026-09-27 (Intel uops.info rows added 2026-09-28). Every source the specification leans on is on disk
under `refs/src/<key>/` (gitignored: other people's texts are not redistributed), re-fetchable by
`refs/fetch_sources.sh` and, for the Intel CPU pages, `refs/fetch_intel.sh`, with byte hashes in
`refs/manifest.sha256` and `refs/manifest-intel.sha256` and text-extraction hashes in
`refs/manifest-extra.sha256`. Quotes are cited as
`<key>:<file>:<line>` against these files; for PDFs the quote target is the `pdftotext -layout` extraction
next to the PDF. Format preference: TeX, then HTML or reStructuredText, then PDF plus extraction. Only
lawful, publicly offered copies were fetched.

Tate's thesis is on disk since 2026-09-28 under the key `tate-thesis`: a scan of Cassels and Froehlich,
Algebraic Number Theory (1967), supplied by TJO; it is not fetched by `fetch_sources.sh` and not redistributed.
The scan has no text layer. `pages/p305.png` to `pages/p347.png` are the book pages of chapter XV, one file
each, and are the ground truth. `tex/p<N>.tex` is an OCR draft of page N by a model (codex `gpt-6-luna`,
xhigh): it is UNTRUSTED, carries the sha256 of its scan in its first lines, and may be cited only after a
referee of another model family has compared it with the scan and recorded that in the file. Until then a
statement of Tate is cited as `tate-thesis:pages/p<N>.png` with the quoted words read from the scan.
`tate-poonen`, `tate-kudla` and `tate-warwick` are open expositions of the same theory and remain in use;
table 2 says which statement each settles.

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
| tate-thesis | J. W. S. Cassels, A. Froehlich (eds.), Algebraic Number Theory, Academic Press 1967; chapter XV: J. T. Tate, Fourier analysis in number fields and Hecke's zeta-functions (thesis, Princeton 1950), pages 305 to 347 | local copy supplied by TJO (not fetched) | tate-thesis/cassels-frohlich.pdf | 01259375a6275b726e836ce209cee40ed6f98a5cefe1aa5ad40f2ad07a63169e | 2026-09-28 | PDF scan (no text layer) + page images + untrusted OCR in TeX | SPEC 6, 8, 9.3.7; milestone 5 |
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
| uops-intel | uops.info instruction page `ADC_11_R64_R64` (all microarchitectures, Alder Lake-P column for the laptop) | https://uops.info/html-instr/ADC_11_R64_R64.html | uops-intel/ADC_11_R64_R64.html | f7368a5a60c7c31156a49879c1eda17b1d73c18c0e8cd3cc3576cac36a921bf1 | 2026-09-28 | HTML | PERF 1 |
| uops-intel | uops.info instruction page `ADC_R64_M64` | https://uops.info/html-instr/ADC_R64_M64.html | uops-intel/ADC_R64_M64.html | 08bf7bb6a884fcefb6e5dcf26b04ae0ae00f7411af17e7b273a06f622fb13bc2 | 2026-09-28 | HTML | PERF 1 |
| uops-intel | uops.info instruction page `ADD_01_R64_R64` | https://uops.info/html-instr/ADD_01_R64_R64.html | uops-intel/ADD_01_R64_R64.html | b0323fd9229921c65c761fc96a6ca807047c5b0901043bca921ed91ce6fca603 | 2026-09-28 | HTML | PERF 1 |
| uops-intel | uops.info instruction page `ADD_R64_I8` | https://uops.info/html-instr/ADD_R64_I8.html | uops-intel/ADD_R64_I8.html | 934a877098e4c55f2d1452974fe5c3b9aaaa1647212e7ed414073aa24837f87a | 2026-09-28 | HTML | PERF 1 |
| uops-intel | uops.info instruction page `ADD_R64_M64` | https://uops.info/html-instr/ADD_R64_M64.html | uops-intel/ADD_R64_M64.html | 98f4740aee0c1cac404f7c0045ff6f68bc6eddad9e96551fcf34299365fa430a | 2026-09-28 | HTML | PERF 1 |
| uops-intel | uops.info instruction page `CMOVB_R64_R64` | https://uops.info/html-instr/CMOVB_R64_R64.html | uops-intel/CMOVB_R64_R64.html | 62bc67687557b2895b3a09a4af10e2e66fa17b33d916e3985af4eeb21071184b | 2026-09-28 | HTML | PERF 1 |
| uops-intel | uops.info instruction page `CMOVNB_R64_R64` | https://uops.info/html-instr/CMOVNB_R64_R64.html | uops-intel/CMOVNB_R64_R64.html | 835fc043ecc7ff3656a83e65dd8268ec56d859441b25c878750c162d0a1b7580 | 2026-09-28 | HTML | PERF 1 |
| uops-intel | uops.info instruction page `CMP_39_R64_R64` | https://uops.info/html-instr/CMP_39_R64_R64.html | uops-intel/CMP_39_R64_R64.html | f398058094a26dc28dce0b2502af9b3fc1fe628b382920678ba7e226df3bf9bc | 2026-09-28 | HTML | PERF 1 |
| uops-intel | uops.info instruction page `IMUL_R64_R64` | https://uops.info/html-instr/IMUL_R64_R64.html | uops-intel/IMUL_R64_R64.html | 919891e67d3289323715ff4e1d2ac482609bd5431f33eca7dd37939e880ee4a7 | 2026-09-28 | HTML | PERF 1 |
| uops-intel | uops.info instruction page `IMUL_R64_R64_I32` | https://uops.info/html-instr/IMUL_R64_R64_I32.html | uops-intel/IMUL_R64_R64_I32.html | 0f10dbe562039609819dd62e446127cd6b5bd3eb9be1f5d8aab5d5a1dd856137 | 2026-09-28 | HTML | PERF 1 |
| uops-intel | uops.info instruction page `IMUL_R64_R64_I8` | https://uops.info/html-instr/IMUL_R64_R64_I8.html | uops-intel/IMUL_R64_R64_I8.html | 19269b14d60d35e3b848397bd5ea455e24c557acf0136cc26b9849fa4c86d200 | 2026-09-28 | HTML | PERF 1 |
| uops-intel | uops.info instruction page `MOV_M64_R64` | https://uops.info/html-instr/MOV_M64_R64.html | uops-intel/MOV_M64_R64.html | 56fa1dc2c26f5a04d7e98bba01b65378c01f4b5cdb9098917276e37adcb95f9f | 2026-09-28 | HTML | PERF 1 |
| uops-intel | uops.info instruction page `MOV_R64_M64` | https://uops.info/html-instr/MOV_R64_M64.html | uops-intel/MOV_R64_M64.html | 0e996c1ee5ee6080e2e2b2627842d8d24c781f0b2d47d1def72512bebb927b55 | 2026-09-28 | HTML | PERF 1 |
| uops-intel | uops.info instruction page `MUL_R64` | https://uops.info/html-instr/MUL_R64.html | uops-intel/MUL_R64.html | c9d8adccabbccb56359a9dfd7e22f1a8c17054fc5ca9b9d229ce37efd7b488f8 | 2026-09-28 | HTML | PERF 1 |
| uops-intel | uops.info instruction page `SHL_R64_CL` | https://uops.info/html-instr/SHL_R64_CL.html | uops-intel/SHL_R64_CL.html | 2d7773985cca3f52d440345b34a4855d3b131fe964d771de098032ab13dad87c | 2026-09-28 | HTML | PERF 1 |
| uops-intel | uops.info instruction page `SHR_R64_CL` | https://uops.info/html-instr/SHR_R64_CL.html | uops-intel/SHR_R64_CL.html | 8255a5ae094993ab6f5667b8223e56e90e29272dbb5045b8533cc8f15b88834d | 2026-09-28 | HTML | PERF 1 |
| uops-intel | uops.info instruction page `SUB_29_R64_R64` | https://uops.info/html-instr/SUB_29_R64_R64.html | uops-intel/SUB_29_R64_R64.html | 9020541a9cd8a8e872d48d1065b49369e5ee44af4c2bfe352331bddba51f4e87 | 2026-09-28 | HTML | PERF 1 |
| uops-intel | uops.info instruction page `VMOVDQU_M256_YMM` | https://uops.info/html-instr/VMOVDQU_M256_YMM.html | uops-intel/VMOVDQU_M256_YMM.html | a64253a852ca25b66b4a5cc0b058502fd88b4d24579e1fccf6f032424026ea92 | 2026-09-28 | HTML | PERF 1 |
| uops-intel | uops.info instruction page `VMOVDQU_YMM_M256` | https://uops.info/html-instr/VMOVDQU_YMM_M256.html | uops-intel/VMOVDQU_YMM_M256.html | b988da3453bf53c2de0755b794b205634ad7545f9d5991099ce6c78e2f15cd69 | 2026-09-28 | HTML | PERF 1 |
| uops-intel | uops.info instruction page `VPMULUDQ_YMM_YMM_YMM` | https://uops.info/html-instr/VPMULUDQ_YMM_YMM_YMM.html | uops-intel/VPMULUDQ_YMM_YMM_YMM.html | dafbbacebe29e23e00db9b8fa1b050ab3e288535ba290157b4db85009a632db2 | 2026-09-28 | HTML | PERF 1 |
| hvh-mult | D. Harvey, J. van der Hoeven, Integer multiplication in time O(n log n), Ann. of Math. 193 (2021), 563-617 | https://hal.science/hal-02070778/file/nlogn.pdf | hvh-mult/nlogn.pdf | 23706afbca829df933be41754743bf760ed01ffe73d499ddb410774c4ee0bf1d | 2026-09-27 | PDF + txt | PERF 3 |
| shoup-ntb | V. Shoup, A Computational Introduction to Number Theory and Algebra, version 2 (Cambridge University Press 2008; author's copy, free at the author's page) | https://shoup.net/ntb/ntb-v2.pdf | shoup-ntb/ntb-v2.pdf | 8e1abc54f4510c3f274dfbed07ea602a6a439ee24b2c916e61abe829b402ec06 | 2026-09-28 | PDF + txt | SPEC 9.2; milestone S.1, S.3 |
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
| flint-3.0.1 | FLINT 3.0.1 documentation, `fmpz_mod_poly.rst` (polynomials over a modulus given by an `fmpz`: `X^e` modulo `f`, gcd) | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/fmpz_mod_poly.rst | flint-3.0.1/fmpz_mod_poly.rst | e51f4d80c8c7d42d7c9ee3f6b4c1e67f149cf5b112bb7db56f1ffd0d3e3a2b72 | 2026-09-29 | reST | milestone S.2 (roots modulo a prime of any size) |
| flint-3.0.1 | FLINT 3.0.1 documentation, `fmpz_mod_poly_factor.rst` (factorisation and root finding over a modulus given by an `fmpz`) | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/fmpz_mod_poly_factor.rst | flint-3.0.1/fmpz_mod_poly_factor.rst | 0f81560d9263a135b0e40c1a0eb5ed5d2bba2664e6afab73bf4419e1c2fdc27a | 2026-09-29 | reST | milestone S.2 (roots modulo a prime of any size) |
| flint-3.0.1 | FLINT 3.0.1 documentation, `nmod_poly.rst` (polynomials over a one-word modulus: `X^e` modulo `f`, gcd, distinct nonzero roots) | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/nmod_poly.rst | flint-3.0.1/nmod_poly.rst | 2dd8bae5ae92f1762641f348e8edfaa2049271ad526609e13509524bc6cedafd | 2026-09-29 | reST | milestone S.2 (roots modulo a prime of any size) |
| flint-3.0.1 | FLINT 3.0.1 documentation, `nmod_poly_factor.rst` (factorisation over a one-word modulus; no root-finding entry) | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/nmod_poly_factor.rst | flint-3.0.1/nmod_poly_factor.rst | 59aa4c4c74f084051e0a74b7e99955480f3c0f065d7e94ea4770dadea6a60348 | 2026-09-29 | reST | milestone S.2 (roots modulo a prime of any size) |
| flint-src-3.0.1 | FLINT 3.0.1 C sources at the tag v3.0.1, the rational reconstruction of `fmpq` (`reconstruct_fmpz.c`, `reconstruct_fmpz_2.c`, `reconstruct_fmpz_2_naive.c`, `get_cfrac.c`, `get_cfrac_helpers.c`, `cfrac_bound.c`) | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/src/fmpq/reconstruct_fmpz_2.c | flint-src-3.0.1/fmpq/ | see refs/manifest.sha256 | 2026-09-28 | C | milestone S.3 (what FLINT does) |
| flint-src-3.0.1 | FLINT 3.0.1 C sources at the tag v3.0.1, the matrix modules: Hermite form and its transformation, Smith form, Howell form and strong echelon form modulo `mod`, nullspace, solving (`fmpz_mat/*.c`, `nmod_mat/*.c`, `fmpz_mod_mat/*.c`) | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/src/fmpz_mat/howell_form_mod.c | flint-src-3.0.1/fmpz_mat/, flint-src-3.0.1/nmod_mat/, flint-src-3.0.1/fmpz_mod_mat/ | see refs/manifest.sha256 | 2026-09-28 | C | milestone S.1 (what FLINT does) |
| flint-src-3.0.1 | FLINT 3.0.1 C sources at the tag v3.0.1, Hensel lifting of factors, counting real roots, p-adic polynomial evaluation (`fmpz_poly/hensel*.c`, `fmpz_poly/num_real_roots*.c`, `padic_poly/*.c`, `arb_fmpz_poly/complex_roots.c`) | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/src/fmpz_poly/hensel_lift.c | flint-src-3.0.1/fmpz_poly/, flint-src-3.0.1/padic_poly/, flint-src-3.0.1/arb_fmpz_poly/ | see refs/manifest.sha256 | 2026-09-28 | C | milestone S.2 (what FLINT does) |
| flint-src-3.0.1 | FLINT 3.0.1 C sources at the tag v3.0.1, the roots modulo a prime of one word: `nmod_poly_roots` (`nmod_poly_factor/roots.c`, declared but not documented), its splitting helper (`nmod_poly/find_distinct_nonzero_roots.c`), the powering and the gcd (`nmod_poly/powmod_ui_binexp.c`, `nmod_poly/powmod_ui_binexp_preinv.c`, `nmod_poly/gcd.c`), the evaluation (`nmod_poly/evaluate_nmod.c`), and `flint.h.in`, the template of `flint.h` (`flint_randinit`) | https://raw.githubusercontent.com/flintlib/flint/v3.0.1/src/nmod_poly_factor/roots.c | flint-src-3.0.1/nmod_poly_factor/roots.c, flint-src-3.0.1/nmod_poly/, flint-src-3.0.1/flint.h.in | see refs/manifest.sha256 | 2026-09-29 | C | milestone S.2 (roots modulo p for every prime of a place, lane s2-slice4) |

Not fetched, recorded as local reference: the FLINT 3.0.1 headers matching these docs are on this machine
under `/usr/include/flint` (161 files, e.g. `padic.h`, `arb.h`, `acb.h`).

The `uops-intel` pages were fetched by `refs/fetch_intel.sh`, which writes `refs/manifest-intel.sha256`; each page
holds every microarchitecture the site covers (the Intel profile of `PERF.md` reads the Alder Lake-P column). The
seven pages shared with `uops-zen2` are byte-identical. The register-form pages among them settle the pending item
on the Zen 2 register forms: `uops-intel/ADD_01_R64_R64.html` has an "AMD Zen 2" block with measured (loop)
throughput 0.25 at line 1320 and documented 0.25 at line 1330, matching the figures `PERF.md` section 1a quotes.

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

## Table 3: milestone S, which source settles which statement

Added 2026-09-28 (lane `s-sources`). The rows are the statements the design of `PLAN.md` section 6, milestone S,
needs. Every quote is verbatim from the extraction named in the third column, and every line number was produced
by `lanes/s-sources/mk_table.py`, which slices the quote out of the file; `lanes/s-sources/check_quotes.py` reads
the table back and checks every quote against the file on disk. Rows of the three sub-tables are independent:
a row states what one source says, nothing else. Where two sources differ, the difference is written in the row
and repeated in the notes after the tables. A pipe inside a cell is written `\|`, a break between two extraction
lines is written " / ".

The rows of the three sub-tables below carry 57 quotes, 9259 characters in all; they are a second set, the count
of 57 in the paragraph at the top of this file belongs to the tables above. `check_quotes.py` reports
`rows: 57, quoted characters: 9259, failures: 0`.

Sources that are not on disk are listed at the end of this section; nothing in the tables is cited from memory.

### S.3 partial rational reconstruction

| Statement needed | Source key | File and line | Quote, verbatim |
|---|---|---|---|
| S.3: existence of a pair, not of a reduced fraction. For `0 < A <= m < A B` there are integers `n`, `d` with `n = c d mod m`, `\|n\| < A`, `0 < \|d\| < B` (Thue's lemma; the modulus is BELOW the product of the bounds, and the bounds of the lemma are STRICT, so the inclusive bounds `A`, `B` of SPEC 9.2 correspond to `r* = A + 1`, `t* = B + 1`; the pair need not be reduced and `d` need not be prime to `m`: for `m = 8`, `c = 3` and the strict bounds 3, 3 the lemma gives the pair `(-2, 2)`, which is not reduced, and the problem of SPEC 9.2 with `A = B = 2` has no solution; row corrected 2026-09-29 after lane s-design and again after its review, `docs/reviews/s-design/review.md`) | shoup-ntb | ntb-v2.txt:2184, 2185 | "Theorem 2.33 (Thue’s lemma). Let n, b, r∗ , t∗ ∈ Z, with 0 < r ∗ ≤ n < r∗ t∗ . / Then there exist r, t ∈ Z with" |
| S.3: the object of the problem is the ratio `n/d`, not the pair `(n, d)`: from one solution one gets all the others by a non-zero multiple | shoup-ntb | ntb-v2.txt:4152, 4153 | "if r ≡ bt (mod n), / and so we can only hope to guarantee that the ratio r/t is unique." |
| S.3: uniqueness. If `2 A B < m` then the ratio is unique: `2 A B < m` is Shoup's `n > 2 r* t*` | shoup-ntb | ntb-v2.txt:4159, 4164 | "Theorem 4.8. Let n, b, r∗ , t∗ ∈ Z with r ∗ ≥ 0, t∗ > 0, and n > 2r∗ t∗ . Further, / Then r/t = r0 /t0 ." |
| S.3: the key inequality of that proof: `\|n1 d2 - n2 d1\| <= 2 A B < m` | shoup-ntb | ntb-v2.txt:4170, 4171 | "However, we also have / \|rt0 − r0 t\| ≤ \|r\|\|t0 \| + \|r0 \|\|t\| ≤ 2r∗ t∗ < n." |
| S.3: the bound is stated with `0 < \|t\| <= t*` for the denominator (a sign is allowed), while the SPEC asks for `0 < d <= B` | shoup-ntb | ntb-v2.txt:4161 | "r ≡ bt (mod n), \|r\| ≤ r∗ , 0 < \|t\| ≤ t∗ ,                    (4.7)" |
| S.3: which row of the remainder sequence is the answer: the smallest index `j` with `rj <= r*`, and the answer is that row | shoup-ntb | ntb-v2.txt:4192, 4196 | "smallest index (among 0, . . . , λ + 1) such that rj ≤ r , and set / r 0 := rj , s0 := sj , and t0 := tj ." |
| S.3: what that row gives, PROVIDED that integers `r`, `s`, `t` exist with `r = n s + b t`, `\|r\| <= r*`, `0 < \|t\| <= t*` (the hypothesis of Theorem 4.9, `ntb-v2.txt:4198` to `4200`, that is: a bounded lattice point exists): then `0 < \|t0\| <= t*`, and under `2 A B < m` the pair is the given one up to a common factor. Without that hypothesis the bound on `\|t0\|` is not claimed ("in every case" was too much). Corrected 2026-09-29 by lane s-design-repair; the review `docs/reviews/s-design/review.md` judged the correction RIGHT. | shoup-ntb | ntb-v2.txt:4202, 4203, 4204 | "(i) 0 < \|t0 \| ≤ t∗ ; / (ii) if n > 2r∗ t∗ , then for some non-zero integer q, / r = r 0 q, s = s0 q, and t = t0 q." |
| S.3: the denominator bound for the chosen row is the hard part of the theorem | shoup-ntb | ntb-v2.txt:4215 | "This is the hardest part of the proof. To this end, let" |
| S.3: the same stopping rule for the non-reconstructive case: the first remainder below `r*` (existence) | shoup-ntb | ntb-v2.txt:4045, 4048 | "smallest index (among 0, . . . , λ + 1) such that rj < r . Then, setting r := rj and / t := tj" |
| S.3: FLINT 3.0.1 `fmpq_reconstruct_fmpz_2` promises exactly the bounded problem with the condition `2 N D < m` and the reducedness of the answer | flint-3.0.1 | fmpq.rst:560, 562, 563, 564, 565, 566, 567 | "Reconstructs a rational number from its residue `a` modulo `m`. / Given a modulus `m > 2`, a residue `0 \le a < m`, and positive `N, D` / satisfying `2ND < m`, this function attempts to find a fraction `n/d` with / `0 \le \|n\| \le N` and `0 < d \le D` such that `\gcd(n,d) = 1` and / `n \equiv ad \pmod m`. If a solution exists, then it is also unique. / The function returns 1 if successful, and 0 to indicate that no solution / exists." |
| S.3: FLINT 3.0.1 `fmpq_reconstruct_fmpz` has no bounds argument; the bounds are fixed at `N = D = floor(sqrt((m-1)/2))` | flint-3.0.1 | fmpq.rst:574 | "Uses the balanced bounds `N = D = \lfloor\sqrt{\frac{m-1}{2}}\rfloor`." |
| S.3: what the FLINT code does: the extended Euclidean algorithm on `(m, a)`, the quotients accumulated into a 2x2 matrix, stopped at the first remainder `<= N` | flint-src-3.0.1 | fmpq/reconstruct_fmpz_2.c:980 | "/* We have A > B > N > 0; accumulate quotients into M until A > N >= B */" |
| S.3: the answer of FLINT is the last remainder `B` with the denominator taken from the accumulated matrix, the sign fixed by the determinant | flint-src-3.0.1 | fmpq/reconstruct_fmpz_2.c:1024, 1026, 1027, 1031 | "FLINT_ASSERT(fmpz_cmp(A, N) > 0 && fmpz_cmp(N, B) >= 0); / fmpz_swap(n, B); / fmpz_swap(d, M->_11); / fmpz_neg(n, n);" |
| S.3: FLINT accepts the row only if the denominator is within `D` and the fraction is reduced; otherwise it returns 0 | flint-src-3.0.1 | fmpq/reconstruct_fmpz_2.c:1036, 1037, 1039, 1040 | "FLINT_ASSERT(fmpz_sgn(d) > 0); / if (fmpz_cmp(d, D) <= 0) / fmpz_gcd(R, n, d); / success = fmpz_is_one(R);" |
| S.3: FLINT tries the integers `a` and `a - m` with denominator 1 before the algorithm | flint-src-3.0.1 | fmpq/reconstruct_fmpz_2.c:940, 941, 949, 950 | "/* Quickly identify small integers */ / if (fmpz_cmp(a, N) <= 0) / fmpz_sub(n, a, m); / if (fmpz_cmpabs(n, N) <= 0)" |
| S.3: the balanced bound of FLINT in the source: `N` is `m >> 1`, lowered by 1 when `m` is even, then `N = D = sqrt(N)` | flint-src-3.0.1 | fmpq/reconstruct_fmpz.c:22, 25, 26 | "fmpz_fdiv_q_2exp(N, m, 1); / fmpz_sqrt(N, N); / result = _fmpq_reconstruct_fmpz_2(n, d, a, m, N, N);" |

### S.1 linear systems modulo `N`

| Statement needed | Source key | File and line | Quote, verbatim |
|---|---|---|---|
| S.1: the ring of the problem: `Z/(N)` is handled as a principal ideal ring, and a residue class ring of a PID is a stable PIR | storjohann-thesis | diss2up.txt:224, 225, 226 | "For the other forms the most general ring we / work over is a principal ideal ring — a commutative ring with identity / in which every ideal is principal." |
| S.1: the use of the form: it is the canonical form for solving systems of linear equations over the ring of entries | storjohann-thesis | diss2up.txt:221, 223 | "A primary use of the Howell form is to solve systems of linear equations / over the domain of entries." |
| S.1: the transforming matrix is unimodular, that is invertible over the ring | storjohann-thesis | diss2up.txt:209, 210 | "The transforming matrix U is unimodular — this simply means that U / is invertible over R." |
| S.1: the Hermite form is not canonical for left equivalence over a PIR (it is canonical only over a PID) | storjohann-thesis | diss2up.txt:887, 888 | "we may conclude that the Hermite form is not a canon- / ical form for left equivalence of matrices over a PIR." |
| S.1: the four conditions an echelon form over a PIR may satisfy; the Howell form is an echelon form with a maximal number of rows | storjohann-thesis | diss2up.txt:417, 418 | "The Howell form of A is an echelon form with a maximal number / of rows." |
| S.1: the conditions of the Howell form itself: (r1) echelon position of the first nonzero entry of each row | storjohann-thesis | diss2up.txt:2058, 2065, 2066 | "(r1) Let r be the number of nonzero rows of H. Then the first r rows / of H are nonzero. / entry in row i. Then 0 = j0 < j1 < j2 < . . . < jr ." |
| S.1: (r2) each pivot generates an ideal, every entry above it is in the ring of multipliers of that ideal | storjohann-thesis | diss2up.txt:2068 | "(r2) H[i, ji ] ∈ A(R) and H[k, ji ] ∈ R(R, H[i, ji ]) for 1 ≤ k < i ≤ r." |
| S.1: (r4) the Howell property: rows `i+1, ..., r` generate the rows of `A` whose first `j_i` entries are zero | storjohann-thesis | diss2up.txt:2070 | "(r4) Rows i + 1, i + 2, . . . , r of H generate Sji (A)." |
| S.1: the name and the consequence: the first `r` rows of `H` are a canonical generating set for the row module `S(A)` | storjohann-thesis | diss2up.txt:2075, 2077 | "H is the Howell canonical form of A. The first r rows of H — the Howell / basis of A — give a canonical generating set for S(A)." |
| S.1: the certificate: a Howell transform is the 5-tuple `(Q, U, C, W, r)`, with `U` unimodular and `W` a kernel of the remaining block `T` | storjohann-thesis | diss2up.txt:2078, 2084, 2094, 2099 | "A Howell transform for A is a tuple (Q, U, C, W, r) / which satisfies and can be written using a conformal block decomposition / with U ∈ Rn×n unimodular, H the Howell basis for A and W a kernel for / that is W ∈ Rn×n and S(W ) = {v ∈ Rn \| vA = 0}." |
| S.1: a kernel of `A` is the product `W Q U C` (it may be dense; a factorisation is also given). It is a LEFT kernel, `S(W) = {v : v A = 0}` (`diss2up.txt:2094` to `2099`): a design with column unknowns `A x = b` applies it to the transpose `A^T` (`docs/proofs/solvers.md` P2.6, P2.8). Corrected 2026-09-29 by lane s-design-repair; the review `docs/reviews/s-design/review.md` judged the correction RIGHT. | storjohann-thesis | diss2up.txt:2120 | "W QU C is a kernel for A" |
| S.1: solving with the transform: `b` is in the row module exactly when the Howell form of the right-hand side has zero last block, and then `x` is read off the transform | storjohann-thesis | diss2up.txt:2112, 2114 | "with the right hand side in Howell form. Then b ∈ S(A) if and only if / b0 = 0. If b ∈ S(A), then xA = b where x ← [ y \| 0 ]U C." |
| S.1: existence and uniqueness of the Howell form over `Z/(N)`: Howell (1986); over an arbitrary PIR: Buchmann and Neis (1996) | storjohann-thesis | diss2up.txt:2119, 2120, 2121, 2122 | "Existence and uniqueness of the Howell form was first proven by Howell / (1986) for matrices over Z/(N ). Howell (1986)’s proof is constructive / and leads to an O(n3 ) basic operations algorithm. Buchmann and Neis / (1996) give a proof of uniqueness" |
| S.1: the one-equation case over `Z/(n)`: solvable iff `gcd(a, n)` divides `b`, and the solution is unique modulo `n / gcd(a, n)` | shoup-ntb | ntb-v2.txt:1221, 1222, 1223, 1224 | "Theorem 2.5. Let a, n ∈ Z with n > 0, and let d := gcd(a, n). / (i) For every b ∈ Z, the congruence az ≡ b (mod n) has a solution z ∈ Z if / and only if d \| b. / (ii) For every z ∈ Z, we have az ≡ 0 (mod n) if and only if z ≡ 0 (mod n/d)." |
| S.1: FLINT 3.0.1 `fmpz_mat_hnf_transform` gives the Hermite form together with `U` such that `U A = H`: the certificate for the integral case | flint-3.0.1 | fmpz_mat.rst:1235, 1237, 1238, 1239 | ".. function:: void fmpz_mat_hnf_transform(fmpz_mat_t H, fmpz_mat_t U, const fmpz_mat_t A) / Computes an integer matrix ``H`` such that ``H`` is the unique (row) / Hermite normal form of ``A`` along with the transformation matrix / ``U`` such that `UA = H`. The algorithm used is selected from the" |
| S.1: FLINT 3.0.1 `fmpz_mat_snf` takes and returns only `S` and `A`: the Smith form comes without a transformation matrix | flint-3.0.1 | fmpz_mat.rst:1314 | ".. function:: void fmpz_mat_snf(fmpz_mat_t S, const fmpz_mat_t A)" |
| S.1: FLINT 3.0.1 `fmpz_mat_howell_form_mod` computes the Howell form modulo a given `mod`, in place, and returns the number of nonzero rows | flint-3.0.1 | fmpz_mat.rst:1168, 1170, 1171, 1172, 1176 | ".. function:: slong fmpz_mat_howell_form_mod(fmpz_mat_t A, const fmpz_t mod) / Transforms `A` such that `A` modulo ``mod`` is the Howell form of the / input matrix modulo ``mod``. / For a definition of the Howell form see [StoMul1998]_. The Howell form / `A` must have at least as many rows as columns." |
| S.1: `nmod_mat_howell_form` is the Howell form for a modulus of one word; its entry names no prime (`nmod_mat.rst:705` to `723`) and its source works with gcd operations against the modulus (`nmod_mat/strong_echelon_form.c:72` to `91`), so a prime is not needed; only the general sentence of the module introduction, `nmod_mat.rst:28` to `30` ("assumed to be a prime number in functions that perform some kind of division, solving, or Gaussian elimination"), could be read as asking for one. The definition used is that of [StoMul1998] Corrected 2026-09-29 by lane s-design-repair; the review `docs/reviews/s-design/review.md` judged the correction RIGHT. | flint-3.0.1 | nmod_mat.rst:716, 718, 719, 720 | ".. function:: slong nmod_mat_howell_form(nmod_mat_t A) / Puts `A` into Howell form and returns the number of non-zero rows. / For a definition of the Howell form see [StoMul1998]_. The Howell form / is computed by first putting `A` into strong echelon form and then ordering" |
| S.1: the strong echelon form of FLINT is the Howell form up to a permutation of the rows, but with the opposite orientation: upper right against lower left | flint-3.0.1 | nmod_mat.rst:707, 710, 711, 712 | "Puts `A` into strong echelon form. The Howell form and the strong echelon / Note that [FieHof2014]_ defines strong echelon form as a lower left normal form, / while the implemented version returns an upper right normal form, / agreeing with the definition of Howell form in [StoMul1998]_." |
| S.1: the source of `nmod_mat_howell_form` works in place and returns only the count of the nonzero rows: no transformation matrix is produced | flint-src-3.0.1 | nmod_mat/howell_form.c:15, 26, 44 | "nmod_mat_howell_form(nmod_mat_t A) / nmod_mat_strong_echelon_form(A); / return k;" |
| S.1: the modular Howell form of `fmpz_mat` is computed with gcds against `N`, not with inverses modulo a prime, so no primality of the modulus is assumed | flint-src-3.0.1 | fmpz_mat/strong_echelon_form_mod.c:67, 78, 80 | "_fmpz_stab(fmpz_t t, const fmpz_t a, const fmpz_t b, const fmpz_t N) / fmpz_gcd(gg, g, N); / fmpz_divexact(bb, N, gg);" |
| S.1: the solving functions of FLINT 3.0.1 for a prime field: a solution is returned if one exists, `0` and a zeroed `X` otherwise; `A` may be singular | flint-3.0.1 | nmod_mat.rst:561, 562, 565, 566, 569 | "Solves the matrix-matrix equation `AX = B` over / is the modulus of `X` which must be a prime number. `X`, `A`, and `B` / Returns `1` if a solution exists; otherwise returns `0` and sets the / elements of `X` to zero. If more than one solution exists, one of the / There are no restrictions on the shape of `A` and it may be singular." |
| S.1: the SOLVING functions of `fmpz_mod_mat` are for a prime modulus and say so; the module as a whole is not limited to prime fields: it also has `fmpz_mod_mat_strong_echelon_form` and `fmpz_mod_mat_howell_form` (`fmpz_mod_mat.rst:278` to `293`), whose entries name no condition on the modulus, and the second is a call of `fmpz_mat_howell_form_mod` (`flint-src-3.0.1:fmpz_mod_mat/howell_form.c:14` to `16`). The quote below is the solving functions only. Corrected 2026-09-29 by lane s-design-repair; the review `docs/reviews/s-design/review.md` judged the correction RIGHT. | flint-3.0.1 | fmpz_mod_mat.rst:376, 380, 388 | "The modulus is assumed to be prime. / Solves the matrix-matrix equation `AX = B` over `Fp`. / The modulus is assumed to be prime." |
| S.1: the kernel over a prime field: `nmod_mat_nullspace` returns a maximal rank matrix with `A X = 0` | flint-3.0.1 | nmod_mat.rst:647, 649, 650, 651 | "Computes the nullspace of `A` and returns the nullity. / More precisely, this function sets `X` to a maximum rank matrix / such that `AX = 0` and returns the rank of `X`. The columns of / `X` will form a basis for the nullspace of `A`." |
| S.1: the kernel over `Q`: `fmpz_mat_nullspace` returns a basis of the right nullspace, entries not minimal | flint-3.0.1 | fmpz_mat.rst:1185, 1186, 1187 | "Computes a basis for the right rational nullspace of `A` and returns / the dimension of the nullspace (or nullity). `B` is set to a matrix with / linearly independent columns and maximal rank such that `AB = 0`" |

### S.2 roots

| Statement needed | Source key | File and line | Quote, verbatim |
|---|---|---|---|
| S.2: Hensel's lemma for a simple root, with the uniqueness of the lifted root | conrad-hensel | hensel.txt:31, 32, 33 | "Theorem 2.1 (Hensel’s lemma). If f (X) ∈ Zp [X] and a ∈ Zp satisfies / f (a) ≡ 0 mod p, f 0 (a) 6≡ 0 mod p / then there is a unique α ∈ Zp such that f (α) = 0 in Zp and α ≡ a mod p." |
| S.2: its proof, by induction on the exponent, digit by digit | conrad-hensel | hensel.txt:37, 38, 39 | "Proof. We will prove by induction that for each n ≥ 1 there is an an ∈ Zp such that / • f (an ) ≡ 0 mod pn , / • an ≡ a mod p." |
| S.2: the stronger form `\|f(a)\| < \|f'(a)\|^2`, with the distance of the root and the uniqueness | conrad-hensel | hensel.txt:315, 316, 317, 318, 319 | "Theorem 4.1 (Hensel’s lemma). Let f (X) ∈ Zp [X] and a ∈ Zp satisfy / \|f (a)\|p < \|f 0 (a)\|2p . / There is a unique α ∈ Zp such that f (α) = 0 in Zp and \|α − a\|p < \|f 0 (a)\|p . Moreover, / (1) \|α − a\|p = \|f (a)/f 0 (a)\|p < \|f 0 (a)\|p , / (2) \|f 0 (α)\|p = \|f 0 (a)\|p ." |
| S.2: the strong form is not a special case of the simple-root form: it also applies when `a mod p` is a multiple root | conrad-hensel | hensel.txt:310, 311 | "It can be applied to cases where a mod p is a multiple / root of f (X) mod p: f (a) ≡ 0 mod p and f 0 (a) ≡ 0 mod p." |
| S.2: Hensel's lemma for `Z[X]` with the explicit lift formula (Baker, Theorem 1.33) | baker-padic | padicnotes.txt:572, 573, 574 | "Theorem 1.33 (Hensel’s Lemma: ﬁrst version). Let f (X) = dk=0 ak X k ∈ Z[X]and sup- / pose that x ∈ Z is a root of f modulo ps (with s ⩾ 1) and that f ′ (x) is a unit modulo p. Then / there is a unique root x′ ∈ Z/ps+1 of f modulo ps+1 satisfying x′ ≡s x; moreover, x′ is given by" |
| S.2: a general version with the hypotheses `f (a) ≡ 0 mod p^(2r-1)` and `f' (a) ≢ 0 mod p^r` (Baker, Theorem 1.37); no uniqueness is stated there | baker-padic | padicnotes.txt:662, 670, 671, 672 | "Theorem 1.37 (Hensel’s Lemma: General Version). Let f (X) ∈ Z[X], r ⩾ 1 and a ∈ Z, / Then there exists a′ ∈ Z such that / f (a′ ) ≡ 0       and       a′ ≡r a. / p2r+1                  p" |
| S.2: a different hypothesis shape over a local field: `\|f (x)\| < 1` and `\|f' (x)\| = 1` for a monic polynomial, with the weaker conclusion `\|y - x\| <= \|f (x)\|` (Thorne, Lemma 3.6) | thorne-padic | jackthornenotes.txt:567, 569, 570, 571, 572 | "Lemma 3.6. Let f (x) ∈ OK [x] be a monic polynomial, and suppose / (1) \|f (x)\| < 1; / (2) \|f 0 (x)\| = 1. / Then there exists a unique y ∈ OK such that f (y) = 0 and \|y − x\| ≤ / \|f (x)\|." |
| S.2: the same note states the simple-root case over `Z_p` and proves it digit by digit (Thorne, Lemma 3.7) | thorne-padic | jackthornenotes.txt:574, 576, 577, 578, 579, 580 | "Lemma 3.7. Let f (x) ∈ Zp [x] be a monic polynomial, and suppose / (1) f (x) ≡ 0 mod p; / (2) f 0 (x) 6≡ 0 mod p. / Then there exists a unique y ∈ Zp such that f (y) = 0 and y ≡ x / mod p. / Proof. Let us prove the second version. We construct the p-adic ex-" |
| S.2: FLINT 3.0.1 `fmpz_poly` lifts factors, not roots: `fmpz_poly_hensel_lift_once` lifts a squarefree product of local factors to `p^N` | flint-3.0.1 | fmpz_poly.rst:2993, 2995, 2997, 2998, 3000 | ".. function:: void fmpz_poly_hensel_lift_once(fmpz_poly_factor_t lifted_fac, const fmpz_poly_t f, const nmod_poly_factor_t local_fac, slong N) / This function does a Hensel lift. / It lifts local factors stored in ``local_fac`` of `f` to `p^N`, / where `N \geq 2`. The lifted factors will be stored in ``lifted_fac``. / intended for end users. The product of local factors must be squarefree." |
| S.2: `fmpz_poly_num_real_roots_sturm` counts the real roots of a squarefree polynomial by a Sturm sequence and returns no isolating intervals. The generic `fmpz_poly_num_real_roots` has the same promise (squarefree input, `fmpz_poly.rst:3264` to `3268`) but is not always Sturm: it uses closed formulas up to degree 4 and the Sturm routine above (`fmpz_poly/num_real_roots.c:107` to `149`). Outside the squarefree hypothesis nothing is promised, and the source does this: it strips the full power `X^i` and adds `i` (so `X^d` returns `d`), throws for the zero polynomial and for a rest of degree 3 or 4 with discriminant 0, and counts 0 for a quadratic rest with `b^2 = 4ac` (`num_real_roots.c:107` to `129`, `:34` to `40`, `:158` to `159`); the installed 3.0.1 returns 2, 3, 4 for `X^2`, `X^3`, `X^4` and 0 for `(X - 1)^2`. The quote is the Sturm routine. Corrected 2026-09-29 by lane s-design-repair; the review `docs/reviews/s-design/review.md` judged the correction RIGHT. | flint-3.0.1 | fmpz_poly.rst:3251, 3253, 3254, 3256 | ".. function:: slong fmpz_poly_num_real_roots_sturm(const fmpz_poly_t pol) / Returns the number of real roots of the squarefree polynomial ``pol`` / using Sturm sequence. / The polynomial is assumed to be squarefree." |
| S.2: the certified isolation of real roots of a real analytic function: a flag of 1 means exactly one root, any other flag is undetermined, and completeness is read off the flags. `arb_calc_isolate_roots` takes a function pointer and is NOT used by the design of S.2 (`docs/proofs/solvers.md` P3.9(3)). Corrected 2026-09-29 by lane s-design-repair; the review `docs/reviews/s-design/review.md` judged the correction RIGHT. | flint-3.0.1 | arb_calc.rst:127, 129, 131, 132, 133 | "* Subintervals with a flag of 1 contain exactly one (single) root. / * Subintervals with any other flag may or may not contain roots. / If no flags other than 1 occur, all roots of the function on *interval* / have been isolated. If there are output subintervals on which the / existence or nonexistence of roots could not be determined," |
| S.2: what the isolation cannot do: roots of multiplicity above one, and roots at the end points, are not isolated | flint-3.0.1 | arb_calc.rst:136, 137, 138 | "bounds for the breaking criteria). Note that roots of multiplicity / higher than one and roots located exactly at endpoints cannot be isolated / by the algorithm." |
| S.2: FLINT 3.0.1 isolates all the roots of an integer polynomial at once: the enclosures are disjoint, so all roots are isolated, but the input must be squarefree | flint-3.0.1 | arb_fmpz_poly.rst:66, 68, 70, 71, 79 | ".. function:: void arb_fmpz_poly_complex_roots(acb_ptr roots, const fmpz_poly_t poly, int flags, slong prec) / Writes to *roots* all the real and complex roots of the polynomial *poly*, / The root enclosures are guaranteed to be disjoint, so that / all roots are isolated. / The input polynomial *must* be squarefree. For a general polynomial," |
| S.2: FLINT's own remark on that function: adequate, but not competitive with the state of the art for real roots alone | flint-3.0.1 | arb_fmpz_poly.rst:103, 104, 105 | "This implementation should be adequate for general use, but it is not / currently competitive with state-of-the-art isolation / methods for finding real roots alone." |
| S.2: the `p`-adic polynomial of FLINT 3.0.1 has no root finding; it offers the integrality test that a Hensel step needs | flint-3.0.1 | padic_poly.rst:198, 200, 201, 202 | ".. function:: int padic_poly_get_fmpz_poly(fmpz_poly_t rop, const padic_poly_t op, const padic_ctx_t ctx) / Sets the integer polynomial ``rop`` to the value of the `p`-adic / polynomial ``op`` and returns `1` if the polynomial is `p`-adically / integral.  Otherwise, returns `0`." |
| S.2: the precision of an evaluation at a `p`-adic point, as FLINT states it | flint-3.0.1 | padic_poly.rst:438, 446, 447 | "Sets the `p`-adic number ``y`` to ``poly`` evaluated at `a`, / `y = F(a)` is defined to precision `N` when `a` is integral and / `N+(n-1)b` when `b < 0`." |
| S.2: `X^p` modulo `g` over `Z/pZ` for a prime `p` that is an `fmpz`: `fmpz_mod_poly_powmod_x_fmpz_preinv`. It asks for `e >= 0` and for `finv`, the precomputed inverse of the reverse of `f`. Its own text stops in the middle of a word after that line, so every further requirement is missing from this entry; the entry says nothing about the modulus being prime, and nothing about it being nonzero | flint-3.0.1 | fmpz_mod_poly.rst:690, 692, 693, 694 | ".. function:: void fmpz_mod_poly_powmod_x_fmpz_preinv(fmpz_mod_poly_t res, const fmpz_t e, const fmpz_mod_poly_t f, const fmpz_mod_poly_t finv, const fmpz_mod_ctx_t ctx) / Sets ``res`` to ``x`` raised to the power ``e`` / modulo ``f``, using sliding window exponentiation. We require / ``e >= 0``. We require ``finv`` to be the inverse of the reverse of" |
| S.2: the degree condition of that routine is stated only for the raw variant, and there it is `lenf > 2`, that is `deg f >= 2`; the output must have room for `lenf - 1` coefficients. The degree of the base `X` is 1, so the requirement is `deg X < deg f`; the base is not an argument of the routine at all | flint-3.0.1 | fmpz_mod_poly.rst:681, 683, 684, 685, 687, 688 | ".. function:: void _fmpz_mod_poly_powmod_x_fmpz_preinv(fmpz * res, const fmpz_t e, const fmpz * f, slong lenf, const fmpz* finv, slong lenfinv, const fmpz_mod_ctx_t ctx) / Sets ``res`` to ``x`` raised to the power ``e`` modulo ``f``, / using sliding window exponentiation. We require ``e > 0``. / We require ``finv`` to be the inverse of the reverse of ``f``. / We require ``lenf > 2``. The output ``res`` must have room for / ``lenf - 1`` coefficients." |
| S.2: the same routine for a one-word `p`, where the exponent `e` is the word `p` itself: `nmod_poly_powmod_x_ui_preinv`, with the same two requirements. The raw variant `_nmod_poly_powmod_x_ui_preinv` states `lenf > 2` (`nmod_poly.rst:917`), as the `fmpz` raw variant does | flint-3.0.1 | nmod_poly.rst:920, 922, 923, 924, 925 | ".. function:: void nmod_poly_powmod_x_ui_preinv(nmod_poly_t res, ulong e, const nmod_poly_t f, const nmod_poly_t finv) / Sets ``res`` to ``x`` raised to the power ``e`` / modulo ``f``, using sliding window exponentiation. We require / ``e >= 0``. We require ``finv`` to be the inverse of the reverse of / ``f``." |
| S.2: the same routine with an `fmpz` exponent and a one-word modulus, `nmod_poly_powmod_x_fmpz_preinv`; here too the degree condition is stated only for the raw variant (`nmod_poly.rst:933`) | flint-3.0.1 | nmod_poly.rst:936, 938, 939, 940, 941 | ".. function:: void nmod_poly_powmod_x_fmpz_preinv(nmod_poly_t res, fmpz_t e, const nmod_poly_t f, const nmod_poly_t finv) / Sets ``res`` to ``x`` raised to the power ``e`` / modulo ``f``, using sliding window exponentiation. We require / ``e >= 0``. We require ``finv`` to be the inverse of the reverse of / ``f``." |
| S.2: the gcd over `Z/pZ` with `p` an `fmpz` is `fmpz_mod_poly_gcd`. It assumes `p` prime, and says why: the gcd is defined in `(Z/(p Z))[X]` only if `p` is prime. The entry says NOTHING about the result being monic, so what `docs/proofs/solvers.md` P3.7(2) reads off the routine, its degree, is the degree of the polynomial that comes out | flint-3.0.1 | fmpz_mod_poly.rst:1030, 1032, 1034, 1035, 1036 | ".. function:: void fmpz_mod_poly_gcd(fmpz_mod_poly_t G, const fmpz_mod_poly_t A, const fmpz_mod_poly_t B, const fmpz_mod_ctx_t ctx) / Sets `G` to the greatest common divisor of `A` and `B`. / In general, the greatest common divisor is defined in the polynomial / ring `(\mathbf{Z}/(p \mathbf{Z}))[X]` if and only if `p` is a prime / number.  Thus, this function assumes that `p` is prime." |
| S.2: monicity of a gcd over `Z/pZ` is stated only for the `xgcd`: `fmpz_mod_poly_xgcd` makes the gcd monic unless it is zero, and `_fmpz_mod_poly_xgcd` (`fmpz_mod_poly.rst:1120`) makes no attempt at all. Nothing of the kind is said for `fmpz_mod_poly_gcd` of the previous row | flint-3.0.1 | fmpz_mod_poly.rst:1129, 1131, 1132, 1133, 1134 | ".. function:: void fmpz_mod_poly_xgcd(fmpz_mod_poly_t G, fmpz_mod_poly_t S, fmpz_mod_poly_t T, const fmpz_mod_poly_t A, const fmpz_mod_poly_t B, const fmpz_mod_ctx_t ctx) / Computes the GCD of `A` and `B`. The GCD of zero polynomials is / defined to be zero, whereas the GCD of the zero polynomial and some other / polynomial `P` is defined to be `P`. Except in the case where / the GCD is zero, the GCD `G` is made monic." |
| S.2: the documented root finder with `p` an `fmpz` is `fmpz_mod_poly_roots`. It requires `f` to be nonzero (it throws otherwise) and it EXPECTS, without checking, that the modulus is prime. It asks for neither squarefreeness nor monicity. It promises all the distinct roots, each written as a factor `x - r_i`, the exponent being the multiplicity when `with_multiplicity` is set. The documentation says nothing of randomisation, and nothing beyond the quoted sentence about a modulus that is not prime | flint-3.0.1 | fmpz_mod_poly_factor.rst:187, 189, 190, 191, 192 | ".. function:: void fmpz_mod_poly_roots(fmpz_mod_poly_factor_t r, const fmpz_mod_poly_t f, int with_multiplicity, const fmpz_mod_ctx_t ctx) / Fill `r` with factors of the form `x - r_i` where the `r_i` are the distinct roots of a nonzero `f` in `Z/pZ`. / It is expected and not checked that the modulus of `ctx` is prime. / If `with\_multiplicity` is zero, the exponent `e_i` of the factor `x - r_i` is `1`. Otherwise, it is the largest `e_i` such that `(x-r_i)^e_i` divides `f`. / This function throws if `f` is zero, but is otherwise always successful." |
| S.2: what the documentation says about a modulus that is NOT prime: `fmpz_mod_poly_roots_factored` takes the prime factorisation `n` of the modulus, expects and does not check it, finds the roots modulo the primes, lifts them to the prime powers and combines them, and MAY fail: a return of `0` says it was not able to find the roots, "possibly because there are too many of them". No completeness is promised for a composite modulus | flint-3.0.1 | fmpz_mod_poly_factor.rst:194, 196, 197, 199, 200 | ".. function:: int fmpz_mod_poly_roots_factored(fmpz_mod_poly_factor_t r, const fmpz_mod_poly_t f, int with_multiplicity, const fmpz_factor_t n, const fmpz_mod_ctx_t ctx) / Fill `r` with factors of the form `x - r_i` where the `r_i` are the distinct roots of a nonzero `f` in `Z/nZ`. / It is expected and not checked that `n` is a prime factorization of the modulus of `ctx`. / The roots are first found modulo the primes in `n`, then lifted to the corresponding prime powers, then combined into roots of the original polynomial `f`. / A return of `1` indicates the function was successful. A return of `0` indicates the function was not able to find the roots, possibly because there are too many of them." |
| S.2: a factorisation into monic irreducible factors is documented for the same ring, and its entry names no condition on the modulus at all: `f` must be non-constant, and the algorithm is chosen by heuristic measurements | flint-3.0.1 | fmpz_mod_poly_factor.rst:152, 154, 155, 156 | ".. function:: void fmpz_mod_poly_factor(fmpz_mod_poly_factor_t res, const fmpz_mod_poly_t f, const fmpz_mod_ctx_t ctx) / Factorises a non-constant polynomial ``f`` into monic irreducible / factors choosing the best algorithm for given modulo and degree. / Choice is based on heuristic measurements." |
| S.2: a squarefree test is documented for the same ring, and it too names no condition on the modulus; the zero polynomial is not squarefree. So a design that needs a squarefree input before it factors can ask for that test itself, and need not assume a prime to read its answer | flint-3.0.1 | fmpz_mod_poly_factor.rst:107, 109, 110 | ".. function:: int fmpz_mod_poly_is_squarefree(const fmpz_mod_poly_t f, const fmpz_mod_ctx_t ctx) / Returns 1 if ``f`` is squarefree, and 0 otherwise. As a special / case, the zero polynomial is not considered squarefree." |
| S.2: NO function of these two modules is documented as testing that the modulus is prime. The one place where the documentation reports something about a composite modulus is `fmpz_mod_poly_is_irreducible_rabin_f`, which either answers the irreducibility question ("even for composite `f`, or it finds a factor of `p`") or returns a nontrivial factor of the modulus; that is a documented way of learning that the modulus is composite, not a primality test | flint-3.0.1 | fmpz_mod_poly_factor.rst:85, 87, 88, 89, 91, 92, 93 | ".. function:: int fmpz_mod_poly_is_irreducible_rabin_f(fmpz_t r, const fmpz_mod_poly_t f, const fmpz_mod_ctx_t ctx) / Either sets `r` to `1` and returns 1 if the polynomial ``f`` is / irreducible or `0` otherwise, or sets `r` to a nontrivial factor of / `p`. / This algorithm correctly determines whether `f` is irreducible over / `\mathbb{Z}/p\mathbb{Z}`, even for composite `f`, or it finds a factor / of `p`." |
| S.2: the context that carries the modulus does not test it either: `fmpz_mod_ctx_init` only expects the modulus to be positive and says nothing about a prime. A design that promises every prime (decision S-D10) must establish primality itself, or take a factor of the modulus out of the `_f` variants of the previous row | flint-3.0.1 | fmpz_mod.rst:20, 22 | ".. function:: void fmpz_mod_ctx_init(fmpz_mod_ctx_t ctx, const fmpz_t n) / Initialise ``ctx`` for arithmetic modulo ``n``, which is expected to be positive." |
| S.2: the gcd over `Z/pZ` for a one-word `p` is `nmod_poly_gcd`. It is made monic except in the case where the gcd is zero, and it says NOTHING about the modulus: unlike `fmpz_mod_poly_gcd` it does not even assume a prime, so for a composite `p` the documentation promises nothing. The raw variant `_nmod_poly_gcd` (`nmod_poly.rst:1713`) makes no attempt at monicity | flint-3.0.1 | nmod_poly.rst:1716, 1718, 1719, 1720, 1721 | ".. function:: void nmod_poly_gcd(nmod_poly_t G, const nmod_poly_t A, const nmod_poly_t B) / Computes the GCD of `A` and `B`. The GCD of zero polynomials is / defined to be zero, whereas the GCD of the zero polynomial and some other / polynomial `P` is defined to be `P`. Except in the case where / the GCD is zero, the GCD `G` is made monic." |
| S.2: the only root routine documented in `nmod_poly` is `nmod_poly_find_distinct_nonzero_roots`, and it is not the routine a root finder wants: it returns `1` only when `A` has `deg(A)` distinct NONZERO roots, so the root `0` is never among them, and it returns `0` without saying anything whenever the number of distinct roots is smaller than the degree. It is the probabilistic method of Rabin and it assumes a prime modulus. The same routine for an `fmpz` modulus is `fmpz_mod_poly_find_distinct_nonzero_roots` (`fmpz_mod_poly.rst:563` to `567`) | flint-3.0.1 | nmod_poly.rst:2392, 2394, 2395, 2396 | ".. function:: int nmod_poly_find_distinct_nonzero_roots(mp_limb_t * roots, const nmod_poly_t A) / If ``A`` has `\deg(A)` distinct nonzero roots in `\mathbb{F}_p`, write these roots out to ``roots[0]`` to ``roots[deg(A) - 1]`` and return ``1``. / Otherwise, return ``0``. It is assumed that ``A`` is nonzero and that the modulus of ``A`` is prime. / This function uses Rabin's probabilistic method via gcd's with `(x + \delta)^{\frac{p-1}{2}} - 1`." |
| S.2: the module `nmod_poly_factor` of FLINT 3.0.1 has NO root-finding entry: the file holds 28 `.. function::` entries, the last is the worker at line 197, the file ends at line 201, and `grep -rn nmod_poly_roots` over the 26 fetched `.rst` files of `flint-3.0.1` returns nothing. The only root routine documented in it is the factorisation of the row above, which promises monic irreducible factors and says nothing about primality. The installed header of FLINT 3.0.1 on this machine, `/usr/include/flint/nmod_poly_factor.h:130`, declares `nmod_poly_roots`, and `:133` declares `nmod_poly_roots_factored`; that header is a local reference recorded in this document, not a fetched file. `[source pending: the documentation of nmod_poly_roots of FLINT 3.0.1, which the installed header declares and none of the 26 fetched .rst files describes]` | flint-3.0.1 | nmod_poly_factor.rst:184, 186, 187, 188 | ".. function:: mp_limb_t nmod_poly_factor(nmod_poly_factor_t res, const nmod_poly_t f) / Factorises a general polynomial ``f`` into monic irreducible factors / and returns the leading coefficient of ``f``, or 0 if ``f`` / is the zero polynomial." | "Sets the `p`-adic number ``y`` to ``poly`` evaluated at `a`, / `y = F(a)` is defined to precision `N` when `a` is integral and / `N+(n-1)b` when `b < 0`." |
| S.2 (lane s2-slice4): `nmod_poly_roots(r, f, 0)` finds the distinct roots of any nonzero `f`, not only of a squarefree or split one: it makes `f` monic and hands it to the helper, whose comment names the method. The helper assumes a prime modulus only through an assertion, which is compiled only in a debug build of FLINT | flint-src-3.0.1 | nmod_poly_factor/roots.c:17, 18, 19, 35, 194, 196, 197 | "Helper function for finding roots. The roots of a monic f are written / with exponent given in mult to r. Uses Rabin's Las Vegas algorithm / via gcd computations with (x + delta)^((p-1)/2) - 1. / FLINT_ASSERT(n_is_probabprime(f->mod.n)); / else / nmod_poly_make_monic(t + 0, f); / _nmod_poly_push_roots(r, t + 0, 1, t + 1, t + 2, t + 3, randstate);" |
| S.2 (lane s2-slice4): what `nmod_poly_roots` does for the zero polynomial (it throws), for degree 0 (no factor: `r->num` stays 0) and for degree 1 (the one factor, made monic, exponent 1) | flint-src-3.0.1 | nmod_poly_factor/roots.c:157, 159, 161, 165, 166, 167, 169, 171, 172 | "r->num = 0; / if (nmod_poly_degree(f) < 2) / if (nmod_poly_degree(f) == 1) / nmod_poly_make_monic(r->p + 0, f); / r->exp[0] = 1; / r->num = 1; / else if (nmod_poly_degree(f) < 0) / flint_throw(FLINT_ERROR, "Exception in nmod_poly_roots: " / "input polynomial is zero.");" |
| S.2 (lane s2-slice4): the roots are returned as linear factors, each `x - r` written as the coefficients `-r, 1` or as a monic factor of the splitting; for `p < 10` they are found by evaluation at every residue, and the root 0 is split off first | flint-src-3.0.1 | nmod_poly_factor/roots.c:38, 41, 43, 48, 49, 58, 63, 64 | "if (f->mod.n < 10) / for (x = 0; x < f->mod.n; x++) / if (0 != nmod_poly_evaluate_nmod(f, x)) / r->p[r->num].coeffs[0] = nmod_neg(x, f->mod); / r->p[r->num].coeffs[1] = 1; / if (f->coeffs[0] == 0) / r->p[r->num].coeffs[0] = 0; / r->p[r->num].coeffs[1] = 1;" |
| S.2 (lane s2-slice4): the first split keeps only the roots in `F_p`: the gcds of `f` with `x^((p-1)/2) - 1` and with `x^((p-1)/2) + 1` | flint-src-3.0.1 | nmod_poly_factor/roots.c:96, 97, 98, 99, 100, 101, 102 | "nmod_poly_zero(a); / nmod_poly_set_coeff_ui(a, 1, 1); / nmod_poly_powmod_ui_binexp_preinv(t, a, (f->mod.n - 1)/2, f, t2); / nmod_poly_sub_ui(t, t, 1); / nmod_poly_gcd(a, t, f); / nmod_poly_add_ui(t, t, 2); / nmod_poly_gcd(b, t, f);" |
| S.2 (lane s2-slice4): the routine is randomised and never reports a failure: a split with a random shift is repeated until it is proper, so a wrong answer is not signalled and the running time is an expected time only. The generator is created by `flint_randinit` in every call | flint-src-3.0.1 | nmod_poly/find_distinct_nonzero_roots.c:15, 29, 33, 34, 40, 42; nmod_poly_factor/roots.c:177 | "/* split f assuming that f has degree(f) distinct nonzero roots in Fp */ / try_again: / nmod_poly_set_coeff_ui(a, 0, n_randint(randstate, f->mod.n)); / nmod_poly_powmod_ui_binexp_preinv(t, a, (f->mod.n - 1)/2, f, t2); / if (0 >= nmod_poly_degree(a) \|\| nmod_poly_degree(a) >= nmod_poly_degree(f)) / goto try_again; / flint_randinit(randstate);" |
| S.2 (lane s2-slice4): `flint_randinit` sets two fixed words, so the random shifts, and with them the result and the time of `nmod_poly_roots`, are the same in every call on the same input | flint-src-3.0.1 | flint.h.in:245, 247, 248, 249, 250 | "void flint_randinit(flint_rand_t state) / state->gmp_init = 0; / #if FLINT64 / state->__randval = UWORD(13845646450878251009); / state->__randval2 = UWORD(13142370077570254774);" |
| S.2 (lane s2-slice4): `X^p` modulo `h` by `nmod_poly_powmod_ui_binexp` with the base `X`: a modulus of length 0 aborts, one of length 1 gives 0, and a base that is not shorter than the modulus is first reduced modulo it, so the base `X` is allowed for `deg h = 1`; exponents up to 2 are special cases | flint-src-3.0.1 | nmod_poly/powmod_ui_binexp.c:67, 70, 73, 74, 78, 83, 84, 90 | "if (lenf == 0) / flint_abort(); / if (lenf == 1) / nmod_poly_zero(res); / if (len >= lenf) / nmod_poly_divrem(t, r, poly, f); / nmod_poly_powmod_ui_binexp(res, r, e, f); / if (e <= 2)" |
| S.2 (lane s2-slice4): `nmod_poly_gcd` of `A` and the zero polynomial is `A` made monic, and a gcd of length 1 is set to the constant 1; otherwise the result is made monic. So the degree read off it is the degree of the monic gcd | flint-src-3.0.1 | nmod_poly/gcd.c:44, 46, 71, 72, 73, 74 | "else if (lenB == 0) /* lenA > lenB = 0 */ / nmod_poly_make_monic(G, A); / if (G->length == 1) / G->coeffs[0] = 1; / else / nmod_poly_make_monic(G, G);" |
| S.2 (lane s2-slice4): `nmod_poly_evaluate_nmod` is Horner's rule with modular products; the zero polynomial evaluates to 0 | flint-src-3.0.1 | nmod_poly/evaluate_nmod.c:21, 22, 29, 32, 34, 35 | "if (len == 0) / return 0; / val = poly[m]; / for ( ; m >= 0; m--) / val = n_mulmod2_preinv(val, c, mod.n, mod.ninv); / val = n_addmod(val, poly[m], mod.n);" |
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
   uniqueness. `2 A B = m` with two solutions (Proposition 13.2) is not contradicted by Shoup: Theorem 4.8 assumes
   `n > 2 r* t*` STRICTLY (`ntb-v2.txt:4159`). For `m = 2`, `A = B = 1` (`2 A B = m`) the pairs `(1, 1)` and
   `(-1, 1)` are two different ratios; their determinant is `1 * 1 - 1 * (-1) = 2 = m`, so they are not a common
   nonzero multiple of one pair, and Shoup's remark at `ntb-v2.txt:4152-4153` (a pair is determined only up to a
   nonzero multiple, so only the ratio can be unique) does not describe them. They illustrate the equality
   boundary: the strict inequality of Theorem 4.8 cannot be weakened to equality. The sharpness example itself is
   a project calculation, not a quote (corrected 2026-09-29 by lane s-design-repair, after the review).
3. **What FLINT guarantees and what it does not (S.3).** The documentation says the answer is unique if it exists
   (`fmpq.rst:565`) and that 0 means no solution. The code, however, accepts the EEA row only when `d <= D` and
   `gcd(n, d) = 1` (`fmpq/reconstruct_fmpz_2.c:1037, 1040`). Under `2 N D < m` these two conditions cannot lose a
   solution: by Shoup's Theorem 4.9(i) the row of the EEA has `|t0| <= t*` whenever a solution with `|t| <= t*`
   exists, and the reduced form of a solution is again a solution inside the same bounds (own calculation). So the
   return value 0 does mean "no solution" under the documented hypotheses. Sharper (`docs/proofs/solvers.md`
   P1.9(3), from P1.6(b)): for the functions and arguments of P1.9(2) (`m >= 1`, `0 <= a < m`, `1 <= N < m`,
   `D >= 1`) a return value 0 caused by a row denominator above `D` still means "no solution" for every `N`, `D`,
   because Shoup's bound `|t0| <= t*` holds whenever a solution within the bounds exists (Theorem 4.9(i),
   `ntb-v2.txt:4202`, whose hypothesis is that such a point exists); only a 0 caused by a row that is not reduced
   loses its meaning when `2 N D >= m`, and then solutions may exist. Uniqueness is promised only for
   `2 N D < m`. The SPEC's fourth result,
   "uniqueness not certified", is not a value FLINT can return; the design must produce it itself.
4. **Existence is not the SPEC's hypothesis (S.3).** `proofs/quotient.md` Proposition 13 claims at most one
   solution when `2 A B < m`; it does not claim that a solution exists. Thue's lemma
   (`ntb-v2.txt:2184`) gives a pair of integers under `0 < r* <= m < r* t*` (the modulus below the product of the
   bounds, which are STRICT: `|r| < r*`, `0 < |t| < t*`; the inclusive `A`, `B` of the SPEC are `r* = A + 1`,
   `t* = B + 1`, `docs/proofs/solvers.md` P1.8(4)), not a reduced fraction. A solver must be prepared to
   return "none" in every range with `A < m`
   (`docs/proofs/solvers.md` Proposition 1.8; this note said `m > A B` until 2026-09-29, which was wrong).
5. **The prime-field assumption (S.1).** `nmod_mat_can_solve` and `fmpz_mod_mat_can_solve` are documented for a
   prime modulus (`nmod_mat.rst:562`, `fmpz_mod_mat.rst:388`), and `fmpz_mod_mat_solve` says "The modulus is
   assumed to be prime". For a general `N` the entry point of FLINT 3.0.1 is `fmpz_mat_howell_form_mod(A, mod)`,
   whose source computes gcds against `N` (`fmpz_mat/strong_echelon_form_mod.c:78, 80`), so no primality is used
   there. The SOLVING functions of `fmpz_mod_mat` therefore cannot serve a composite `N`; its Howell entry
   `fmpz_mod_mat_howell_form` (`fmpz_mod_mat.rst:287`) names no condition on the modulus and is a call of
   `fmpz_mat_howell_form_mod` (`fmpz_mod_mat/howell_form.c:14` to `16`), so it does (corrected 2026-09-29 by lane
   s-design-repair, after the review). Both Howell entries read `(col, col)` of a NONEMPTY matrix and need at least
   as many rows as columns (`fmpz_mod_mat.rst:293`); an empty matrix returns at once
   (`fmpz_mat/strong_echelon_form_mod.c:167` to `168`).
6. **The certificate is not where FLINT puts it (S.1).** `fmpz_mat_hnf_transform` returns `U` with `U A = H`
   (`fmpz_mat.rst:1239`); `fmpz_mat_snf` has no such argument (`fmpz_mat.rst:1314`); and
   `nmod_mat_howell_form` works in place and returns only the number of nonzero rows
   (`nmod_mat/howell_form.c:15, 44`). The transformation for the Howell form is, in Storjohann's terms, the tuple
   `(Q, U, C, W, r)` (`diss2up.txt:2078`), and the LEFT kernel `{v : v A = 0}` (`diss2up.txt:2099`) is the product
   `W Q U C` (`diss2up.txt:2120`); the design, whose unknowns are columns, transposes. In the
   three fetched matrix modules the only documented output that is a transformation matrix is the `U` of
   `fmpz_mat_hnf_transform` (`grep -n transform` on the three `.rst` files: `fmpz_mat.rst:1235` is the only
   occurrence that is not about a similarity transform or a conversion to dense form). The design of the
   certificate must therefore either track the row operations itself or take them from the HNF over the integers.
7. **The orientation of the echelon form (S.1).** FLINT returns an upper right normal form and says so
   (`nmod_mat.rst:711`); the strong echelon form of [FieHof2014] is a lower left normal form. Storjohann's
   conditions (r1) to (r4) (`diss2up.txt:2058, 2068, 2070`) are the ones FLINT attributes to [StoMul1998]. A
   design that writes its own row reduction must fix the orientation before it states a normal form.
8. **Two shapes of Hensel's lemma (S.2).** Conrad's Theorem 2.1 (`hensel.txt:31`) is the simple-root form with
   the unique lift; his Theorem 4.1 (`hensel.txt:315`) is the strong form `|f(a)| < |f'(a)|^2`, which also applies
   when `a mod p` is a multiple root (`hensel.txt:310-311`), and it adds `|alpha - a| = |f(a)/f'(a)|`. Thorne's
   Lemma 3.6 (`jackthornenotes.txt:567`) has different hypotheses: `f` monic, `|f(x)| < 1`, `|f'(x)| = 1`, and the
   weaker conclusion `|y - x| <= |f(x)|`, not `|f(x)/f'(x)|`. Baker's Theorem 1.37 (`padicnotes.txt:662`) has a
   third shape, `f(a) ≡ 0 mod p^(2r-1)` and `f'(a) ≢ 0 mod p^r`, and states no uniqueness. The square root of the
   SPEC at 2 ("unit part 1 modulo 8") is the case `f = X^2 − u`, `a = 1`, `p = 2` of Theorem 4.1, in which
   `|f(1)|_2 <= 1/8 <= 1/4 = |f'(1)|^2_2`; that reading is an own calculation, not a quotation. The
   digit-by-digit proofs are in `hensel.txt:37` and `jackthornenotes.txt:580`.
9. **What FLINT does not have (S.2).** `fmpz_poly` lifts factors, not roots (`fmpz_poly.rst:2997`), and
   `padic_poly` has no root finding at all; the greps behind this statement are in the report of the lane. Simple
   roots by Hensel lifting have to be written in the project. For real roots, FLINT 3.0.1 offers two things:
   `fmpz_poly_num_real_roots_sturm` counts the real roots of a squarefree polynomial and returns no intervals
   (`fmpz_poly.rst:3253`), while `arb_fmpz_poly_complex_roots` isolates all the roots of a squarefree integer
   polynomial, real ones first in ascending order (`arb_fmpz_poly.rst:68, 70, 73`), and `arb_calc_isolate_roots`
   isolates the roots of a real analytic function with an explicit completeness rule read off the flags
   (`arb_calc.rst:127, 131`). `arb_calc_isolate_roots` is NOT used by the design of S.2: it takes a function pointer
   (`conventions.md` 12.7), and for polynomials the design uses the squarefree part, FLINT's count and the
   enclosures of `arb_fmpz_poly_complex_roots`, tested by exact signs (`docs/proofs/solvers.md` P3.8 to P3.10). The
   flag rule ("complete when no flag other than 1 occurs") is FLINT's statement for that function, and the earlier
   instruction of this note to exclude endpoint and multiple roots from the input was not needed: a
   completeness-status interface may leave such inputs unresolved instead (corrected 2026-09-29 by lane
   s-design-repair, after the review).

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

### URLs tried on 2026-09-28 that did not give a source

Every fetch of this lane used `curl --max-time 180`; a URL that failed once was tried once more. These are the
addresses that were tried and did not deliver a document (HTTP status in brackets; a document that arrived but
was not the paper wanted is marked):

- `https://kconrad.math.uconn.edu/blurb/papers/hensel.pdf` [404], `.../smithnormalform.pdf` [404],
  `.../rationalcrt.pdf` [404], `https://kconrad.math.uconn.edu/blurb/papers/` [404],
  `https://kconrad.math.uconn.edu/blurb/index.php` [404] (the site moved to `blurbs/`; the Hensel note was
  found at `blurbs/gradnumthy/hensel.pdf`).
- `https://kconrad.math.uconn.edu/blurbs/ugradnumthy/smith.pdf` [404],
  `https://kconrad.math.uconn.edu/blurbs/ugradnumthy/smithnormalform.pdf` [404] (item 8 above).
- `https://shoup.net/ntb/ntb.pdf` [404]; the book is at `https://shoup.net/ntb/ntb-v2.pdf`, which was fetched.
- `http://www.cs.sfu.ca/~mmonagan/Research/papers/ISSAC2004.pdf` [404] (item 2 above; no other address of
  Monagan's paper was tried, the arXiv API query for the title returned no hit).
- `https://www.inf.ethz.ch/personal/astorjoh/PhDThesis.pdf` [an XML error page, 711 bytes] and
  `https://www.algorism.com/~storjohann/PhDThesis.pdf` [an HTML error page, 114 bytes]; the dissertation was
  fetched from `https://cs.uwaterloo.ca/~astorjoh/diss2up.pdf`.
- `https://wstein.org/ent/ea.pdf` [404] (a candidate for a text with the Hermite form; not needed afterwards).
- The arXiv API query `all:"Howell form"` returned 0 entries, so no arXiv copy of [StoMul1998] or [FieHof2014]
  was found through it.
- No address was tried for Wang 1981, Wang-Guy-Davenport 1982, Collins and Encarnacion 1995, Howell 1986 or
  von zur Gathen and Gerhard: these are not offered free, and the brief asks for a lawful copy only.

### Sources pending for milestone S

1. A statement of the *maximal quotient* rational reconstruction, if the design of S.3 wants to certify existence
   with a bound pair larger than `A B < m`: `[source pending: Monagan 2004, item 2 above]`.
2. The complexity bounds of the modular linear algebra algorithms of [StoMul1998] and [FieHof2014], if the design
   of S.1 needs them for `PERF.md`: `[source pending: Storjohann and Mulders 1998; Fiedler and Hofmann]`.
3. Nothing is pending for S.2: Hensel's lemma in both forms is on disk with proofs, and the FLINT contract for
   real root isolation is on disk.
4. For the roots modulo a prime of any size (decision S-D10, `docs/SPEC.md` 15.3, `docs/proofs/solvers.md`
   P3.7(2)) the following is still missing. Item 3 above is about the `p`-adic and the real-root part of S.2 and
   stands; this item is about the modular part. The documentation of FLINT 3.0.1 that is on disk says which
   function to call and what it requires, and leaves open what it does.
   `[source pending: the documentation of nmod_poly_roots of FLINT 3.0.1. The installed header
   /usr/include/flint/nmod_poly_factor.h:130 declares it, but the 26 fetched .rst files of flint-3.0.1 do not
   document it: the module nmod_poly_factor ends at its worker entry (line 197) and has no root-finding
   section]`
   `[source pending: the C sources at the tag v3.0.1 of fmpz_mod_poly/powmod_x_fmpz_preinv.c,
   fmpz_mod_poly/gcd.c, fmpz_mod_poly_factor/roots.c, nmod_poly_factor/roots.c. They would settle what the
   documentation does not say: whether fmpz_mod_poly_roots is randomised, whether fmpz_mod_poly_gcd returns a
   monic polynomial, and what these routines do when the modulus is composite]`

5. Settled on 2026-09-29 by lane s2-slice4 for the one-word modulus: the C sources of `nmod_poly_roots` and of
   what it calls are on disk (`flint-src-3.0.1/nmod_poly_factor/roots.c`, `nmod_poly/find_distinct_nonzero_roots.c`,
   `nmod_poly/powmod_ui_binexp.c`, `nmod_poly/powmod_ui_binexp_preinv.c`, `nmod_poly/gcd.c`, `flint.h.in`; rows of
   table 3, S.2). They replace the missing documentation for what the library relies on: `nmod_poly_roots` is
   randomised with a fixed seed, never reports a failure, and throws for the zero polynomial; its candidates are
   therefore tested (`src/roots.c`, the roots modulo p). The `fmpz_mod_poly` sources of item 4 stay pending; the
   library does not call those routines, because a prime of a place is one word (`include/adelefeld/roots.h`,
   "The prime").

## Sources pending

1. Tate's thesis itself, for "section 2.2" and Tate's own signs (SPEC 6).
2. A source for the names "arithmetic" and "geometric" convention of the cyclotomic action (SPEC 9.3.7);
   the two formulas themselves are quoted from milne-cft.
3. An open text with proofs of the domains of convergence of the p-adic sine, cosine, sinh and cosh series
   (SPEC 9.3.2). The fetched texts cover exp and log with proofs (granville-ntr, evertse-padic, baker-padic)
   but not the trigonometric series.
4. Settled on 2026-09-28: the register-register form `add r64, r64` on Zen 2 is in the fetched page
   `uops-intel/ADD_01_R64_R64.html` (the "AMD Zen 2" block); the site serves the memory form `ADD_R64_M64`
   under the bare name `ADD_R64_M64`. See the note after table 1.
