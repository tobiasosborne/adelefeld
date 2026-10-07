#!/bin/sh
# Only the three character tests. One mutant at a time, two compiler jobs.
set -eu
export ASAN_OPTIONS=detect_leaks=0
b=lanes/c-slice3/mut-base
timeout 90 make -s -j2 INV=1 SAN=1 BUILD="$b" "$b/test_char" "$b/test_char_eval" "$b/test_char_class"
flags='-Iinclude -Itests -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror'
flags="$flags -fsanitize=address,undefined -fno-omit-frame-pointer -DADF_CHECK_INVARIANTS"
timeout 30 cc $flags -DADF_CHAR_WRAP_SETUP tests/test_char.c "$b"/support/*.o "$b/libadelefeld.a" \
    -Wl,--wrap=dirichlet_group_init -Wl,--wrap=adf_phase_get_acb -Wl,--wrap=arb_sqrt_ui \
    -lflint -lgmp -lm -pthread -o "$b/test_char_wrap"
timeout 30 cc $flags -DADF_CHAR_EVAL_WRAP tests/test_char_eval.c "$b"/support/*.o "$b/libadelefeld.a" \
    -Wl,--wrap=dirichlet_group_init -Wl,--wrap=adf_phase_get_acb -Wl,--wrap=arb_sqrt_ui \
    -lflint -lgmp -lm -pthread -o "$b/test_char_eval_wrap"
timeout 30 cc $flags -DADF_CHAR_CLASS_WRAP tests/test_char_class.c "$b"/support/*.o "$b/libadelefeld.a" \
    -Wl,--wrap=dirichlet_group_init -Wl,--wrap=adf_phase_get_acb -Wl,--wrap=arb_pow \
    -Wl,--wrap=acb_pow -Wl,--wrap=acb_mul -Wl,--wrap=adf_idclass_set_idele \
    -lflint -lgmp -lm -pthread -o "$b/test_char_class_wrap"
timeout 30 "$b/test_char_wrap"
timeout 30 "$b/test_char_eval_wrap"
timeout 30 "$b/test_char_class_wrap"
