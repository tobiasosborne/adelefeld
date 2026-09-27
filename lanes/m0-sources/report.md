# Report, lane m0-sources: work package 0.2 (sources on disk with hashes)

Date: 2026-09-27. Everything below was done in this working tree; no git command that changes state was run
(`git clone` and read-only `git rev-parse`/`git archive` were used as the brief requires for the package pin;
`git checkout` is forbidden by the lane rules and was not used).

## What was done

1. `refs/` was created as the ground-truth store: `fetch_sources.sh`, `README.md`, `.gitignore` (ignoring
   `src/`), `manifest.sha256`, `manifest-extra.sha256`, and `refs/src/` with 17 source keys. Formats follow
   the preference order: TeX where available (PARI manual), then reStructuredText (FLINT docs) and HTML
   (uops.info), then PDF with a `pdftotext -layout` extraction next to it.
2. `docs/sources.md` was written: table 1 has one row per source (32 rows, full reference, URL, path, sha256,
   date, format, use); table 2 has one row per `[unverified]` and `[standard]` label in `docs/SPEC.md` (7
   statement rows, with 12 sub-rows for the 9.3.7 catalogue of formulas), each settled by a machine-checked
   verbatim quotation or a "kept, because ..." reason.
3. Coverage of the brief's wanted list:
   - Item 1 (Tate's thesis): no lawful open copy exists. Three independent open expositions were fetched as
     named substitutes: `tate-poonen` (MIT 18.786 notes), `tate-kudla` (chapter copy on a university course
     page), `tate-warwick` (seminar notes). All state the additive character, self-dual measures, local and
     global zeta integrals and the functional equation.
   - Item 2 (Hertogh): `hertogh-thesis` (author's copy of the Leiden 2021 thesis; canonical record
     hdl.handle.net/1887/3249353) and `adeles-pkg` (SageMath package `adeles`, cloned, snapshot taken at the
     pinned commit `1acd6362bbedccb35aac7a343eeead17d443b0d1`, recorded in `adeles-pkg/COMMIT`).
   - Item 3 (adeles and ideles with proofs): `milne-cft`, `milne-ant`.
   - Item 4 (p-adic functions): `granville-ntr` (Granville, Number Theory Revealed, App. 16; a book offered
     openly by its author), `baker-padic`, `evertse-padic`, `thorne-padic`. These cover `exp` and `log` with
     the domains of convergence. No open text with proofs for `sin`, `cos`, `sinh`, `cosh` was found; see
     "sources pending".
   - Item 5 (Hilbert symbol, Kronecker): `hilbert-bristol` (formulas at odd p, at 2 and at the real place,
     with proof), `hilbert-mit` (norm reformulation), `hilbert-msp` (Vostokov, higher norm-residue symbols,
     kept for later tiers), and the Kronecker rules in `pari-doc:usersch3.tex`.
   - Item 6 (FLINT and PARI): the eight `.rst` files of FLINT at tag v3.0.1 (`padic`, `fmpz_mod`, `nmod`,
     `arb`, `acb`, `acb_dirichlet`, `fmpq`, `ulong_extras`) and the PARI/GP 2.17.4 manual source
     (`usersch1.tex` to `usersch8.tex`, `appb.tex`, `appd.tex`; `bnfcertify`, `hilbert`, `kronecker`,
     `bnrinit` are all in `usersch3.tex`). The matching FLINT headers are on this machine under
     `/usr/include/flint`; this is recorded in `docs/sources.md`.
   - Item 7 (PERF): `uops-zen2` (the seven uops.info instruction pages covering the instructions and the AMD
     Zen 2 column of PERF.md section 1: `ADD_R64_M64`, `ADC_R64_M64`, `MUL_R64`, `IMUL_R64_R64`,
     `IMUL_R64_R64_I8`, `IMUL_R64_R64_I32`, `VPMULUDQ_YMM_YMM_YMM`) and `hvh-mult` (Harvey and van der
     Hoeven, from HAL hal-02070778v2).
4. The three checks the brief names were done against the sources on disk and are recorded in table 2 of
   `docs/sources.md` and in "findings" below: the signs of the additive character and the Fourier transform
   (SPEC 6), Hertogh's sum/product rules and his equality (SPEC 14), the two reciprocity conventions
   (SPEC 9.3.7).

## Files written (all inside the lane's ownership)

- `refs/.gitignore`, `refs/README.md`, `refs/fetch_sources.sh`, `refs/manifest.sha256`,
  `refs/manifest-extra.sha256`, `refs/src/` (gitignored, 17 keys).
- `docs/sources.md`.
- `lanes/m0-sources/report.md` (this file).

## Checks run, with results

1. `refs/fetch_sources.sh` (first run, then a second run). Result: 30 downloads plus the package
   clone and snapshot; second run: every file reported "have", manifests rewritten. Clone provenance line:
   "adeles clone HEAD: 1acd6362bbedccb35aac7a343eeead17d443b0d1 (pinned:
   1acd6362bbedccb35aac7a343eeead17d443b0d1)". Manifest sizes: `manifest.sha256` 2203 files,
   `manifest-extra.sha256` 48 files (the `pdftotext -layout` extractions and package text files).
2. `refs/fetch_sources.sh --check` (the hash re-check the brief demands; internally `sha256sum -c` on both
   manifests). Result: exit status 0; 2251 lines "OK" (2203 + 48); 0 "FAILED".
3. Quote checker (python one-shot run against `docs/sources.md` and `refs/src/`): 57 quoted strings parsed,
   0 not found in their source files (all verbatim substrings); second run checking the cited line numbers:
   57 checked, 0 mismatches. The checker unescapes the documented `\xNN` and `\|` renderings.
4. `grep -c "\[unverified\]\|\[standard\]" docs/SPEC.md`: 9 hits. Two are the label definitions (SPEC lines
   18, 20); 7 are statements, and table 2 of `docs/sources.md` has exactly 7 rows plus the formula sub-rows.
5. `file` on the fetched PDFs: all report "PDF document" (e.g. `tate-poonen/notes.pdf` PDF 1.7,
   `hvh-mult/nlogn.pdf` PDF 1.4, 46 pages).
6. Harvey and van der Hoeven identity check, `head refs/src/hvh-mult/nlogn.txt`: title "Integer
   multiplication in time O(n log n)", "David Harvey, Joris van der Hoeven", "Annals of Mathematics, 2021",
   hal-02070778v2. This matches the PERF.md citation (Annals of Mathematics 193 (2021) 563-617).
7. uops.info figures of PERF.md section 1 against the fetched pages (Zen 2 measurement blocks):
   - `MUL_R64`: "Latency operand 1 -> 2: 3", "operand 1 -> 3: 4", "operand 1 -> 4: 4"; throughput
     "Measured (loop): 1.00". Matches "latency 3 cycles to the low half, 4 to the high half; 1 per cycle".
   - `IMUL_R64_R64_I8` and `_I32`: "Latency operand 2 -> 1: 3"; throughput 1.00. Matches "latency 3 cycles".
   - `VPMULUDQ_YMM_YMM_YMM`: "Latency operand 2 -> 1: 3"; "Measured (loop): 1.00". Matches "latency 3;
     1 per cycle".
   - `ADC_R64_M64`: "Latency operand 3 -> 1: 1", "operand 3 -> 3: 1", with "Operand 3 (r/w, suppressed):
     Flags (AF: w, CF: r/w, OF: w, PF: w, SF: w, ZF: w)". This is consistent with "carry-to-carry latency
     1 cycle". See finding P2 for the throughput figure.
   - `ADD_R64_M64`: "Latency operand 1 -> 1: 1". Matches "latency 1 cycle". See finding P1 for the
     throughput figure.
8. Line lengths in `docs/sources.md`: 11 lines over 116 characters outside tables, all of them citation-plus-
   verbatim-quote data lines (documented in the file); the prose keeps the limit.

## What is not done

1. Item 4 of the brief is only met for `exp` and `log`. No open text with proofs of the domains of
   convergence of the p-adic `sin`, `cos`, `sinh`, `cosh` series was found. The SPEC statements at 9.3.2 are
   labelled `[proved]` (project proofs), so no SPEC label currently rests on the missing text; the ground
   truth for the implementation (work package 1F) is still to be sourced.
2. Tate's thesis itself is not on disk (no lawful open copy), so "section 2.2" of the SPEC attribution cannot
   be checked and Tate's own signs cannot be quoted first-hand.
3. The `--force` path of `fetch_sources.sh` (full re-download) was not exercised; the fetch path was
   exercised twice and the hash check once.
4. The names "arithmetic" and "geometric" for the two reciprocity conventions are not covered by a source;
   the two formulas themselves are quoted.

## Sources pending

1. A lawful copy of Tate's thesis ("Fourier analysis in number fields and Hecke's zeta-functions"), for the
   reference "section 2.2" and Tate's own signs (SPEC 6). `[source pending: Tate's thesis]`
2. A source for the names "arithmetic" and "geometric" convention of the cyclotomic action (SPEC 9.3.7).
   `[source pending: a source naming the two reciprocity conventions]`
3. An open text with proofs of the domains of convergence of the p-adic `sin`, `cos`, `sinh`, `cosh` series
   (SPEC 9.3.2). `[source pending: p-adic trigonometric series convergence, with proofs]`
4. A uops.info page for the register-register forms `add r64, r64` and `adc r64, r64` on Zen 2 (finding P1).
   `[source pending: uops.info Zen 2 figures for the register form of add/adc]`

## Findings against the specification

No statement of `docs/SPEC.md` was found to be wrong. Three checked points in detail:

1. SPEC 6 (signs of the additive character and the Fourier transform). The additive character
   `psi(x) = exp(2 pi i ( - x_inf + sum {x_p}_p ))` matches the standard psi of `tate-poonen:notes.txt:693`
   and `:700` exactly (minus sign at R, `psi(1/p^n) = e^(2 pi i / p^n)` at Q_p). Triviality on Q is quoted
   (`tate-kudla:kudla-1.txt:903`, "trivial on k"). The transform `hat f(y) = integral f(x) conj(psi(xy)) dx`
   carries the conjugate; `tate-poonen:notes.txt:740` states "Tate originally took the complex conjugate of
   the additive character", so the SPEC is Tate's original convention. Both fetched expositions define the
   transform without the conjugate. This is a convention difference between references, documented in
   `docs/sources.md` 2.2; it is not a contradiction of the SPEC. The attribution "Convention of Tate's
   thesis, section 2.2" stays unverified (source pending).
2. SPEC 14 (Hertogh). All three sub-claims are confirmed by quotations: his sum and product rules are
   exactly the formulas of SPEC 4.3 with "smallest represented subsets (with respect to inclusion)" (the
   SPEC's tightness); his multiplicative p-adics keep a precision per prime and multiply with
   `min(p(a), p(b))`; his `==` is the overlap test "loose equivalence", which the thesis itself says "fails
   to be transitive". No difference found.
3. SPEC 9.3.7 (two reciprocity conventions). Milne's notes contain both maps: the canonical isomorphism
   (z -> z^(u')) and the global reciprocity map as "the reciprocal of" it (z -> z^(1/u')) at
   `milne-cft:CFT.txt:9883-9886`. Milne's normalisation (uniformiser idele to the Frobenius element,
   `:9904-9908`, with Frobenius x -> x^q, `:1307`) gives z -> z^p for the SPEC's test idele, which the SPEC
   calls the arithmetic convention. The formulas and the test vector are confirmed. Milne's notes do not use
   the names "arithmetic" and "geometric" (source pending).

Two findings against `docs/PERF.md` (not the specification):

- P1. PERF.md section 1 says "`add r64` | latency 1 cycle; 4 per cycle | uops.info (Zen 2)". The latency
  matches the fetched page (`ADD_R64_M64`, Zen 2: "Latency operand 1 -> 1: 1"). The figure "4 per cycle"
  matches the page's "Documentation" column ("Throughput: 0.25"). The page's Zen 2 MEASURED throughput is
  "Measured (loop): 0.50", i.e. 2 per cycle, for the memory form; uops.info serves no page for the
  register-register form under these names (`ADD_R64_R64` and `ADD_R64_R64` variants return 404; the
  instruction list has "ADD (R64, M64)" and no "ADD (R64, R64)"). The row should say which column is meant.
- P2. The same for "`adc r64`": the Zen 2 measured throughput of `ADC_R64_M64` is 0.50 (2 per cycle), the
  documentation figure is 0.25 (4 per cycle). The carry-to-carry latency of 1 cycle is consistent with the
  measured flag latencies ("Latency operand 3 -> 1: 1", "operand 3 -> 3: 1", operand 3 = Flags).

PERF.md's other uops rows (`mul r64`, `imul r64, imm`, `vpmuludq` 256-bit) match the Zen 2 measurements on
the fetched pages exactly, as itemised in the checks above.
