# Lane d-tate: design of milestone 5, the Tate integrals, before code

A design document before code, the map for thin slices (`docs/workflow.md` rules 1 and 3), as `docs/api-3c.md`
(lane d-char) and `docs/api-4.md` (lane d-functions4) are for milestones 3 and 4: read `lanes/d-functions4/
brief.md` and `docs/api-4.md` for the form and the length (at most 500 lines; declarations with full comment
blocks, statements with proofs, acceptance tests, slices; nothing twice). No change of `src/` or `include/`.
Write for an implementer who will build it in lanes of about an hour each, and for a reviewer who will try to
refute you.

Milestone 5 (`docs/PLAN.md` lines 335-343; the acceptance test of the whole library, `docs/SPEC.md` section 8,
lines 470-502): 5.1 local integrals (unramified factors; ramified with the inverse character; infinity for
both parities; exit: closed forms); 5.2 the global value for `Re(s) > 1`, zeta and primitive characters (exit:
compared with FLINT's values completed by hand; the width shrinks with precision); 5.3 the continuation by
splitting and Poisson summation, the poles (exit: independent of `acb_dirichlet`; tested at and near poles);
5.4 the functional equation as a computed identity (both sides overlap with finite width, for both parities
and a non-real character).

What is fixed and NOT yours to change: `docs/SPEC.md` 8 (the integral, the test vectors, the acceptance), 5
(377-384: the idele class characters, `omega_chi = conj(chi(u'))`), 9.3.7 (poles: an exact pole `DOMAIN`, a
ball meeting a pole `NOT_DETERMINED`, outputs untouched); `docs/conventions.md` 6.1 to 6.5 (signs, measures, the
finite transform, Gauss sums and root numbers, the local constants `gamma_p`, `epsilon_p`, `L_p` at 912-956 with
`G_minus` of the NEGATIVE kernel, the real place `gamma_inf`, the class-group test vector of 6.5:957-969), the
status row "Integrals, Poisson summation" of 3.2 (225); the proofs `docs/proofs/analysis.md` Proposition 9
(331: local Tate integrals and local gamma factors), 10 (401: the real integral, both parities), 11 (452:
global test vectors and the conjugate idele character), 12 (496: splitting at norm 1, poles, residues), 13
(564: the primitive theta identity and the completed functional equation, `W_chi`), Lemma 14 (627: an
exponential integral bound), Proposition 15 (652: explicit truncation and quadrature certificates; the
qualification recorded in SPEC 15.4 N-D23: input radii bound the final width); `proto/analysis_checks.py`
(`check_local_integrals`, `check_local_gamma`, `check_real_local`, `check_idele_character`,
`check_global_integral`, `check_theta`, `check_functional_equation`, `check_continuation`,
`check_quadrature_bound`, `check_poles`, `check_continuation_bounds`, `check_certified_continuation`,
`check_poisson_right_and_idele`, `check_general_splitting`, `check_general_adelic_bound`,
`check_composite_conductor`: the exact and numerical checks that exist; your oracle may import it); what the
library has NOW, which the design must consume and not redo: `include/adelefeld/localfactor.h` (the local zeta
factor of the trivial character, `docs/design/local-zeta.md` Z1 to Z9, N-D20), `char.h` (`adf_char`: the
primitive character with conductor, `chi_phase`, `chi`, `conj`, `eval_ucoset`, `eval_idclass`, `eval_idele`
when slice c lands, `gauss_sum`, `root_number`; `docs/api-3c.md`, `docs/api-3d.md`), `psi.h` (the additive
character and `adf_phase_get_acb`), `ffun.h`, `rfun.h`, `tensor.h` (`adf_ffun`, `adf_rfun`, their transforms,
`adf_rfun_derivative`, `adf_rfun_integral`, `adf_tensor_eval`, `adf_tensor_integral`, `adf_tensor_poisson` with
certified tails and cutoffs; `docs/api-4.md` section 7's paragraph "Milestone 5 calls..." at lines 337-345: the
zeta vector `D = M = 1`, `f[0] = 1`, `P = [1]`, `A = 1`; for primitive `chi` of conductor `C` and parity `e`:
`D = 1`, `M = C`, `f[j] = chi(j)` extended by zero, `P = x^e`; `docs/api-4a.md` to `4c.md`), `idele.h`,
`idclass.h`, `ucoset.h`, `lball.h` (the local balls for 5.1), `gfunc.h` (the all-places forms); FLINT's
`acb_dirichlet` (`refs/src/flint-3.0.1/acb_dirichlet.rst`: `acb_dirichlet_l`, the Hurwitz zeta, the completed
`xi` with its normalisation, which SPEC 8 says differs from ours **[checked]**: write the exact conversion).

**You own:** `docs/api-5.md` (new), `proto/tate_checks.py` (new; exact where possible: the Euler factors and
local constants as exact expressions in `chi(p)` phases; the global values by `mpmath` at 60 digits and by
`python-flint`'s `acb_dirichlet` where importable, with a stated margin; the continuation by the splitting
formula with the certified tails; may import `proto/analysis_checks.py`, `proto/char_checks.py`,
`proto/functions4_checks.py`), `lanes/d-tate/`. Everything else is read-only (a defect of SPEC, the
conventions, the proofs or the goldens is a finding with the counterexample computed by your script). No git
command that changes state, no `bd`. At most 2 cores; every program under `timeout`; none over 120 s. Two other
lanes write `src/char.c`, `src/dump.c` in other worktrees: not your files.

## What `docs/api-5.md` contains

1. **The objects.** Does milestone 5 need a new value type (a Tate integral result with its domain, the
   "results of integrals with their domain" of `docs/PLAN.md` 4; a pole object for 5.3), or do `acb` outputs
   with statuses and `where` arguments suffice? Decide and say why; if a type is needed, its predicate, set
   statement, init, `is_canonical`, text and dump declarations in the style of `docs/api-3.md` 2.5.
2. **5.1, the local integrals**: `Z_p(f, eta, s)` for the local test functions the global vectors use
   (`1_(Z_p)`, `eta_p^(-1) 1_(Z_p^x)`) and a quasi-character `eta` given by `alpha = eta(p)` and the unit
   restriction (an `adf_char` at `p`, or `(alpha, chi_p)`: decide the signature), with the closed forms of
   Proposition 9: `a = 0`: `(1 - alpha p^-s)^-1`; `a > 0`: the local constant `gamma_p = alpha^a p^(-as)
   G_minus(eta_0^-1)` and `L_p = 1`; `epsilon_p`; at the real place `Z_inf(phi_e, eta, s) = pi^(-(s+e)/2)
   Gamma((s+e)/2)` and `gamma_inf`, `epsilon_inf = i^e` (Proposition 10); the poles (`s = 2 pi i k / log p`
   at `p`; `s = 0, -2, -4, ...` at the real place) with the status rule of SPEC 9.3.7 and conventions 6.4;
   how `adf_local_zeta_factor_at` (the trivial character, already built) is the special case; `G_minus` with
   the negative kernel through `adf_phase_get_acb` with a negated angle (never a helper called "the Fourier
   kernel", `docs/api-3.md` 3.4); statuses, aliasing, cost, caps (D1 of milestone 4 applies to the function
   factors; a new cap for the conductor through `ADF_CHAR_MOD_MAX`).
3. **5.2, the global value for `Re(s) > 1`**: `Z(f, omega, s) = integral over the ideles` as the product of
   the local integrals for the test vectors of conventions 6.5 (Proposition 11): the zeta case
   `pi^(-s/2) Gamma(s/2) zeta(s)` and `pi^(-(s+e)/2) Gamma((s+e)/2) L(s, chi)` for primitive `chi` (the idele
   character `omega_chi = conj(chi(u'))`, the stored `adf_char` of `conj(chi)` with `s = 0`); which calls
   compute it: by the Euler product with a certified tail (how many primes for a target width; the bound) or
   by the Mellin integral of the theta series through `adf_tensor_poisson` (Proposition 12's splitting also
   works for `Re(s) > 1`): choose the primary algorithm for 5.2 and say what 5.3 reuses; the acceptance
   against FLINT (`acb_dirichlet_l` completed by hand: write the completion factor exactly) and the width
   shrinking with precision (what input radii prevent: N-D23's qualification).
4. **5.3, the continuation**: Proposition 12's splitting at norm 1: `Z(f, omega, s) = integral over t >= 1
   of (...) t^s + integral over t >= 1 of (...) t^(1-s) + the pole terms` (write the formula exactly as the
   proof has it, with the Poisson step that moves `t < 1` to `t > 1` through `adf_tensor_poisson` on the
   dilated tensor: the dilation of both factors by the idele `t`, `docs/api-4.md` 333-335); the two Mellin
   integrals over `[1, infinity)` by the quadrature and truncation certificates of Lemma 14 and Proposition
   15 (the explicit `N`, the quadrature rule, the error bound; `arb_calc` or the library's own rule: decide);
   the poles at `s = 0` and `s = 1` for conductor 1 only, with residues `-f(0)` and `hat f(0)`; the status
   rule at and near the poles (exact pole `DOMAIN`; a ball meeting a pole `NOT_DETERMINED`; a ball near the
   pole: the enclosure through the residue term, or the status: decide and prove); independence of
   `acb_dirichlet` (used in tests only).
5. **5.4, the functional equation**: `Lambda(s, chi) = W_chi Lambda(1 - s, conj(chi))` with `W_chi =
   tau(chi) / (i^e sqrt(C))` through `adf_char_root_number` (Proposition 13), as a computed identity: the call
   that returns both sides (or the caller computes them: say which), the test that both sides overlap with
   finite width for both parities and a non-real character (the PLAN exit), and what a failure would mean.
6. **Statements to add to `analysis.md`** (numbered steps, proofs written out) for what 2 to 5 need beyond the
   propositions: the Euler product tail bound for `Re(s) > 1`; the completion factor against FLINT; the
   error bound of the quadrature as the code applies it; the enclosure near a pole; the dilation of the
   tensor by `t` and its effect on the Poisson cutoffs.
7. **Acceptance tests and faults**: per function what a test must check so that it fails for a wrong
   implementation (exact small cases from the oracle; the PLAN exits of 5.1 to 5.4; `zeta(2) = pi^2/6`
   completed; `L(1, chi_4) = pi/4`; the root number of a real character `W = 1` on the eight golden
   characters; the pole of zeta at `s = 1` with residue 1 completed, by the splitting); six faults per work
   package (the inverse character not taken at a ramified prime; `G_minus` with the positive kernel; the
   completion factor `C^((s+e)/2)` dropped; the pole term with the wrong sign; the quadrature tail bound
   without Lemma 14's factor; the two sides of the functional equation computed from each other).
8. **Decisions for TJO**: few and real (the primary algorithm of 5.2; the result type; the quadrature rule;
   the caps).
9. **Thin slices**: in the order 5.1 to 5.4, each the smallest piece a user can call (driver `adf` command,
   Julia `ccall`), with its functions, oracle group and decisions; the first slice (the local integrals at a
   prime and at infinity for the trivial and a ramified character, against the closed forms) in one lane of
   about an hour.
10. **Findings** against SPEC, PLAN, conventions, proofs, goldens, each with the counterexample.

`proto/tate_checks.py` must run: `timeout 120 python3 proto/tate_checks.py` prints each group with its counts
and ends with the number of checks; every example of the design is computed by it. It provides the oracle
functions for the implementation lanes: `local_integral(p, alpha, chi_p, s)`, `local_gamma`, `local_epsilon`,
`real_integral(e, s)`, `global_value(chi, s)` with the completion, `continuation(chi, s, bits)` by the
splitting with its certified tails, `functional_equation_sides(chi, s)`.

## Report

`lanes/d-tate/report.md`, written once, at the end; notes in `lanes/d-tate/progress.md` as you go. In the
report: the interface in twenty lines; the decisions; the findings; what the oracle checks, with numbers and
what would have made a check fail; what is not designed and why; sources pending. No praise, no summary of
what the files already say. Aim to finish within about ninety minutes.
