# Running notes

Read CLAUDE.md and the brief's contracts, repair report, and named PLAN milestones.
Only this lane is writable. No repository test suite is being run.
The one normal build was started as:
`timeout 60 make -j2 BUILD=lanes/n-review2/build > lanes/n-review2/build.log 2>&1`.

Monotonicity counterexample verified in monotonicity.log: mid = 793/512, rad = 741/512, digits = 1.
Level 2 gives 2 +/- 1.9 (positive); level 3 gives 1.5 +/- 1.5 (not positive).
The exact ball has positive lower endpoint 13/128. This is a dyadic Arb-admissible ball.
This is an own calculation, not a quoted theorem.

The normal archive built successfully. Four renamed printer objects also exist.
prepare_text.py extracted the old sources with read-only git show at 674c9db and renamed 17 public symbols.
The current copy adds counters; the old copy changes names only. Neither repository source was edited.

Completed probes so far:
- C levels 2..5 match the exact Python counterexample (level.log).
- 320 generated balls: 1280 current/instrumented/old/class calls, 0 mismatches (diff.log).
- Driver: 368 commands, 368 lines, 0 unexpected outputs, 0 stderr bytes under ASan/UBSan.
- Extreme local exponents: 18 huge inputs and 4 small inputs, 1152 checks, 0 failures (edge.log).
  UBSan instruments the reproducer only, not the normal archive's local arithmetic.
- Focused INV reproducer includes the two read-only sball/rfunc implementation files.
  Positive control: 14 allocations, 135160 bytes. All 36 excessive-precision calls: LIMIT, 0 allocations.
- Work family: b=4000 has 2410 levels, 9642410 work; b=5500 prints, work 18230314.
  b=8000 refuses in its second pass; b=100000 refuses after 335 levels, len=0, about 3.07 s.
- Exact powers 2^99999 and 2^-99999 at digits=1000000 print in 2 levels (200000 work).
- Exact 1+2^-8000000 at digits=1 prints a 30-byte enclosure in 2 levels, 16000005 work, about 3.54 s.
- A replay with a proved skipped-level bound scanned b=6001..8000 (2000 inputs).
  First refusal is b=7463, 538 refusals, 0 reversals. Direct boundary checks remain to be run.
  Therefore the statement that refusal starts at about 5500 bits needs correction.

All computational attacks are now complete.
- Direct b=7462: prints 4522 bytes, work 33538722. Direct b=7463: NULL, len 0.
- Own proof in proofs.md C: the least level of this family is D+3, D=X(2^(b-1)); both passes are exact.
  Total work is 2(D+2)max(64,b+1), nondecreasing in b. This proves 7463 is the first refusal.
- Own proof in proofs.md B gives k >= X(rad)-X(mid-rad)+1, without assuming monotonicity.
  This contradicts the broad lower-bound justification in SPEC N-D11 and api-2.md decision N-D11.
  It does not prove a lower bound from the separate stored exponents alone.
- Expanded differential: both old idele and old class printers, 320 balls, 1600 calls, 0 failures.
- Focused source-including UBSan edge reproducer: 1152 checks, 0 failures, 0 diagnostics.
- Refusal leak counter: 3 paths, 7086 allocation events, peak 4071 blocks, final 0 blocks.
- Driver LIMIT: 1000 nines prints for idele and class; 1500 nines gives LIMIT for both; 0 stderr bytes.
- C2: 1191 p inputs, 4764 bound checks, 0 failures; maximum log2 absolute error upper 3.702e-15.
  proofs.md D proves the inequality conditional on explicit rounding/error hypotheses.
  A universal sourced libm error bound and a refs source for the rounding model remain pending.
- arithmetic_checks.py: 5 exact checks, 0 failures.

Two MINOR documentation findings are ready: the incorrect family threshold, and the assertion that a lower
bound needs monotonicity / none exists. No BLOCKER or MAJOR code finding was obtained by these probes.
checks.md records build, compilation, and every executed numerical/checking command.
