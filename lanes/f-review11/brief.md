# Lane f-review11: bug hunt through the quadratic symbols and the Hilbert symbol (lane f-slice12, `src/symbol.c`)

Your task is to REFUTE, not to confirm. Lane f-slice12 (codex `gpt-6.1-sol`; you are of another model family on
purpose) landed today, not reviewed: `include/adelefeld/symbol.h` (113 lines), `src/symbol.c` (351 lines): the
Legendre, Jacobi and Kronecker symbols on `fmpz`, `adf_fball`, `adf_ucoset`; the Hilbert symbol
`adf_lball_hilbert`, `adf_real_hilbert`, `adf_rat_hilbert_at`, `adf_idele_hilbert_at`; the driver commands
`legendre`, `jacobi`, `kronecker`, `hilbert_at` (`tools/adf/adf.c`, `tools/adf/README.md`); statements Y1 to Y8
(`docs/api-1f9.md`); decision N-D18 (`docs/SPEC.md` 15.4, last row). Contract: the comment blocks of
`symbol.h`; `docs/SPEC.md` 9.3.7 (lines 685-720); `docs/proofs/catalogue.md` (Definition 1, Propositions 2, 3,
the section "Hilbert symbols"). The lane's report is `lanes/f-slice12/report.md`.

**You own:** `lanes/f-review11/` only. Everything else is read-only. No git command that changes state, no
`bd`. At most 2 cores. Build the archive once: `timeout 600 make -j2 BUILD=lanes/f-review11/build
lanes/f-review11/build/libadelefeld.a`, link your programs against it (`-Iinclude -lflint -lgmp -lm`). Do not
run the repository's test suites. Every program under `timeout`, none over 120 s.

Hunt, in this order; stop a line of attack after a few thousand cases without a finding:
1. **Your own oracle, from the definitions, not from `proto/symbol_checks.py` or `proto/catalogue_checks.py`**
   (do not import them) and not from FLINT: the Legendre symbol by counting squares modulo `p`; Jacobi as the
   product over the prime factors of the lower entry; Kronecker with the conventions the header states for
   lower entries 0, negative, even (check each convention against the header's sentence AND against
   `catalogue.md` Definition 1: a disagreement between the two texts is a finding); the Hilbert symbol
   `(a, b)_p` by deciding whether `a x^2 + b y^2 = z^2` has a primitive solution modulo `p^k` (state the `k`
   you use and why it suffices: Hensel), at the real place by signs. Compare with the C functions on: all
   `a, b` in a box of small integers and rationals with every valuation pattern at `p` in 2, 3, 5, 7, 13; all
   64 pairs of square classes at 2; lower entries 0, 1, -1, 2, negative, even, large; numerators of 2000 bits.
   A wrong sign is a BLOCKER.
2. **Finite precision.** For `adf_lball_hilbert`: balls `u p^m + p^N Z_p` of every small `m`, `N`: when the
   function returns `OK`, enumerate the ball modulo `p^(N+3)` and check that EVERY point gives that sign (a
   point with the other sign is a BLOCKER); when it returns `NOT_DETERMINED`, find two points with different
   signs (if all points agree and the header promised a decision, a MAJOR; if the header only promises a
   conservative certificate, say which sentence). A ball containing 0; the exact 0. The same for
   `adf_idele_hilbert_at` (unit cosets with `v_p(N)` from 0 to 4, contents with a power of `p`, the exact units)
   and for the residue symbols on `adf_fball` and `adf_ucoset` (the header says the certificate is the
   sufficient modulus of Proposition 2: `OK` must be right for every point; a refusal is not a finding unless
   a sentence promises more).
3. **Identities as tests of the code, not of the theory** (2000 random cases each, thousands of bits):
   bilinearity and symmetry of the Hilbert symbol, `(a, -a) = 1`, `(a, 1 - a) = 1`, the product formula over
   the primes dividing `2 a b` and the real place; quadratic reciprocity and the two supplements for Jacobi.
4. **Statuses and outputs**: every status the header names, `where` (written on `OK`? right place?), the value
   untouched on a status other than `OK`, `where = NULL`, two local balls at different primes, the real place
   passed to a prime-only function and the reverse, non-finite `arb` inputs.
5. **Memory**: two of your programs under `-fsanitize=address,undefined` with `ASAN_OPTIONS=detect_leaks=1`
   (the author's sandbox could not run the leak check).
6. **The driver**: thirty hostile lines for the four commands (operand types, counts, places that are not
   places, 4 as a prime, `2^64`, text of balls and ideles); printed values against the library's.

Report: `lanes/f-review11/result.md`, written once, at the end (the harness refuses the name `report.md` for a
Claude subagent); notes in `lanes/f-review11/progress.md` as you go. For each finding: severity (BLOCKER: a
wrong sign, an `OK` that some point of the input contradicts, a memory fault, undefined behaviour; MAJOR;
MINOR), the input, what the code returns, what is true and why, the command that reproduces it. Then what you
attacked without result, with counts and what would have made a case fail. No praise, no summary. Give the same
text as your final message.
