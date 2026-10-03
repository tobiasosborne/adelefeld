# Lane f-repair6 report

2026-10-04. F1 to F9 are repaired. No library source, specification, declaration or tracker was changed.
All 16 original faults fail at least one program. The normal tests and the one check-all run pass.
The required LeakSanitizer runs reach 0 failed checks, then fail because this sandbox uses ptrace.
Address/undefined-behaviour checks pass. Supplemental Valgrind checks are detailed below, including their limits.

## Repairs

1. F1: P1 and lpow.h now name the roots 2 and -2 of 4 at 5, with residues 2 and 3.
   A seed still names one powered value before reduction. Injectivity holds exactly when
   gcd(e,gcd(n,p-1))=1, replacing p-1 by 2 at 2, provided a nonzero input has a root.
   The proof writes the root set as b times a cyclic group of order d; the powering kernel has gcd(e,d) elements.
   Reduction is sufficient, not necessary. Own exact checks: 20800 cyclic-group comparisons, 0 failures.
2. F2: lpow.h replaces the false bound by K with P3's Nr=j+rel_r,
   rel_r=max(1,K-e'j-v_p(e')), references R4 for the root's working precisions, and states P6's bounds.
   The endpoint j=-10, e'=2, K=0 gives rel_r=20 and Nr=10. Absolute N alone does not bound relative work.
3. F3: P8 uses |e'|<=2^63, including LONG_MIN. P9's integer endpoint is corrected too.
   New C checks exercise LONG_MIN in powrat at denominator 3 and in the exact-integer powunit route.
4. F4: the three missing endpoints are explicit. For a negative power on a zero ball, p^(n'l), n'l>=N,
   is a nonzero domain witness. A p^k with k>=N and n' not dividing k witnesses the complement.
   At 2 and H=1 every odd-unit power is 1 modulo 2. At R=1 in R8, A_R=t_R=1 and both inverses are omitted.
   Own checks: 6048 zero-ball comparisons, 4096 powers modulo 2, 1443 R=1 CRT comparisons; 0 failures.
5. F5: R8 counts the two modular powerings per digit as well as explicit products modulo p.
   Digit work is O(sum((a-b)(q+log P))), plus setup, CRT reconstruction and the final check.
   Integer powers of q and their updates fit the same bound. Enumeration adds d products and sorting.
   Binary powering is documented in refs/src/flint-3.0.1/ulong_extras.rst:537-545.
6. F6: 30 new powunit ball rows cover each uniquely minimal term of R twice at 3,5,7,65537 and 2^64-59.
   Small primes use whole finite image sets. Large primes use two certified image points at distance p^R.
   At least one misses the ball of exponent R+1 with the expected centre. The C tests check witnesses,
   centres, radii, coarse requests and aliases. They include u=1+p Z_p, s=Z_p, N=2.
7. F7: 144 powrat ball centres at both large primes are lifted independently in Python by
   t^n'=x^e' modulo p^K, K=2,3,4, with n'=2,3,4,5 and gcd(e',n')=1.
   The C test checks both the stored centre and its equation. All 174 power rows also run through _at.
   powers_at_prime now states the limits of its expectations from the same local routine.
8. F8: every centre of all six large lists is checked by integer modular powering at K=20.
   This covers 106850 branch entries, including all 65536 at 65537 and all word-prime lists.
   The 16 samples are removed. Twelve more rows check the seed-42 reproducer, its alias and its full list.
   A supplemental fault at R9's fused write site fails the complete-list check.
9. F9: repair6_early_limit_cost guards refusal of 824329 branches at 2^64-59 within 0.02 CPU seconds.
   A valid listing at N=40 cost 4.131082 seconds, 206.55 times the guard. Initial refusal cost 0.000279 seconds;
   the final native test measured 0.000002 seconds. Every caller slot, identifier and len is preserved.
   A pure deferral of early_status until after identifiers fails exactly this one cost check.
   R6 and lroot.h name the cost property and its test.

All changed statements were re-read against powrat_branch, principal and the integer route in lpow.c,
and dth_root, early_status, branch and all_branches in lroot.c. These files were read-only.

## Files written

- tests/test_lpow.c, tests/test_lroot.c, tests/test_rfunc_prime.c: power/root tests and related comments only.
- proto/lpow_checks.py, proto/lroot_checks.py: additive --repair6 generators; existing fixture paths untouched.
- docs/api-1f6.md, docs/api-1f5.md; comment blocks only in include/adelefeld/lpow.h and lroot.h.
- tests/ref/vectors/f-repair6/powunit.jsonl, powrat.jsonl, roots.jsonl.
- Lane sources: mutations.py, proof_checks.py, early_cost.c, cleanup_main.c, leak_checks.py, leak_new_lpow.c.
- Lane records: progress.md, redgreen.log, fault-table.md, report.md, and the logs/status files named below.
  Generated objects, archives, binaries, fault source copies and compile/link/run logs
  are under build* in this lane.

## Fixtures and oracles

| File in tests/ref/vectors/f-repair6 | Rows | Precision | Bytes |
|---|---:|---|---:|
| powunit.jsonl | 30 | centre mod p^R; comparison H=R+1=2..5 | 5000 |
| powrat.jsonl | 144 | centre and equation mod p^K, K=2,3,4; j=0 | 19914 |
| roots.jsonl | 12 | centres of +-42 and equation mod p^K, K=2,3,4 | 961 |

Total: 186 rows, 25875 bytes. The generators import only the standard library and use exact integers.
The 18 small-prime powunit rows exhaust the image sets. The 12 large-prime rows certify witness pairs instead:
even enumerating 65537^2 residues is too large. Witness outputs are certified residues of actual integer powers.
Their difference valuation is resolved at H=R+1. Infinite image equality is P5, not a claimed large enumeration.
For powrat, each next Hensel digit solves a linear congruence with unit derivative; the final equation and
seed are checked again. No expected centre is obtained from the C library, logarithms or exponentials.

## Red and green

Commands use timeout 150 python3 -B lanes/f-repair6/mutations.py, followed by the listed arguments.
The script compiles a changed source copy and links its object before the unchanged archive.
Compile/link/test commands inside it use timeout 150. Memory is bounded to 1 GiB per child.

Initial order: prepare; 6 7; 14 16; baseline. This precedes sentence edits and F9 additions.

| Fault | lpow failed checks | lroot failed checks | rfunc failed checks |
|---|---:|---:|---:|
| F6 R_drop_A_B_65537 | 8 | 0 | 4 |
| F7 powrat_wrong_centre_65537 | 288 | 0 | 144 |
| F8 root_wrong_unsampled_lift | 72 | 6 | 36 |
| unchanged library | 0 | 0 | 0 |

Initial green totals: 3074134, 1537090, 526806 checks. Logs: red-F6.log, red-F7-F8.log, green.log.
After F9: prepare; 12 13; 16 17. Supplemental fused-lift fault: 0/10/0 failed checks.
After LONG_MIN checks: prepare; 17 18; baseline. Pure early-status deferral: 0/1/0 failed checks.
Final green totals: 3074149, 2361423, 526806 checks, all with 0 failures.
Logs: red-F9.log, red-fused-F8.log, red-deferral-F9.log, green-final.log; combined record redgreen.log.

## Complete original fault table

Commands: timeout 150 python3 -B lanes/f-repair6/mutations.py 0 8, then the same command with 8 16.
The original 16 definitions are retained. The early_status declaration text is adapted to f-repair5's code.
Flags match the review: -std=gnu11 -O2 -g -Iinclude -Itests -Isrc; link -lflint -lgmp -lm.
48 executions, 16 faults killed, 0 survivors, 0 timeouts. Two rfunc executions end with signal 11 after failures;
their emitted failed-check counts are shown because no final summary exists. All other counts are final summaries.
Logs: faults-0-8.log, faults-8-16.log and build/faults/results.jsonl.

| Fault | lpow failed/checks | lroot failed/checks | rfunc failed/checks |
|---|---:|---:|---:|
| E_ej_plus1 | 62159/3034379 | 0/2361423 | 5/526806 |
| E_relative_minus1 | 81054/3074149 | 0/2361423 | 288/526806 |
| E_denominator_plus1 | 81054/3074149 | 0/2361423 | 288/526806 |
| E_numerator_plus1 | 62159/3034379 | 0/2361423 | 5/526806 |
| R_drop_A_beta | 3685/3073885 | 0/2361423 | 21/526806 |
| R_drop_B_alpha | 4253/3072313 | 0/2361423 | 20/526806 |
| R_drop_A_B_65537 | 8/3074149 | 0/2361423 | 4/526806 |
| hull_one_negative_sign | 30/3074149 | 0/2361423 | 0/526806 |
| powunit_K_N | 10102/3068871 | 0/2361423 | 61/526806 |
| root_K_N | 3796/3074149 | 32155/2196884 | signal 11; 13 emitted |
| zeta_order_d_over_q | 0/3074149 | 180609/2361424 | 3617/526806 |
| CRT_swap_two_idempotents | 0/3074149 | 501/2356593 | 0/526806 |
| early_status_always_OK | 0/3074149 | 1919/1873492 | 2120/526806 |
| seed_mod_p_minus1 | 210847/2941296 | 53985/2361423 | signal 11; 11 emitted |
| powrat_wrong_centre_65537 | 288/3074149 | 0/2361423 | 144/526806 |
| root_wrong_unsampled_lift | 72/3074149 | 6/2361423 | 36/526806 |

The fault outcomes are also checked for agreement between exit status and failed-check counts: 48 agree.
After R9, always-OK early_status also omits initialization needed by the fused chain, so it now fails value tests.
The additional pure-deferral fault preserves that initialization and isolates the requested cost property.
The original seed-42 fault affects seeded calls; most list centres now bypass its branch() write.
The additional fused-write fault demonstrates that the every-centre list check itself fails.

## Other checks and performance

Build commands, each exit 0, with at most two jobs:

```text
timeout 180 make -s -j2 BUILD=lanes/f-repair6/build \
  lanes/f-repair6/build/test_lpow lanes/f-repair6/build/test_lroot \
  lanes/f-repair6/build/test_rfunc_prime
timeout 180 make -s -j2 SAN=1 BUILD=lanes/f-repair6/build-san \
  lanes/f-repair6/build-san/test_lpow lanes/f-repair6/build-san/test_lroot \
  lanes/f-repair6/build-san/test_rfunc_prime
```

The normal build was run before additions, after additions and after header comments.
Logs: build.log, build-san.log.
For each T=test_lpow,test_lroot,test_rfunc_prime, before and after additions:
timeout 30 /usr/bin/time -f 'CPU user=%U sys=%S wall=%e' lanes/f-repair6/build/T.
Every exit is 0. CPU below is user plus system time; single runs, not a speedup claim.

| Program | Before checks | After checks | Before CPU s | After CPU s | After failed checks |
|---|---:|---:|---:|---:|---:|
| test_lpow | 3066274 | 3074149 | 6.41 | 5.58 | 0 |
| test_lroot | 1323173 | 2361423 | 1.11 | 1.54 | 0 |
| test_rfunc_prime | 522217 | 526806 | 0.09 | 0.10 | 0 |

Total CPU: 7.61 before, 7.22 after; observed change -0.39 s, below the 10 s increase limit.
Logs: before-lpow/lroot/rfunc.log and after-lpow/lroot/rfunc.log.
All six complete centre checks total 0.409 s CPU: 0.073, 0.051, 0.004, 0.222, 0.008, 0.051 s.
The largest individual check is 0.222 s, below 5 s; the entire final native test_lroot costs 1.54 s.

Generators: timeout 30 python3 -B proto/lpow_checks.py --repair6 and the same command for lroot_checks.py.
Both successful runs exit 0, with the rows/bytes above. The first lpow attempt exited 1 because the second
A+beta case had a tie at B=3; changing that input to B=4 makes the term strictly minimal. No expected result
or assertion was weakened. Default generators were not run, since they write other lanes' fixtures.
timeout 30 python3 -B lanes/f-repair6/proof_checks.py ran twice, exit 0, with the counts above;
the final run also certifies the R9 comment counterexample below. Log: proof-checks.log.

Early-cost calibration:

```text
timeout 60 cc -std=c11 -O2 -Iinclude lanes/f-repair6/early_cost.c \
  lanes/f-repair6/build/libadelefeld.a -lflint -lgmp -lm -o lanes/f-repair6/build/early_cost
timeout 30 lanes/f-repair6/build/early_cost
```

Both exit 0. Refusal / N=0 listing / N=40 listing CPU: 0.000279 / 0.433500 / 4.131082 s.
Log: early-cost.log. The always-OK fault's refused call costs 0.217694 s and misses the 0.02 s guard.

Sanitizers, for each T as above:
ASAN_OPTIONS=detect_leaks=1 timeout 60 /usr/bin/time -f 'CPU user=%U sys=%S wall=%e'
lanes/f-repair6/build-san/T. All reach the final normal check totals with 0 failures, then exit 1.
All three logs explicitly report that LeakSanitizer cannot work under ptrace. Logs: san-lpow/lroot/rfunc.log.
ASAN_OPTIONS=detect_leaks=0 timeout 60 lanes/f-repair6/build-san/T: all exit 0, same counts, no address or
undefined-behaviour report. Logs: san-no-leaks-lpow/lroot/rfunc.log. This does not certify LeakSanitizer.

One acceptance run: timeout 900 make -j2 check-all, default build/, exit 0.
75 C programs, 0 failed checks; driver 51 cases / 100982 expected lines; exports 432/432;
Julia smoke test 48/48; both tool self-tests pass. Logs: check-all.log, check-all.status.
Last line: check-all passed: make check, driver, exports, julia, mutate-selftest, memcheck-selftest

Supplemental memory checks: timeout 10 valgrind --version reports 3.22.0.
The common Valgrind flags are --leak-check=full --show-leak-kinds=all
--errors-for-leak-kinds=definite,indirect,possible --error-exitcode=99.
Raw binaries: timeout 180 valgrind FLAGS lanes/f-repair6/build/test_lpow and test_lroot.
Lpow times out, exit 124, no summary. Root exits 99; 0 definite/indirect loss, 14821560 bytes possibly lost,
and 2 instrumented cost failures: R9 listing 0.722 s against 0.45 s; centre verification 9.454 s against 5 s.
Logs: valgrind-lpow.log/status, valgrind-lroot.log/status. No cost guard is relaxed.

FLINT documents cleanup of retained caches in refs/src/flint-3.0.1/memory.rst:29-44.
timeout 180 python3 -B lanes/f-repair6/leak_checks.py T, for each T, runs the original tests unchanged,
then flint_cleanup_master, with a 170 s inner Valgrind bound. Full lpow again times out, exit 124.
Root completes: 2361423 checks, 2 cost failures, exit 1; instrumented times 1.308 and 13.653 s.
Rfunc completes: 526806 checks, 0 failures, exit 0. Both have 0 bytes in use and 0 Valgrind errors.
Logs: valgrind-clean-test_lpow/lroot/rfunc_prime.log/status.
The lane-only leak_new_lpow.c selects the 2 new tests, keeping every assertion, then cleans the caches.
It was compiled with timeout 60 cc -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -Iinclude -Itests,
linking the lane's support objects/archive and -lflint -lgmp -lm as test_lpow-new-cleanup.
timeout 60 valgrind FLAGS lanes/f-repair6/build/test_lpow-new-cleanup exits 0:
7875 checks, 0 failures, 7807 allocations and frees, 0 bytes in use, 0 Valgrind errors.
Logs: valgrind-new-lpow.log/status. The full lpow leak check remains incomplete.

Record checks also validate the 48 fault outcomes, six final normal/sanitizer summaries, fixture sizes,
and changed prose line lengths. These use timeout 30 python3 -B; 0 assertion failures.
Existing long table rows in api-1f6 and an existing generator line were left unchanged.
Final report QA first found 2 lines of 117 characters (exit 1); they were wrapped before repeating the check.

## Additional prose finding

R9's existing test comment and timing sentence described x=3^n+p^30 Z_p. The code actually constructs
x=w+p^30 Z_p, w=3^n mod p. Both owned sentences now name that input. f-repair5/result.md repeats the
incorrect description and is read-only here. At p=2^64-59, n=24068:
w=5016686989854155913; 3^n mod p^2=101245085488030368830923462363895328769;
(3^n-w)/p mod p=5488507081980157208, nonzero. Thus 3^n is outside the actual ball already modulo p^2.
This is a comment correction, not a different numerical test. No executable statement changed after check-all.

## Findings against the specification

None. No new returned-value test fails on the unchanged library. The review's F1-F5 corrections check out.
The F8/F9 fault behaviour changed after R9 as described above; this does not refute the historical review.
The sanitizer restriction and instrumented timing failures are validation limits, not numerical counterexamples.

## What is not done and sources pending

- LeakSanitizer itself is unverified: all detect_leaks=1 runs fail under the sandbox's ptrace restriction.
- Full test_lpow under Valgrind exceeds both time bounds. All its new checks were verified separately.
- No large-prime image enumeration or long fuzz campaign. Large-prime witness checks are stated as such.
- No new source is pending. Exact internal FLINT multiplication counts are not claimed.
  Existing naming sources pending in functions.md remain outside this repair.

No git state-changing command, bd, package installation or subagent was used. The report was written at the end.
