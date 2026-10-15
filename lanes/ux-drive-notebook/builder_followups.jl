# %% Builder's suggested ball constructor; the earlier integer-argument path stays unfixed.
FBall(Rat(1), Rat(8 * 1009 * 1013))
# %% Builder's suggested division by zero.
Rat(1) / Rat(0)
# %% Builder's suggested uncertain root.
roots(adf"(* ; 1 mod 4)", 2; at = 2)
# %% Residue form beyond 97.
root(-1, 2; at = 101, seed = 10, prec = 8)
# %% Missing local-ball wrapper named by the report.
LBall
# %% Missing idele wrapper named by the report.
Idele
# %% Literal whose outward printed enclosure touches 1 but the stored upper endpoint does not.
x = adf"(0.99 +/- 0.0099 ; 0 mod 1)"
# %% An exact addition leaves the set, but its source is no longer a literal.
y = x + Rat(0)
# %% Project should keep the stored real coordinate.
project(y, :real)
# %% The stored interval is below 1 and above 0; its finite progression is the integers.
reconstruct(y)
