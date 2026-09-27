#!/usr/bin/env bash
# Run one pi lane. Usage: tools/orch/pi_lane.sh <lane> <provider/model> [thinking]
# JSON event log is always written (pi has been seen to print nothing without logging); success is judged by
# lanes/<lane>/report.md, never by stdout.
set -u
LANE="$1"; MODEL="$2"; THINK="${3:-high}"
REPO="$(cd "$(dirname "$0")/../.." && pwd)"; cd "$REPO"
D="lanes/$LANE"; LOG="$D/lane.log"; mkdir -p "$D/sessions"
PROMPT="$(cat lanes/COMMON.md; echo; echo "Your lane directory: $D"; echo; cat "$D/brief.md")"
attempt=1
while [ $attempt -le "${MAXRETRY:-3}" ]; do
  [ -e lanes/STOP ] && { echo "$(date -Is) STOP" >> "$LOG"; exit 2; }
  echo "$(date -Is) attempt $attempt start ($MODEL $THINK)" >> "$LOG"
  if [ $attempt -eq 1 ]; then MSG="$PROMPT"; C=""; else MSG="You stopped before writing $D/report.md. Keep what is written, read $D/brief.md again, continue from the first unfinished item, and finish with $D/report.md."; C="-c"; fi
  timeout "${LANE_TIMEOUT:-5400}" pi -p $C --mode json --session-dir "$D/sessions" -nc --model "$MODEL" --thinking "$THINK" "$MSG" \
    >> "$D/events.jsonl" 2>> "$D/stderr.log" < /dev/null; rc=$?
  echo "$(date -Is) attempt $attempt exit $rc" >> "$LOG"
  if [ -s "$D/report.md" ]; then echo "$(date -Is) DONE" >> "$LOG"; exit 0; fi
  attempt=$((attempt+1)); sleep 30
done
echo "$(date -Is) FAILED" >> "$LOG"; exit 1
