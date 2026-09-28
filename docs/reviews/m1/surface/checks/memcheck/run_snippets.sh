#!/bin/sh
# run_snippets.sh: for each snippet s*.c, (1) what tools/memcheck/check_uninit.py reports,
# (2) whether valgrind finds a memory error when the snippet runs (the real defect).
# Run from the repository root:  sh docs/reviews/m1/surface/checks/memcheck/run_snippets.sh
D=docs/reviews/m1/surface/checks/memcheck
mkdir -p build/review_memcheck
for f in "$D"/s*.c; do
    b=build/review_memcheck/$(basename "$f" .c)
    cc -std=c11 -O0 -g -Iinclude "$f" build/libadelefeld.a -lflint -lgmp -lm -o "$b" 2>/dev/null \
        || { echo "$f: does not build"; continue; }
    findings=$(python3 tools/memcheck/check_uninit.py --category use-before-init "$f" | wc -l)
    cbi=$(python3 tools/memcheck/check_uninit.py --category clear-before-init "$f" | wc -l)
    valgrind -q --error-exitcode=77 "$b" > /dev/null 2> "$b.vg"
    rc=$?
    errs=$(grep -c '^==[0-9]*== [A-Z]' "$b.vg")
    first=$(grep -m1 '^==[0-9]*== [A-Z]' "$b.vg" | sed 's/^==[0-9]*== //')
    echo "$(basename "$f"): checker use-before-init $findings, clear-before-init $cbi;" \
         "valgrind exit $rc, $errs error line(s): $first"
done
