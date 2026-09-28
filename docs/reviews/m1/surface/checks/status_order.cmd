# status_order.cmd: tools/adf/README.md says a kind with no typed parser "gives error: UNSUPPORTED,
# and that is decided before every other check"; and "Every other pair is error: DOMAIN".
# build/adf docs/reviews/m1/surface/checks/status_order.cmd
# expected by the README: UNSUPPORTED on each of the next five lines
add [5 mod 6] with 1/0
add 1/0 with [5 mod 6]
add [5 mod 6] with (1e100001 ; 0)
reconstruct (* ; 1 mod 2) with [5 mod 6] with 1
reconstruct (* ; 1 mod 2) with 0 with [5 mod 6]
# expected by the README: DOMAIN (a pair of types that is not combined), as for cap with a ball
cap (* ; 3 mod 12) with (* ; 1 mod 2)
reconstruct (* ; 1 mod 2) with (* ; 0 mod 1) with 1
reconstruct (* ; 1 mod 2) with (0 ; 0) with 1
