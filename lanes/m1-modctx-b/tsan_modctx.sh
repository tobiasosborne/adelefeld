#!/bin/sh
# lanes/m1-modctx-b/tsan_modctx.sh: build tests/test_modctx.c with the thread sanitizer and
# run it.  The Makefile has no thread-sanitizer target, so this script does it: one context,
# two pthreads reading it at the same time (docs/conventions.md 4.5).
#
# Run from anywhere; the script changes to the repository root itself.
#
# On a kernel with too many randomized address bits, ThreadSanitizer aborts with
# "unexpected memory mapping"; `setarch -R` disables ASLR for the run.  The script uses it
# when it is present and reports the TSan output either way.
set -e
cd "$(dirname "$0")/../.."
mkdir -p build/tsan
cc -std=c11 -O1 -g -fno-omit-frame-pointer -fsanitize=thread -Iinclude -Itests \
    tests/test_modctx.c tests/support/jsonl.c tests/support/golden.c src/modctx.c \
    -lflint -lgmp -lm -o build/tsan/test_modctx
if command -v setarch > /dev/null 2>&1; then
    exec setarch "$(uname -m)" -R env TSAN_OPTIONS="halt_on_error=0" ./build/tsan/test_modctx
else
    exec env TSAN_OPTIONS="halt_on_error=0" ./build/tsan/test_modctx
fi