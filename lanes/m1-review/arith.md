# Reviewer `arith`: exact rationals, finite balls (global), adeles, reconstruction

Read `lanes/m1-review/COMMON.md`; it binds you. Authors: pi models (deepseek-flash, space-bunny-alpha).
Files under review: `src/rat.c`, `src/fball.c` (the global backend; the local parts are another reviewer's),
`src/adele.c`, `src/recon.c`, `src/cap.c` is NOT yours. Headers: `rat.h`, `fball.h`, `adele.h`, `recon.h`.
Proofs: `docs/proofs/precision.md`, `docs/proofs/quotient.md` Proposition 11. `docs/SPEC.md` 4.1 to 4.5, 9.2.

Look in particular at: the tight product and its radius `gcd(a M, b N, N M)` for rational centres and radii
with different denominators, zero centres, negative centres, exact operands mixed with inexact ones; the
canonical form after every operation; `adf_fball_contains`, `overlaps`, `equal_set`, `compare` at the
boundaries; `adf_fball_div_rat`, `mul_rat` with negative and zero scalars; the real coordinate of an adele
under `add_rat`, `mul_rat`, `div_rat` (the code converts `q` to a ball at `prec` first, since FLINT has no
`arb_add_fmpq`: is every result still an enclosure, also at `prec = 2`, also when the output aliases the
input?); `adf_adele_reconstruct` with the exact dyadic end points of an `arb` of huge or tiny exponent, and
`adf_fball_reconstruct` at the end points of the closed interval; the excused mutants of these files in
`tools/mutate/equivalent.txt` (is each one equivalent for every input? in particular the claim that
`arb_add` and `arb_mul` give the identical ball when the operands are exchanged, and `src/recon.c:111`).
