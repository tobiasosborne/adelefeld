#!/bin/sh
# Run from the repository root. All generated files remain in this review directory.
# Each subprocess uses no more than two build jobs. Checks run sequentially.
set -eu
D=docs/reviews/m1/local/checks
export PYTHONDONTWRITEBYTECODE=1
make -j2 BUILD="$D/build" all > "$D/build.log" 2>&1
cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude "$D/probe.c" \
    "$D/build/libadelefeld.a" -lflint -lgmp -lm -o "$D/probe"
cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude "$D/cap_local.c" \
    "$D/build/libadelefeld.a" -lflint -lgmp -lm -o "$D/cap_local"
"$D/cap_local" > "$D/cap_local.log"
cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude "$D/invalid_fixture.c" \
    "$D/build/libadelefeld.a" -lflint -lgmp -lm -o "$D/invalid_fixture"
"$D/invalid_fixture" > "$D/invalid_fixture.log"
python3 "$D/oracle.py" > "$D/oracle.log"
python3 "$D/oracle.py" --reference > "$D/reference.log"
# LeakSanitizer cannot run in the traced sandbox. Valgrind checks leaks below.
export ASAN_OPTIONS=detect_leaks=0
make -j2 BUILD="$D/san" SAN=1 check > "$D/san-noleak.log" 2>&1
cc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -fno-omit-frame-pointer -Iinclude "$D/probe.c" "$D/san/libadelefeld.a" \
    -lflint -lgmp -lm -o "$D/probe-san"
python3 "$D/oracle.py" --probe "$D/probe-san" > "$D/oracle-san.log"
set +e
"$D/probe-san" bad-context > "$D/bad-context.log" 2>&1
BAD_CONTEXT_STATUS=$?
set -e
printf 'bad-context exit=%s\n' "$BAD_CONTEXT_STATUS"
test "$BAD_CONTEXT_STATUS" -eq 1
python3 "$D/oracle.py" --mode small --probe "$D/memory.sh" > "$D/memory.log"
make -j2 BUILD="$D/invariants" CPPFLAGS='-Iinclude -DADF_CHECK_INVARIANTS' all \
    > "$D/invariants-build.log" 2>&1
cc -std=c11 -O2 -g -Wall -Wextra -Werror -DADF_CHECK_INVARIANTS -Iinclude \
    "$D/invariant_check.c" "$D/invariants/libadelefeld.a" -lflint -lgmp -lm -o "$D/invariant_check"
"$D/invariant_check" > "$D/invariant_check.log"
python3 "$D/mutant.py" > "$D/mutant.log" 2>&1
printf 'All review commands completed; findings remain in the recorded outputs.\n'
