#!/bin/sh
# tests/test_exports.sh: the exported symbols of the library against the declarations of the
# public headers (docs/conventions.md 12.1, the first rule of section 12; decisions CV-40 and
# CV-43; work package 1.9 of docs/PLAN.md: "nm -D of the library against the declarations of the
# header"). conventions 12.2 (no variadic function) is checked here as well.
#
# What the script does, in this order:
#   1. builds the whole of src/ as a shared object with `cc -shared -fPIC` in the scratch
#      directory build/exports/, and copies it to build/libadelefeld.so, where tests/test_dlopen.c
#      looks for it;
#   2. reads the function names declared in include/adelefeld/*.h out of a translation unit that
#      includes <adelefeld.h>, compiled with `-aux-info` (the same tool that
#      lanes/m1-headers/gen_api_table.py uses for docs/api-m1.md);
#   3. lists the defined dynamic symbols of the shared object with `nm -D --defined-only`;
#   4. prints three lists: declared and exported, declared and not exported, exported and not
#      declared.
#
# What makes the script fail:
#   - a declared function that is variadic (conventions 12.2);
#   - an exported symbol that no header declares: a name that a binding could reach but not call
#     through the interface, and a function that no public operation uses. The list must be empty
#     (conventions 12.1: "Every public operation is an exported function", which says nothing about
#     the other way round, so a symbol that is exported and not declared is a fault). A name that
#     begins with an underscore is judged when it begins with "_adf": conventions 4.1 item 5 gives
#     the underscore functions a place in the interface ("Underscore functions state their aliasing
#     rules"), so "_adf_*" is a name space of this library. The other underscore names are reserved
#     to the implementation (C11 7.1.3), belong to the linker or to another library, and are
#     printed and not judged.
# The second list, declared and not exported, does not make the script fail while the library is
# incomplete: the count is printed, and a name is listed so that the gap is visible. It will
# become a failure when the milestone closes.
#
# The script is not run by `make check` (the Makefile takes tests/test_*.c only). Run it before
# `make check`, from the repository root:
#
#     sh tests/test_exports.sh && make -j2 check
#
# The environment: CC (default cc), AUX_CC (default gcc; step 2 needs gcc). Exit status 0 when nothing failed, 1 otherwise.

set -u

CC=${CC:-cc}
scratch=build/exports
so=$scratch/libadelefeld.so
fail=0

if [ ! -d include/adelefeld ] || [ ! -d src ]; then
    echo "test_exports: run this from the repository root" >&2
    exit 2
fi

rm -rf "$scratch"
mkdir -p "$scratch"

# ---- 1. the shared object ----

echo "== building the shared object"
# shellcheck disable=SC2086
if ! $CC -shared -fPIC -Iinclude -std=c11 -O1 -g -Wall -Wextra \
        src/*.c -o "$so" -lflint -lgmp -lm; then
    echo "test_exports: the shared object did not build" >&2
    exit 1
fi
cp "$so" build/libadelefeld.so
echo "   $so, copied to build/libadelefeld.so"

# ---- 2. the declarations of the public headers ----

printf '#include <adelefeld.h>\n' > "$scratch/aux.c"
# -aux-info writes one line for every declaration with a prototype, prefixed by the file and the
# line it stands at, and it writes a declaration again at every place where the header is read a
# second time, so the lines are made unique. The lines of the FLINT and system headers are left
# out: only the headers of this library are the interface. -aux-info is an option of gcc alone
# (clang: "unknown argument"), so this one step uses AUX_CC (default gcc) whatever CC is; the
# shared object above is built with CC.
AUX_CC=${AUX_CC:-gcc}
$AUX_CC -Iinclude -std=c11 -aux-info "$scratch/declared.aux" -c "$scratch/aux.c" -o "$scratch/aux.o" \
    || { echo "test_exports: the headers did not compile" >&2; exit 1; }

grep '^/\* include/adelefeld/' "$scratch/declared.aux" \
    | sed -e 's|^/\* ||' -e 's| \*/ |\t|' | sort -u > "$scratch/declared.txt"

# A declaration without parentheses is not a function declaration; there are none in the headers,
# and one that appeared would be a variable of the interface, which this script does not judge.
awk -F'\t' '$2 ~ /\(/ { print $1 "\t" $2 }' "$scratch/declared.txt" > "$scratch/functions.txt"

cut -f2 "$scratch/functions.txt" | sed -e 's/ *(.*//' -e 's/[[:space:]]*$//' \
    | awk '{ print $NF }' | sed -e 's/^\*//' -e 's/^\*//' | sort -u > "$scratch/declared_names.txt"

n_declared=$(wc -l < "$scratch/declared_names.txt" | tr -d ' ')
echo "== $n_declared function names declared in include/adelefeld/*.h"

# ---- 3. conventions 12.2: no variadic function ----

echo "== variadic declarations (conventions 12.2)"
variadic=$(cut -f2 "$scratch/functions.txt" | grep -c '\.\.\.' || true)
if [ "$variadic" -eq 0 ]; then
    echo "   none"
else
    grep '\.\.\.' "$scratch/functions.txt" | cut -f1,2
    fail=1
fi

# ---- 4. the defined dynamic symbols ----

nm -D --defined-only "$so" | awk 'NF >= 3 { print $3 }' | sort -u > "$scratch/exported_all.txt"
# A name that begins with an underscore is reserved to the implementation (C11 7.1.3) and belongs
# to the linker or to a library, not to the interface of adelefeld: those are printed and not
# judged.  A name that begins with "_adf" is a name of this library (conventions 4.1, item 5), and
# is judged like every other name.
grep -v '^_' "$scratch/exported_all.txt" > "$scratch/exported_plain.txt"
{ cat "$scratch/exported_plain.txt"; grep '^_adf' "$scratch/exported_all.txt"; } \
    | sort -u > "$scratch/exported.txt"
n_reserved=$(wc -l < "$scratch/exported_all.txt" | tr -d ' ')
n_reserved=$((n_reserved - $(wc -l < "$scratch/exported.txt" | tr -d ' ')))
n_exported=$(wc -l < "$scratch/exported.txt" | tr -d ' ')
echo "== $n_exported defined dynamic symbols that are public names, $n_reserved reserved name(s)"

# ---- 5. the three lists ----

comm -12 "$scratch/declared_names.txt" "$scratch/exported.txt" > "$scratch/both.txt"
comm -23 "$scratch/declared_names.txt" "$scratch/exported.txt" > "$scratch/missing.txt"
comm -13 "$scratch/declared_names.txt" "$scratch/exported.txt" > "$scratch/extra.txt"

n_both=$(wc -l < "$scratch/both.txt" | tr -d ' ')
n_missing=$(wc -l < "$scratch/missing.txt" | tr -d ' ')
n_extra=$(wc -l < "$scratch/extra.txt" | tr -d ' ')

echo "== declared and exported: $n_both"
cat "$scratch/both.txt"

echo "== declared and not exported: $n_missing (the library is not complete; not a failure)"
cat "$scratch/missing.txt"

echo "== exported and not declared: $n_extra (must be empty)"
if [ "$n_extra" -ne 0 ]; then
    cat "$scratch/extra.txt"
    while read -r name; do
        [ -n "$name" ] || continue
        echo "   $name is exported by the library and declared by no header of include/adelefeld/"
    done < "$scratch/extra.txt"
    fail=1
else
    echo "   (empty)"
fi

if [ "$n_reserved" -ne 0 ]; then
    echo "== exported names that begin with an underscore and are not names of this library,"
    echo "   not judged (the names that begin with _adf are judged, and are in the lists above)"
    grep '^_' "$scratch/exported_all.txt" | grep -v '^_adf'
fi

# ---- 6. the summary ----

if [ "$fail" -eq 0 ]; then
    echo "test_exports: passed: $n_both of $n_declared declared functions are exported," \
         "$n_missing are not implemented yet, no exported name is undeclared, no variadic function"
    exit 0
fi
echo "test_exports: FAILED" >&2
exit 1
