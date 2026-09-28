#!/bin/sh
# lanes/m1-headers/check_headers.sh: the checks of the public headers that `make check` cannot do
# from a single C translation unit (lane m1-headers). Run from the repository root:
#
#     sh lanes/m1-headers/check_headers.sh
#
# 1. Each header alone, and the umbrella header, compiles as C11 with gcc and clang
#    (-Wall -Wextra -Wpedantic -pedantic-errors -Werror), and as C++17 with g++ and clang++ if
#    present (extern "C" guards; no compiler extension).
# 2. With ADF_INLINES_C defined, the header-inline functions compile into external definitions:
#    nm lists every one of them as a defined text symbol (conventions 12.1).
# 3. gcc -aux-info lists every declared function: none is variadic (conventions 12.2), every name
#    starts with adf_ (conventions 2.1), and the count per header is printed.
# 4. tests/test_headers.c and tests/test_abi.c build and pass with clang as well.
# Exit status 0 when every check passes. Compiles only; at most one compiler process at a time.

set -u
root=$(pwd)
scratch=${TMPDIR:-/tmp}/adf-m1-headers-check.$$
mkdir -p "$scratch"
fail=0
n=0
nfail=0
warn="-Wall -Wextra -Wpedantic -pedantic-errors -Werror"

have() { command -v "$1" > /dev/null 2>&1; }

# A C++ compiler is used only if it finds its own standard library (gmp.h includes <iosfwd> in C++).
cxx_ok() {
    have "$1" || return 1
    printf '#include <iosfwd>\nint main() { return 0; }\n' > "$scratch/probe.cpp"
    "$1" -std=c++17 -fsyntax-only "$scratch/probe.cpp" > /dev/null 2>&1
}
cxx_list=""
for c in g++ clang++; do
    if cxx_ok "$c"; then cxx_list="$cxx_list $c"; else echo "note: $c skipped (absent, or it finds no C++ standard library)"; fi
done

headers="adelefeld.h"
for h in "$root"/include/adelefeld/*.h; do headers="$headers adelefeld/$(basename "$h")"; done

for h in $headers; do
    printf '#include <%s>\nint main(void) { return 0; }\n' "$h" > "$scratch/one.c"
    cp "$scratch/one.c" "$scratch/one.cpp"
    for c in "gcc -std=c11" "clang -std=c11"; do
        set -- $c
        have "$1" || continue
        n=$((n + 1))
        if ! $c $warn -I"$root/include" -fsyntax-only "$scratch/one.c" 2> "$scratch/err"; then
            echo "FAIL $c: $h"; sed -n '1,5p' "$scratch/err"; fail=1; nfail=$((nfail + 1))
        fi
    done
    for c in $cxx_list; do
        n=$((n + 1))
        if ! $c -std=c++17 $warn -I"$root/include" -fsyntax-only "$scratch/one.cpp" 2> "$scratch/err"; then
            echo "FAIL $c -std=c++17: $h"; sed -n '1,5p' "$scratch/err"; fail=1; nfail=$((nfail + 1))
        fi
    done
done
echo "1. single-header compilations (C11:$(have gcc && echo ' gcc')$(have clang && echo ' clang'); C++17:$cxx_list): $n, failures: $nfail"

# 2. ADF_INLINES_C gives exported definitions of the inline functions.
printf '#define ADF_INLINES_C\n#include <adelefeld.h>\n' > "$scratch/inl.c"
inline_names=$(grep -h -A1 '^ADF_INLINE' "$root"/include/adelefeld/*.h \
    | grep -o 'adf_[a-z_0-9]*(' | tr -d '(' | sort -u)
if gcc -std=c11 $warn -Wno-missing-prototypes -I"$root/include" -c "$scratch/inl.c" -o "$scratch/inl.o"; then
    defined=$(nm "$scratch/inl.o" | awk '$2 == "T" { print $3 }' | sort -u)
    missing=0; count=0
    for f in $inline_names; do
        count=$((count + 1))
        echo "$defined" | grep -qx "$f" || { echo "FAIL not exported with ADF_INLINES_C: $f"; missing=1; fail=1; }
    done
    echo "2. header-inline functions: $count, exported as T with ADF_INLINES_C: $([ $missing -eq 0 ] && echo all || echo not all)"
else
    echo "FAIL: ADF_INLINES_C translation unit does not compile"; fail=1
fi

# 3. Declarations: count per header, no variadic function, names adf_*.
printf '#include <adelefeld.h>\n' > "$scratch/all.c"
gcc -std=c11 -I"$root/include" -fsyntax-only -aux-info "$scratch/aux.txt" "$scratch/all.c"
grep 'include/adelefeld' "$scratch/aux.txt" > "$scratch/decl.txt"
echo "3. declared functions per header:"
sed -E 's|^/\* ([^:]*):.*|\1|' "$scratch/decl.txt" | sed "s|^$root/||" | sort | uniq -c
echo "   total: $(wc -l < "$scratch/decl.txt")"
if grep -q '\.\.\.' "$scratch/decl.txt"; then echo "FAIL variadic declaration"; fail=1; fi
bad=$(sed -E 's|^/\*[^*]*\*/ ||; s|;.*||' "$scratch/decl.txt" \
    | awk '{ if (match($0, /[A-Za-z_][A-Za-z_0-9]* \(/)) print substr($0, RSTART, RLENGTH - 2) }' \
    | grep -v '^adf_' || true)
if [ -n "$bad" ]; then echo "FAIL names without adf_: $bad"; fail=1; else echo "   variadic: 0; names without adf_: 0"; fi

# 4. The two tests with clang.
if have clang; then
    for t in test_headers test_abi; do
        if clang -std=c11 -Wall -Wextra -Wpedantic -Werror -I"$root/include" -I"$root/tests" \
            "$root/tests/$t.c" -o "$scratch/$t" -lflint -lgmp 2> "$scratch/err" \
            && "$scratch/$t" > "$scratch/out"; then
            echo "4. clang $t: $(tail -1 "$scratch/out")"
        else
            echo "FAIL clang $t"; sed -n '1,5p' "$scratch/err"; fail=1
        fi
    done
fi

rm -rf "$scratch"
if [ $fail -eq 0 ]; then echo "check_headers: passed"; else echo "check_headers: FAILED"; fi
exit $fail
