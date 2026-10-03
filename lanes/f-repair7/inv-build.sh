#!/bin/sh
# inv-build.sh: the debug build of the library (-DADF_CHECK_INVARIANTS) for lane f-repair7.
#
# The prescribed command is
#     timeout 600 make -j2 BUILD=lanes/f-repair7/build-inv INV=1 lanes/f-repair7/build-inv/test_gfunc
# It fails on 2026-10-04 in this worktree because src/text.c (owned by another lane, f-slice11, not repaired here)
# does not compile under the flag: two entry checks of the printers use __func__, #x outside a macro
# (src/text.c:2622-2623 and :2942-2943). Every other file of src/ compiles. So this script does the same work by
# hand: each src/*.c with the flag, except text.o, which is taken from the release build of this lane (that object
# has no entry check, which no case of test_gfunc.c uses).
set -e
root=$(cd "$(dirname "$0")/../.." && pwd)
cd "$root"
build=lanes/f-repair7/build-inv
mkdir -p "$build"
cp lanes/f-repair7/build/text.o "$build/text.o"
fail=0
for f in src/*.c; do
    b=$(basename "$f" .c)
    [ "$b" = text ] && continue
    cc -Iinclude -DADF_CHECK_INVARIANTS -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror \
       -c "$f" -o "$build/$b.o" || fail=1
done
[ "$fail" = 0 ] || exit 1
ar rcs "$build/libadelefeld.a" "$build"/*.o
cc -Iinclude -Itests -DADF_CHECK_INVARIANTS -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -pthread \
   tests/test_gfunc.c lanes/f-repair7/build/support/jsonl.o lanes/f-repair7/build/support/golden.o "$build/libadelefeld.a" \
   -lflint -lgmp -lm -pthread -o "$build/test_gfunc"
echo "built $build/test_gfunc"