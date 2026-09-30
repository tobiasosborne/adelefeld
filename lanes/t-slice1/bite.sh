#!/bin/sh
# lanes/t-slice1/bite.sh: the tests bite.  Each fault is made in a scratch copy under build/bite/ (never in src/),
# tests/test_text_idele.c is built and run against it, and the number of failed checks is printed.  A fault that
# leaves the test green would be a gap of the test.
#
#   sh lanes/t-slice1/bite.sh          (from the repository root; about 1 minute per fault)
#
# The faults are named by what they break; the sed program of each is the second argument of fault below.

set -u
LC_ALL=C
export LC_ALL

ROOT=$(pwd)
SCRATCH=$ROOT/build/bite

fault()
{
    name=$1
    file=$2
    prog=$3
    d=$SCRATCH/$name
    rm -rf "$d"
    mkdir -p "$d/tests/ref/vectors" "$d/tests/support"
    cp -r "$ROOT/src" "$ROOT/include" "$d/"
    cp "$ROOT/Makefile" "$d/"
    cp "$ROOT/tests/test_runner.h" "$ROOT/tests/test_text_idele.c" "$d/tests/"
    cp "$ROOT/tests/support/"* "$d/tests/support/"
    cp -r "$ROOT/tests/golden" "$d/tests/"
    cp -r "$ROOT/tests/ref/vectors/t-slice1" "$d/tests/ref/vectors/"
    cp "$d/$file" "$d/$file.orig"
    sed -i "$prog" "$d/$file"
    if cmp -s "$d/$file" "$d/$file.orig"; then
        echo "fault $name: the sed program changed nothing"
        return
    fi
    if ! (cd "$d" && make -j2 build/test_text_idele > make.log 2>&1); then
        echo "fault $name: the copy does not build; see $d/make.log"
        return
    fi
    (cd "$d" && timeout 600 build/test_text_idele > run.log 2>&1)
    echo "fault $name ($file): $(tail -n 1 "$d/run.log")"
}

# 1. the coprimality of the residue and the modulus is not checked
fault no_gcd src/text.c 's/^    ok = fmpz_is_one(g);$/    ok = 1;/'
# 2. the constrained printer takes the first level whatever the condition (the printed ball can contain 0)
fault no_constraint src/text.c 's/^            if (tx_interval_sat(M, R, positive))$/            if (1)/'
# 3. the fallback to kernel B is removed: the enclosing ball is the only ball
fault no_kernel src/text.c 's/^    st = adf_idele_ball_from_ends(t, l, h, sign, p);$/    st = adf_idele_ball_from_ends(t, l, h, sign, p); st = ADF_NOT_DETERMINED;/'
# 4. an idele must be positive: the negative side of the condition "excludes 0" is dropped
fault positive_only src/text.c 's/!(fmpq_sgn(lo) > 0 || fmpq_sgn(hi) < 0) : !(fmpq_sgn(lo) > 0))/!(fmpq_sgn(lo) > 0) : !(fmpq_sgn(lo) > 0))/'
# 5. the unit is printed as stored, not in normal form
fault stored_unit src/text_idele.c 's/^    adf_ucoset_normalise(v, u);$/    adf_ucoset_set(v, u);/'
# 6. the limit on a decimal exponent is not applied to the real part
fault no_exp_limit src/text.c 's/if (form != ADF_TX_FORM_UCOSET \&\& tx_real_over(s, \&real, lim))/if (0)/'
# 7. the digits of the printer: the first level has n significant digits fewer (an off-by-one of 9.5, nk = n + k - 2)
fault digits_off_by_one src/text.c 's/^    slong nk = n + k - 2, q, q2, unit;$/    slong nk = n + k - 3, q, q2, unit;/'
