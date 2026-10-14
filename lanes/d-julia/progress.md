# Progress

Read the required rules, contracts, headers and four Julia fixtures.
The shared object build reports 624 declared and exported functions, 0 missing, 0 extra.
Choose an aligned heap allocation owned by a mutable Julia handle for the first package.
This avoids assumptions about Julia object addresses and keeps context roots outside C storage.
The extra allocation is a measured cost to revisit, not a claim that inline storage cannot work.
The probe will count 100000 init/clear/free pairs and use large rational arithmetic.
The design must distinguish the pre-startup GMP workaround from what __init__ can do.
Pkg.status() with the original depot failed with EROFS; use a writable lane depot for later checks.

Rat probe: 100000 clears/frees, 200000 arithmetic calls, 100000 exact oracle comparisons, 40 GC calls.
ABI probe: 19 rows, 38 size/alignment comparisons, 11 status names, 2 invalid sentinels; accessor absent.
Design checks: at most 450 lines, no line above 116, 44 Julia fixture files named.
Two writable-depot Pkg.status attempts did not complete. One started automatic precompile workers without
the intended worker cap. The next disabled compiled modules, hit timeout, and needed an explicit interrupt.
No Pkg.status result is claimed. Read-only discovery reports Nemo=nothing and FLINT_jll=nothing.
