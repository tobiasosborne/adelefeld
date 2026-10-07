#!exit 1
# SPEC 5, api-3c 3/7, CV-58, ideles P15. Expected values are derived here before running.
# t=2, u'=-1; odd chi_3(2)(-1)=-1. Thus the design example gives -2 in both modes.
char_eval char(q=3, n=2, s=(1) + (0)*i) with (-2 ; 1 * [1])
char_eval_strict char(q=3, n=2, s=(1) + (0)*i) with (-2 ; 1 * [1])
# Class coordinate is already sign-corrected: same value. Content 2 gives t=1 and value -1.
char_eval char(q=3, n=2, s=(1) + (0)*i) with <2 ; [-1]>
char_eval char(q=3, n=2, s=(1) + (0)*i) with (-2 ; 2 * [1])
# Content 1/3 gives t=6; content 6/5 gives t=5/3. Choose inf=-6 and r=6/5 for exact t=5.
char_eval char(q=3, n=2, s=(1) + (0)*i) with (-2 ; 1/3 * [1])
char_eval char(q=3, n=2, s=(1) + (0)*i) with (-6 ; 6/5 * [1])
# Diagonal -1 has u' = (-1)(-1)=1, t=1. Positive real part leaves the finite -1.
char_eval char(q=3, n=2, s=(1) + (0)*i) with (-1 ; 1 * [-1])
char_eval char(q=3, n=2, s=(1) + (0)*i) with (2 ; 1 * [-1])
# Real chi_5(4) is even, so negative real part contributes +1; t=2.
char_eval char(q=5, n=4, s=(1) + (0)*i) with (-2 ; 1 * [1])
# t=1 gives power 1 even with complex s radii. chi_5(2)(2)=i.
char_eval_strict char(q=5, n=2, s=(2 +/- 0.5) + (3 +/- 0.5)*i) with <1 ; [2 mod 5]>
# Real-power exact half-integer: 4^-1/2=1/2, times chi(-1)=-1.
char_eval_strict char(q=3, n=2, s=(-0.5) + (0)*i) with <4 ; [-1]>
# s=0 and ambiguous odd quadratic image: real line [-1,1], radius successor prints 1.1.
char_eval char(q=3, n=2, s=(0) + (0)*i) with <2 ; [1 mod 1]>
char_eval_strict char(q=3, n=2, s=(0) + (0)*i) with <2 ; [1 mod 1]>
char_eval_strict char(q=5, n=2, s=(0) + (0)*i) with (-2 ; 1 * [1 mod 1])
# Invalid class t and idele real input are rejected by the constructors.
char_eval char(q=3, n=2, s=(1) + (0)*i) with <1 +/- 1 ; [1]>
char_eval char(q=3, n=2, s=(1) + (0)*i) with <-1 ; [1]>
char_eval char(q=3, n=2, s=(1) + (0)*i) with (0 ; 1 * [1])
char_eval char(q=3, n=2, s=(1) + (0)*i) with [1]
char_eval 1 with <1 ; [1]>
char_eval char(q=3, n=2, s=(1) + (0)*i)
prec 2097153
