# Lane s13-repair: the four findings of the review `docs/reviews/s13/review.md`

Read first: `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 5, the mutation run, is replaced by item 5
below); `docs/reviews/s13/review.md` and the programs of the reviewer in `lanes/s13-review/`;
`include/adelefeld/resid.h`, `src/resid.c` (`adf_resid_verify_result` and the statics it calls);
`docs/proofs/solvers.md` Propositions 1.7 and 1.11; `docs/api-s.md` section 2 (the row of
`adf_resid_verify_result`, note 4); `include/adelefeld/linsolve.h`, `src/linsolve.c`
(`adf_linsolve_fball`, `adf_linsol_verify_fball`); decision S-D8 in `docs/SPEC.md` 15.3.

**You own:** `include/adelefeld/resid.h`, `src/resid.c`, `tests/test_resid_rest.c`,
`include/adelefeld/linsolve.h`, `src/linsolve.c`, `tests/test_linsolve_rest.c`,
`tests/test_s13_repair.c` (new), `docs/api-s.md` (ONLY the row of `adf_resid_verify_result` and note 4
of section 2), `lanes/s13-repair/`. Everything else is read-only.

Each repair is red first: the test, written from the input of the reviewer, fails on the present code;
then the code. Log in `lanes/s13-repair/redgreen.log`.

1. **Findings 1 and 2: what `adf_resid_verify_result` verifies.** Decided by the orchestrator: the
   verifier accepts the claim "`adf_resid_reconstruct` with these arguments, THIS `limit` included,
   returned this status (with this `q` for `OK`)", as the header and `docs/api-s.md` say, and not the
   claim about the whole set of solutions. With `ell = max(limit, 0)` and `X = floor(B/abs(T))`:
   - the fast cases (empty box, `A >= m`, `abs(T) > B`, `2 A B < m`) are as they are now;
   - in the range `A < m <= 2 A B`, `abs(T) <= B`, the verifier runs the same cut search as the function,
     at most `min(ell, X)` rounds, and accepts `NOT_UNIQUE` exactly when two reduced points are found in
     these rounds; `NOT_DETERMINED` exactly when `X > ell` and fewer than two were found; `OK` exactly
     when `X <= ell` and exactly one was found and it is `q`; `NO_SOLUTION` exactly when `X <= ell` and
     none was found.
   - Consequences to test: the three inputs of finding 1 are refused and the true status
     `NOT_DETERMINED` is accepted; the input of finding 2 (`m = 2`, `c = 0`, `A = 1`,
     `B = 5000000000`, `limit = 0`, claim `OK`) returns 0 at once (under `timeout 5`), and every call of
     the verifier costs at most `min(ell, X)` rounds; `X` above a word with a small limit is now decided
     (the test `verify_result_when_X_is_above_a_word` changes: `NOT_UNIQUE` at limit 1 is accepted); every
     result of `adf_resid_reconstruct` is still accepted, for every limit of the grid; a changed claim is
     refused or is what the function returns (recount with the function itself AND with the enumeration of
     the test restricted to the rounds `x <= ell`).
   - This is sound by Proposition 1.11 (every claim it accepts is true, since it accepts fewer claims than
     before). Rewrite the comment of the declaration, the cost sentence, the row of `docs/api-s.md` and note
     4 so that they say this, and remove the sentence "0 also where the claim may be true but this function
     cannot decide it" if it is no longer true; if a case remains where it is true, name the case.
2. **Finding 3: `adf_linsolve_fball` and `adf_linsol_verify_fball` allocate before `DOMAIN`, `LIMIT` and
   `UNSUPPORTED`.** Decide `DOMAIN` (the number of rows), then `LIMIT` (`r + c` against
   `ADF_LINSOLVE_DIM_MAX`, from the sizes), then `UNSUPPORTED` (an exact ball) before anything is
   allocated. Test with the allocation hooks of FLINT as `lanes/s13-review/probe_linsolve.c` does: 0
   allocations on each of the three statuses, `sol` untouched. If a ball cannot be read for exactness
   without an allocation (a local ball), say so in the header and in the report, with the smallest
   number of allocations that is possible and why.
3. **Finding 4: the branch of `tests/test_resid_rest.c` near line 2048 that asserts nothing.** Give it
   the assertion it was meant to have (for a `NOT_UNIQUE` line of the vectors: `q` untouched, the
   certificate as the vector says, the verifier accepts the claim and refuses `OK` with the first
   solution), or remove the branch if the surrounding test already asserts all of that; say which.
4. Show that the new tests bite, in a scratch copy under `build/`, one change at a time: the condition
   `X <= ell` removed from the `OK` case; from the `NO_SOLUTION` case; the search of the verifier run
   to `X` instead of `min(ell, X)`; the `LIMIT` test of `adf_linsolve_fball` moved back behind the
   allocation.
5. `make clean && make check-all`, `make clean && make -j2 check SAN=1`, `make clean && make -j2 check
   CC=clang`, `sh lanes/m1-headers/check_headers.sh` pass; every test program under `timeout`. Give the
   last line of each. Run the four programs of the reviewer (`probe_resid`, `probe_sets`,
   `probe_linsolve`, `probe_verify_time 5000000000`; compile commands in the review) against the repaired
   library and give what each prints at the end.
