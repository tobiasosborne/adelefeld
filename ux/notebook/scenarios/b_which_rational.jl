# Scenario B of docs/ux/brief-1-adelica-notebook.md: which rational is this? By recover and reconstruct.
# A notebook of Adelica: cells separated by `# %%` lines (the rest of the line is the note shown above
# the cell). The two lines below are a preamble, not a cell (see ux/notebook/README.md).
using Adelefeld
using Adelica

# %% Step 1: the residue of a long computation, as a finite ball: 146 + 1000 Zhat.
c = adf"146 mod 1000"

# %% Step 2: the rational n/d with n = 146 d mod 1000, |n| <= 30, 0 < d <= 15 (SPEC 9.2, partial data).
r = recover(c; num = 30, den = 15)

# %% Step 3: the full adelic ball from the real estimate 3.14 +/- 0.01 and the finite part 1/7 + Zhat.
x = adf"(3.14 +/- 0.01 ; 1/7 mod 1)"

# %% Step 4: reconstruction from the full ball (SPEC 9.2, first item): 1/7 + k meets [3.13, 3.15] at k = 3.
reconstruct(x)

# %% Step 5: wider bounds, 2 * 40 * 30 = 2400 > 1000: the set is still decided, by the Euclidean rows.
recover(c; num = 40, den = 30)

# %% Step 6: bounds so wide that two fractions fit the box: an open status, with what can change it.
recover(c; num = 400, den = 300)

# %% Step 7: a residue mod the prime 2003 whose search is cut short by the driver's search limit 1000.
recover(adf"1 mod 2003"; num = 3, den = 1001)

# %% Step 8: the answer 22/7 at the real place and at 2 and 7, as the driver's partial ball.
project(r, :real, 2, 7)
