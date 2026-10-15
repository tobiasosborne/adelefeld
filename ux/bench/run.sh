#!/bin/sh
# Regenerate the fixtures of the Bench slice: every experiment of ux/bench/experiments/ is swept by build/adf and
# drawn. Run from anywhere; the driver must be built (make -j2 && make -C tools/adf, in the repository root).
#
#   sh ux/bench/run.sh            sweep and draw every experiment into ux/bench/out/
#   sh ux/bench/run.sh --check    sweep and draw into a temporary directory and diff the records and pages with
#                                 ux/bench/out/ (the *.run.json files hold times and are not compared; a page's
#                                 footer names the records it read, so the temporary directory is replaced by
#                                 ux/bench/out/ in the page before the diff)
#   sh ux/bench/run.sh NAME...    only the experiments named
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
check=0
if [ "${1:-}" = "--check" ]; then check=1; shift; fi
if [ $# -eq 0 ]; then
  set --
  for f in "$here"/experiments/*.json; do
    case "$f" in *.findings.json) ;; *) set -- "$@" "$(basename "$f" .json)" ;; esac
  done
fi
out="$here/out"
if [ $check -eq 1 ]; then out=$(cd "$(mktemp -d)" && pwd -P); fi
status=0
for name in "$@"; do
  timeout 300 python3 "$here/sweep.py" "$here/experiments/$name.json" --driver "$root/build/adf" --out "$out"
  timeout 60 python3 "$here/render.py" "$here/experiments/$name.json" --out "$out"
  if [ $check -eq 1 ]; then
    sed "s|<code>$out/|<code>ux/bench/out/|g" "$out/$name.html" > "$out/$name.html.norm"
    mv "$out/$name.html.norm" "$out/$name.html"
    for ext in records.jsonl html; do
      if diff -q "$here/out/$name.$ext" "$out/$name.$ext" >/dev/null; then
        echo "check $name.$ext: identical"
      else
        echo "check $name.$ext: DIFFERS"; status=1
      fi
    done
  fi
done
if [ $check -eq 1 ]; then rm -rf "$out"; fi
exit $status
