#!/bin/sh
# judge_all.sh: judges every mutant named by specs.txt, five at a time, one after the other.
cd "$(dirname "$0")/../.." || exit 1
: > lanes/m1-repair-tools/judge-all.log
split -l 5 -d lanes/m1-repair-tools/specs.txt /tmp/claude-1000/-home-tobias-Projects-adelefeld/6946c397-2b3c-4594-9551-8de026f8e152/scratchpad/batch.
for b in /tmp/claude-1000/-home-tobias-Projects-adelefeld/6946c397-2b3c-4594-9551-8de026f8e152/scratchpad/batch.*; do
    avail=$(free -g | awk '/^Mem:/ {print $7}')
    while [ "$avail" -lt 6 ]; do sleep 20; avail=$(free -g | awk '/^Mem:/ {print $7}'); done
    python3 lanes/m1-repair-tools/judge.py $(cat "$b"| tr '\n' ' ') >> lanes/m1-repair-tools/judge-all.log 2>&1
done
echo DONE >> lanes/m1-repair-tools/judge-all.log
