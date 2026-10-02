# 1F.7: same arity, place syntax, and accepted kinds as exp_at.
#!exit 1
sin_at 5
sin_at 5 with
sin_at 5 with 5 7
sin_at 5 with +5
sin_at 5 with 4
sin_at [1] with 5
sin_at ((1) + (2)*i ; 5) with real
cos_at 5
cos_at 5 with
cos_at 5 with 5 7
cos_at 5 with +5
cos_at 5 with 4
cos_at [1] with 5
cos_at ((1) + (2)*i ; 5) with real
sinh_at 5
sinh_at 5 with
sinh_at 5 with 5 7
sinh_at 5 with +5
sinh_at 5 with 4
sinh_at [1] with 5
sinh_at ((1) + (2)*i ; 5) with real
cosh_at 5
cosh_at 5 with
cosh_at 5 with 5 7
cosh_at 5 with +5
cosh_at 5 with 4
cosh_at [1] with 5
cosh_at ((1) + (2)*i ; 5) with real
