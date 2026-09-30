# n-review1 progress

Part A: original edge.c rebuilt against current release and INV archives.
R3: LIMIT at 2, output empty, both inputs canonical. Original case closed.
R4: 6 original entry cases and neg control abort, shell exit 134, with public function names.
R5: 5 LONG_MAX probes return LIMIT, exit 0, no allocation abort.
R7: the zero-centre exception is now explicit in L4a.
R8: S5 now includes the non-finite-result exception. Original input still returns NOT_DETERMINED.
R6: source releases the temporary arb. Dynamic Julia probe pending.
Limits: original R1 and R2 still return LIMIT; this is the explicit N-D7 decision.

Part B: original findings.c rebuilt. Exact 5 at prec 2 gives 1 +/- 1/2 for class and norm.
Exact 3 to exponent -1 at prec 2 gives 1/4 +/- 1/8. Exact 2 to exponent -1 gives 1/2.
Simple hull radius 3; smallest hull radius 6. These numerical results are unchanged.
All four allocator probes at excessive precision give LIMIT, 0 calls, 0 bytes, release and INV.
F2 to F5 closure depends on the repaired text; it is read separately.

Builds: timeout 180 make -j2 BUILD=lanes/n-review1/build and its INV subdirectory completed, exit 0.
No files outside lanes/n-review1 were written.

Part A extension: the before-allocation precision promise is still false under INV=1.
partial_alloc.c uses a canonical partial ball with a large exact rational at 5.
At prec 2097153 all 7 tested functions at the real place and all 3 ring operations allocate
14 times, requesting 135160 bytes, before LIMIT. Release makes 0 calls. R5 is therefore OPEN
for the repair's complete contract; the original allocation-size abort is closed.
The original Julia leak probe ran: 20 calls, 0 indirectly lost bytes. Its old 1600-byte record is absent.
The process exits 99 for 50143 bytes in 56 runtime leak blocks; this is not a clean-process leak check.

Part C: own local oracle: 45006 checks, 0 mismatches; 256480 power points, 73216 split points,
14014 fractional-part points. At p=2 a canonical ball with v=-1, N=LONG_MAX overflows N-v
before returning LIMIT. UBSan exits 1. set_fball on H=6^33554433 and A=2^67108866 returns
LIMIT after the full valuation of H: the linker wrapper records returned valuation 33554433.
The unwrapped run used 16.770667 CPU seconds; the wrapped run used 7.501012 CPU seconds.

Part D: own text oracle: 5782 calls, 33344 checks, 0 mismatches. 2213 real round trips enclose.
The constrained printer on midpoint 2^99999 + 1/2, radius 2^99999, digits 1, times out at 170 s.
Both stored binary exponents are 100000, so M1-D6 admits the value.
Driver blocker: project and exp_at accept the new unit types and silently read their unused adele field.
For (5 ; 5 * [1]), project at 5 prints 0; exp_at prints exact 1. The documented domain rejects this type.
