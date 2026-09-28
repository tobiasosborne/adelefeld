#!/bin/sh
# lanes/m1-headers/mutate_headers.sh: mutation check of tests/test_headers.c and tests/test_abi.c.
#
# Each mutant is one sed edit of one public header, applied to a copy of include/ in a scratch
# directory (the working tree is never touched). Both tests are built against the copy and run;
# a mutant is killed when the build fails or a test fails. The script exits 1 if a mutant survives.
# Run from the repository root:  sh lanes/m1-headers/mutate_headers.sh

set -u
root=$(pwd)
scratch=${TMPDIR:-/tmp}/adf-m1-headers-mutants.$$
cc=${CC:-cc}
killed=0
survived=0

mutant() {
    file=$1; expr=$2; what=$3
    rm -rf "$scratch"; mkdir -p "$scratch"
    cp -R "$root/include" "$scratch/include"
    sed -i "$expr" "$scratch/include/$file"
    if cmp -s "$root/include/$file" "$scratch/include/$file"; then
        echo "BAD MUTANT (no change): $what"; survived=$((survived + 1)); return
    fi
    ok=1
    for t in test_headers test_abi; do
        if ! "$cc" -std=c11 -Wall -Wextra -Wpedantic -Werror -I"$scratch/include" -I"$root/tests" \
            "$root/tests/$t.c" -o "$scratch/$t" -lflint -lgmp > /dev/null 2>&1; then
            ok=0; continue
        fi
        if ! "$scratch/$t" > /dev/null 2>&1; then ok=0; fi
    done
    if [ $ok -eq 0 ]; then
        echo "killed:   $what"; killed=$((killed + 1))
    else
        echo "SURVIVED: $what"; survived=$((survived + 1))
    fi
}

mutant adelefeld/status.h 's/#define ADF_DOMAIN             7/#define ADF_DOMAIN             8/' \
    "ADF_DOMAIN 7 -> 8"
mutant adelefeld/status.h 's/#define ADF_NOT_DETERMINED     1/#define ADF_NOT_DETERMINED     2/' \
    "ADF_NOT_DETERMINED 1 -> 2"
mutant adelefeld/status.h 's/#define ADF_CMP_UNDECIDED      2/#define ADF_CMP_UNDECIDED      1/' \
    "ADF_CMP_UNDECIDED 2 -> 1"
mutant adelefeld/status.h 's/return "NOT_UNIT";/return "NOT_A_UNIT";/' \
    "status name NOT_UNIT misspelt"
mutant adelefeld/status.h 's/default: return "UNKNOWN";/default: return "OK";/' \
    "unknown status named OK"
mutant adelefeld/fball.h '/^    int backend;$/d; s/^    ulong \* res;$/    ulong * res;\n    int backend;/' \
    "adf_fball: backend moved after res"
mutant adelefeld/fball.h 's/#define ADF_LOCAL  1/#define ADF_LOCAL  2/' \
    "ADF_LOCAL 1 -> 2"
mutant adelefeld/scaled.h 's/^    int exact;$/    slong exact;/' \
    "adf_scaled: exact int -> slong"
mutant adelefeld/scaled.h 's/^int adf_scaled_set_context(adf_scaled_t y, int \* lost,/int adf_scaled_set_context(adf_scaled_t y, slong * lost,/' \
    "adf_scaled_set_context: lost int* -> slong*"
mutant adelefeld/text.h 's/ADF_TEXT_IDELE = 5,/ADF_TEXT_IDELE = 6,/; s/ADF_TEXT_IDCLASS = 6,/ADF_TEXT_IDCLASS = 5,/' \
    "text kinds IDELE and IDCLASS exchanged"
mutant adelefeld/text.h 's/#define ADF_TEXT_MAX_EXP10_DEFAULT  100000/#define ADF_TEXT_MAX_EXP10_DEFAULT  100001/' \
    "max_exp10 default 100000 -> 100001"
mutant adelefeld/text.h 's/^    slong max_prec;$/    int max_prec;/' \
    "adf_text_limits_t: max_prec slong -> int"
mutant adelefeld/modctx.h 's/^    slong k;$/    int k;/' \
    "adf_ctx_desc_t: k slong -> int"
mutant adelefeld/modctx.h 's/^void adf_ctx_desc_init(adf_ctx_desc_t \* d);/int adf_ctx_desc_init(adf_ctx_desc_t * d);/' \
    "adf_ctx_desc_init returns int (closure C2 says void)"
mutant adelefeld/modctx.h 's/size_t occurrence,/slong occurrence,/' \
    "adf_modctx_new_from_dump: occurrence size_t -> slong (closure C2, conventions 12.10)"
mutant adelefeld/dump.h 's/const adf_modctx_struct \* const \* binds, size_t nbinds,/const adf_modctx_struct * const * binds, slong nbinds,/' \
    "load_str_binds: nbinds size_t -> slong"
mutant adelefeld/place.h 's/^    ulong opaque;$/    unsigned int opaque;/' \
    "adf_place_t: 4 bytes"
mutant adelefeld/place.h 's/^int adf_place_prime(adf_place_t \* v, ulong p);/adf_place_t adf_place_prime(ulong p);/' \
    "adf_place_prime returns the place (conventions 7 has a status)"
mutant adelefeld/adele.h 's/^    arb_t inf;$/    acb_t inf;/' \
    "adf_adele: real part replaced by a complex one"
mutant adelefeld/fball.h 's/^int adf_fball_compare(/void adf_fball_compare(/' \
    "adf_fball_compare returns void"
mutant adelefeld.h 's/#define ADF_VERSION_MINOR 1/#define ADF_VERSION_MINOR 0/' \
    "version 0.1.0 -> 0.0.0"

rm -rf "$scratch"
echo "mutants: $((killed + survived)), killed: $killed, survived: $survived"
[ $survived -eq 0 ]
