# Lane s3-slice1: partial rational reconstruction in the range `2 A B < m`, end to end

Issue adf-1y1. `docs/workflow.md` rules 1 and 2: the smallest piece that a user can call and that computes
something true. Do not widen it. Decisions S-D1, S-D2, S-D5 are accepted by TJO (2026-09-29): a type
`adf_resid`; the library's own Euclidean loop, FLINT's `fmpq_reconstruct_fmpz_2` in tests only; `A < 0` or
`B < 1` is `NO_SOLUTION`.

Read first: `lanes/COMMON-C.md` (rule 6 does not hold for `resid.h`: in this lane the header is yours);
`docs/proofs/solvers.md` lines 73 to 255 (Definition 1.1, Definition 1.3, Lemma 1.4, Proposition 1.6) and
section 0 for (EEA); `docs/api-s.md` sections 1 and 2; `proto/solvers_checks.py` (`eea_pair`,
`cert_pair_ok`, `recon_partial`, `check_s3_edge`, `check_s3_complete`); `include/adelefeld/recon.h` and
`rat.h` for the style of a header; `docs/conventions.md` sections 2 to 4.

**You own:** `include/adelefeld/resid.h` (new), the one include line in `include/adelefeld.h`,
`src/resid.c` (new), `tests/test_resid.c` (new), `tests/ref/vectors/s3-slice1/` (new),
`tests/fuzz/diff_resid.py` (new), `tools/adf/adf.c` and `tests/test_driver.sh` (additions for one command),
`tests/test_abi.c` (additions), `lanes/s3-slice1/`. Everything else is read-only.

## What is built

1. `include/adelefeld/resid.h`, with the comment blocks in the style of `recon.h`:
   - `adf_resid_struct { fmpz c; fmpz m; }`, `adf_resid_t`; `adf_recon_cert_struct { fmpz Rp, Tp, R, T;
     int kind; }`, `adf_recon_cert_t` (layouts of `docs/api-s.md` section 1).
   - `adf_resid_init`, `adf_resid_clear`, `adf_resid_set_fmpz2`, `adf_resid_get_fmpz2`,
     `adf_resid_is_canonical`; `adf_recon_cert_init`, `adf_recon_cert_clear`.
   - `int adf_resid_reconstruct(adf_rat_t q, adf_recon_cert_t cert, const adf_resid_t x, const fmpz_t A,
     const fmpz_t B, slong limit)`: the final signature of `docs/api-s.md` section 2.
   Nothing else of section 2 is declared in this slice.
2. What `adf_resid_reconstruct` returns in this slice, in this order:
   - `A < 0` or `B < 1`: `ADF_NO_SOLUTION`; `cert` gets `kind = 0`.
   - `2 A B >= m`: `ADF_UNSUPPORTED`, nothing written. The header says that this status is temporary and
     goes away with slice 2. `limit` is not used in this slice; the header says so.
   - otherwise (`2 A B < m`, so `A < m`): the certificate pair by the Euclidean loop of Lemma 1.4 (stop at the
     first remainder `<= A`; only the `t` column is needed, not `s`); `abs(T) > B`: `NO_SOLUTION`
     (Proposition 1.6 (b)); `gcd(R, T) = 1`: `OK` and `q = sigma R / abs(T)`; else `NO_SOLUTION`
     (Proposition 1.6 (c), Remark 1: do not divide the row by the gcd). `cert` (may be NULL) gets the pair.
   - `q` is untouched on every status other than `OK`. `2 A B` is computed as `fmpz`, never in a word.
3. Tests first (red, then green; log in `lanes/s3-slice1/redgreen.log`), in `tests/test_resid.c`:
   - Brute force by Definition 1.1, written in the test itself (enumerate `n`, `d`): every `m <= 36`, `c`
     from `-m` to `2 m`, every `A` from 0 to `2 m`, `B` in `{1, 2, 3, 5, m - 1, m, m + 1, 2 m}` where
     positive. For `2 A B < m` the status and `q` must agree with the enumeration; for `2 A B >= m` the
     status must be `UNSUPPORTED`. Count the cases of each status and print the counts; the test fails if
     any of `OK`, `NO_SOLUTION` by (b), `NO_SOLUTION` by (c) has count 0.
   - Vectors written by `lanes/s3-slice1/gen_vectors.py` from `recon_partial` (import it from
     `proto/solvers_checks.py`, do not copy it) into `tests/ref/vectors/s3-slice1/recon.jsonl`: the cases of
     `check_s3_edge` that lie in the range, and random fractions `n/d` of 20, 64, 300 and 2000 bits with a
     modulus `m > 2 A B` just above the bound and far above it, also with `gcd(R, T) > 1` cases
     (`m = 12`, `c = 6`, `A = 1`, `B = 5` is one). Read them with `tests/support/jsonl.h`.
   - The certificate: (C1) to (C4) of Definition 1.3 tested in the test on every returned pair.
   - The boundary: `2 A B = m - 1`, `= m`, `= m + 1` for large `m`; `A = 0`; `m = 1`, `m = 2`; `B` huge
     with `A = 0`; `A < 0`; `B < 1`; `q` untouched (set it to a sentinel before the call); `cert = NULL`;
     `q` aliasing nothing else. `set_fmpz2` with `m < 1` is `ADF_DOMAIN` and reduces `c` into `[0, m)`.
   - A wrapper test against `fmpq_reconstruct_fmpz_2` inside `2 A B < m`, `m > 2`, `A >= 1`, labelled as such.
4. One command of the driver `adf`: `adf resid <c> <m> <A> <B>` prints the fraction, or the name of the
   status, in the way the driver prints for `reconstruct`. Read how `tools/adf/adf.c` handles
   `reconstruct` and M1-D1 in `docs/SPEC.md` section 15 first. Three cases in `tests/test_driver.sh`
   (a fraction; `NO_SOLUTION`; `UNSUPPORTED`). If the driver's language cannot take four integers without a
   change of its grammar, do not change the grammar: write `tests/julia/resid.jl` with a `ccall` instead,
   and say so in the report.
5. `tests/fuzz/diff_resid.py --seconds N --seed S`: random `(m, c, A, B)` of mixed sizes (small, 64 bit
   boundary, thousands of bits; half of them built from a planted fraction), the same input to the C
   function (through `ctypes` on `build/libadelefeld.so`, or through the driver if `ctypes` is not
   practical) and to `recon_partial`; it asserts equal status and equal `q`, and `UNSUPPORTED` exactly when
   `2 A B >= m`; it prints the number of cases of each status. Run it for 180 seconds and call that run a
   smoke test in the report. The orchestrator runs the long run.
6. Show that the tests bite: in a scratch copy under `build/`, change each of these in `src/resid.c`, one at
   a time, and record in `redgreen.log` which test fails: `r <= A` to `r < A`; `abs(T) > B` to
   `abs(T) >= B`; `2 A B < m` to `2 A B <= m`; the gcd test removed; the sign `sigma` dropped. This
   replaces the mutation run of `lanes/COMMON-C.md` rule 5 for this lane; do not run `make mutate`.
7. `make clean && make -j2 check`, `make clean && make -j2 check SAN=1`, `make clean && make -j2 check
   CC=clang`, `sh tests/test_driver.sh`, `sh tests/test_exports.sh` pass. Give the last line of each.

Not in this slice: the range `m <= 2 A B`, `adf_resid_reconstruct_first`, the verifier, `set_rat`,
`set_fball_forget`, `swap`, `identical`, text and dump forms, benchmarks.
