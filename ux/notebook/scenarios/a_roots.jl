# Scenario A of docs/ux/brief-1-adelica-notebook.md: the square roots of 9, then of -1, at 5.
# A notebook of Adelica: cells separated by `# %%` lines (the rest of the line is the note shown above
# the cell). The two lines below are a preamble, not a cell (see ux/notebook/README.md).
using Adelefeld
using Adelica

# %% Step 1: the square roots of 9 at 5. A fan of two chips, labelled by the seed (the residue mod 5).
roots(9, 2; at = 5)

# %% Step 2: what a click on the chip [3] of In [1] writes. The branch is in the code, not in a mode.
root(9, 2; at = 5, seed = 3)

# %% Step 3: -1 instead of 9. The roots are irrational; 8 digits at the default precision, fog above 5^8.
roots(-1, 2; at = 5)

# %% Step 4: the branch [2] with twelve more digits (what a click on the chip and then the fog writes).
root(-1, 2; at = 5, seed = 2, prec = 20)

# %% Step 5: 2 is not a square mod 5, so no branch exists: a status card, not an empty fan.
roots(2, 2; at = 5)
