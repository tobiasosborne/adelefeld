# Lane t5-slice5: the functional equation command (slice 5e of `docs/api-5.md`)

Milestone 5 (the Tate integrals, `docs/PLAN.md` lines 335-343, `docs/SPEC.md` 8) is designed in `docs/api-5.md`
(decisions D1 to D4: `docs/SPEC.md` 15.4 N-D24; oracle `proto/tate_checks.py`, run it: `timeout 120 python3 -B
proto/tate_checks.py`) and built through slices 5a to 5d: `adf_local_tate_at`, `adf_local_gamma_at`,
`adf_local_epsilon_at` (`localfactor.h`, statements `docs/api-5a.md`), `adf_tate_vector`, `adf_tate_integral`,
`adf_tate_continue`, `adf_tate_completed` (`tate.h`, statements `docs/api-5b.md`). You build **slice 5e** of
design section 8 (lines 382-385): the caller-computed functional equation, end to end. Design section 5 (lines
250-260) fixes it: the caller computes `A = adf_tate_completed(chi, s)`, `B = adf_tate_completed(conj(chi),
1 - s)`, `R = adf_char_root_number(chi) * B`, with `W = tau / (i^e sqrt(C))` (the positive finite Gauss sum,
P13); there is NO function that returns an equation by copying or intersecting a side, and no new C type.
Both sides must be finite and overlap, for both parities and a non-real character, and each must overlap an
independent completed FLINT value ("overlap alone can hold for two equal wrong values"). Nonoverlap means a
contract is wrong (a finding); an honest `LIMIT` or `NOT_DETERMINED` is a failed acceptance case, not a
refutation. `C = 1` exact poles are status tests.

What you deliver:
1. The driver command `adf tate_fe CHI with S with BITS` (`tools/adf/adf.c`, the grammar of `tate_completed`;
   its section appended to `tools/adf/README.md`): it makes the two guarded calls and the root number and
   prints BOTH sides and their widths, not a forced equality: one line `A | R | overlap=yes|no | s=S` with
   `A` and `R` printed as the driver prints complex balls, or a status line when a call fails (which call
   failed, and its status). Decide and document whether `BITS` applies to each side (design: "guard targets for
   B and W": the oracle `functional_equation_sides` uses `bits + 4` on each side; state what you do and why the
   final widths of `A` and `R` are then bounded; the root number is computed at the working precision of
   `adf_char_root_number`, so `R`'s width includes `W`'s radius times `|B|`).
2. The test `tests/test_tate_fe.c`: for `C = 1`, the 17 characters of `tests/golden/gauss.tsv` (both parities;
   the non-real ones among them: `(5, 2)`, `(5, 3)`, `(7, 3)`, ...; the raw `(8, 7)` lowers to conductor 4: say
   what your test does with it) at `s` in `3/8 + 3i/4`, `1/2`, `1/2 + 14 i`, `-1`, `2 + 3 i`, `1/4 + i`, and
   balls with radii `2^-60`: both sides `OK`, finite, each coordinate width at most the bound you state,
   overlapping each other AND an independent completed FLINT value read from vectors (your generator under your
   lane directory writes `tests/ref/vectors/t5-slice5/`, at most 300 KB, with python-flint's `acb_dirichlet`
   value completed by hand and mpmath as a second source; `W` from `tests/golden/gauss.tsv`); the independence
   test of design section 7 (5.4): replace one side by a disjoint sentinel and require that side alone to
   change (equality of equal fake sides is not a certificate; no shared evaluator between the two sides);
   `C = 1` at exact `0` and `1`: the status of each side (`A` is `DOMAIN` at `s = 1`; what is `B` at
   `1 - s = 0`? both are statuses, not values); a ball meeting a pole; the trivial zeros of `L` (both sides
   finite); the six faults of design section 7 for 5.4 (no conjugate; inverse `W`; negative `tau`; incomplete
   completion; `-s` for `1 - s`; one side copied from the other) each planted in a scratch copy of the test's
   own side-evaluation or of the driver command: the test must fail. The forbidden `acb_dirichlet` path of
   design 4 is not called by the library: your test does not need the link-wrap, but the FLINT values come from
   vectors, not from a call in the same binary.
3. `tests/driver/tate-fe.cmd` with `.out`: 10 or more lines derived BEFORE the run from the vectors (zeta at
   `1/2`: `A = R` since `W = 1` and `conj(chi) = chi`, both sides print the same enclosure? say what the printer
   shows; `chi_4` at `3/8 + 3i/4`; a non-real character; a pole status; `BITS` out of range; a wrong operand
   kind), and `tests/julia/tate_fe.jl` that calls `adf_tate_completed` twice and `adf_char_root_number` once
   (design 8 item 5; the ABI of `tests/julia/tate_continue.jl`), checks overlap and finite width, registered in
   `tests/test_julia.sh` as the `tate_continue` block is.
4. Statements: append "Slice 5e" to `docs/api-5b.md`: what the command computes by reference to design 5,
   P13 (the functional equation `Lambda(s, chi) = W Lambda(1 - s, conj chi)`: cite the step of
   `docs/proofs/analysis.md` 564-626 that states it with this `W`), the widths of the two sides, why overlap
   of two independently computed sides with a third independent value is the acceptance of SPEC 8; statuses;
   "Check:" lines; decisions where the design is silent.
5. Mutation testing is not useful for a driver command; instead the fault table of item 2 and the acceptance
   numbers. The oracle group `functional_equation` of `proto/tate_checks.py` (`check_poles_fe`, lines 668-696)
   is the model: run it and compare your cases with its five pairs.

Read first (CLAUDE.md rules 3, 4): `lanes/COMMON.md`, `lanes/COMMON-C.md`; `docs/api-5.md` sections 1, 5, 7
(5.4), 8 (item 5), 9; `docs/api-5b.md` in full (what `adf_tate_completed` promises: statuses, the order of the
checks, the width contract, the poles; "Slice 5d" S9 to S14); `include/adelefeld/tate.h`,
`include/adelefeld/char.h` (`adf_char_conj`, `adf_char_root_number`: its precision and status contract),
`docs/api-3c.md`, `3d.md` (the root number: conventions 6.4, `W = tau / (i^e sqrt C)`, the 17 goldens);
`docs/SPEC.md` 8 (470-502: the acceptance); `docs/proofs/analysis.md` Proposition 13 (564-626);
`docs/conventions.md` 6.4, 6.5; `tools/adf/adf.c` (`adf_drv_tate`, lines about 3490-3535: the printer of the
value, the mode text, and the `with` grammar), `tools/adf/README.md` (the `tate_*` sections); `tests/golden/
gauss.tsv`; `refs/src/flint-3.0.1/acb_dirichlet.rst` (the `L` normalisation for the reference: quote lines).

**You own:** the command in `tools/adf/adf.c` and its section in `tools/adf/README.md`, `tests/test_tate_fe.c`,
`tests/ref/vectors/t5-slice5/`, new `tests/driver/tate-fe.*`, `tests/julia/tate_fe.jl` and its block in
`tests/test_julia.sh`, the appended section of `docs/api-5b.md`, `lanes/t5-slice5/`. No library code changes:
if a side cannot be computed as the design says, that is a finding, not a patch to `src/`. Everything else is
read-only. No git command that changes state, no `bd`. Lane t5-repair1 changes tests under `tests/test_tate*.c`
and `tests/test_tate_gamma.c` in another worktree; you do not touch those files.

**Checks at the end** (commands and numbers in the report; at most 2 jobs; every program under `timeout`):
`test_tate_fe` under plain, `SAN=1` with `ASAN_OPTIONS=detect_leaks=1`, `INV=1`, `CC=clang`, in build directories
under your lane directory; `sh tests/test_driver.sh`; `sh tests/test_julia.sh`; `python3 tools/memcheck/
check_uninit.py` on your test file and `tools/adf/adf.c`; NOT `check-all`. Remove build trees and scratch
copies; no generated log above 100 KB in the lane directory. Lines at most 116 characters; files end with a
newline. Aim to finish within about ninety minutes; the order of importance: the test with the 17 characters
and the FLINT overlap, the command, the fixture, the statements, the faults.

Report: `lanes/t5-slice5/result.md` (a Claude subagent cannot write `report.md`; give the same text as your
final message): what is done; the acceptance table (character, `s`, widths of `A` and `R`, overlap, FLINT
overlap); the fault table; findings against the design, the oracle and the proofs; sources pending; what is
not done.
