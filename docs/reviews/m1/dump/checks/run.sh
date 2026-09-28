#!/bin/sh
# Run from the repository root. Every generated file stays in the review directory.
set -eu
P=docs/reviews/m1/dump/checks
make -j2 BUILD="$P/build" CFLAGS='-std=c11 -O2 -g -fPIC -Wall -Wextra -Wpedantic -Werror' \
    "$P/build/test_dump" "$P/build/test_dump_ctx" "$P/build/test_dump_golden" > "$P/build.log" 2>&1
cc -std=c11 -O2 -g -Wall -Wextra -Werror -fPIC -Iinclude "$P/bridge.c" \
    "$P/build/libadelefeld.a" -lflint -lgmp -lm -shared -o "$P/bridge.so"
cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude "$P/bridge.c" \
    "$P/build/libadelefeld.a" -lflint -lgmp -lm -o "$P/bridge"
cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude "$P/roundtrip.c" \
    "$P/build/libadelefeld.a" -lflint -lgmp -lm -o "$P/roundtrip"
for t in test_dump test_dump_ctx test_dump_golden; do "$P/build/$t"; done > "$P/baseline.log" 2>&1
"$P/bridge" > "$P/memory.log" 2>&1
"$P/roundtrip" > "$P/roundtrip.log" 2>&1
python3 -B "$P/status_findings.py" > "$P/status_findings.log" 2>&1
# Keep the nonzero result: the reference and C disagree. Do not suppress the assertion in Python.
if python3 -B "$P/differential.py" > "$P/differential.log" 2>&1; then
    echo 'differential_exit=0'
else
    echo "differential_exit=$? (inspect differential.log)"
fi
python3 -B "$P/cost.py" > "$P/cost.log" 2>&1
valgrind --leak-check=full --errors-for-leak-kinds=definite,indirect --error-exitcode=99 \
    "$P/bridge" > "$P/valgrind-memory.log" 2>&1
valgrind --leak-check=full --errors-for-leak-kinds=definite,indirect --error-exitcode=99 \
    "$P/roundtrip" > "$P/valgrind-roundtrip.log" 2>&1
make -j2 BUILD="$P/san" SAN=1 "$P/san/test_dump" "$P/san/test_dump_ctx" \
    "$P/san/test_dump_golden" > "$P/san-build.log" 2>&1
cc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -fno-omit-frame-pointer -Iinclude "$P/bridge.c" "$P/san/libadelefeld.a" \
    -lflint -lgmp -lm -o "$P/bridge-san"
cc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -fno-omit-frame-pointer -Iinclude "$P/roundtrip.c" "$P/san/libadelefeld.a" \
    -lflint -lgmp -lm -o "$P/roundtrip-san"
for t in test_dump test_dump_ctx test_dump_golden; do "$P/san/$t"; done > "$P/san.log" 2>&1
"$P/bridge-san" > "$P/san-memory.log" 2>&1
"$P/roundtrip-san" > "$P/san-roundtrip.log" 2>&1
