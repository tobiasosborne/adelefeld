# Lane i-slice1: progress (running notes)

Started 2026-09-29 22:46 (clock). Worktree at f0a23fc. `refs/src` linked to the main checkout.

## Read

- CLAUDE.md, lanes/COMMON.md, lanes/COMMON-C.md, docs/workflow.md, the brief.
- SPEC 4.1-4.5, 5, 15; PLAN milestone 2; docs/proofs/ideles.md (all); conventions 2, 3, 4, 5.6, 5.7, 12.
- adele.h, resid.h, common.h (style); src/invariants.h; tests/golden/ucoset.tsv, idele.tsv.
- Design lane d-ideles (worktree agent-acc17965b8910c1f2): progress.md (finding 3), probe_arb.c,
  part 2 of proto/ideles_checks.py (unit cosets, real kernel, idele arithmetic). docs/api-2.md was never
  written there.
- FLINT docs: arf.rst (rounding semantics 22-39, ARF_PREC_EXACT 92-112, get_mag 403, set_mag 411, add 560,
  sub 574, mul 590, div 662, fmpz_div_fmpz 676, get_fmpq 310), arb.rst (7, 597, 606, 660), mag.rst 1-20.

## Plan

1. Headers ucoset.h, idele.h; docs/api-2.md section of the slice with "Statements to add".
2. proto/ideles_checks.py: copy the reviewed-by-me parts of part 2 (cosets, kernel, idele mul/inv/set_rat).
3. lanes/i-slice1/gen_vectors.py -> tests/ref/vectors/i-slice1/*.jsonl.
4. tests/test_ucoset.c, tests/test_idele.c red; then src/ucoset.c, src/idele.c green.
5. tests/julia/ideles.jl + a line in tests/test_julia.sh.
6. Faults in a scratch copy under build/; the four check commands.

## State

- [x] 1 headers ucoset.h, idele.h; adelefeld.h lines; docs/api-2.md section 1 (Statements A to E)
- [x] 2 proto part 2: `timeout 180 python3 proto/ideles_checks.py part2` 3 checks pass, 6.6 s
- [x] 3 vectors: `python3 lanes/i-slice1/gen_vectors.py`: ucoset 1803 lines, idele 1758 lines (489 ND)
- [x] 4 red/green logged in redgreen.log; test_ucoset 11 tests 43178 checks, test_idele 12 tests 8613 checks
- [x] make -j2 check: 59 programs pass (23:10)
- [x] 5 julia: tests/julia/ideles.jl 23 tests pass; test_julia.sh passes (with LD_PRELOAD)
- [x] 6 faults F1-F4 all caught (faults.log); check-all, SAN, clang, check_headers pass (logs here);
      INV=1 build of the two tests passes, probe aborts on a bad coset
- [x] result.md (23:35)
