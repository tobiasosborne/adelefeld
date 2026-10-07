#!exit 1
# Slice 4d (lane f4-slice2): docs/api-4.md 6, rfun_eval R with X, X a real ball. Expected lines derived by hand
# before the run: exp(0) = 1 exactly; exp(-pi/4) = 0.4559381277|66: at digits 5 M = 0.45594, R = 1.87e-6 rounded
# up to 1.9e-6; exp(-pi) = 0.04321391826|38: M = 0.043214, R = 8.17e-8 rounded up to 8.2e-8.
prec 128
digits 5
rfun_eval rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with 0
rfun_eval rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with 0.5
# the sign test of PLAN 4.3 with the shift 1/2: phi(x) = exp(-pi (x - 1/2)^2) is exp(-pi) at -1/2 and at 3/2;
# a wrong sign of B would give 1 at -1/2.
rfun_eval rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(3.14159265358979323846264338327950288419716939937510582097494459 +/- 1e-60) + (0)*i, C=(-0.785398163397448309615660845819875721049292349843776455243736148 +/- 1e-60) + (0)*i)) with -0.5
rfun_eval rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with 1/3
rfun_eval rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with 0 +/- 1e100001
rfun_eval 1/3 with 0
