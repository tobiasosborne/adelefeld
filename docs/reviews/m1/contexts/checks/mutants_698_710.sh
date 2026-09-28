#!/bin/bash
# mutants_698_710.sh: the two `logic` mutants of src/modctx.c that lane m1-modctx-b excused
# (report finding 6): `||` -> `&&` at line 698 (after the version) and line 710 (after the
# field). Each mutant is compiled with tests/test_modctx.c at -O0 and at -O2, without sanitizer,
# and the test (its page-end test dump_input_without_terminator puts "adf1" and "adf1 Q" at the
# end of a mapping) is run. A killed mutant shows a non-zero exit.
# Run from the worktree root. Uses a scratch copy; src/ is not changed.
set -u
root=$(pwd)
tmp=$(mktemp -d)
cp -r src include tests "$tmp"/
for line in 698 710; do
    cp "$root/src/modctx.c" "$tmp/src/modctx.c"
    sed -i "${line}s/pos >= len || s\[pos\] != ' '/pos >= len \&\& s[pos] != ' '/" "$tmp/src/modctx.c"
    printf 'line %s mutated to: %s\n' "$line" "$(sed -n "${line}p" "$tmp/src/modctx.c" | sed 's/^ *//')"
    for opt in -O0 -O2; do
        cc -std=c11 $opt -g -I"$tmp/include" -I"$tmp/tests" "$tmp"/src/*.c "$tmp/tests/test_modctx.c" \
           "$tmp"/tests/support/*.c -lflint -lgmp -lm -lpthread -o "$tmp/t" 2>/dev/null || { echo "  build failed"; continue; }
        (cd "$root" && "$tmp/t" > "$tmp/out" 2>&1)
        rc=$?
        printf '  %s: exit %s; %s\n' "$opt" "$rc" "$(grep -E 'FAIL|passed|failed|Segmentation' "$tmp/out" | tail -1)"
    done
done
rm -rf "$tmp"
