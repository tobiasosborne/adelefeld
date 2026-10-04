# Lane f-review12: bug hunt through the second catalogue group (lane f-slice13, `src/catalogue.c`)

Your task is to REFUTE, not to confirm. Lane f-slice13 (codex `gpt-6.1-sol`; you are of another model family on
purpose) landed on 2026-10-04, not reviewed: `include/adelefeld/catalogue.h` (101 lines), `src/catalogue.c`
(270 lines): `adf_fball_binom`, `adf_fball_binom_tight` (binomial coefficients of a profinite integer);
`adf_ucoset_profpow`, `adf_ucoset_profpow_coarse`, `adf_ucoset_profpow_fine` (the profinite power of a unit
coset); `adf_idclass_cyclo_exp_u`, `adf_idclass_cyclo_exp_uinv` (the cyclotomic action); the driver commands
`binom`, `binomtight`, `profpow`, `profpowcoarse`, `profpowfine`, `haar_volume`, `cyclo_exp_u`,
`cyclo_exp_uinv` (`tools/adf/adf.c`, `tools/adf/README.md`); statements Y9 to Y15 (`docs/api-1f9.md`); decision
N-D19 (`docs/SPEC.md` 15.4, last row). Contract: the comment blocks of `catalogue.h`; `docs/SPEC.md` 9.3.7 (the
rows "profinite power", "binomial coefficient", "cyclotomic action", "Haar volume"); `docs/proofs/catalogue.md`
Propositions 10 to 15 (lines 202-343). The lane's report is `lanes/f-slice13/report.md`.

**You own:** `lanes/f-review12/` only. Everything else is read-only. No git command that changes state, no
`bd`. At most 2 cores. Build the archive once: `timeout 600 make -j2 BUILD=lanes/f-review12/build
lanes/f-review12/build/libadelefeld.a`, link your programs against it (`-Iinclude -lflint -lgmp -lm`). Do not
run the repository's test suites. Every program under `timeout`, none over 120 s.

Hunt, in this order; stop a line of attack after a few thousand cases without a finding:
1. **Your own oracle, from the definitions, in exact integers**, not from `proto/catalogue_checks.py` (do not
   import it) and not from the lane's tests:
   - Binomials: for `x = a + N Zhat` the true image set `{binom(a + N j, k) : j in Z}` modulo a large modulus
     `L` (take `L` a multiple of `N k!` times extra primes; enumerate `j` over a full period). When the function
     returns `OK` with ball `c + R Zhat`: every value of the image must lie in it (a value outside is a
     BLOCKER). For `adf_fball_binom_tight`: `R` must be the smallest radius, that is the gcd of all differences
     of image values (a larger ball that still contains the image is a MAJOR against the header's word
     "smallest"; note `R = 0` when the image is one point). Negative `a`, `a < k`, `k = 0, 1`, `N = 0` (exact
     integers, also negative: `binom(-1, k) = (-1)^k`), `N = 1`, `N` with large prime powers, `N` and `a` of
     2000 bits, `k` at `ADF_BINOM_K_MAX`, `ADF_BINOM_TIGHT_K_MAX` and one above. The PLAN vector:
     `(a, N, k) = (0, 8, 4)` has smallest radius 2. Nonintegral inputs `(A + H Zhat)/d`: the header's rule
     (`DOMAIN` if `gcd(H, d)` does not divide `A`, else `NOT_DETERMINED`) against your own enumeration of
     whether the set meets `Zhat`.
   - Profinite power: for `a = c U(N)` and `x = e + M Zhat`, the true set of values modulo a test modulus `T`
     is `{(c + N i)^(e + M j) mod T : i, j, with c + N i a unit modulo T}` (units of `Zhat` reduce onto the
     units modulo `T` that are `c` modulo `gcd(N, T)`: enumerate those directly; exponents: `j` over a full
     period of the exponent of the unit group modulo `T`). Strict: `OK` exactly when the set modulo `canon(N)`
     is one class (check both directions against the header: `OK` with two classes is a BLOCKER; a refusal
     when it is one class is a MAJOR if the header promises "exactly when"). Coarse: the result `c^e U(D)`,
     `D = gcd(N, c^M - 1)`: every true value lies in it. Fine: every true value lies in the returned coset
     (BLOCKER otherwise) AND the returned modulus is the finest: for each prime `q` (dividing `N` or not, up to
     a bound) the largest power `q^t` at which the true set is one class must equal `v_q` of the returned
     canonical modulus (a coarser result is a MAJOR against the word "finest"). The PLAN vector: `N = 5, c = 2,
     e = 2, M = 4` gives modulus 120, centre 49. Negative `e`, `e = 0` exact (`[1]`), `M = 0`, `M = 1`,
     `N = 1, 2, 4, 8, 16` (the group modulo `2^k` is not cyclic), `N` twice an odd number (the canonical form),
     exact bases `[1]`, `[-1]` with exponents of known and unknown parity, `g` at `ADF_PROFPOW_FINE_G_MAX`
     and one above, `N` of 2000 bits (identities only).
   - Cyclotomic action: for a class with unit coset `c U(N)` and `n >= 1`: `OK` exactly when every unit of the
     coset has the same residue modulo `n` (enumerate units modulo `lcm(N, n)`); then `j = c mod n` for
     `_exp_u`, and `j c = 1 mod n` for `_exp_uinv`, `0 <= j < n`. `n = 1`, `n = 2`, `n = 2 m` with `m` odd,
     `n <= 0`, `j` aliased with `n`, exact units, a class whose content is not 1 (read `idclass.h`: which
     representative carries the unit coset, and does the function use the right one? The test vector of SPEC
     9.3.7: the idele with `p` at the place `p` and 1 elsewhere acts as `z -> z^p` under `_exp_uinv` for `n`
     prime to `p`; build that idele through the public constructors and through the driver and check it).
2. **Identities as tests of the code** (1000 random cases each, some of thousands of bits): Pascal's rule and
   Vandermonde on exact integers; `binom` against `binom_tight` (the tight ball inside the conservative one);
   `a^(x + y)` against `a^x a^y` and `(a^x)^y` against `a^(x y)` for exact exponents; coarse result contains
   the strict one when strict is `OK`; fine modulus divisible by the coarse one; `_exp_u` times `_exp_uinv`
   is 1 modulo `n`.
3. **Statuses and outputs**: every status the header names; the order `LIMIT` before the domain check where
   the header says so; the value untouched on a status other than `OK`; `where` untouched always, `where =
   NULL`; `y` aliased with `x` or `a`; canonical outputs (use the predicates of `fball.h`, `ucoset.h`).
4. **Memory**: two of your programs under `-fsanitize=address,undefined` with `ASAN_OPTIONS=detect_leaks=1`
   (the author's sandbox could not run the leak check).
5. **The driver**: thirty hostile lines for the eight commands (operand types, counts, `k` negative or
   `2^64`, `n = 0`, text of balls, cosets and classes); printed values against the library's.

Report: `lanes/f-review12/result.md`, written once, at the end (the harness refuses the name `report.md` for a
Claude subagent); notes in `lanes/f-review12/progress.md` as you go. For each finding: severity (BLOCKER: a
true value outside a returned set, an `OK` that some point of the input contradicts, a memory fault, undefined
behaviour; MAJOR; MINOR), the input, what the code returns, what is true and why, the command that reproduces
it. Then what you attacked without result, with counts and what would have made a case fail. No praise, no
summary. Give the same text as your final message. Do not commit binaries or build trees: delete
`lanes/f-review12/build` and your executables at the end.
