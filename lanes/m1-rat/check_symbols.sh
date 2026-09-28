#!/bin/sh
# lanes/m1-rat/check_symbols.sh: every function of status.h, place.h and rat.h is a defined
# symbol of the library (conventions 12.1, DECISION CV-40, the brief of lane m1-rat).
#
#     sh lanes/m1-rat/check_symbols.sh
#
# Run from the repository root. The script
#   1. reads the declarations of include/adelefeld/status.h, place.h and rat.h with
#      `gcc -aux-info` (so the list is the header's, not a copy of it),
#   2. builds build/libadelefeld.a and, in build/m1-rat/, a shared object from the same sources,
#   3. lists the defined symbols of the archive with `nm` and of the shared object with
#      `nm -D --defined-only`, and compares both with the declarations.
# A function that is missing from either is a failure. At most one compiler process at a time.
# Exit status 0 when every check passes.

set -u
root=$(pwd)
scratch=build/m1-rat
headers="status.h place.h rat.h"
fail=0

have() { command -v "$1" > /dev/null 2>&1; }
if ! have gcc || ! have nm; then echo "check_symbols: gcc and nm are needed"; exit 1; fi

mkdir -p "$scratch"
if ! make -s all > "$scratch/make.log" 2>&1; then
    echo "check_symbols: the library does not build"; sed -n '1,10p' "$scratch/make.log"; exit 1
fi

# 1. the declarations of the three headers, from the headers themselves.
: > "$scratch/names.txt"
total=0
for h in $headers; do
    printf '#include <adelefeld/%s>\n' "$h" > "$scratch/one.c"
    gcc -std=c11 -Iinclude -fsyntax-only -aux-info "$scratch/aux.txt" "$scratch/one.c" || fail=1
    names=$(grep "adelefeld/$h:" "$scratch/aux.txt" \
        | sed -E 's|/\*[^*]*\*/ ||; s|;.*||' \
        | awk '{ if (match($0, /[A-Za-z_][A-Za-z_0-9]* \(/)) print substr($0, RSTART, RLENGTH - 2) }' \
        | sort -u)
    for f in $names; do echo "$f" >> "$scratch/names.txt"; done
    k=$(echo "$names" | grep -c '[^[:space:]]')
    echo "1. $h: $k declarations"
    total=$((total + k))
done
sort -u "$scratch/names.txt" -o "$scratch/names.txt"
n=$(wc -l < "$scratch/names.txt")
echo "   the three headers declare $n functions in all"
if [ "$n" -ne "$total" ]; then
    echo "FAIL: a name is declared in two headers: $n distinct, $total counted"; fail=1
fi
if [ "$n" -lt 30 ]; then echo "FAIL: the list of declarations is too short: $n"; fail=1; fi
sed 's/^/   /' "$scratch/names.txt"

# 2. the shared object, from the same sources.
: > "$scratch/objs.txt"
for s in src/*.c; do
    o="$scratch/$(basename "$s" .c).pic.o"
    gcc -std=c11 -O2 -fPIC -Iinclude -c "$s" -o "$o" || fail=1
    echo "$o" >> "$scratch/objs.txt"
done
gcc -shared -o "$scratch/libadelefeld.so" $(cat "$scratch/objs.txt") -lflint -lgmp -lm || fail=1

# 3. the comparison, first on the archive, then on the shared object.
for kind in archive shared; do
    if [ "$kind" = archive ]; then
        file=build/libadelefeld.a
        defined=$(nm --defined-only "$file" | awk '$2 ~ /^[TDBR]$/ { print $3 }' | sort -u)
        how="nm --defined-only $file"
    else
        file=$scratch/libadelefeld.so
        defined=$(nm -D --defined-only "$file" | awk '$2 ~ /^[TDBR]$/ { print $3 }' | sort -u)
        how="nm -D --defined-only $file"
    fi
    missing=0
    while read -r f; do
        echo "$defined" | grep -qx "$f" || { echo "FAIL not defined in the $kind: $f"; missing=1; fail=1; }
    done < "$scratch/names.txt"
    echo "2. $kind ($how): $([ $missing -eq 0 ] && echo "all $n functions defined" \
        || echo "not all")"
    # The three types of this lane also answer the layout queries, which are header-inline.
    for f in adf_sizeof_place adf_alignof_place adf_sizeof_rat adf_alignof_rat adf_status_str; do
        echo "$defined" | grep -qx "$f" || { echo "FAIL not defined in the $kind: $f"; fail=1; }
    done
done

# 4. no operation of these three headers may exist only in the header: every declaration above is
#    in the table above, which is what step 3 compared. The count of exported adf_ symbols is
#    printed, so that a reader can see what the lane's inlines.c adds.
echo "3. exported adf_ symbols of the shared object: $(nm -D --defined-only "$scratch/libadelefeld.so" \
    | awk '$2 ~ /^[TDBR]$/ && $3 ~ /^adf_/ { print $3 }' | sort -u | wc -l)"

if [ $fail -eq 0 ]; then echo "check_symbols: passed"; else echo "check_symbols: FAILED"; fi
exit $fail
