#!/usr/bin/env bash
# Fetch every source cited in docs/sources.md into refs/src/<key>/ (gitignored) and record sha256
# hashes in refs/manifest.sha256 (fetched bytes) and refs/manifest-extra.sha256 (text extractions
# made with pdftotext -layout). Quotes in the project cite <key>:<file>:<line> against these files.
# Format preference (work package 0.2): TeX, then HTML/reStructuredText, then PDF plus extraction.
# Only lawful, publicly offered copies. Usage:
#   ./fetch_sources.sh          fetch whatever is missing, then (re)write both manifests
#   ./fetch_sources.sh --check  fetch nothing; verify both manifests with sha256sum -c
#   ./fetch_sources.sh --force  re-download everything
set -eu
cd "$(dirname "$0")"

MODE=fetch
FORCE=0
for arg in "$@"; do
  case "$arg" in
    --check) MODE=check ;;
    --force) FORCE=1 ;;
    *) echo "unknown argument: $arg" >&2; exit 2 ;;
  esac
done

if [ "$MODE" = check ]; then
  echo "== manifest.sha256 (fetched bytes)"
  sha256sum -c manifest.sha256
  echo "== manifest-extra.sha256 (pdftotext extractions)"
  sha256sum -c manifest-extra.sha256
  exit 0
fi

get() { # get URL DEST  -- download DEST from URL unless it exists (or --force)
  url=$1; dest=$2
  if [ -s "$dest" ] && [ "$FORCE" = 0 ]; then
    echo "have $dest"
    return 0
  fi
  mkdir -p "$(dirname "$dest")"
  echo "get  $dest"
  curl -sSL --retry 2 --max-time 180 -o "$dest" "$url"
}

pdf2txt() { # pdf2txt FILE -- make FILE.txt next to FILE with pdftotext -layout, if available
  if command -v pdftotext >/dev/null 2>&1 && { [ "$FORCE" = 1 ] || [ ! -s "${1%.pdf}.txt" ]; }; then
    pdftotext -layout "$1" "${1%.pdf}.txt"
  fi
}

# ---------------------------------------------------------------- Tate's thesis (no lawful original)
get "https://math.mit.edu/~poonen/786/notes.pdf" src/tate-poonen/notes.pdf
get "https://u.cs.biu.ac.il/~reznikov/courses/kudla-1.pdf" src/tate-kudla/kudla-1.pdf
get "https://warwick.ac.uk/fac/sci/maths/people/staff/sheth/tatesthesis_notes.pdf" \
    src/tate-warwick/tatesthesis_notes.pdf

# ---------------------------------------------------------------- Adeles and ideles, with proofs
get "https://www.jmilne.org/math/CourseNotes/CFT.pdf" src/milne-cft/CFT.pdf
get "https://www.jmilne.org/math/CourseNotes/ANT.pdf" src/milne-ant/ANT.pdf

# ------------------------------------------------- Hertogh thesis (author's copy; Leiden hdl 1887/3249353)
PIN=1acd6362bbedccb35aac7a343eeead17d443b0d1   # mathehertogh/adeles, "Add link to thesis artifact", 2025-11-25
get "https://raw.githubusercontent.com/mathehertogh/adeles/$PIN/Computing_with_adeles_and_ideles.pdf" \
    src/hertogh-thesis/thesis.pdf

# Sage package adeles: clone, then extract the pinned commit with git archive (read-only; the lane
# rules forbid git checkout). COMMIT records the pin that the snapshot is taken at.
if [ ! -d src/adeles-pkg/adeles/.git ]; then
  mkdir -p src/adeles-pkg
  echo "clone mathehertogh/adeles"
  git clone --quiet https://github.com/mathehertogh/adeles.git src/adeles-pkg/adeles
fi
head=$(git -C src/adeles-pkg/adeles rev-parse HEAD)
echo "adeles clone HEAD: $head (pinned: $PIN)"
printf '%s\n' "$PIN" > src/adeles-pkg/COMMIT
rm -rf src/adeles-pkg/snapshot
mkdir -p src/adeles-pkg/snapshot
git -C src/adeles-pkg/adeles archive "$PIN" | tar -x -C src/adeles-pkg/snapshot

# ---------------------------------------------------------------- p-adic exp, log (sin, cos pending)
get "https://dms.umontreal.ca/~revealed/Appendix16.pdf" src/granville-ntr/Appendix16.pdf
get "https://people.willamette.edu/~cstarr/math356/Notes/padicnotes.pdf" src/baker-padic/padicnotes.pdf
get "https://kconrad.math.uconn.edu/math5020f11/evertsepadicnotes.pdf" src/evertse-padic/evertsepadicnotes.pdf
get "https://kconrad.math.uconn.edu/math5020f11/jackthornenotes.pdf" src/thorne-padic/jackthornenotes.pdf

# ---------------------------------------------------------------- Hilbert symbol, Kronecker symbol
get "https://ocw.mit.edu/courses/18-786-number-theory-ii-class-field-theory-spring-2016/27f59437d11cbc1a6039b250e4c325ca_MIT18_786S16_lec2.pdf" \
    src/hilbert-mit/lec2.pdf
get "https://msp.org/gtm/2000/03/gtm-2000-03-008p.pdf" src/hilbert-msp/explicit-hilbert.pdf
get "https://people.maths.bris.ac.uk/~malab/PDFs/Algae_are_more_numb_19.pdf" src/hilbert-bristol/lecture19.pdf

# ---------------------------------------------------------------- FLINT 3.0.1 documentation (.rst)
for m in padic fmpz_mod nmod arb acb acb_dirichlet fmpq ulong_extras; do
  get "https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/$m.rst" "src/flint-3.0.1/$m.rst"
done

# ---------------------------------------------------------------- PARI/GP manual source (TeX)
PARIV=2.17.4
if [ ! -s src/pari-doc/pari.tar.gz ] || [ "$FORCE" = 1 ]; then
  mkdir -p src/pari-doc
  echo "get  src/pari-doc/pari.tar.gz"
  curl -sSL --retry 2 --max-time 300 -o src/pari-doc/pari.tar.gz \
    "https://pari.math.u-bordeaux.fr/pub/pari/unix/pari-$PARIV.tar.gz"
fi
if [ ! -s src/pari-doc/usersch3.tex ] || [ "$FORCE" = 1 ]; then
  echo "extract PARI manual TeX"
  tar -xzf src/pari-doc/pari.tar.gz -C src/pari-doc --strip-components=2 "pari-$PARIV/doc/usersch1.tex" \
    "pari-$PARIV/doc/usersch2.tex" "pari-$PARIV/doc/usersch3.tex" "pari-$PARIV/doc/usersch4.tex" \
    "pari-$PARIV/doc/usersch5.tex" "pari-$PARIV/doc/usersch6.tex" "pari-$PARIV/doc/usersch7.tex" \
    "pari-$PARIV/doc/usersch8.tex" "pari-$PARIV/doc/appb.tex" "pari-$PARIV/doc/appd.tex"
fi

# ---------------------------------------------------------------- uops.info instruction pages (Zen 2)
for n in ADD_R64_M64 ADC_R64_M64 MUL_R64 IMUL_R64_R64 IMUL_R64_R64_I8 IMUL_R64_R64_I32 VPMULUDQ_YMM_YMM_YMM; do
  get "https://uops.info/html-instr/$n.html" "src/uops-zen2/$n.html"
done

# ---------------------------------------------------------------- Harvey -- van der Hoeven (HAL)
get "https://hal.science/hal-02070778/file/nlogn.pdf" src/hvh-mult/nlogn.pdf

# ---------------------------------------------------------------- text extractions
if command -v pdftotext >/dev/null 2>&1; then
  find src -type f -name '*.pdf' | LC_ALL=C sort | while read -r f; do pdf2txt "$f"; done
else
  echo "pdftotext not found: PDF text extractions skipped" >&2
fi

# ---------------------------------------------------------------- manifests
find src -type f ! -path '*/.git/*' ! -name '*.txt' | LC_ALL=C sort | xargs sha256sum > manifest.sha256
find src -type f ! -path '*/.git/*' -name '*.txt' | LC_ALL=C sort | xargs -r sha256sum > manifest-extra.sha256
echo "== manifest.sha256: $(wc -l < manifest.sha256) files"
echo "== manifest-extra.sha256: $(wc -l < manifest-extra.sha256) files"
