#!/bin/sh
# The mutation tool runs this in its copy. Two compiler jobs, individually bounded programs.
set -eu
export ASAN_OPTIONS=detect_leaks=0
b=lanes/c-slice2/mut-base
timeout 90 make -s -j2 INV=1 SAN=1 BUILD="$b" "$b/test_char" "$b/test_char_eval"
timeout 30 cc -Iinclude -Itests -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror \
    -fsanitize=address,undefined -fno-omit-frame-pointer -DADF_CHECK_INVARIANTS -DADF_CHAR_WRAP_SETUP \
    tests/test_char.c "$b"/support/*.o "$b/libadelefeld.a" \
    -Wl,--wrap=dirichlet_group_init -Wl,--wrap=adf_phase_get_acb -Wl,--wrap=arb_sqrt_ui \
    -lflint -lgmp -lm -pthread -o "$b/test_char_wrap"
timeout 30 cc -Iinclude -Itests -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror \
    -fsanitize=address,undefined -fno-omit-frame-pointer -DADF_CHECK_INVARIANTS -DADF_CHAR_EVAL_WRAP \
    tests/test_char_eval.c "$b"/support/*.o "$b/libadelefeld.a" \
    -Wl,--wrap=dirichlet_group_init -Wl,--wrap=adf_phase_get_acb -Wl,--wrap=arb_sqrt_ui \
    -lflint -lgmp -lm -pthread -o "$b/test_char_eval_wrap"
timeout 30 "$b/test_char_wrap"
timeout 30 "$b/test_char_eval_wrap"
