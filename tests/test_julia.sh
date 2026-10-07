#!/bin/sh
# tests/test_julia.sh: the Julia ccall smoke test of work package 1.9 (docs/PLAN.md row 1.9:
# "a Julia ccall smoke test where Julia is installed"; docs/SPEC.md 10 item 4, M0-D12).
#
# What the script does, in this order:
#   1. builds the shared library the same way tests/test_exports.sh does (it calls that script,
#      unless build/libadelefeld.so is already there and newer than every src/*.c and
#      include/adelefeld/*.h file, in which case it is reused, as the brief asks);
#   2. if `julia` is not on PATH, prints "julia: not installed, skipped" and exits 0;
#   3. otherwise runs `julia --startup-file=no tests/julia/smoke.jl build/libadelefeld.so`.
#
# Run from the repository root:
#
#     sh tests/test_julia.sh
#
# Environment quirk, not an adelefeld interface fault (recorded in lanes/m1-julia/report.md,
# section "Findings"): this Julia (through juliaup) bundles its own libgmp.so.10, built without
# the plain symbol __gmpn_modexact_1_odd (only a CPU-dispatched __gmpn_modexact_1_odd_x86_64);
# Julia's runtime loads that library before any user code runs. The system libflint.so.18 was
# linked against the system libgmp.so.10, which does export the plain symbol. Both libraries
# share the SONAME "libgmp.so.10", so the dynamic linker's normal soname-based reuse makes every
# later loader of "libgmp.so.10" (including our dlopen of libadelefeld.so, which needs libflint,
# which needs libgmp) resolve against whichever copy loaded first. If that is Julia's bundled
# one, dlopen of libadelefeld.so fails with "undefined symbol: __gmpn_modexact_1_odd". The fix is
# LD_PRELOAD of the system libgmp.so.10, which must happen before the dynamic linker processes
# Julia's own executable, i.e. before Julia starts: this script runs Julia once, and only if that
# run fails with exactly this symptom does it retry once with LD_PRELOAD set to the system
# libgmp.so.10 found by ldconfig. A system without this Julia/gmp mismatch never takes that path.
#
# Exit status: 0 if the build succeeds and (Julia is absent, or the Julia run passes); 1 if the
# build fails or the Julia run fails. Environment: JULIA (default: julia).

set -u

JULIA=${JULIA:-julia}
so=build/libadelefeld.so

if [ ! -d include/adelefeld ] || [ ! -d src ] || [ ! -d tests/julia ]; then
    echo "test_julia: run this from the repository root" >&2
    exit 2
fi

# ---- 1. the shared library ----

need_build=1
if [ -f "$so" ]; then
    need_build=0
    for f in src/*.c include/adelefeld.h include/adelefeld/*.h; do
        if [ "$f" -nt "$so" ]; then
            need_build=1
            break
        fi
    done
fi

if [ "$need_build" -eq 1 ]; then
    echo "== $so is missing or stale; building it with tests/test_exports.sh"
    if ! sh tests/test_exports.sh; then
        echo "test_julia: tests/test_exports.sh failed; no library to test" >&2
        exit 1
    fi
else
    echo "== $so is up to date with src/*.c and include/adelefeld/*.h; reusing it"
fi

if [ ! -f "$so" ]; then
    echo "test_julia: $so was not produced" >&2
    exit 1
fi

# ---- 2. is Julia installed? ----

if ! command -v "$JULIA" > /dev/null 2>&1; then
    echo "julia: not installed, skipped"
    exit 0
fi

echo "== $($JULIA --version)"

# Slice a, api-3c 7: character layout and Gauss ccall in its own bounded process.
char_output=$(mktemp)
if timeout 60 "$JULIA" --startup-file=no tests/julia/char.jl "$so" > "$char_output" 2>&1; then
    cat "$char_output"
elif grep -q '__gmpn_modexact_1_odd' "$char_output" 2>/dev/null \
        && char_gmp=$(ldconfig -p 2> /dev/null | awk '/libgmp\.so\.10 /{print $NF; exit}') \
        && [ -n "$char_gmp" ] \
        && LD_PRELOAD="$char_gmp" timeout 60 "$JULIA" --startup-file=no tests/julia/char.jl "$so"; then
    echo "== tests/julia/char.jl passed with LD_PRELOAD=$char_gmp"
else
    cat "$char_output"
    rm -f "$char_output"
    echo "test_julia: tests/julia/char.jl FAILED" >&2
    exit 1
fi
rm -f "$char_output"

# ---- 2b. ideles (milestone 2, slice 1, lane i-slice1): tests/julia/ideles.jl, its own process ----

ideles_output=$(mktemp)
if "$JULIA" --startup-file=no tests/julia/ideles.jl "$so" > "$ideles_output" 2>&1; then
    cat "$ideles_output"
elif grep -q '__gmpn_modexact_1_odd' "$ideles_output" 2>/dev/null \
        && ideles_gmp=$(ldconfig -p 2> /dev/null | awk '/libgmp\.so\.10 /{print $NF; exit}') \
        && [ -n "$ideles_gmp" ] \
        && LD_PRELOAD="$ideles_gmp" "$JULIA" --startup-file=no tests/julia/ideles.jl "$so"; then
    echo "== tests/julia/ideles.jl passed with LD_PRELOAD=$ideles_gmp"
else
    cat "$ideles_output"
    rm -f "$ideles_output"
    echo "test_julia: tests/julia/ideles.jl FAILED" >&2
    exit 1
fi
rm -f "$ideles_output"

# ---- 2c. idele classes (milestone 2, slice 2, lane i-slice2): tests/julia/idclass.jl, its own process ----

idclass_output=$(mktemp)
if "$JULIA" --startup-file=no tests/julia/idclass.jl "$so" > "$idclass_output" 2>&1; then
    cat "$idclass_output"
elif grep -q '__gmpn_modexact_1_odd' "$idclass_output" 2>/dev/null \
        && idclass_gmp=$(ldconfig -p 2> /dev/null | awk '/libgmp\.so\.10 /{print $NF; exit}') \
        && [ -n "$idclass_gmp" ] \
        && LD_PRELOAD="$idclass_gmp" "$JULIA" --startup-file=no tests/julia/idclass.jl "$so"; then
    echo "== tests/julia/idclass.jl passed with LD_PRELOAD=$idclass_gmp"
else
    cat "$idclass_output"
    rm -f "$idclass_output"
    echo "test_julia: tests/julia/idclass.jl FAILED" >&2
    exit 1
fi
rm -f "$idclass_output"

# ---- 2d. powers, hulls, division (milestone 2, slice 3, lane i-slice3): tests/julia/idmap.jl, its own process ----

idmap_output=$(mktemp)
if "$JULIA" --startup-file=no tests/julia/idmap.jl "$so" > "$idmap_output" 2>&1; then
    cat "$idmap_output"
elif grep -q '__gmpn_modexact_1_odd' "$idmap_output" 2>/dev/null \
        && idmap_gmp=$(ldconfig -p 2> /dev/null | awk '/libgmp\.so\.10 /{print $NF; exit}') \
        && [ -n "$idmap_gmp" ] \
        && LD_PRELOAD="$idmap_gmp" "$JULIA" --startup-file=no tests/julia/idmap.jl "$so"; then
    echo "== tests/julia/idmap.jl passed with LD_PRELOAD=$idmap_gmp"
else
    cat "$idmap_output"
    rm -f "$idmap_output"
    echo "test_julia: tests/julia/idmap.jl FAILED" >&2
    exit 1
fi
rm -f "$idmap_output"

# ---- 2e. the value form of ideles (milestone 2, lane t-slice1): tests/julia/text_idele.jl, its own process ----

text_idele_output=$(mktemp)
if "$JULIA" --startup-file=no tests/julia/text_idele.jl "$so" > "$text_idele_output" 2>&1; then
    cat "$text_idele_output"
elif grep -q '__gmpn_modexact_1_odd' "$text_idele_output" 2>/dev/null \
        && text_idele_gmp=$(ldconfig -p 2> /dev/null | awk '/libgmp\.so\.10 /{print $NF; exit}') \
        && [ -n "$text_idele_gmp" ] \
        && LD_PRELOAD="$text_idele_gmp" "$JULIA" --startup-file=no tests/julia/text_idele.jl "$so"; then
    echo "== tests/julia/text_idele.jl passed with LD_PRELOAD=$text_idele_gmp"
else
    cat "$text_idele_output"
    rm -f "$text_idele_output"
    echo "test_julia: tests/julia/text_idele.jl FAILED" >&2
    exit 1
fi
rm -f "$text_idele_output"

# ---- 2f. the value form of local balls and partial balls (lane t-slice2): tests/julia/text_local.jl ----

text_local_output=$(mktemp)
if "$JULIA" --startup-file=no tests/julia/text_local.jl "$so" > "$text_local_output" 2>&1; then
    cat "$text_local_output"
elif grep -q '__gmpn_modexact_1_odd' "$text_local_output" 2>/dev/null \
        && text_local_gmp=$(ldconfig -p 2> /dev/null | awk '/libgmp\.so\.10 /{print $NF; exit}') \
        && [ -n "$text_local_gmp" ] \
        && LD_PRELOAD="$text_local_gmp" "$JULIA" --startup-file=no tests/julia/text_local.jl "$so"; then
    echo "== tests/julia/text_local.jl passed with LD_PRELOAD=$text_local_gmp"
else
    cat "$text_local_output"
    rm -f "$text_local_output"
    echo "test_julia: tests/julia/text_local.jl FAILED" >&2
    exit 1
fi
rm -f "$text_local_output"

# ---- 3. run the smoke test ----

# Slice 3.2-a: the adelic Tate additive character.
psi_output=$(mktemp)
if timeout 60 "$JULIA" --startup-file=no tests/julia/psi.jl "$so" > "$psi_output" 2>&1; then
    cat "$psi_output"
elif grep -q '__gmpn_modexact_1_odd' "$psi_output" 2>/dev/null \
        && psi_gmp=$(ldconfig -p 2>/dev/null | awk '/libgmp\.so\.10 /{print $NF; exit}') \
        && [ -n "$psi_gmp" ] \
        && LD_PRELOAD="$psi_gmp" timeout 60 "$JULIA" --startup-file=no tests/julia/psi.jl "$so"; then
    echo "== tests/julia/psi.jl passed with LD_PRELOAD=$psi_gmp"
else
    cat "$psi_output"
    rm -f "$psi_output"
    echo "test_julia: tests/julia/psi.jl FAILED" >&2
    exit 1
fi
rm -f "$psi_output"

# Slice 4d (lane f4-slice2): real test functions, adf_rfun_translate_rat.
rfun_output=$(mktemp)
if timeout 60 "$JULIA" --startup-file=no tests/julia/rfun.jl "$so" > "$rfun_output" 2>&1; then
    cat "$rfun_output"
elif grep -q '__gmpn_modexact_1_odd' "$rfun_output" 2>/dev/null \
        && rfun_gmp=$(ldconfig -p 2>/dev/null | awk '/libgmp\.so\.10 /{print $NF; exit}') \
        && [ -n "$rfun_gmp" ] \
        && LD_PRELOAD="$rfun_gmp" timeout 60 "$JULIA" --startup-file=no tests/julia/rfun.jl "$so"; then
    echo "== tests/julia/rfun.jl passed with LD_PRELOAD=$rfun_gmp"
else
    cat "$rfun_output"
    rm -f "$rfun_output"
    echo "test_julia: tests/julia/rfun.jl FAILED" >&2
    exit 1
fi
rm -f "$rfun_output"

# Slice 4a: finite function layout, direct Fourier transform, alias and LIMIT calls.
ffun_output=$(mktemp)
if timeout 60 "$JULIA" --startup-file=no tests/julia/ffun.jl "$so" > "$ffun_output" 2>&1; then
    cat "$ffun_output"
elif grep -q '__gmpn_modexact_1_odd' "$ffun_output" 2>/dev/null \
        && ffun_gmp=$(ldconfig -p 2>/dev/null | awk '/libgmp\.so\.10 /{print $NF; exit}') \
        && [ -n "$ffun_gmp" ] \
        && LD_PRELOAD="$ffun_gmp" timeout 60 "$JULIA" --startup-file=no tests/julia/ffun.jl "$so"; then
    echo "== tests/julia/ffun.jl passed with LD_PRELOAD=$ffun_gmp"
else
    cat "$ffun_output"
    rm -f "$ffun_output"
    echo "test_julia: tests/julia/ffun.jl FAILED" >&2
    exit 1
fi
rm -f "$ffun_output"

# Slice 4b: finite algebra and the adf_ffun_dilate_rat call of design section 9.
ffun_algebra_output=$(mktemp)
if timeout 60 "$JULIA" --startup-file=no tests/julia/ffun_algebra.jl "$so" > "$ffun_algebra_output" 2>&1; then
    cat "$ffun_algebra_output"
elif grep -q '__gmpn_modexact_1_odd' "$ffun_algebra_output" 2>/dev/null \
        && ffun_algebra_gmp=$(ldconfig -p 2>/dev/null | awk '/libgmp\.so\.10 /{print $NF; exit}') \
        && [ -n "$ffun_algebra_gmp" ] \
        && LD_PRELOAD="$ffun_algebra_gmp" timeout 60 "$JULIA" --startup-file=no \
            tests/julia/ffun_algebra.jl "$so"; then
    echo "== tests/julia/ffun_algebra.jl passed with LD_PRELOAD=$ffun_algebra_gmp"
else
    cat "$ffun_algebra_output"
    rm -f "$ffun_algebra_output"
    echo "test_julia: tests/julia/ffun_algebra.jl FAILED" >&2
    exit 1
fi
rm -f "$ffun_algebra_output"

# Slice 3.1-e: bounded quotient set queries.
qsets_output=$(mktemp)
if timeout 60 "$JULIA" --startup-file=no tests/julia/qclass_sets.jl "$so" > "$qsets_output" 2>&1; then
    cat "$qsets_output"
elif grep -q '__gmpn_modexact_1_odd' "$qsets_output" 2>/dev/null \
        && qsets_gmp=$(ldconfig -p 2>/dev/null | awk '/libgmp\.so\.10 /{print $NF; exit}') \
        && [ -n "$qsets_gmp" ] \
        && LD_PRELOAD="$qsets_gmp" timeout 60 "$JULIA" --startup-file=no tests/julia/qclass_sets.jl "$so"; then
    echo "== tests/julia/qclass_sets.jl passed with LD_PRELOAD=$qsets_gmp"
else
    cat "$qsets_output"
    rm -f "$qsets_output"
    echo "test_julia: tests/julia/qclass_sets.jl FAILED" >&2
    exit 1
fi
rm -f "$qsets_output"

# Slice 3.1-f: negation and addition of quotient classes.
qarith_output=$(mktemp)
if timeout 60 "$JULIA" --startup-file=no tests/julia/qclass_arith.jl "$so" > "$qarith_output" 2>&1; then
    cat "$qarith_output"
elif grep -q '__gmpn_modexact_1_odd' "$qarith_output" 2>/dev/null \
        && qarith_gmp=$(ldconfig -p 2>/dev/null | awk '/libgmp\.so\.10 /{print $NF; exit}') \
        && [ -n "$qarith_gmp" ] \
        && LD_PRELOAD="$qarith_gmp" timeout 60 "$JULIA" --startup-file=no tests/julia/qclass_arith.jl "$so"; then
    echo "== tests/julia/qclass_arith.jl passed with LD_PRELOAD=$qarith_gmp"
else
    cat "$qarith_output"
    rm -f "$qarith_output"
    echo "test_julia: tests/julia/qclass_arith.jl FAILED" >&2
    exit 1
fi
rm -f "$qarith_output"

# Slice 3.1-a: quotient lift, storage access and rational translation.
qclass_output=$(mktemp)
if timeout 60 "$JULIA" --startup-file=no tests/julia/qclass.jl "$so" > "$qclass_output" 2>&1; then
    cat "$qclass_output"
elif grep -q '__gmpn_modexact_1_odd' "$qclass_output" 2>/dev/null \
        && qclass_gmp=$(ldconfig -p 2>/dev/null | awk '/libgmp\.so\.10 /{print $NF; exit}') \
        && [ -n "$qclass_gmp" ] \
        && LD_PRELOAD="$qclass_gmp" timeout 60 "$JULIA" --startup-file=no tests/julia/qclass.jl "$so"; then
    echo "== tests/julia/qclass.jl passed with LD_PRELOAD=$qclass_gmp"
else
    cat "$qclass_output"
    rm -f "$qclass_output"
    echo "test_julia: tests/julia/qclass.jl FAILED" >&2
    exit 1
fi
rm -f "$qclass_output"

# Slices 3.2-b and 3.2-c (lane q-slice5): the class and local Tate character.
psi_class_output=$(mktemp)
if timeout 60 "$JULIA" --startup-file=no tests/julia/psi_class.jl "$so" > "$psi_class_output" 2>&1; then
    cat "$psi_class_output"
elif grep -q '__gmpn_modexact_1_odd' "$psi_class_output" 2>/dev/null \
        && psi_class_gmp=$(ldconfig -p 2>/dev/null | awk '/libgmp\.so\.10 /{print $NF; exit}') \
        && [ -n "$psi_class_gmp" ] \
        && LD_PRELOAD="$psi_class_gmp" timeout 60 "$JULIA" --startup-file=no tests/julia/psi_class.jl "$so"; then
    echo "== tests/julia/psi_class.jl passed with LD_PRELOAD=$psi_class_gmp"
else
    cat "$psi_class_output"
    rm -f "$psi_class_output"
    echo "test_julia: tests/julia/psi_class.jl FAILED" >&2
    exit 1
fi
rm -f "$psi_class_output"

# Slice 3.1-d union value text and lossless qclass dumps.
qclass_text_output=$(mktemp)
if timeout 60 "$JULIA" --startup-file=no tests/julia/qclass_text.jl "$so" > "$qclass_text_output" 2>&1; then
    cat "$qclass_text_output"
elif grep -q '__gmpn_modexact_1_odd' "$qclass_text_output" 2>/dev/null \
        && qclass_text_gmp=$(ldconfig -p 2>/dev/null | awk '/libgmp\.so\.10 /{print $NF; exit}') \
        && [ -n "$qclass_text_gmp" ] \
        && LD_PRELOAD="$qclass_text_gmp" timeout 60 "$JULIA" --startup-file=no \
            tests/julia/qclass_text.jl "$so"; then
    echo "== tests/julia/qclass_text.jl passed with LD_PRELOAD=$qclass_text_gmp"
else
    cat "$qclass_text_output"
    rm -f "$qclass_text_output"
    echo "test_julia: tests/julia/qclass_text.jl FAILED" >&2
    exit 1
fi
rm -f "$qclass_text_output"

run_output=$(mktemp)
if "$JULIA" --startup-file=no tests/julia/smoke.jl "$so" > "$run_output" 2>&1; then
    cat "$run_output"
    rm -f "$run_output"
    # lane f-slice1: the ccall test of adf_lball (tests/julia/lball.jl)
    if ! "$JULIA" --startup-file=no tests/julia/lball.jl "$so"; then
        echo "test_julia: lball.jl FAILED" >&2
        exit 1
    fi
    # lane f-slice3: the ccall test of the split, the fractional part and the powers (tests/julia/lball2.jl)
    if ! "$JULIA" --startup-file=no tests/julia/lball2.jl "$so"; then
        echo "test_julia: lball2.jl FAILED" >&2
        exit 1
    fi
    # lane f-slice2: the ccall test of adf_sball and the real functions (tests/julia/sball.jl)
    if ! "$JULIA" --startup-file=no tests/julia/sball.jl "$so"; then
        echo "test_julia: sball.jl FAILED" >&2
        exit 1
    fi
    # lane f-slice4: the ccall test of exp, log and Log at a prime (tests/julia/lfunc.jl)
    if ! "$JULIA" --startup-file=no tests/julia/lfunc.jl "$so"; then
        echo "test_julia: lfunc.jl FAILED" >&2
        exit 1
    fi
    # lane f-slice6: the ccall test of exp, log and Log at a prime of a partial ball (tests/julia/f_at.jl)
    if ! "$JULIA" --startup-file=no tests/julia/f_at.jl "$so"; then
        echo "test_julia: f_at.jl FAILED" >&2
        exit 1
    fi
    timeout 60 "$JULIA" --startup-file=no tests/julia/lfunc_trig.jl "$so" || exit 1
    timeout 60 "$JULIA" --startup-file=no tests/julia/lroot.jl "$so" || exit 1
    timeout 60 "$JULIA" --startup-file=no tests/julia/lpow.jl "$so" || exit 1
    timeout 60 "$JULIA" --startup-file=no tests/julia/gfunc.jl "$so" || exit 1
    timeout 60 "$JULIA" --startup-file=no tests/julia/gfunc_log.jl "$so" || exit 1
    timeout 60 "$JULIA" --startup-file=no tests/julia/symbol.jl "$so" || exit 1
    timeout 60 "$JULIA" --startup-file=no tests/julia/catalogue.jl "$so" || exit 1
    timeout 60 "$JULIA" --startup-file=no tests/julia/localfactor.jl "$so" || exit 1
    echo "test_julia: passed"
    exit 0
fi

# One retry with LD_PRELOAD of the system libgmp.so.10, for the environment quirk above.
if grep -q '__gmpn_modexact_1_odd' "$run_output" 2>/dev/null; then
    sys_gmp=$(ldconfig -p 2> /dev/null | awk '/libgmp\.so\.10 /{print $NF; exit}')
    if [ -n "${sys_gmp:-}" ] && [ -f "$sys_gmp" ]; then
        echo "== retrying with LD_PRELOAD=$sys_gmp (Julia's bundled libgmp lacks a symbol" \
             "libflint needs; see the header comment of this script)"
        if LD_PRELOAD="$sys_gmp" "$JULIA" --startup-file=no tests/julia/smoke.jl "$so"; then
            rm -f "$run_output"
            if ! LD_PRELOAD="$sys_gmp" "$JULIA" --startup-file=no tests/julia/lball.jl "$so"; then
                echo "test_julia: lball.jl FAILED (with LD_PRELOAD=$sys_gmp)" >&2
                exit 1
            fi
            if ! LD_PRELOAD="$sys_gmp" "$JULIA" --startup-file=no tests/julia/lball2.jl "$so"; then
                echo "test_julia: lball2.jl FAILED (with LD_PRELOAD=$sys_gmp)" >&2
                exit 1
            fi
            if ! LD_PRELOAD="$sys_gmp" "$JULIA" --startup-file=no tests/julia/sball.jl "$so"; then
                echo "test_julia: sball.jl FAILED (with LD_PRELOAD=$sys_gmp)" >&2
                exit 1
            fi
            if ! LD_PRELOAD="$sys_gmp" "$JULIA" --startup-file=no tests/julia/lfunc.jl "$so"; then
                echo "test_julia: lfunc.jl FAILED (with LD_PRELOAD=$sys_gmp)" >&2
                exit 1
            fi
            if ! LD_PRELOAD="$sys_gmp" "$JULIA" --startup-file=no tests/julia/f_at.jl "$so"; then
                echo "test_julia: f_at.jl FAILED (with LD_PRELOAD=$sys_gmp)" >&2
                exit 1
            fi
            LD_PRELOAD="$sys_gmp" timeout 60 "$JULIA" --startup-file=no tests/julia/lfunc_trig.jl "$so" || exit 1
            LD_PRELOAD="$sys_gmp" timeout 60 "$JULIA" --startup-file=no tests/julia/lroot.jl "$so" || exit 1
            LD_PRELOAD="$sys_gmp" timeout 60 "$JULIA" --startup-file=no tests/julia/lpow.jl "$so" || exit 1
            LD_PRELOAD="$sys_gmp" timeout 60 "$JULIA" --startup-file=no tests/julia/gfunc.jl "$so" || exit 1
            LD_PRELOAD="$sys_gmp" timeout 60 "$JULIA" --startup-file=no tests/julia/gfunc_log.jl "$so" || exit 1
            LD_PRELOAD="$sys_gmp" timeout 60 "$JULIA" --startup-file=no tests/julia/symbol.jl "$so" || exit 1
            LD_PRELOAD="$sys_gmp" timeout 60 "$JULIA" --startup-file=no tests/julia/catalogue.jl "$so" || exit 1
            LD_PRELOAD="$sys_gmp" timeout 60 "$JULIA" --startup-file=no tests/julia/localfactor.jl "$so" || exit 1
            echo "test_julia: passed (with LD_PRELOAD=$sys_gmp)"
            exit 0
        fi
        rm -f "$run_output"
        echo "test_julia: FAILED (even with LD_PRELOAD=$sys_gmp)" >&2
        exit 1
    fi
fi

cat "$run_output"
rm -f "$run_output"
echo "test_julia: FAILED" >&2
exit 1
