#!/bin/sh
# lanes/f-slice9/faults.sh: plant one fault at a time in a scratch copy of src/lpow.c under lanes/f-slice9/build/faults
# and run test_lpow and test_rfunc_prime there. Each fault must turn a test red.
R=$(pwd)
F=$R/lanes/f-slice9/build/faults
rm -rf "$F"; mkdir -p "$F"
plant() {   # name, sed expression
    d=$F/$1; mkdir -p "$d"
    cp -r Makefile include src tests "$d"/
    sed -i "$2" "$d/src/lpow.c"
    if cmp -s src/lpow.c "$d/src/lpow.c"; then echo "$1: PATCH DID NOT APPLY"; return; fi
    (cd "$d" && make -s -j2 BUILD=b b/test_lpow b/test_rfunc_prime > build.log 2>&1) || { echo "$1: build failed"; return; }
    a=$(cd "$d" && timeout 300 ./b/test_lpow 2>&1 | tail -1)
    b=$(cd "$d" && timeout 300 ./b/test_rfunc_prime 2>&1 | tail -1)
    echo "$1: test_lpow: $a | test_rfunc_prime: $b"
}
plant F1_fraction_not_reduced 's|    g = n_gcd(ae, n); |    g = 1; (void) n_gcd; |'
plant F2_no_A_plus_B_term 's|        if (A < INF \&\& B < INF) R = min2(R, add_inf(A, B));|        /* A + B dropped */|'
plant F3_sign_ignored_for_odd_s 's|st = principal(res, u, s, A, B, beta, w0, (w0 == -1 \&\& par) ? -1 : 1, Nc);|st = principal(res, u, s, A, B, beta, w0, 1, Nc);|'
plant F4_beta_from_the_ball 's|    beta = fmpq_is_zero(s->u) ? INF : s->v; |    beta = s->exact \&\& fmpq_is_zero(s->u) ? INF : s->v; |'
plant F5_hull_exponent_2 '290s|min2(Nc, 1)|min2(Nc, 2)|'
plant F6_no_v_e_in_Eprime 's|K = min2(K, add_inf(ej, clamp(x->N - x->v - s + ve)));|K = min2(K, add_inf(ej, clamp(x->N - x->v - s)));|'
