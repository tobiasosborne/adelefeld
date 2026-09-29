#!/usr/bin/env bash
# The mutation sweep over src/ (item 5 of lanes/m1-repair-tools/brief.md), run by the orchestrator.
# One file at a time, small files first; --san and the judge `make check INV=1` (so that the lines
# ADF_INV_... are live: in the release build they are compiled away and their mutants survive by
# construction; the first run, in release-run/, spent a third of its mutants on them); seed 20260928;
# at most LIMIT mutants for each file.
# A file whose run passes FILE_TIMEOUT seconds is stopped; its survivors so far are in its log
# (mutate.py prints a survivor when it finds it). Results: lanes/m1-sweep/<file>.log, sweep.md.
set -u
cd "$(dirname "$0")/../.."
D=lanes/m1-sweep
LIMIT="${LIMIT:-200}"; FILE_TIMEOUT="${FILE_TIMEOUT:-6000}"
[ -s "$D/sweep.md" ] || printf '| file | result | seconds | last line of the tool |\n|---|---|---|---|\n' > "$D/sweep.md"
for f in inlines place status cap rat common fball_local recon adele modctx scaled fball text dump; do
  [ -e "$D/STOP" ] && { echo "STOP" >> "$D/sweep.md"; exit 2; }
  grep -q "^| src/$f.c |" "$D/sweep.md" && continue
  while [ "$(free -g | awk '/^Mem/{print $7}')" -lt 6 ]; do sleep 60; done
  s=$(date +%s)
  timeout "$FILE_TIMEOUT" python3 tools/mutate/mutate.py --root . --scratch "/tmp/claude-1000/adf-sweep" \
    --files "src/$f.c" --jobs 2 --seed 20260928 --limit "$LIMIT" --timeout 300 --san \
    --make "make -s -j2 check INV=1" --copy Makefile include src tests lanes > "$D/$f.log" 2>&1
  rc=$?
  printf '| src/%s.c | exit %s | %s | %s |\n' "$f" "$rc" "$(( $(date +%s) - s ))" \
    "$(grep '^mutate: [0-9]* mutants' "$D/$f.log" | tail -1)" >> "$D/sweep.md"
done
echo "DONE $(date -Is)" >> "$D/sweep.md"
