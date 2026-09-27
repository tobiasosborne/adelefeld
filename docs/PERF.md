# adelfeld: performance floors, draft 0

Date: 2026-09-27. Method: the user-level skill `perf-bounds`. **The metric: a measured time or size counts only as a
ratio to a derived lower bound (a floor), with the floor's model and assumptions stated.** Nothing of adelfeld is
implemented, so every measurement below is of an existing FLINT or GMP primitive that the milestone-1 types will be
built from. They show how much room there is, and where.

Kinds of number, kept apart throughout:

- **PROVED**: a theorem in a stated model (counting states, reading the input).
- **MODEL**: follows from hardware assumptions; valid while they hold.
- **BEST-KNOWN**: the cost of the best known algorithm; an upper bound on the optimum, never a floor.

## 1. Hardware assumptions

| Quantity | Value | Source |
|---|---|---|
| Machine | AMD Threadripper 3970X (Zen 2), 32 cores, under WSL2 | measured (`floors.py probe`) |
| Vector instructions | AVX2, FMA, BMI2, ADX; no AVX-512 | measured |
| Clock | 3.7 to 4.5 GHz, so one cycle is 0.222 to 0.270 ns | nominal 3693 MHz measured; boost value typical, verify |
| Cycle counters | not available under this WSL2 kernel | measured; every ratio below is therefore a range |
| `add`, `adc` 64-bit | latency 1 cycle | typical, verify |
| `mul` 64x64 -> 128 | latency 3 cycles, one per 2 cycles in throughput on Zen 2 | typical, verify |
| Loads and stores | 2 loads and 1 store per cycle | typical, verify |
| Caches | L1 32 KiB, L2 512 KiB per core | measured |
| glibc allocation | `max(32, round16(n + 8))` bytes for a request of `n` | measured |
| FLINT 3.0.1 sizes | `fmpz` 8, `arb` 48, `acb` 96, `nmod_t` 24 bytes | measured |

## 2. Space floors for the milestone-1 types

Counting states (PROVED). A profinite ball with integer centre reduced modulo an integer radius `N` of at most `n`
bits: the radius ranges over about `2^n` values and, given the radius, the centre over `N` values.

| Object | Information floor | Remark |
|---|---|---|
| Centre, when the radius is shared by many values (held in a context) | `log2 N` bits | `N < 2^62`: one word, 8 bytes |
| Centre and radius, each value carrying its own radius | about `2 log2 N` bits | `N < 2^62`: 16 bytes |
| The same with a denominator `d` (fractional centre) | add `log2 d` bits | |
| Real ball at 128 bits | about 168 bits = 21 bytes | model: 127 mantissa + sign + 32 exponent + about 8 radius bits; the last two figures are assumptions |
| Adele = real ball at 128 bits + centre, shared radius `N < 2^62` | about 29 bytes | |
| Residue modulo `20!` | 61.08 bits: one word | |
| The same as 8 residues modulo `2^18, 3^8, 5^4, 7^2, 11, 13, 17, 19` | the same 61.08 bits | 65 bits if each field is packed separately; 32 bytes as 8 x 32-bit; 64 bytes as 8 x 64-bit |

Practical tiers for the two natural layouts of a small adele (real part 128 bits, `N < 2^62`):

| Layout | Bytes | Ratio to the 29-byte floor |
|---|---|---|
| Packed by hand: 2 limbs mantissa, 4 bytes exponent, 1 byte radius, 1 word centre | 29, rounded to 32 | x1.10 |
| `arb` + one word centre, radius in a shared context | 48 + 8 = 56 | x1.93 |
| `arb` + centre and radius as two `fmpq` (four `fmpz`, all small) | 48 + 32 = 80 | x2.76 (against the 29-byte line; x2.2 against a 37-byte own-radius floor) |
| One cache line | 64 | x2.21 |

Reading: an adele with a word-sized modulus fits in one cache line in every layout except the fully general one.
When centre or radius exceeds 62 bits, `fmpz` allocates (528 bytes of heap for 512 bytes of limbs at 4096 bits,
x1.08), and the ratio tends to 1 as the size grows.

## 3. Time floors and the reference measurements

Measured 2026-09-27 with `bench/baseline.c` (gcc 13.3 `-O2`, pinned to cpu 2, minimum of 15 trials; output in
`bench/baseline_2026-09-27.txt`). Floors converted at 0.222 to 0.270 ns per cycle.

| Object | Operation | Size | Floor (kind: model) | Floor value | Measured (min) | Ratio |
|---|---|---|---|---|---|---|
| residue, shared word modulus | add, dependent chain | 62 bit | MODEL: one `add` on the chain | 1 cycle = 0.22-0.27 ns | 0.74 ns (`nmod_add`) | x2.7-x3.3 |
| residue, shared word modulus | multiply, dependent chain | 62 bit | MODEL: one 64x64 multiply on the chain | 3 cycles = 0.67-0.81 ns | 4.99 ns (`nmod_mul`) | x6.2-x7.5 |
| two word radii | gcd | 62 bit | PROVED: read the input; no better floor known | 1 cycle | 90.8 ns (`n_gcd`) | x340-x410 |
| integer | add | 4096 bit | MODEL: carry chain, 1 cycle per limb | 64 cycles = 14.2-17.3 ns | 26.9 ns (`fmpz_add`) | x1.6-x1.9 |
| integer | multiply | 4096 x 4096 bit | MODEL: write 128 limbs at one store per cycle | 128 cycles = 28.4-34.6 ns | 1559 ns (`fmpz_mul`) | x45-x55 |
| integer | reduce | 8192 by 4096 bit | MODEL: read 192 limbs at two loads per cycle | 96 cycles = 21.3-25.9 ns | 2840 ns (`fmpz_mod`) | x110-x133 |
| two radii | gcd | 4096 bit | MODEL: read 128 limbs | 64 cycles = 14.2-17.3 ns | 16480 ns (`fmpz_gcd`) | x950-x1160 |
| real ball | add | 128 bit | MODEL: two-limb carry chain | 2 cycles = 0.44-0.54 ns | 36.8 ns (`arb_add`) | x68-x83 |
| real ball | multiply | 128 bit | MODEL: three 64x64 multiplies at one per 2 cycles | 6 cycles = 1.33-1.62 ns | 20.9 ns (`arb_mul`) | x13-x16 |

Best-known reference lines (not floors): modular multiplication by Montgomery or Barrett reduction uses about three
multiplies, about 9 cycles on a chain, so `nmod_mul` is at about x2 of best-known; multiplication and division of
`n`-bit integers cost `O(n log n)` by the best known algorithm, with only the linear floor proved; gcd is
quasi-linear by the best known algorithm. (Citations from memory; verify before quoting.)

What the floors ignore: function-call overhead, the branch for the conditional subtraction, allocation, rounding and
radius bookkeeping in `arb`, normalisation of `fmpz` results.

## 4. What the numbers say about the design

1. **The gcd in the tight precision rule is the expensive step.** The tight product rule of `SPEC.md` section 4.2
   needs `gcd(aM, bN, NM)`. At word size a gcd costs about 18 modular multiplications (90.8 ns against 4.99 ns), at
   4096 bits about 10 multiplications. A multiplication that tracks the tight radius is therefore dominated by
   bookkeeping. The plan provides two precision policies: **tight** (the radius is recomputed, with gcds) and
   **capped** (the radius is a fixed modulus held in a context, as FLINT's `padic` does with its precision; the result
   is a valid but possibly larger ball, and the product is one modular multiplication).
2. **Small values are where the overhead is.** Large integers are already within x2 of their floor for addition.
   Real balls at 128 bits are at x13 to x83: `arb` pays for generality (arbitrary exponents, arbitrary precision) on
   every call. A fixed-precision small ball kernel is the place where a new implementation can gain an order of
   magnitude; whether it is worth writing is a decision for milestone 1b, after the generic version exists.
3. **The local form must use moduli below 2^32 to vectorise on this machine.** AVX2 has no 64x64 -> 128 multiply;
   it has 32x32 -> 64 in four lanes. So the tuple form stores prime-power residues below `2^32` (or below `2^50` with
   the floating-point multiply trick), four to a vector. Floors for the vector kernels are to be derived when the
   kernels are specified (instruction count x throughput, and the memory-bandwidth rung for long tuples).
4. **Shared radius halves the space and removes the radius arithmetic.** Both the space table and point 1 favour a
   context that holds the modulus, with per-value radii only in the tight policy.

## 5. Rows still to be derived

Conversion between global and local form (CRT); the finite Fourier transform of length `D M`; rational
reconstruction; text output and parsing; the vector kernels of point 3; the idele class product. Each gets a row in
section 3 before its implementation is called finished.
