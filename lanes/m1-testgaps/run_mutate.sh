#!/bin/sh
# lanes/m1-testgaps/run_mutate.sh: the mutation runs of the brief, one after the other, never two
# at a time. Every run writes its whole output to lanes/m1-testgaps/mutate-<label>.log.
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"
for f in rat cap recon adele fball; do
    echo "=== make mutate FILES=src/$f.c LIMIT=400 JOBS=2   $(date -u +%H:%M:%S)" \
        >> lanes/m1-testgaps/mutate-runs.txt
    make mutate FILES="src/$f.c" LIMIT=400 JOBS=2 > "lanes/m1-testgaps/mutate-$f.log" 2>&1
    echo "exit $?   $(date -u +%H:%M:%S)" >> lanes/m1-testgaps/mutate-runs.txt
    grep "^mutate: [0-9]* mutants in" "lanes/m1-testgaps/mutate-$f.log" \
        >> lanes/m1-testgaps/mutate-runs.txt
done
for k in 1 2 3; do
    echo "=== make mutate FILES=src/recon.c LIMIT=400 JOBS=2, repeat $k   $(date -u +%H:%M:%S)" \
        >> lanes/m1-testgaps/mutate-runs.txt
    make mutate FILES=src/recon.c LIMIT=400 JOBS=2 > "lanes/m1-testgaps/mutate-recon-$k.log" 2>&1
    echo "exit $?   $(date -u +%H:%M:%S)" >> lanes/m1-testgaps/mutate-runs.txt
    grep "^mutate: [0-9]* mutants in" "lanes/m1-testgaps/mutate-recon-$k.log" \
        >> lanes/m1-testgaps/mutate-runs.txt
done
echo "=== all runs done   $(date -u +%H:%M:%S)" >> lanes/m1-testgaps/mutate-runs.txt
