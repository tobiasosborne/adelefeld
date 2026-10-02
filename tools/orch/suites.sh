#!/usr/bin/env bash
# Run the acceptance suites of one tree in separate build directories, in parallel, and print one line each.
#
#   tools/orch/suites.sh [-j N] [-o LOGDIR] [SUITE ...]
#
# SUITE is one or more of: all (make check-all), san (make check SAN=1), clang (make check CC=clang),
# inv (make check INV=1), headers (lanes/m1-headers/check_headers.sh). The default is all four make suites
# and headers. Each make suite builds into build/suite-<name>/ (the Makefile's BUILD variable), so the
# suites do not share objects and can run at the same time; `make clean` is not needed between them.
# -j N is the make parallelism of EACH suite (default 1). Logs go to LOGDIR (default
# build/suite-logs/), one file per suite; the summary is the last line of each log and the exit code.
# The script exits non-zero if any suite failed.
#
# Written on 2026-10-02 after a hand-run of three suites in one shell lost two of them (the detached
# subshells did not survive the end of the orchestrator's command).
set -u
REPO="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$REPO"
J=1
LOGDIR="$REPO/build/suite-logs"
while [ $# -gt 0 ]; do
  case "$1" in
    -j) J="$2"; shift 2 ;;
    -o) LOGDIR="$2"; shift 2 ;;
    -*) echo "unknown option $1" >&2; exit 2 ;;
    *) break ;;
  esac
done
SUITES="$*"
[ -n "$SUITES" ] || SUITES="all san clang inv headers"
mkdir -p "$LOGDIR"

run_suite() { # run_suite NAME -- runs one suite, writes LOGDIR/NAME.log, returns its exit code
  name=$1; log="$LOGDIR/$name.log"
  case "$name" in
    all)     make -j"$J" BUILD=build/suite-all check-all ;;
    san)     make -j"$J" BUILD=build/suite-san check SAN=1 ;;
    clang)   make -j"$J" BUILD=build/suite-clang check CC=clang ;;
    inv)     make -j"$J" BUILD=build/suite-inv check INV=1 ;;
    headers) sh lanes/m1-headers/check_headers.sh ;;
    *) echo "unknown suite $name"; return 2 ;;
  esac > "$log" 2>&1
  rc=$?
  echo "exit $rc" >> "$log"
  return $rc
}

pids=""
for s in $SUITES; do
  run_suite "$s" &
  pids="$pids $!:$s"
done

fail=0
for ps in $pids; do
  pid=${ps%%:*}; s=${ps#*:}
  if wait "$pid"; then rc=0; else rc=$?; fail=1; fi
  last=$(grep -v '^exit ' "$LOGDIR/$s.log" | tail -1 | cut -c1-110)
  printf '%-8s exit %d  %s\n' "$s" "$rc" "$last"
done
echo "logs: $LOGDIR"
exit $fail
