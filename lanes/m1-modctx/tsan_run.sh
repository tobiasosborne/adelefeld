#!/bin/sh
# lanes/m1-modctx/tsan_run.sh: build tests/test_modctx.c under the thread sanitizer and run it.
#
# The Makefile has no thread-sanitizer target (lane brief of m1-modctx), so this script compiles
# the test with -fsanitize=thread directly. It needs no library build: the test links the one
# source file it exercises (src/modctx.c), the two support readers and FLINT.
#
# Run from anywhere:  sh lanes/m1-modctx/tsan_run.sh
# The exit status is that of the test program (a data race also makes TSan exit non-zero).
set -e
cd "$(dirname "$0")/../.."

CC=${CC:-cc}
OUT=build/test_modctx_tsan

mkdir -p build
$CC -Iinclude -Itests -std=c11 -O1 -g -Wall -Wextra -Wpedantic \
    -fsanitize=thread -fno-omit-frame-pointer \
    tests/test_modctx.c tests/support/jsonl.c tests/support/golden.c src/modctx.c \
    -lflint -lgmp -lm -o "$OUT"

TSAN_OPTIONS="halt_on_error=0 second_deadlock_stack=1"
export TSAN_OPTIONS

# TSan needs a stable address space: "FATAL: ThreadSanitizer: unexpected memory mapping" is
# the known conflict with high-entropy ASLR, so the run disables ASLR where setarch allows it.
if command -v setarch >/dev/null 2>&1; then
    setarch "$(uname -m)" -R "./$OUT"
else
    "./$OUT"
fi
