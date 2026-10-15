# %% Copy the chip suggestion, evaluate it, then use its fog suggestion.
root(-1, 2; at = 5, seed = 2)
# %% Copy the fog suggestion into the notebook and evaluate again.
root(-1, 2; at = 5, seed = 2, prec = 20)
# %% Scenario A step 5 as written in the brief.
equal(teichmuller(2; at = 5), root(-1, 2; at = 5, seed = 2, prec = 20))
# %% The comparison that the brief asks to show in the same weight.
compare(teichmuller(2; at = 5), root(-1, 2; at = 5, seed = 2, prec = 20))
