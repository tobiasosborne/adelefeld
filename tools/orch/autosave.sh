#!/usr/bin/env bash
# Save the work of every lane worktree as a WIP commit on its own branch and push the branch, every
# AUTOSAVE_EVERY seconds (default 300), until UNTIL (epoch seconds) or until lanes/AUTOSAVE_STOP exists.
# It needs no model and no quota: what a lane has written is on origin even if its session ends.
# It never touches master and never merges. Usage: tools/orch/autosave.sh [hours]
set -u
REPO="$(cd "$(dirname "$0")/../.." && pwd)"; LOG="$REPO/../adelefeld-wt/autosave.log"
UNTIL=$(( $(date +%s) + ${1:-6} * 3600 ))
while [ "$(date +%s)" -lt "$UNTIL" ] && [ ! -e "$REPO/lanes/AUTOSAVE_STOP" ]; do
  git -C "$REPO" worktree list --porcelain | awk '/^worktree /{print $2}' | while read -r wt; do
    [ "$wt" = "$REPO" ] && continue
    [ -d "$wt" ] || continue
    br=$(git -C "$wt" branch --show-current 2>/dev/null); [ -n "$br" ] || continue
    [ -n "$(git -C "$wt" status --porcelain 2>/dev/null | head -1)" ] || continue
    git -C "$wt" add -A >/dev/null 2>&1 \
      && git -C "$wt" commit -q -m "WIP autosave $br $(date -Is)" >/dev/null 2>&1 \
      && git -C "$wt" push -q -f origin "$br" >/dev/null 2>&1 \
      && echo "$(date -Is) saved $br" >> "$LOG"
  done
  sleep "${AUTOSAVE_EVERY:-300}"
done
echo "$(date -Is) autosave ends" >> "$LOG"
