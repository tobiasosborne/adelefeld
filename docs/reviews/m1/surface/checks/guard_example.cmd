# guard_example.cmd: the example of tools/adf/README.md for "a value that an operation produced"
# build/adf docs/reviews/m1/surface/checks/guard_example.cmd
# the README's example: each operand alone is already over the guard (binary exponent about 166097)
show (1e50000 ; 0)
add (1e50000 ; 0) with (1e50000 ; 0)
# an example where only the result is over the guard (each operand about 2^66439, product 2^132877)
show (1e20000 ; 0)
mul (1e20000 ; 0) with (1e20000 ; 0)
# prec above the guard: every rounded result at prec > about 100000 has a radius below 2^-100000
prec 100100
div (1 ; 0) with 3
add (1 ; 0) with 1/3
prec 99000
div (1 ; 0) with 3
