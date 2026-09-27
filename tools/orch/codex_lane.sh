#!/usr/bin/env bash
# Run one codex lane, detached by the caller. Usage: tools/orch/codex_lane.sh <lane> <model> [effort]
# Reads lanes/COMMON.md and lanes/<lane>/brief.md; writes lanes/<lane>/{stdout.log,last.md,session.id,lane.log}.
# Resumes the session after a failure (pattern of riemann-channel/scripts/astra_lane.sh).
set -u
LANE="$1"; MODEL="$2"; EFFORT="${3:-xhigh}"
REPO="$(cd "$(dirname "$0")/../.." && pwd)"; cd "$REPO"
D="lanes/$LANE"; OUT="$D/stdout.log"; LOG="$D/lane.log"; SID="$D/session.id"
[ -e lanes/STOP ] && { echo "$(date -Is) STOP present, not started" >> "$LOG"; exit 2; }
PROMPT="$(cat lanes/COMMON.md; echo; echo "Your lane directory: $D"; echo; cat "$D/brief.md")"
attempt=1
while [ $attempt -le "${MAXRETRY:-4}" ]; do
  echo "$(date -Is) attempt $attempt start ($MODEL $EFFORT)" >> "$LOG"
  if [ ! -s "$SID" ]; then
    codex exec -m "$MODEL" -c "model_reasoning_effort=\"$EFFORT\"" -s workspace-write --skip-git-repo-check \
      -o "$D/last.md" "$PROMPT" >> "$OUT" 2>&1 < /dev/null; rc=$?
    grep -oE 'session id: [0-9a-f-]{36}' "$OUT" | tail -1 | awk '{print $3}' > "$SID" 2>/dev/null || true
  else
    codex exec resume -c "model_reasoning_effort=\"$EFFORT\"" -c 'sandbox_mode="workspace-write"' --skip-git-repo-check \
      -o "$D/last.md" "$(cat "$SID")" "You were interrupted. Keep what is written, read $D/brief.md and your files again, continue from the first unfinished item, and finish with $D/report.md." >> "$OUT" 2>&1 < /dev/null; rc=$?
  fi
  echo "$(date -Is) attempt $attempt exit $rc" >> "$LOG"
  if [ $rc -eq 0 ] && [ -s "$D/report.md" ]; then echo "$(date -Is) DONE" >> "$LOG"; exit 0; fi
  [ -e lanes/STOP ] && { echo "$(date -Is) STOP" >> "$LOG"; exit 2; }
  attempt=$((attempt+1)); sleep 60
done
echo "$(date -Is) FAILED" >> "$LOG"; exit 1
