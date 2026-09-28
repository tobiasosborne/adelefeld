# refs

Ground truth for `adelefeld`: a local copy of every source the specification leans on, under `refs/src/<key>/`
(this directory is gitignored: other people's texts are not redistributed here). `fetch_sources.sh` re-fetches
every source; `fetch_intel.sh` re-fetches the Intel uops.info pages into `refs/src/uops-intel/`. `manifest.sha256`
holds the sha256 of every fetched byte, `manifest-intel.sha256` the sha256 of the Intel pages, and
`manifest-extra.sha256` the sha256 of the text extractions made with `pdftotext -layout`. Quotes in the project are
cited as `<key>:<file>:<line>` against these files.

## Usage

    ./fetch_sources.sh          # fetch whatever is missing, then (re)write both manifests
    ./fetch_sources.sh --check  # fetch nothing; verify both manifests with sha256sum -c
    ./fetch_sources.sh --force  # re-download everything
    ./fetch_intel.sh            # the same for the Intel uops.info pages (refs/src/uops-intel/,
                                # manifest-intel.sha256)
    ./fetch_intel.sh --check    # verify refs/manifest-intel.sha256 only

## Preference order for formats

TeX source, then HTML or reStructuredText, then PDF with a `pdftotext -layout` extraction next to it. Only
lawful, publicly offered copies: arXiv, authors' and university pages, institutional repositories, official
project repositories. No shadow libraries.

## What is not here

Tate's thesis ("Fourier analysis in number fields and Hecke's zeta-functions", 1950; printed in
Cassels-Froehlich, Algebraic Number Theory, 1967) is not offered as an open copy, so `fetch_sources.sh` does not
fetch it. A scan supplied by TJO lies under `refs/src/tate-thesis/` (key `tate-thesis`): the page images
`pages/p<N>.png` are the ground truth; `tex/p<N>.tex` is an untrusted OCR draft kept next to its scan, to be
refereed against it (`docs/sources.md`). The keys `tate-poonen`, `tate-kudla`, `tate-warwick` are open
expositions of the same conventions.

## Keys

| Key | Source |
|---|---|
| `tate-poonen` | B. Poonen, Tate's thesis (MIT 18.786 notes) |
| `tate-kudla` | S. Kudla, Tate's thesis (chapter; copy on a university course page) |
| `tate-warwick` | A. Sheth et al., Tate's thesis (Warwick seminar notes) |
| `milne-cft` | J. S. Milne, Class Field Theory (course notes) |
| `milne-ant` | J. S. Milne, Algebraic Number Theory (course notes) |
| `hertogh-thesis` | M. Hertogh, Computing with adèles and idèles (Leiden MSc thesis, author's copy) |
| `adeles-pkg` | SageMath package `adeles`, pinned commit (recorded in `COMMIT`) |
| `granville-ntr` | A. Granville, Number Theory Revealed, Appendix 16 (p-adic log and exp) |
| `baker-padic` | A. J. Baker, An Introduction to p-adic Numbers and p-adic Analysis |
| `evertse-padic` | J. H. Evertse, p-adic numbers (course notes) |
| `thorne-padic` | J. Thorne, p-adic analysis, p-adic arithmetic (course notes) |
| `hilbert-mit` | MIT 18.786 lecture 2, Hilbert symbols |
| `hilbert-msp` | S. V. Vostokov, Explicit formulas for the Hilbert symbol (higher norm-residue symbols) |
| `hilbert-bristol` | The Hilbert symbol (Bristol lecture notes; formulas at odd p, at 2 and at R, with proof) |
| `flint-3.0.1` | FLINT 3.0.1 documentation (.rst from the FLINT repository at tag v3.0.1) |
| `pari-doc` | PARI/GP manual source (usersch*.tex from the PARI sources) |
| `uops-zen2` | uops.info instruction pages (Zen 2 measurements) |
| `uops-intel` | uops.info instruction pages (all microarchitectures; Alder Lake-P column for the Intel profile), fetched by `fetch_intel.sh` |
| `hvh-mult` | D. Harvey, J. van der Hoeven, Integer multiplication in time O(n log n) (HAL) |

The matching FLINT headers for version 3.0.1 are on this machine under `/usr/include/flint` and are not
fetched; `docs/sources.md` records the paths.
