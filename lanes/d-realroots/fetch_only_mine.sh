#!/usr/bin/env bash
# Lane d-realroots: fetch only the additions of this lane to refs/fetch_sources.sh, with the same commands,
# without running the whole script (which rewrites the snapshot of adeles-pkg and both manifests while other
# lanes read refs/src). Prints the sha256 lines to be merged into refs/manifest.sha256.
set -eu
cd "$(dirname "$0")/../../refs"
FORCE=0
get() {
  url=$1; dest=$2
  if [ -s "$dest" ] && [ "$FORCE" = 0 ]; then
    echo "have $dest" >&2
    return 0
  fi
  mkdir -p "$(dirname "$dest")"
  echo "get  $dest" >&2
  curl -sSL --retry 2 --max-time 180 -o "$dest" "$url"
}
FLINTSRC_RR="acb_poly/find_roots.c acb_poly/refine_roots_durand_kerner.c acb_poly/validate_roots.c
acb_poly/validate_real_roots.c acb_poly/root_bound_fujiwara.c
fmpz_poly/taylor_shift.c fmpz_poly/taylor_shift_horner.c fmpz_poly/taylor_shift_divconquer.c
fmpz_poly/scale_2exp.c fmpz_poly/signature.c fmpz_poly/evaluate_fmpz.c fmpz_poly/evaluate_horner_fmpz.c
fmpz_poly/evaluate_divconquer_fmpz.c fmpz_poly/evaluate_fmpq.c fmpz_poly/evaluate_horner_fmpq.c
fmpz_poly/evaluate_divconquer_fmpq.c
arb_calc/isolate_roots.c arb_calc/refine_root_bisect.c arb_calc/refine_root_newton.c"
for f in $FLINTSRC_RR; do
  get "https://raw.githubusercontent.com/flintlib/flint/v3.0.1/src/$f" "src/flint-src-3.0.1/$f"
done
get "https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/acb_poly.rst" src/flint-3.0.1/acb_poly.rst
arxiv_tex() {
  get "https://arxiv.org/e-print/$1" "src/$2/$1.tar.gz"
  if [ ! -d "src/$2/tex" ] || [ "$FORCE" = 1 ]; then
    mkdir -p "src/$2/tex"
    tar -xzf "src/$2/$1.tar.gz" -C "src/$2/tex"
  fi
}
arxiv_tex 1308.4088v2 sagraloff-mehlhorn
arxiv_tex 1104.1362v3 kerber-sagraloff
{
  for f in $FLINTSRC_RR; do echo "src/flint-src-3.0.1/$f"; done
  echo src/flint-3.0.1/acb_poly.rst
  find src/sagraloff-mehlhorn/ src/kerber-sagraloff/ -type f ! -name '*.txt'
} | LC_ALL=C sort | xargs sha256sum
