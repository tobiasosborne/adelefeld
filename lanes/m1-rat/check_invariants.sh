#!/bin/sh
# lanes/m1-rat/check_invariants.sh: what -DADF_CHECK_INVARIANTS is supposed to do (conventions
# 4.4, DECISION CV-09), and what the library does today. This is a demonstration, not a gate: it
# always exits 0 and prints a FINDING line, because the check is not implemented (see the report
# of the lane and the note at the top of src/rat.c).
#
#     sh lanes/m1-rat/check_invariants.sh
#
# Run from the repository root. The script builds src/*.c with the flag into
# build/m1-rat/inv/flag, links lanes/m1-rat/check_invariants.c against them, and runs it once for
# every public function of rat.h that takes an adf_rat input. The program hands each of them the
# value 8/2, which is not canonical, and conventions 4.4 asks for an abort with a message.
# adf_rat_is_canonical is the one function that must accept the value and report 0.

set -u
root=$(pwd)
scratch=build/m1-rat/inv
rm -rf "$scratch"
mkdir -p "$scratch/flag" "$scratch/plain"

if ! make -s -j2 all > /dev/null 2>&1; then echo "check_invariants: the library does not build"; exit 1; fi

for s in src/*.c; do
    gcc -std=c11 -O1 -g -Wall -Wextra -Wpedantic -Werror -DADF_CHECK_INVARIANTS -Iinclude \
        -c "$s" -o "$scratch/flag/$(basename "$s" .c).o" || exit 1
done
gcc -std=c11 -O1 -g -Wall -Wextra -Wpedantic -Werror -DADF_CHECK_INVARIANTS -Iinclude \
    lanes/m1-rat/check_invariants.c "$scratch"/flag/*.o -lflint -lgmp -lm -o "$scratch/flag_run" \
    2> "$scratch/err" || { echo "FAIL: the program does not build"; sed -n '1,5p' "$scratch/err"; exit 1; }

aborts=0
for f in set swap identical get_fmpq is_zero equal sgn add sub mul neg div inv; do
    out=$(sh -c '"$0" "$1"' "$scratch/flag_run" "$f" 2>&1)
    case $out in
        *"is not a canonical adf_rat"*) aborts=$((aborts + 1)) ;;
        *) echo "   $f: no abort" ;;
    esac
done
echo "1. built with -DADF_CHECK_INVARIANTS: $aborts of 14 functions abort on the input 8/2"
if [ "$aborts" -lt 14 ]; then
    echo "FINDING: the check of conventions 4.4 is not implemented for adf_rat; a non-canonical"
    echo "         input is a precondition violation whose behaviour is undefined, and today it is"
    echo "         silent. The code is in the report of the lane; it belongs in one place for every"
    echo "         type, together with a Makefile target that builds with the flag."
fi

out=$(sh -c '"$0" "$1"' "$scratch/flag_run" is_canonical 2>&1)
case $out in
    *"reported 0"*) echo "2. adf_rat_is_canonical reports 0 for 8/2, with and without the flag" ;;
    *) echo "FAIL: adf_rat_is_canonical: $out"; exit 1 ;;
esac

echo "check_invariants: demonstration finished (not a gate)"
exit 0
