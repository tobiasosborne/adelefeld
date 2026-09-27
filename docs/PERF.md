# adelefeld: performance floors, draft 1

Date: 2026-09-27. Method: the user-level skill `perf-bounds`. Draft 1 applies findings F1 to F6 of the design review
(`reviews/astra-2026-09-27/review.md`). Several ratios of draft 0 are withdrawn; section 6 lists them.

**The metric.** A measured time or size counts only as a ratio to a derived lower bound (a floor). A ratio is
reported only when

1. the floor is for a stated problem (inputs, required accuracy, what is cached, what must be written), and
2. the measurement is of the same kind as the floor: a dependent chain against a latency floor, a batch of
   independent operations against a throughput floor.

Kinds of number, kept apart throughout:

- **PROVED**: a theorem in a stated model (counting states, reading the input).
- **MODEL**: follows from hardware assumptions; valid while they hold. "MODEL, restricted to X" means the floor holds
  only for implementations that use the algorithm X.
- **REFERENCE**: the cost of a known algorithm; an upper bound on the optimum, never a floor.

Nothing of adelefeld is implemented. Every measurement below is of an existing FLINT or GMP primitive.

## 1. Hardware assumptions

| Quantity | Value | Source |
|---|---|---|
| Machine | AMD Threadripper 3970X (Zen 2), 32 cores, seen through WSL2 | measured in the guest; not an audit of the physical machine |
| Vector instructions | AVX2, FMA, BMI2, ADX; no AVX-512 | measured |
| Clock | 3.7 GHz base, up to 4.5 GHz boost; the clock during the benchmark was not calibrated | vendor specification; all conversions to ns are conditional on this interval |
| Counters | `rdtscp` works in the guest; performance counters for core cycles were not available | measured; time-stamp ticks are not core cycles |
| `add r64` | latency 1 cycle; 4 per cycle | uops.info (Zen 2), as read by the reviewer; not re-read by us |
| `adc r64` | carry-to-carry latency 1 cycle | same |
| `mul r64` (64x64 -> 128) | latency 3 cycles to the low half, 4 to the high half; 1 per cycle | same. Draft 0 had "one per 2 cycles", the figure for the previous chip generation; corrected (F1) |
| `vpmuludq` (256 bit) | four products 32x32 -> 64 per instruction; latency 3; 1 per cycle | same |
| Loads and stores, vector model | two 32-byte loads and one 32-byte store per cycle, data in L1, aligned: 64 bytes read and 32 bytes written per cycle | MODEL assumption, from memory |
| Loads and stores, scalar model | two 8-byte loads and one 8-byte store per cycle | MODEL assumption, from memory; a different model, not to be mixed with the vector model |
| Caches | L1 32 KiB, L2 512 KiB per core | measured in the guest |
| glibc allocation | a request of `n` bytes takes `max(32, round16(n + 8))` bytes, for small requests | measured |
| FLINT 3.0.1 sizes | `fmpz` 8, `fmpq` 16, `arb` 48, `acb` 96, `nmod_t` 24 bytes | measured |

## 2. Space floors

Rule: count only the states that the measured type admits (F2).

| Object, with its restriction | States | Floor |
|---|---|---|
| Integer centre, radius a fixed known integer `N` | `N` | `ceil(log2 N)` bits. PROVED |
| Integer centre and integer radius, `1 <= N <= B`, centre reduced | `B(B+1)/2` | `ceil(log2(B(B+1)/2))` bits; for `B = 2^62 - 1`: 123 bits, 16 bytes. PROVED |
| The same with a fixed shared denominator `d` | `N d` for each `N` | add `log2 d` bits. PROVED |
| Variable denominator, fractional radius, exact rational | unbounded unless bounds are stated | no floor independent of the bounds |
| Residue modulo `20!` | `20!` | 61.08 bits, 62 rounded up. PROVED |
| The same as 8 residues modulo `2^18, 3^8, 5^4, 7^2, 11, 13, 17, 19` | the same | the same 61.08 bits; 65 bits if each field is packed separately |

Ratios for the residue modulo `20!`: one 64-bit word x1.05; eight 32-bit fields x4.19; eight 64-bit fields x8.38.

**Restricted layouts (MODEL, not information floors).** A real ball restricted to a 127-bit mantissa, a sign, a
32-bit exponent and an 8-bit radius code has `2^168` encodings, 21 bytes. A general `arb` has unbounded exponent and a
radius with its own exponent, so it is a different object; its 48 bytes against these 21 describe a choice of
features, not waste. Likewise "real ball as above plus one word centre with shared radius" is a 29-byte restricted
layout.

| Layout | Bytes | Compared with |
|---|---|---|
| `arb` + one word centre, radius in a shared context | 56 | the 29-byte restricted layout: x1.93 |
| `arb` + finite ball as two `fmpq`, all small | 80 | not comparable with the 29-byte layout (different states); against a 16-byte two-integer finite part + 21-byte real part: x2.16 |
| 4096-bit integer on the heap | 528-byte block for 512 bytes of limbs | x1.03 for the block alone; with the 16-byte `mpz` struct and the 8-byte `fmpz` handle, 552 bytes, x1.08 |

Sizes measured by `sizeof` do not include shared contexts or temporaries; a context is charged as its size divided
by the number of values that share it. An object of at most 64 bytes lies in one cache line only if it is aligned.

## 3. Time floors and the reference measurements

Measured 2026-09-27 with `bench/baseline.c` (gcc 13.3 `-O2`, pinned to cpu 2; output in
`bench/baseline_2026-09-27.txt`). What each row of that file measures (F4):

| Row | Kind of measurement |
|---|---|
| `nmod_add`, `nmod_mul` | dependent chain (output feeds the next input): a latency measurement |
| `n_gcd` | a chain mixed with the generation of the next inputs: neither pure latency nor throughput |
| `fmpz_*`, `arb_*` | the same call repeated on the same operands into the same output: a call rate. 15 trials; `fmpz_gcd` 9 trials |

Floors, and ratios where the kinds match. Conversion: 0.222 to 0.270 ns per cycle, conditional on section 1.

| Operation | Floor | Measured | Ratio |
|---|---|---|---|
| Word add, chain, 62-bit modulus | MODEL: one `add` on the chain, 1 cycle | 0.74 ns | x2.7 to x3.3. Weak floor: the correction step is not counted |
| Word multiply, chain, 62-bit modulus | MODEL: one high-half multiply on the chain, 4 cycles | 4.99 ns | x4.6 to x5.6. Weak floor |
| The same, restricted to the compiled FLINT kernel | MODEL, restricted: the dependency path read off the compiled loop is at least 14 cycles (two chained multiplies, add, adc, a dependent `imul`, subtraction) | 4.99 ns | x1.3 to x1.6 |
| Word gcd | PROVED: the inputs must be read; no numeric floor in cycles | 90.8 ns | no ratio |
| 4096-bit add | MODEL, vector I/O: read 1024 bytes, write 520: `max(16, 17) = 17` cycles. MODEL, restricted to ripple carry: 64 cycles | 26.9 ns (call rate) | no ratio: the measurement is a call rate |
| 4096 x 4096-bit multiply | MODEL, vector I/O: 32 cycles. Scalar stores: 128 cycles | 1559 ns (call rate) | no ratio |
| 8192 by 4096-bit reduction | MODEL, vector I/O: read 1536 bytes: 24 cycles. Scalar reads: 96 | 2840 ns (call rate) | no ratio |
| 4096-bit gcd | MODEL, vector I/O: 16 cycles. Scalar reads: 64 | 16480 ns (call rate) | no ratio |
| 128-bit real ball add, multiply | not yet defined: the problem needs a stated accuracy, exponent range and radius rule | 36.8 ns, 20.9 ns (call rate) | no ratio |

The I/O floors say only that an operation cannot be faster than moving its data. They do not say that the
arithmetic can come near them.

REFERENCE lines: multiplication of `n`-bit integers in `O(n log n)` (Harvey and van der Hoeven, Annals of
Mathematics 193 (2021) 563-617; existence of the paper checked by the reviewer); division in `O(M(n))` and gcd in
`O(M(n) log n)` for a multiplication cost `M(n)` (from memory; sources to be fetched). Montgomery or Barrett
reduction is an example of an algorithm with about three multiplies; it is not known to be optimal.

## 4. Floors for the operations still to be written

Let `b = ceil(n/8)` for an `n`-bit modulus, `k` the number of coprime blocks below `2^32`, `L = D M` the length of a
finite function, `W` the bytes per stored complex coefficient. Vector I/O model of section 1; set-up of contexts is
a separate row, amortised only over a stated number of uses. (F6)

| Operation | Floor | REFERENCE (not a floor) |
|---|---|---|
| Global integer to `k` residues | PROVED: all `n` bits are read, `k` residues written. MODEL: `max(b/64, 4k/32)` cycles | remainder tree, `O(M(n) log k)` |
| `k` residues to the global integer | PROVED: linear in `n`. MODEL: `max(4k/64, b/32)` cycles | recombination tree, `O(M(n) log k)` |
| Finite Fourier transform, length `L` | PROVED: `L` coefficients read and written. MODEL: `L W / 32` cycles | direct `O(L^2)`; fast transform `O(L log L)` operations. No `L log L` floor is claimed |
| Partial rational reconstruction, `n` bits | PROVED: linear in `n` | continued fractions; half-gcd `O(M(n) log n)` |
| Rational in a full adelic ball | PROVED: read the endpoints, write the candidate | two integer divisions with remainder |
| Product of `k` residues below `2^32`, AVX2, kernels using the native 32x32 product | MODEL, restricted: at least `ceil(k/4)` `vpmuludq`, so `k/4` cycles, before any reduction | the reduction, the shuffles and the corrections are to be added from the actual kernel |
| Sum of `k` residues, the same layout | MODEL: I/O, `k/8` cycles with 32-bit fields, `k/4` with 64-bit fields | |
| Text dump or parse of `B` bytes | PROVED: `B` bytes written or read | conversion to decimal has its own cost |
| Idele class product at a shared modulus | one unit-residue product and one real ball product; no numeric floor until the real ball row is defined | |
| Elementary function at the real place, `p` bits of accuracy | PROVED: `p` output bits written. No useful floor known | `arb`'s algorithms; bit-burst and binary splitting, quasi-linear in `p` (from memory) |
| p-adic `exp`, `log` to precision `p^n` | PROVED: `n log2 p` output bits written | FLINT's `padic_exp`, `padic_log` (rectangular splitting, balanced variants) |

Example: `n = 4096`, `k = 128`: both conversions have an I/O floor of 16 cycles.

## 5. Hypotheses for the design (to be tested, not conclusions)

(F5) The measurements of section 3 are of primitives. They suggest questions; they do not settle them.

1. **Is the gcd in the tight rule the main cost of a tight product?** A word gcd took 90.8 ns against 4.99 ns for a
   modular product, but these are different workloads. In structured cases the gcd is trivial: at a shared integer
   radius with unit residues it is the radius itself. To be measured on complete tight and scaled kernels, with
   typical and adversarial radii, including the cost of normalising rationals.
2. **Does the scaled policy pay?** It saves gcds in products and pays for the rational scale and for alignment in
   sums. To be measured on the same expression in both policies.
3. **Vector kernels.** Blocks below `2^32` are a reasonable first restriction for AVX2, not a necessity: wider
   products can be assembled from partial products, and blocks below `2^50` can use floating-point multiplication
   with a proof of the rounding.
4. **Real balls.** `arb` is kept. A special small real ball is considered only after a complete operation has been
   profiled, its accuracy contract is written, and a matched floor exists. No gain is predicted.

## 6. Withdrawn from draft 0

| Statement of draft 0 | Reason |
|---|---|
| Word multiply at x6.2 to x7.5 of floor | the floor used the low-half latency; and against the compiled kernel the ratio is x1.3 to x1.6 |
| Word gcd at x340 to x410 | the "1 cycle" floor was not a floor in cycles |
| 4096-bit add at x1.6 to x1.9 | call rate compared with a chain floor; the chain floor holds only for ripple carry |
| Multiply, reduce, gcd at 4096 bits: x45 to x1160 | call rates compared with scalar I/O floors |
| Real ball add x68 to x83, multiply x13 to x16 | floors assumed an algorithm and no accuracy contract; multiply throughput was wrong |
| "An order of magnitude can be gained on small balls" | rested on the two rows above |
| "The local form must use moduli below 2^32" | too strong; see hypothesis 3 |
| "Shared radius halves the space" | true only for the finite payload with shared scale and modulus |

## 7. Benchmark contract for new rows

Every new benchmark states: latency chain, independent batch, or call rate; the operand families and their actual
bit lengths; the seed; warm or allocating; affinity; compiler, flags and library versions; the clock source. Results
are consumed outside the timed region. When a floor is a dependency path, the compiled loop is saved next to the
result. Minimum, median and maximum are reported. `bench/baseline.c` predates this contract and is kept as a dated
reference; its replacement is work package 0.5.
