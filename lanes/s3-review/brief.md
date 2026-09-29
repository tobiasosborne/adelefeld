# Lane s3-review: adversarial review of partial rational reconstruction (slices 1 and 2)

Your task is to REFUTE, not to confirm. The code under review is `src/resid.c` with its header
`include/adelefeld/resid.h` (about 330 lines), written by Claude Sonnet. Its contract: the header;
`docs/api-s.md` section 2; `docs/proofs/solvers.md` section 1 (Definition 1.1 to Proposition 1.7);
decisions S-D1 to S-D5 in `docs/SPEC.md` 15.3.

**You own:** `lanes/s3-review/` only. Everything else is read-only. No git, no `bd`. At most 2 cores.
Build the library with `make -j2` and link your own programs against `build/libadelefeld.a`
(see how `Makefile` builds a test), or use `ctypes` on `build/libadelefeld.so` after
`sh tests/test_exports.sh`.

## What counts as a finding

An input `(c, m, A, B, limit)`, or a sequence of calls, for which `adf_resid_reconstruct`,
`adf_recon_cert_check`, `adf_resid_set_fmpz2`, `adf_resid_get_fmpz2` or `adf_resid_is_canonical`
- returns a status or a fraction that Definition 1.1 contradicts (your own oracle: the enumeration of the
  definition, written by you, not `proto/solvers_checks.py` and not the tests of the author);
- returns `NOT_DETERMINED` or `OK` against Proposition 1.7;
- writes an output that the header says is untouched, or a certificate that fails (C1) to (C4), or accepts
  a false certificate;
- aborts, reads or writes out of bounds, leaks, or runs for a time that the header does not admit;
- differs between aliased and unaliased arguments where the header permits the aliasing.
Also a finding: a sentence of the header that the code does not keep, and a test of
`tests/test_resid.c` or `tests/test_resid_full.c` that cannot fail.

Places to attack first: the word conversions (`limit`, `X = floor(B/abs(T))`, `WORD_MAX`, `limit < 0`);
`A = 0`, `m = 1`, `m = 2`, `c` not reduced (a struct written by hand with `c >= m` or `c < 0`: what does the
header promise?); `B` far above `m`; the bounds of `y` in a round; the case where the two solutions of a
round come from the same `x`; the sign of `T`; `q` aliasing a member of another argument; a `cert` that
aliases nothing but is reused between calls.

## Report

`lanes/s3-review/report.md`, written once, at the end. For each finding: severity (BLOCKER: a wrong
result or memory fault inside the contract; MAJOR; MINOR), the input, what the code returns, what is
true and why, and the command that reproduces it with a program in your lane directory. Then: what you
attacked without result, with the number of cases and what would have made a case fail. No praise, no
summary of the code.
