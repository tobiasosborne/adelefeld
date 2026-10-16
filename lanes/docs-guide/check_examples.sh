#!/bin/sh
# lanes/docs-guide/check_examples.sh: rerun every example of docs/GUIDE.md and diff it against the guide.
# From the repository root:   sh lanes/docs-guide/check_examples.sh
# Needs build/adf and build/libadelefeld.a (make -j2 && make -C tools/adf) and, for the Julia example,
# build/libadelefeld.so (sh tests/test_exports.sh). Prints one line per example and
# "N sessions, M differ; ..."; exit status 1 if anything differs. The work is in check_examples.py.
set -eu
cd "$(dirname "$0")/../.."
for f in build/adf build/libadelefeld.a build/libadelefeld.so; do
    [ -f "$f" ] || { echo "check_examples: $f is missing; build it first (see the comment above)" >&2; exit 2; }
done
exec timeout 900 python3 lanes/docs-guide/check_examples.py "$@"
