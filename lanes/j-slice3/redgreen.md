# Red-green: step J-a.3

Every gate is `timeout 200 sh lanes/j-slice3/gate.sh`, redirected to the named log. The script has
its own timeout 180, one Julia thread, one precompile task, offline Pkg, the GMP preload, and a lane depot
with the trailing colon. TMPDIR is in the lane. No test or fixture from the first two steps was changed.

| Run | Exit | J-a.1 | J-a.2 | J-a.3 |
|---|---:|---|---|---|
| before.log, before writing the new tests | 0 | 904/904 | 1015/1015 | absent |
| red.log, tests before implementation | 1 | 904/904 | 1015/1015 | 17 pass, 14 fail, 12 error / 43 |
| green1.log, first implementation | 1 | 904/904 | 1015/1015 | 1543 pass, 5 fail, 1548 total |
| green2.log, isolated GC check and numerical precision checks | 0 | 904/904 | 1015/1015 | 1557/1557 |
| green3.log, further boundary checks | 1 | 904/904 | 1015/1015 | 1644 pass, 1 fail / 1645 |
| green4.log, stable exact-count checks | 0 | 904/904 | 1015/1015 | 1645/1645 |
| final.log, before the default-limit regression | 0 | 904/904 | 1015/1015 | 1645/1645 |
| limits-red.log, counted eager keyword default | 1 | 904/904 | 1015/1015 | 1648 pass, 1 fail / 1649 |
| limits-green.log | 124 | not reported | not reported | no completed summary |
| limits-green2.log, final implementation | 0 | 904/904 | 1015/1015 | 1649/1649 |

The five failures in green1 were all the zero-live assertion. The enclosing testset still rooted 49 earlier
input handles. The GC test now runs in a fresh Julia process: baseline 0; 10000 loop iterations per new type,
making a value, a copy, a sum and a negative value; four collections per type; LIVE=0 and ERRORS=0 for each.
The zero requirement was retained. The other checks in green1 passed without a mathematical change.

The single green3 failure was a live-count snapshot during precision refusals: automatic GC released
311 unrelated earlier owners, so the count fell from 628 to 317. The exact-count checks now disable
automatic GC in the parent testset and restore it in a finally. They still require exact equality,
and cannot hide an unmatched allocation behind an unrelated finalizer. The GC acceptance subprocess
starts with automatic GC enabled and still requires zero live owners after four collections.

Negative literal powers had their own red-green regression, timeout 30 with the gate environment:
lanes/j-slice3/powers_test.jl gave 9 pass, 6 fail, 15 total (powers-red.log), then 15/15
(powers-green.log). Julia's inv rewrite had bypassed the ^ method for negative literals; one shared
literal_pow method now forwards to the three existing ^ refusals.

The eager TextLimits() keyword default was a binding defect. On 24 invalid precision inputs it made
24 C default-limit calls before throwing ArgumentError. The new counting fixture links FAULT_MISSING
to the core (unlike the old missing-symbol fixture) so that its status names are correct and all symbols
resolve. COUNT_LIMITS overrides only the default-limit writer. The parser now defers that call until
after precision validation, using a nothing sentinel. This does not change the C default limits.
The first green attempt reached its 180 s bound while waiting on a subprocess; no complete test summary
was printed. A bounded four-type parsing smoke check passed, and the full rerun passed 3568/3568:
ERRORS=24 LIMIT_CALLS=0 LIVE=0. The cause of the timeout was not isolated.

Tests query actual numerical balls through test-only FLINT containment functions, with all addresses
inside preserve. They do not use the printed enclosure to stand in for the stored ball. The oracle is exact:

1. Each random coordinate is n/2^20 with |n| <= 1000000, so its numerator uses at most 20 bits.
2. A real sum or difference has a numerator of at most 21 bits; a product at most 40 bits.
3. A complex product's coordinates are sums or differences of two products. Their numerators have at most
   41 bits and denominator 2^40. Complex sums and differences need at most 21 numerator bits.
4. These dyadic numbers all fit at 256 bits. The BigFloat arithmetic introduces no rounding in this matrix.

The 30 real and 30 complex adele round trips check enclosure and equal finite sets. They do not demand
identical numerical balls or fixed-point text (conventions 9.6).
