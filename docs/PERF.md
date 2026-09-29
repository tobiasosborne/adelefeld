# adelefeld: performance floors, version 1.1

Date: 2026-09-28 (version 1.0: 2026-09-27). Method: the user-level skill `perf-bounds`. Draft 1 applied findings F1
to F6 of the design review (`reviews/astra-2026-09-27/review.md`); draft 2 applies round 2
(`reviews/astra-2026-09-27-r2/review.md`: F1 to F4, F6, N11). Several ratios of draft 0 are withdrawn; section 6
lists them.

**Change log of version 1.1** (2026-09-28):

- Findings P1 and P2 of work package 0.2 (`lanes/m0-sources/report.md`) applied in section 1a: every uops.info
  figure now says which column it is (measured or documented) and which instruction form, with the file and line on
  disk; the figures of the Zen 2 profile that version 1.0 took from memory (`imul r64, imm`, loads and stores) are
  now quoted.
- A second hardware profile, section 1b: the Intel Core i7-1365U laptop on which development and the harness of
  work package 0.5 run, with its model inputs quoted from uops.info (Alder Lake-P column, the closest one uops.info
  has) and fetched by `refs/fetch_intel.sh`.
- Section 3b: the floors of section 3 derived for that profile in the same manner, and the six word rows of the
  harness (run `2026-09-27T221654Z`, `quiet_machine: no`) set against them as provisional, with a ratio only where
  measurement and floor are of the same kind. Section 3 of version 1.0 is section 3a, unchanged in substance.

**Hardware profiles.** Every floor in cycles holds only for the profile whose model inputs it uses. A measurement is
compared only with the floors of the profile of the machine it ran on. Profile Zen 2 (sections 1a, 3a) is the
machine of the baseline `bench/baseline.c`; profile Intel (sections 1b, 3b) is the development laptop.

**All I/O floors below hold under a compulsory-I/O contract:** fresh dense inputs that must all be read, outputs
newly written in a stated layout, data in a stated cache level. Without it, cached results, exact zeros, early exits
and small tagged values can be faster than the floor. "PROVED" for a reading bound means: in the worst case over
dense inputs of the stated size, not for every call.

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

**How uops.info figures are cited.** A uops.info instruction page holds, per microarchitecture, a block of
measurements ("Latency operand i -> j", "Throughput: Measured (loop)", "Measured (unrolled)", "Port usage") and, for
some, a block "Documentation" with the vendor's figures. Throughput is given in cycles per instruction (a reciprocal
throughput: 0.25 means 4 per cycle). Unless a row says otherwise, this document uses the **measured (loop)**
throughput and the measured latency of the **register form** of the instruction. Citations are
`key:file:line` under `refs/src/`; `uops-zen2` holds seven pages fetched by `refs/fetch_sources.sh`, `uops-intel`
twenty pages fetched by `refs/fetch_intel.sh` (hashes in `refs/manifest-intel.sha256`). The seven pages common to
both keys are byte-identical. Each page covers all microarchitectures, so `uops-intel` also gives the Zen 2 figures
of the register forms that `uops-zen2` lacks.

### 1a. Profile Zen 2 (the baseline machine)

| Quantity | Value | Source |
|---|---|---|
| Machine | AMD Threadripper 3970X (Zen 2), 32 cores, seen through WSL2 | measured in the guest; not an audit of the physical machine |
| Vector instructions | AVX2, FMA, BMI2, ADX; no AVX-512 | measured |
| Clock | 3.7 GHz base, up to 4.5 GHz boost; the clock during the benchmark was not calibrated | vendor specification; all conversions to ns are conditional on this interval |
| Counters | `rdtscp` works in the guest; the hardware cycle event could not be opened (`perf_event_open` failed, errno 2, in the reviewer's probe) | measured in these runs; time-stamp ticks are not core cycles |
| `add r64, r64` | latency 1 cycle; 4 per cycle (measured 0.25 cycles per instruction, and the documentation says the same) | uops.info, Zen 2, register form: latency `uops-intel:ADD_01_R64_R64.html:1307-1308`, "Measured (loop): 0.25" `:1320`, documentation "Throughput: 0.25" `:1330`. The memory form `ADD_R64_M64`, which version 1.0 was checked against, measures 0.50 (2 per cycle, `uops-zen2:ADD_R64_M64.html:1716`) against a documented 0.25 (`:1726`) (finding P1) |
| `adc r64, r64` | carry-to-carry latency 1 cycle. Throughput is not used by any floor: measured about 3 per cycle for the register form, documented 4 per cycle | uops.info, Zen 2, register form: "Latency operand 3 -> 1: 1" (operand 3 is the flags) `uops-intel:ADC_11_R64_R64.html:1505-1506`; "Measured (loop): 0.35", "(unrolled): 0.33" `:1512-1513`; documentation 0.25 `:1522`. The memory form measures 0.50 (`uops-zen2:ADC_R64_M64.html:1908`), documented 0.25 (`:1918`) (finding P2) |
| `mul r64` (64x64 -> 128) | latency 3 cycles to the low half, 4 to the high half; 1 per cycle | uops.info, Zen 2: `uops-zen2:MUL_R64.html:1545-1549` (operand 2 is `RAX`, operand 3 is `RDX`), "Measured (loop): 1.00" `:1555`. Draft 0 had "one per 2 cycles", the figure for the previous chip generation; corrected (F1) |
| `imul r64, r64, imm` and `imul r64, r64` | latency 3 cycles; 1 per cycle | uops.info, Zen 2: `uops-zen2:IMUL_R64_R64_I8.html:1010-1011`, `:1017`; the two-operand form that the 14-cycle path below contains (`imul rdx,rdi`, bytes `48 0f af d7` in `bench/asm/chain_mul_run.s`): `uops-zen2:IMUL_R64_R64.html:1225-1226`, `:1232` (version 1.0: "from memory", and the immediate form) |
| `vpmuludq` (256 bit) | four products 32x32 -> 64 per instruction; latency 3; 1 per cycle | uops.info, Zen 2: `uops-zen2:VPMULUDQ_YMM_YMM_YMM.html:721-725`, `:729` |
| Loads and stores, vector model | two 32-byte loads and one 32-byte store per cycle, data in L1, aligned: 64 bytes read and 32 bytes written per cycle | each rate measured alone on Zen 2: 256-bit load 0.50 (`uops-intel:VMOVDQU_YMM_M256.html:900`), 256-bit store 1.00 (`uops-intel:VMOVDQU_M256_YMM.html:902`). MODEL assumption: that they hold together |
| Loads and stores, scalar model | two 8-byte loads and one 8-byte store per cycle | measured alone on Zen 2: `uops-intel:MOV_R64_M64.html:1188` (0.50), `uops-intel:MOV_M64_R64.html:1184` (1.00). MODEL assumption, as above; a different model, not to be mixed with the vector model |
| Caches | L1 32 KiB, L2 512 KiB per core | measured in the guest |
| glibc allocation | a request of `n` bytes takes `max(32, round16(n + 8))` bytes, for small requests | MODEL of glibc's small chunks, consistent with the measured usable sizes (`malloc(512)` reports 520 usable bytes); not a measurement of resident memory |
| FLINT 3.0.1 sizes | `fmpz` 8, `fmpq` 16, `arb` 48, `acb` 96, `nmod_t` 24 bytes | measured |

### 1b. Profile Intel (the development laptop; provisional)

Measured on 2026-09-28 with `lscpu`, `/proc/cpuinfo` and `/sys`. The machine runs Linux 6.8 directly, not in a
virtual machine.

| Quantity | Value | Source |
|---|---|---|
| Machine | 13th Gen Intel Core i7-1365U, family 6, model 186, stepping 3; one socket, 10 cores, 12 threads | measured (`lscpu`, `/proc/cpuinfo`) |
| Model name in the kernel | model 186 = `0xBA` is `INTEL_RAPTORLAKE_P`; the table gives no core name for it (it names "Raptor Cove" for the desktop model `0xB7` and "Golden Cove" for Alder Lake) | `/usr/src/linux-headers-6.8.0-117/arch/x86/include/asm/intel-family.h:172-180` (local file, not under `refs/`) |
| Two kinds of core | performance cores: cpus 0 to 3 (two cores with two threads each), up to 5.2 GHz, base 1.8 GHz; efficiency cores: cpus 4 to 11 (eight cores, one thread each), up to 3.9 GHz, base 1.3 GHz | measured: `/sys/devices/cpu_core/cpus` = `0-3`, `/sys/devices/cpu_atom/cpus` = `4-11`; `lscpu -e`; `cpufreq/base_frequency` |
| Core of the harness | **cpu 2, a performance core** (thread sibling: cpu 3). `bench/bench_word --run` pins to it by `sched_setaffinity` and records `affinity_actual_cpu: 2` | measured; `bench/results/2026-09-27T221654Z_word.txt` |
| Microarchitecture of the model inputs | uops.info has no Raptor Lake column. The rows below use its **Alder Lake-P** column (the performance core of Alder Lake, Golden Cove), the closest microarchitecture it covers. That the performance core of this chip has the same latencies and ports is an assumption, not a measurement; the measured add latency and clock below are consistent with it | uops.info section list, e.g. `uops-intel:MUL_R64.html:254` ("Alder Lake-P") |
| Vector instructions | AVX2, FMA, BMI2, ADX, AVX-VNNI; no AVX-512 | measured (`/proc/cpuinfo` flags) |
| Clock during the run | 3.70 to 3.80 GHz on cpu 2. A chain of dependent `add r64, r64` (`lanes/m0-amend/clockprobe.c`, latency 1 assumed) ran at 3.782 / 3.785 / 3.786 adds per ns (min / median / max of 9) just before the harness and 3.783 / 3.785 / 3.785 just after; the kernel's estimate `scaling_cur_freq` of cpu 2, sampled every 10 ms during the run, read 3.80 GHz in 188 and 3.70 GHz in 1 of the 189 busy samples. Governor `powersave`, turbo on. Conversion used below: 0.2632 to 0.2703 ns per cycle | measured; `lanes/m0-amend/run.log`, `lanes/m0-amend/freq_cpu2.txt` |
| Counters | the time-stamp counter runs at 2.688 ticks per ns, invariant; ticks are not core cycles. The hardware cycle event is closed to users (`perf_event_paranoid` = 4) | measured |
| Caches | cpu 2: L1d 48 KiB, L1i 32 KiB, L2 1.25 MiB (own core), L3 12 MiB (shared); efficiency cores: L1d 32 KiB, L2 2 MiB per cluster of four | measured (`/sys/devices/system/cpu/cpu*/cache`) |
| `add r64, r64`; `sub`, `cmp r64, r64` | latency 1; 0.20 cycles per instruction (5 per cycle); ports p0156B | `uops-intel:ADD_01_R64_R64.html:230-231, 238, 247`; `SUB_29_R64_R64.html:284-285, 298, 307`; `CMP_39_R64_R64.html:197-198, 202, 211` |
| `add r64, imm8` | latency **0** to the register (eliminated: 0 uops executed); 1 to the flags | `uops-intel:ADD_R64_I8.html:190-194, 201` |
| `adc r64, r64` | latency 1 from each input, the carry included; 0.50; p06 | `uops-intel:ADC_11_R64_R64.html:251-264, 271, 280` |
| `cmovb`, `cmovae` (`cmovnb`) `r64, r64` | latency 1 from the flags and from each register; 0.50; p06 | `uops-intel:CMOVB_R64_R64.html:215-222, 226, 238`; `CMOVNB_R64_R64.html:215-222, 226, 238` |
| `shr`, `shl r64, cl` | latency 1 to the register; 2 uops, both on p06; 1.00 | `uops-intel:SHR_R64_CL.html:260-261, 280, 284, 292`; `SHL_R64_CL.html:260-261, 280, 284, 292` |
| `mul r64` (64x64 -> 128) | latency 3 to the low half (`RAX`), 4 to the high half (`RDX`), from either input; 1 per cycle; ports p1 + p5 | `uops-intel:MUL_R64.html:260-276, 280, 292` |
| `imul r64, r64`; `imul r64, r64, imm8` | latency 3; 1 per cycle; p1 | `uops-intel:IMUL_R64_R64.html:232-233, 240, 249`; `IMUL_R64_R64_I8.html:191-192, 199, 208` |
| `vpmuludq` (256 bit) | latency 5; 0.50 (2 per cycle); p01 | `uops-intel:VPMULUDQ_YMM_YMM_YMM.html:179-183, 187, 196` |
| Loads | 64-bit and 256-bit loads: 0.33 each (3 per cycle); p23A | `uops-intel:MOV_R64_M64.html:216, 225`; `VMOVDQU_YMM_M256.html:207, 216` |
| Stores | 64-bit and 256-bit stores: 0.50 each (2 per cycle); p49 + p78 | `uops-intel:MOV_M64_R64.html:216, 225`; `VMOVDQU_M256_YMM.html:207, 216` |
| Loads and stores, vector model | three 32-byte loads and two 32-byte stores per cycle, data in L1: 96 bytes read and 64 bytes written per cycle | MODEL assumption: each rate above is measured alone |
| Loads and stores, scalar model | three 8-byte loads and two 8-byte stores per cycle: 24 bytes read and 16 written | MODEL assumption, as above; not to be mixed with the vector model |
| Ports | a port starts at most one uop per cycle; a uop listed as `p06` goes to one of ports 0 and 6 | MODEL assumption (the model behind uops.info's "computed from the port usage") |
| FLINT 3.0.1 sizes | as in 1a | measured on this machine (`conventions.md` section 12) |

## 2. Space floors

Rule: count only the states that the measured type admits (F2).

| Object, with its restriction | States | Floor |
|---|---|---|
| Integer centre, radius a fixed known integer `N` | `N` | `ceil(log2 N)` bits. PROVED |
| Integer centre and integer radius, `1 <= N <= B`, centre reduced | `B(B+1)/2` | `ceil(log2(B(B+1)/2))` bits; for `B = 2^62 - 1`: 123 bits, 16 bytes. PROVED |
| Integer radius `N`, centres on the fixed grid `(1/d) Z` with a shared `d` | `N d` for each `N` | add `log2 d` bits. PROVED. Not the count for fractional radii |
| Variable denominator, fractional radius, exact rational | unbounded unless bounds are stated | no floor independent of the bounds |
| Residue modulo `20!` | `20!` | 61.08 bits (entropy), 62 rounded up. PROVED |
| The same as 8 residues modulo `2^18, 3^8, 5^4, 7^2, 11, 13, 17, 19` | the same | the same 61.08 bits; 65 bits if each field is packed separately |

Ratios for the residue modulo `20!`, against the entropy of 61.08 bits: one 64-bit word x1.05; eight 32-bit fields
x4.19; eight 64-bit fields x8.38.

**Restricted layouts (MODEL, not information floors).** A real ball restricted to a 127-bit mantissa, a sign, a
32-bit exponent and an 8-bit radius code has `2^168` encodings, 21 bytes. A general `arb` has unbounded exponent and a
radius with its own exponent, so it is a different object; its 48 bytes against these 21 describe a choice of
features, not waste. Likewise "real ball as above plus one word centre with shared radius" is a 29-byte restricted
layout.

| Layout | Bytes | Compared with |
|---|---|---|
| `arb` + one word centre, radius in a shared context | 56 | the 29-byte restricted layout: x1.93 |
| `arb` + finite ball as two `fmpq`, all small | 80 | no ratio: two rationals allow independent denominators, and a general `arb` has other states than the restricted real layout |
| `arb` + finite ball `(A, H, d)` | to be measured | its own state count, to be derived with the layout (work package 0.4) |
| 4096-bit integer on the heap | 528-byte block for 512 bytes of limbs (allocator model) | x1.03 for the block alone; with the 16-byte `mpz` struct and the 8-byte `fmpz` handle, 552 bytes, x1.08. Estimates of payload plus selected overhead |

Sizes measured by `sizeof` do not include shared contexts or temporaries; a context is charged as its size divided
by the number of values that share it. An object of at most 64 bytes lies in one cache line only if it is aligned.

## 3. Time floors and the reference measurements

### 3a. Profile Zen 2

Measured 2026-09-27 with `bench/baseline.c` (sha256 `690ca1b2...08b1`, the source as committed in `4b143d9`; gcc 13.3
`-O2`, pinned to cpu 2; output in `bench/baseline_2026-09-27.txt`). The source is kept byte for byte as it was
measured; its known defects (an unused seed argument; the constant `b` may equal the modulus for other seeds) are
repaired in its replacement, work package 0.5, not here. What each row of that file measures (F4):

| Row | Kind of measurement |
|---|---|
| `nmod_add`, `nmod_mul` | dependent chain (output feeds the next input): a latency measurement |
| `n_gcd` | a chain mixed with the generation of the next inputs: neither pure latency nor throughput |
| `fmpz_*`, `arb_*` | the same call repeated on the same fixed operands into the same warm output. 15 trials; `fmpz_gcd` 9 trials. A call rate can be set against a throughput floor when the contracts match; here they do not (fixed operands, reused output, no stated accuracy) |

Floors, and ratios where the kinds match. Conversion: 0.222 to 0.270 ns per cycle, conditional on section 1a.

| Operation | Floor | Measured | Ratio |
|---|---|---|---|
| Word add, chain, 62-bit modulus | MODEL, restricted to kernels with an `add` on the chain: 1 cycle | 0.74 ns | x2.7 to x3.3, conditional on the clock interval. Weak floor: the correction step is not counted |
| Word multiply, chain, 62-bit modulus | MODEL, restricted to kernels with a high-half multiply on the chain: 4 cycles | 4.99 ns | x4.6 to x5.6, conditional. Weak floor |
| The same, restricted to the compiled FLINT kernel | MODEL, restricted: the dependency path read off the compiled loop is at least 14 cycles (two chained multiplies, add, adc, a dependent `imul`, subtraction; the `add rdx,1` counts 1 cycle, its Zen 2 latency, `uops-intel:ADD_R64_I8.html:1090-1091`) | 4.99 ns | x1.3 to x1.6, conditional on the clock interval and on the latencies of section 1a; the path does not include all corrections |
| Word gcd | PROVED: the inputs must be read; no numeric floor in cycles | 90.8 ns | no ratio |
| 4096-bit add | MODEL, vector I/O: read 1024 bytes, write 520: `max(16, 16.25) = 16.25` cycles per result (17 if each result needs whole store instructions of its own). MODEL, restricted to ripple carry: 64 cycles | 26.9 ns | no ratio: contracts do not match |
| 4096 x 4096-bit multiply | MODEL, vector I/O: 32 cycles. Scalar stores: 128 cycles | 1559 ns | no ratio |
| 8192 by 4096-bit reduction | MODEL, vector I/O: read 1536 bytes: 24 cycles. Scalar reads: 96 | 2840 ns | no ratio |
| 4096-bit gcd | MODEL, vector I/O: 16 cycles. Scalar reads: 64 | 16480 ns | no ratio |
| 128-bit real ball add, multiply | not yet defined: the problem needs a stated accuracy, exponent range and radius rule | 36.8 ns, 20.9 ns | no ratio |

The I/O floors say only that an operation cannot be faster than moving its data. They do not say that the
arithmetic can come near them.

REFERENCE lines: multiplication of `n`-bit integers in `O(n log n)` (Harvey and van der Hoeven, Annals of
Mathematics 193 (2021) 563-617; on disk as `hvh-mult`, title, authors and journal at `hvh-mult:nlogn.txt:1-7`);
division in `O(M(n))` and gcd in `O(M(n) log n)` for a multiplication cost `M(n)` (from memory; sources to be
fetched). Montgomery or Barrett reduction is an example of an algorithm with about three multiplies; it is not known
to be optimal.

### 3b. Profile Intel (provisional)

The six word rows of the harness of work package 0.5 (`bench/bench_word.c`), run once on 2026-09-28 at 00:16 local
time: `bench/results/2026-09-27T221654Z_word.txt` (`quiet_machine: no`: other agents were running on the laptop; gcc
13.3.0 `-O2 -g`, FLINT 3.0.1, GMP 6.3.0, modulus `2^62 - 57`, 15 trials, pinned to cpu 2, `CLOCK_MONOTONIC_RAW`).
The table gives the median in ns and in cycles at 3.70 to 3.80 GHz (section 1b). The earlier run
`2026-09-27T202944Z_word.txt` differs by at most 0.3 % in each median. **These rows are provisional**: one run on a
shared machine, a clock measured next to the run and not inside it, and model inputs borrowed from Alder Lake-P.

Floors of two strengths, as in 3a: (a) restricted to kernels that contain the named instruction per result; (b)
restricted to the compiled FLINT kernel, read off its instructions. A chain is set against a latency floor, a batch
against a throughput floor. For (b), an instruction whose latency or port is not quoted in section 1b (`lea`, `mov`,
the fused `cmp`/`jne` of the loop) is counted as 0 cycles and 0 uops, which can only lower the floor.

| Row | Kind | Floor | Measured (median) | Ratio |
|---|---|---|---|---|
| `nmod_add` chain | latency | (a) MODEL, restricted to kernels with an `add` on the chain: 1 cycle. (b) MODEL, restricted to the compiled kernel (`bench/asm/chain_add_run.s`, loop `26d0`-`26eb`): the value `rax` goes through `sub r9,rax`, `cmp rcx,r9`, `cmovb rax,r8`: 3 cycles | 0.8116 ns; 3.00 to 3.08 cycles | (a) x3.0 to x3.1; (b) x1.00 to x1.03 |
| `nmod_mul` chain | latency | (a) MODEL, restricted to kernels with a high-half multiply on the chain: 4 cycles. (b) MODEL, restricted to the compiled kernel (`bench/asm/chain_mul_run.s`, loop `2730`-`2777`): `mul r9` (4 to `rdx`), `mul rdx` (4 to `rdx`; its low half at 3 feeds `add rax,rsi`, which sets the carry by cycle 8), `adc rdx,rcx` (1), `add rdx,0x1` (0), `imul rdx,rdi` (3), `sub rsi,rdx` (1), `cmp rax,rsi` (1), `cmovb rsi,rdx` (1), `sub rax,rdi` (1), `cmovae rsi,rax` (1), `shr rsi,cl` (1): 18 cycles | 4.9639 ns; 18.37 to 18.86 cycles | (a) x4.6 to x4.7; (b) x1.02 to x1.05 |
| `n_mulmod2_preinv` chain | latency | (a) as above: 4 cycles. No floor (b): the loop calls `n_ll_mod_preinv` in the library, whose code is not in the saved file | 5.9580 ns; 22.04 to 22.64 cycles | (a) x5.5 to x5.7 |
| `nmod_add` batch, 1024 pairs in L1 | throughput | (a) MODEL, restricted to kernels with one `add` per result: 0.20 cycles per result. (b) MODEL, restricted to the compiled kernel (`batch_add_run`, loop `27b8`-`27e2` of `bench/bench_word`, read with `objdump`): per result 2 loads (0.67 cycles), 1 store (0.5), and `sub`, `sub`, `cmp` (p0156B) and `cmovae` (p06): 4 uops on 5 ports, 0.8 cycles; the loop counter `add rax,0x1` is eliminated. 0.8 cycles per result | 0.5351 ns; 1.98 to 2.03 cycles | (a) x9.9 to x10.2; (b) x2.5 |
| `nmod_mul` batch, 1024 pairs in L1 | throughput | (a) MODEL, restricted to kernels with one full 64x64 -> 128 product per result: 1 cycle. (b) MODEL, restricted to the compiled kernel (`batch_mul_run`, loop `2f78`-`2fd3`): on p06 `shl` (2 uops), `adc`, `cmovb`, `cmovae`, `shr` (2 uops): 7 uops on 2 ports, 3.5 cycles; on p1 two `mul` and one `imul`: 3 cycles; all 17 ALU uops on 5 ports: 3.4 cycles. 3.5 cycles per result | 1.4690 ns; 5.44 to 5.58 cycles | (a) x5.4 to x5.6; (b) x1.55 to x1.59 |
| `n_mulmod2_preinv` batch, 1024 pairs in L1 | throughput | (a) as above: 1 cycle. No floor (b): library call | 2.9162 ns; 10.79 to 11.08 cycles | (a) x10.8 to x11.1 |

What the rows say, and what they do not. The two compiled chains run at their dependency paths: the ratios (b) of
x1.00 to x1.05 leave no room for a faster schedule of the same instructions, only for other instructions. The
multiply-chain floor (b) first came out at 19 cycles with `add rdx,0x1` counted as 1 cycle; the measurement of
18.4 to 18.9 cycles contradicted it, and the Alder Lake-P page shows that this instruction is eliminated (latency
0). A measurement below a floor is a refutation of the model, not a fast kernel. The batch rows are a factor 1.6
(multiply) and 2.5 (add) above floors (b), which count only ports; the front end, which issues a bounded number of
uops per cycle, is not in the model, because no source for its width is on disk.

The 4096-bit rows of 3a were not measured on this machine. Their I/O floors in this profile, derived as in 3a with
the models of section 1b:

| Operation | Floor, profile Intel | The same in profile Zen 2 (3a) |
|---|---|---|
| 4096-bit add: read 1024 bytes, write 520 | MODEL, vector I/O: `max(1024/96, 520/64) = max(10.7, 8.1) = 10.7` cycles. MODEL, restricted to ripple carry: 64 cycles (`adc` latency 1) | 16.25; 64 |
| 4096 x 4096-bit multiply: read 1024, write 1024 | MODEL, vector I/O: `max(10.7, 16) = 16` cycles. Scalar: `max(1024/24, 1024/16) = 64` cycles | 32; 128 |
| 8192 by 4096-bit reduction: read 1536, write 512 | MODEL, vector I/O: `max(16, 8) = 16` cycles. Scalar: `1536/24 = 64` cycles | 24; 96 |
| 4096-bit gcd: read 1024, write 512 | MODEL, vector I/O: 10.7 cycles. Scalar: 42.7 cycles | 16; 64 |

## 4. Floors for the operations still to be written

Let `b = ceil(n/8)` for an `n`-bit modulus, `k` the number of coprime blocks below `2^32`, `L = D M` the length of a
finite function, `W` the bytes per stored complex coefficient. Vector I/O model of section 1a; set-up of contexts is
a separate row, amortised only over a stated number of uses. (F6) In profile Intel (section 1b) divide bytes read
by 96 instead of 64 and bytes written by 64 instead of 32, and the `vpmuludq` row becomes `k/8` cycles (two per
cycle). The example below becomes `max(512/96, 512/64) = 8` cycles for both conversions.

| Operation | Floor | REFERENCE (not a floor) |
|---|---|---|
| Global integer to `k` residues | PROVED, worst case over dense inputs: `n` bits read, `k` residues written. MODEL: `max(b/64, 4k/32)` cycles | remainder tree, `O(M(n) log k)` |
| `k` residues to the global integer | PROVED, worst case over dense inputs: linear in `n`. MODEL: `max(4k/64, b/32)` cycles | recombination tree, `O(M(n) log k)` |
| Finite Fourier transform, length `L`, dense | PROVED, worst case: `L` coefficients read and written. MODEL: `L W / 32` cycles | direct `O(L^2)`; fast transform `O(L log L)` operations. No `L log L` floor is claimed |
| Partial rational reconstruction, `n` bits | PROVED, worst case over non-degenerate bounds: linear in `n` | continued fractions; half-gcd `O(M(n) log n)` |
| Rational in a full adelic ball | PROVED: read the endpoints, write the candidate | two integer divisions with remainder |
| Product of `k` residues below `2^32`, AVX2, kernels using the native 32x32 product | MODEL, restricted: `ceil(k/4)` `vpmuludq` for one vector of `k`, so at least `k/4` cycles, before any reduction | the reduction, the shuffles and the corrections are to be added from the actual kernel |
| Sum of `k` residues, the same layout | MODEL: I/O, `k/8` cycles with 32-bit fields, `k/4` with 64-bit fields | |
| Text dump of `B` bytes; parse of a valid text of `B` bytes | PROVED: `B` bytes written or read (an invalid text can be rejected at a prefix) | conversion to decimal has its own cost |
| Idele class product at a shared modulus | one unit-residue product and one real ball product; no numeric floor until the real ball row is defined | |
| Function at the real place, dense `b`-bit mantissa written out | MODEL: I/O floor for `ceil(b/8)` output bytes plus exponent and radius. Holds for the worst case; `exp(0) = 1` is short at every precision. No arithmetic floor | `arb`'s algorithms (no general cost statement is made here) |
| p-adic `exp`, `log`, residue written out padded to `n` digits | MODEL: I/O floor for the output bytes. The number of possible outputs on the principal disc is `p^(n-c)`, `c = 1` (odd `p`) or 2: an information count, not a cost per call | FLINT's `padic_exp`, `padic_log` (rectangular and balanced variants) |
| Real roots of an integer polynomial `g` of degree `n`, balls of accuracy `prec` with the certificate of `solvers.md` P3.8 (`adf_roots_real`; lane r-slice1, `docs/design/real-roots.md` section 6) | PROVED: read `g`, write the `n` balls. MODEL, restricted to results certified by exact signs at the end points (decision 3 of r-slice1): the exact sign of `g` at both end points of each ball that is not exact, one for an exact ball; measured as `eval2` of `bench/bench_roots_real.c` on the end points the implementation outputs, which is an upper estimate of this floor (shortest end points can be shorter, by up to half after a last quadratic step) | REFERENCE: the Descartes method, `O~(n^4 tau^2)` bit operations (`sagraloff-mehlhorn:arxivfinal.tex:302`, a citation of a paper not on disk). Measured 2026-09-29, a shared laptop (load 3.5), `taskset -c 2`, median of 5 (`lanes/r-slice1/runs/bench_after.txt`): time / `eval2` is 20 to 29 for the roots `2^e`, `2^e + 1` (`e` = 600 to 100000; 0.014 s at `e = 100000`, 111.5 s at `e = 1500` before), 6.0 to 9.5 for `X^2 - 2` (`prec` 53 to `2^21`), 5.5 for `X^5 - X - 1` at `2^16`, 21 to 27 for Wilkinson 20 and a Mignotte polynomial, 76 to 317 for `n` roots near `2^300` (`n` = 16 to 2), 262 to 747 for the cluster `2^e + 1/3`, `2^e + 2/3` (`e` = 3000 to 30000: the descent of the Descartes tree). A call rate against a floor of the same kind (one call, one input). The distance from the floor is at least these ratios |

Each row for a function records: prime size, input valuation, absolute or relative accuracy, branch, and which
context data are reused.

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
result. Minimum, median and maximum are reported. A result converted to cycles also states the hardware profile and
how the core clock was obtained: the time-stamp rate that the harness records is not the core clock (section 1b), so
a clock probe next to the run, or a cycle counter where one is open, is needed. `bench/baseline.c` predates this
contract and is kept as a dated reference; its replacement is work package 0.5.
