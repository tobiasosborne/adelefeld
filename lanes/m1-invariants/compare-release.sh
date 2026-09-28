#!/bin/sh
# lanes/m1-invariants/compare-release.sh: step 6 of the brief. Compares the disassembly (objdump -d,
# without -g) of every object of the release build (no -DADF_CHECK_INVARIANTS) with the objects of
# the same commit before the lane's changes, kept in build-base/ (built from the unchanged src/ with
# the same command: make -j2 all).
#
#   usage, from the repository root, after `make clean && make -j2 all`:
#       sh lanes/m1-invariants/compare-release.sh build-base build
#
# The first three lines of objdump's output (the file name and the format) are cut, since they name
# the directory. Prints one line per object and the number of instructions lines compared.
set -u
base=${1:-build-base}
new=${2:-build}
tmp=${TMPDIR:-/tmp}/adf-compare-$$
mkdir -p "$tmp"
bad=0
for o in "$base"/*.o; do
    n=$(basename "$o")
    objdump -d "$o" | tail -n +4 > "$tmp/old.txt"
    objdump -d "$new/$n" | tail -n +4 > "$tmp/new.txt"
    lines=$(wc -l < "$tmp/new.txt" | tr -d ' ')
    if cmp -s "$tmp/old.txt" "$tmp/new.txt"; then
        echo "$n identical ($lines lines of disassembly)"
    else
        echo "$n DIFFERENT"
        diff "$tmp/old.txt" "$tmp/new.txt" | head -20
        bad=1
    fi
done
rm -rf "$tmp"
exit $bad
