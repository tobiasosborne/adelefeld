#!exit 1
# Slice 4d (lane f4-slice2): docs/api-4.md 2 and 9 item 4; conventions 5.12, 9.2, 9.4. Expected lines derived
# by hand before the run: the canonical text is the template of 9.4; exact trailing zeros are removed, inner
# zeros and an inexact zero kept; Re(A) <= 0 is DOMAIN; another kind of operand is DOMAIN.
rfun rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))
rfun rfun()
rfun rfun( term( P = [ (0) + (0)*i , (1) + (0)*i , (0) + (0)*i ] , A=(1)+(0)*i , B=(0)+(0)*i , C=(0)+(0)*i ) )
rfun rfun(term(P=[(1) + (0)*i, (0 +/- 0.5) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))
# the shorthand of the brief is not a sentence of rfun_v: a coefficient is a complex number
rfun rfun(term(P=[1], A=1, B=0, C=0))
rfun rfun(term(P=[(1) + (0)*i], A=(0) + (1)*i, B=(0) + (0)*i, C=(0) + (0)*i))
rfun 1/3
