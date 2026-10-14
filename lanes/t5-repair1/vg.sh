#!/bin/sh
# valgrind runs of lane t5-repair1 (brief item 4), from the worktree root, two at a time:
#   sh lanes/t5-repair1/vg.sh
# Each: timeout 900 valgrind --error-exitcode=9 --track-origins=yes --leak-check=full
#   --errors-for-leak-kinds=definite,indirect on the -O1 -g build lanes/t5-repair1/build-vg.
V=lanes/t5-repair1/build-vg
L=lanes/t5-repair1
run() {
    name=$1; shift
    start=$(date +%s)
    timeout 900 valgrind --error-exitcode=9 --track-origins=yes --leak-check=full \
        --errors-for-leak-kinds=definite,indirect --log-file=$L/vg-$name.log "$@" > $L/vg-$name.out 2>&1
    echo "$name exit $? in $(( $(date +%s) - start )) s" >> $L/vg-summary.txt
}
: > $L/vg-summary.txt
run values $V/test_tate_continue values &
run gamma $V/test_tate_gamma &
wait
run widths $V/test_tate_continue widths &
run statuses $V/test_tate_continue statuses &
wait
run jbound $V/test_tate_continue jbound
