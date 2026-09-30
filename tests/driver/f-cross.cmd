#!exit 1
# f-cross: project, exp_at, log_at with an operand of the kinds ucoset, idele, idclass: ADF_UNSUPPORTED (review n-review1 D1;
# tools/adf/README.md admits a rational, a finite ball or an adele).  Expected lines are those of the review.
project [1] with 5
project [5 mod 6] with 5
project (5 ; 5 * [1]) with 5
project (1 ; 1 * [1]) with 5
project <1 ; [1]> with 5
exp_at [1] with 5
exp_at [5 mod 6] with 5
exp_at (5 ; 5 * [1]) with 5
exp_at (1 ; 1 * [1]) with 5
exp_at <1 ; [1]> with 5
log_at [1] with 5
log_at [5 mod 6] with 5
log_at (5 ; 5 * [1]) with 5
log_at (1 ; 1 * [1]) with 5
log_at <1 ; [1]> with 5
prec 2
project 5 with 5
exp_at 5 with 5
log_at 6 with 5
project (1 ; 1) with 2 real
type [p=5: 1 + O(5^2)]
