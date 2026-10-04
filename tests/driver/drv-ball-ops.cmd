# drv-ball-ops: a partial ball of the value form as the operand X of the commands at places (lane
# drv-ball, step 2: project, exp_at, log_at, sin_at, cos_at, sinh_at, cosh_at and powunit_at, which
# take X as the first operand; include/adelefeld/sball.h).
#
# Every expected line is written from the lines of tests/driver/f-project.out and
# tests/driver/f-at-prime.out, which lanes/f-slice6 computed from the series of
# docs/proofs/functions.md Definition 1 with exact Fractions, and from conventions 9.4 for the text of
# the projection.  None is copied from the output of the program.
#!exit 1
# at a prime the setting prec is the requested ABSOLUTE precision N (adelefeld/rfunc.h, "A PRIME")
prec 8
# the rational 4 is read as the adele (4 ; 4) (SPEC 4.1); its component at 2 is the exact 4
project 4 with 2
# the same value stored as a partial ball of the value form, projected to the same place
project {p=2: 4} with 2
# exp_at of the same two operands.  f-at-prime.out line 6: exp(4) at 2 with N = 8 is 77 + O(2^8),
# because 1 + 4 + 8 = 13 modulo 16 and the centre 77 is 13 modulo 16
exp_at 4 with 2
exp_at {p=2: 4} with 2
# a partial ball with two places, projected to both and to one of them (conventions 7: the canonical
# order of places, real first, then the primes increasing)
project {p=2: 4; p=5: 1} with 2 5
project {p=2: 4; p=5: 1} with 5
# a component that is a ball and not an exact value is copied as it is
project {p=2: 4 + O(2^3)} with 2
# the real place: the real component is copied, and the real part of the adele of the exact 0 is exact
project 0 with real 2
project {inf: 0; p=2: 0} with real 2
# a place that is no place of the partial ball: the partial ball holds no component there, so there is
# no value to project: DOMAIN (the same status as a finite ball with the place real)
exp_at {p=3: 4} with 2
project {} with 2
# a place named twice: DOMAIN, as adf_sball_project answers it for a rational, a finite ball or an adele
project {p=2: 4} with real real
# a complex partial ball: the functions at places are the real ones, so the request is UNSUPPORTED
# (include/adelefeld/sball.h, "A PRIME")
exp_at {inf: (1) + (0)*i} with real
