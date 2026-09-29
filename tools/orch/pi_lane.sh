#!/usr/bin/env bash
# Run one pi lane. Usage: tools/orch/pi_lane.sh <lane> <provider/model> [thinking]
# JSON event log is always written (pi has been seen to print nothing without logging); success is judged by
# lanes/<lane>/report.md, never by stdout.
set -u
LANE="$1"; MODEL="$2"; THINK="${3:-high}"
# LANE_ROOT: the tree the lane works in (set by wt_lane.sh, so that a worktree uses the runner of master).
REPO="${LANE_ROOT:-$(cd "$(dirname "$0")/../.." && pwd)}"; cd "$REPO"
D="lanes/$LANE"; LOG="$D/lane.log"; mkdir -p "$D/sessions"
PROMPT="$(cat lanes/COMMON.md; echo; echo "Your lane directory: $D"; echo; cat "$D/brief.md")"
# The provider of a free model often answers a turn with nothing ("Provider returned an empty response");
# pi -p then ends the run with exit 0. Such an end is not an attempt of the model: the session is continued
# after a short wait, up to SOFTMAX times in all, and does not count against MAXRETRY.
empty_end() { tail -n 40 "$D/events.jsonl" 2>/dev/null | grep '"type":"turn_end"' | tail -n 1 | grep -q 'empty response'; }
attempt=1; soft=0; started="${RESUME:-0}"   # RESUME=1: continue the last session of the lane
while [ $attempt -le "${MAXRETRY:-3}" ]; do
  [ -e lanes/STOP ] && { echo "$(date -Is) STOP" >> "$LOG"; exit 2; }
  echo "$(date -Is) attempt $attempt start ($MODEL $THINK) soft $soft" >> "$LOG"
  if [ $started -eq 0 ]; then MSG="$PROMPT"; C=""; else MSG="You stopped before writing $D/report.md. Keep what is written, read $D/brief.md again, continue from the first unfinished item, and finish with $D/report.md."; C="-c"; fi
  started=1
  # Watchdog: pi has no time limit for a shell command of the model. If the event log has not grown for
  # STALL seconds, the test programs and scripts running inside this tree are killed, so that the model
  # gets its answer and goes on.
  ( while sleep 60; do
      age=$(( $(date +%s) - $(stat -c %Y "$D/events.jsonl" 2>/dev/null || date +%s) ))
      if [ "$age" -gt "${STALL:-600}" ]; then
        for pid in $(pgrep -f "^(\./|$REPO/)?build/|^python3 " 2>/dev/null); do
          if [ "$(readlink /proc/$pid/cwd 2>/dev/null)" = "$REPO" ]; then
            echo "$(date -Is) watchdog: no event for $age s, kill $pid ($(tr '\0' ' ' < /proc/$pid/cmdline | cut -c1-80))" >> "$LOG"
            kill "$pid" 2>/dev/null
          fi
        done
      fi
    done ) & wd=$!
  timeout "${LANE_TIMEOUT:-5400}" pi -p $C --mode json --session-dir "$D/sessions" -nc --model "$MODEL" --thinking "$THINK" "$MSG" \
    >> "$D/events.jsonl" 2>> "$D/stderr.log" < /dev/null; rc=$?
  kill "$wd" 2>/dev/null
  echo "$(date -Is) attempt $attempt exit $rc" >> "$LOG"
  if [ -s "$D/report.md" ]; then echo "$(date -Is) DONE" >> "$LOG"; exit 0; fi
  if empty_end && [ $soft -lt "${SOFTMAX:-40}" ]; then
    soft=$((soft+1)); echo "$(date -Is) empty response of the provider ($soft), continuing" >> "$LOG"; sleep 20; continue
  fi
  attempt=$((attempt+1)); sleep 30
done
echo "$(date -Is) FAILED" >> "$LOG"; exit 1
