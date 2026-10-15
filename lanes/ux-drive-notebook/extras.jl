# %% The brief's exact rational place strip.
Rat(6, 35)
# %% Zero is exact and is not a unit at any place.
Rat(0)
# %% A ball containing zero determines a lower bound, not a point valuation.
FBall(0, 25)
# %% Exact numerator prime above the advertised trial bound.
Rat(1009)
# %% Cofactor 1009*1013, both primes above the advertised trial bound.
adf"1 mod 1022117"
# %% Branch selection must keep the requested 20 digits.
roots(-1, 2; at = 5, prec = 20)
# %% A branch suggestion should preserve the named input for later cells.
r = roots(9, 2; at = 5)
# %% An invalid place is not a proof of nonexistence at a prime.
root(9, 2; at = 4, seed = 3)
# %% Teichmueller call alone, independently of equal and compare.
teichmuller(2; at = 5)
