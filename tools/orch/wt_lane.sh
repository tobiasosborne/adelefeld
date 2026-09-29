#!/usr/bin/env bash
# Run a lane in its own git worktree, so that parallel lanes never see each other's half-written files.
# Usage: tools/orch/wt_lane.sh <lane> <pi|codex> <model> [effort]
# The worktree is ../adelefeld-wt/<lane> on branch lane/<lane>, created from the current HEAD. The lane's brief
# must be committed. The orchestrator commits in the worktree afterwards and merges the branch.
set -eu
LANE="$1"; TOOL="$2"; MODEL="$3"; EFFORT="${4:-high}"
REPO="$(cd "$(dirname "$0")/../.." && pwd)"; WT="$REPO/../adelefeld-wt/$LANE"
mkdir -p "$REPO/../adelefeld-wt"
if [ ! -d "$WT" ]; then git -C "$REPO" worktree add -q -b "lane/$LANE" "$WT" HEAD; fi
[ -e "$WT/refs/src" ] || ln -s "$REPO/refs/src" "$WT/refs/src"
cd "$WT"
case "$TOOL" in
  pi)    LANE_ROOT="$WT" exec "$REPO/tools/orch/pi_lane.sh" "$LANE" "$MODEL" "$EFFORT" ;;
  codex) exec tools/orch/codex_lane.sh "$LANE" "$MODEL" "$EFFORT" ;;
  *) echo "unknown tool $TOOL" >&2; exit 2 ;;
esac
