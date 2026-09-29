# Lane s3-slice3: the rest of `resid.h` (S.3 complete)

Slices 1 and 2 are on master: `adf_resid_reconstruct` in the whole range and `adf_recon_cert_check`
(`include/adelefeld/resid.h`, `src/resid.c`, `tests/test_resid.c`, `tests/test_resid_full.c`,
`lanes/s3-slice2/result.md`). This lane adds the remaining functions of `docs/api-s.md` section 2.
A review of the existing functions runs at the same time: do NOT change the existing functions of
`src/resid.c` or their tests; only add.

Do NOT create `lanes/s3-slice3/report.md` before all the work is done: the runner takes the existence of
that file as the end of the lane. Keep `lanes/s3-slice3/progress.md` while you work, one section for each
function.

Read first: `lanes/COMMON-C.md` (rule 6 does not hold for `resid.h`: you add declarations to it; rule 5 is
replaced by item 4 below); `docs/api-s.md` sections 1 and 2 with all notes; `docs/proofs/solvers.md` lines
73 to 494 (Lemma 1.2, Propositions 1.7, 1.10, 1.11); `proto/solvers_checks.py` (`recon_first`,
`recon_verify_result`, `is_solution`, `check_s3_verify`, `check_s3_forget`); `include/adelefeld/fball.h`
(the canonical triple, `adf_fball_get_fmpz3`) and `rat.h`.

**You own:** `include/adelefeld/resid.h` and `src/resid.c` (additions only), `tests/test_resid_rest.c`
(new), `tests/ref/vectors/s3-slice3/` (new), `tests/julia/resid.jl` (additions), `lanes/s3-slice3/`.
Everything else is read-only; the pins of new layouts, if any, go into `tests/test_resid_rest.c`.

## What is built, one function after the other, each with its test first (red, then green, logged in
`lanes/s3-slice3/redgreen.log`)

1. `adf_resid_set`, `adf_resid_swap`, `adf_resid_identical`; `adf_recon_cert_set`, `adf_recon_cert_swap`,
   `adf_recon_cert_identical`, `adf_recon_cert_is_canonical` (`docs/conventions.md` 2.3).
2. `adf_resid_set_rat`, `adf_resid_contains_rat` (`solvers` L1.2): for every `m <= 40` and every reduced
   `n/d` with `abs(n), d <= 12`: `set_rat` is `OK` exactly when `gcd(d, m) = 1`, the result contains `q`
   by the definition (`m` divides `n - c d`), and `contains_rat` agrees with the definition for every `c`.
3. `adf_resid_set_fball_forget` (`solvers` P1.10(4)): every rational `(A + H k)/d` of the ball, `k` from
   -20 to 20, is contained in the result; `DOMAIN` for `H = 0` and for `gcd(d, H) > 1`, `x` untouched; the
   example of note 1: for `5 + 6 Zhat`, `adf_fball_reconstruct` on `[0, 1]` gives `NO_SOLUTION`, and after
   `forget`, `adf_resid_reconstruct` with `A = 1`, `B = 5` gives `NOT_UNIQUE`, with `B = 4` gives `OK` and
   `-1/1`. A ball in the local backend gives the same result as the same ball in the global backend.
4. `adf_resid_reconstruct_first` (decision S-D4): it may call the same static search as
   `adf_resid_reconstruct`; if that needs the search to return its first point on `NOT_UNIQUE` and
   `NOT_DETERMINED`, add an argument to the static function without changing what
   `adf_resid_reconstruct` returns, and run `tests/test_resid.c` and `tests/test_resid_full.c` after it.
   Test: on the grid `m <= 24`, `c` from 0 to `m - 1`, `A` from 0 to `2 m`, `B` in
   `{1, 2, 3, 5, m, 2 m}`, limits -1, 0, 1, 2, 5, large: status, `q` and `count` against an enumeration
   written in the test and against vectors of `recon_first` (script `lanes/s3-slice3/gen_vectors.py`,
   import the reference, do not copy it). `count` is 1 only if the set has one element, 2 only if it has
   at least two, 0 with `OK` only after a cut search.
5. `adf_resid_verify_result` (`solvers` P1.11, note 4 of section 2): every result of
   `adf_resid_reconstruct` on the grid of item 4 is accepted; for each, the changed claims (every other
   status; `q` changed in numerator, in denominator, in sign; a certificate with one entry changed;
   `kind` flipped) are refused, or the changed claim is true by the enumeration. Print how many changed
   claims were refused and how many were true; the test fails if no claim was refused for one of the
   kinds of change. A `NOT_UNIQUE` is not verified by a second call with a lowered `B` (note 4).
6. `tests/julia/resid.jl`: one call of `adf_resid_reconstruct_first` and one of
   `adf_resid_set_fball_forget`.
7. `make clean && make -j2 check`, the same with `SAN=1` and with `CC=clang`, `sh tests/test_exports.sh`,
   `sh tests/test_julia.sh` pass. Give the last line of each.

Not in this lane: the driver, text and dump forms (decision S-D12: none), benchmarks, `make mutate`.
