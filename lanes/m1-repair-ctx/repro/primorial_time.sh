#!/bin/bash
# primorial_time.sh: runs primorial_time on hostile arguments, each under a 60 s timeout and a
# 4 GB address-space limit; prints the status line or how the process ended (exit 124 = timeout,
# 134 = SIGABRT). Expected answer for each e >= 2 row with n > 2^(64/e): ADF_UNSUPPORTED.
cd "$(dirname "$0")"
export LC_ALL=C
ulimit -v 4000000
tmp=$(mktemp)
for a in "100000 4" "10000000 3" "100000000 3" "8589934592 2" "1099511627776 2" \
         "18446744073709551615 2" "18446744073709551615 3" "18446744073709551615 1"; do
    start=$(date +%s%N)
    timeout 60 ./primorial_time $a > "$tmp" 2>&1
    rc=$?
    end=$(date +%s%N)
    printf 'args %-24s exit=%-4s wall=%6.1fs  %s\n' "$a" "$rc" "$(( (end - start) / 1000000 ))e-3" \
        "$(tail -2 "$tmp" | tr '\n' ' ')"
done
rm -f "$tmp"
