# Lane m1-cap: the absolute cap on adf_fball, global backend (work package 1.7, part)

NOTE: this file did not exist when the lane started (checked at the first commands of the session); it is
written here, by the lane itself, from the task given by the orchestrator, so that the lane directory carries
the same record as every other lane. Written before any test or code of the lane.

Read `lanes/COMMON.md`, `lanes/COMMON-C.md`. Then `include/adelefeld/scaled.h` (the five declarations at its
end), `include/adelefeld/fball.h` (the tight operations these five call, and the aliasing rule they inherit),
`include/adelefeld/rat.h`, `include/adelefeld/status.h`; `docs/conventions.md` section 5.4 ("The absolute cap"
paragraph) and its table row `CV-23`, `CV-47`; `docs/proofs/policies.md` section 3, Definition 13 to
Proposition 15 (line 251 to 301); `docs/SPEC.md` 4.4 item 3; `docs/api-m1.md` rows for the five functions and
"Choices" item 5 (`ADF_DOMAIN` when `C <= 0`, undocumented row of conventions 3.2). Reference:
`tests/ref/adfref/policies.py` function `absolute_cap`; its vectors are `tests/ref/vectors/policies.jsonl`,
op `absolute_cap` (300 rows).

**You own:** `src/cap.c`, `tests/test_cap.c`, `tests/test_cap_vectors.c`, `tests/ref/vectors/m1-cap/`,
`lanes/m1-cap/` (this directory). `tools/mutate/equivalent.txt` is append-only shared ground, as the lane
`m1-fball` used it (`lanes/m1-fball/report.md`); an entry here is appended, never rewritten.

Implement, for the global backend only, the five functions declared at the end of `scaled.h`:
`adf_fball_cap`, `adf_fball_add_cap`, `adf_fball_sub_cap`, `adf_fball_mul_cap`, `adf_fball_mul_rat_cap`.
`adf_fball_cap(y, x, C)` applies Definition 13 directly to `x`'s own set. The other four run the tight
operation of `fball.h` (or `adf_fball_mul_rat` for the last one) into a fresh temporary, then cap that
temporary into the output, so that every permitted aliasing of the output with an input is safe. Status
`ADF_DOMAIN` if `C <= 0`, output untouched (`docs/api-m1.md` Choices item 5). The header does not say what
happens when an input ball is in the local backend (`ADF_LOCAL`); `fball.c` of `m1-fball` leaves such an
output untouched for the tight operations it cannot read (its own `HEADER-FINDING`). This lane reads the same
finding and, for consistency and because guessing a result the code cannot certify is against COMMON-C rule 3,
returns `ADF_UNSUPPORTED` with the output untouched for a non-global input, marked `HEADER-FINDING` in the
code and listed in the report. Conventions 3.3 (statuses combine by maximum, `ADF_UNSUPPORTED` = 8 >
`ADF_DOMAIN` = 7) fixes the order when both conditions hold.

Tests must cover, besides the rules of COMMON-C: `adf_fball_cap` against every row of
`tests/ref/vectors/policies.jsonl` op `absolute_cap` (300 rows); the exact case (radius 0) kept untouched by
the cap (Proposition 14.2); the invariant that a capped radius divides `C` (Proposition 15.1); idempotence of
the cap (Proposition 15.2); that a sum of two already-capped values needs no further cap (Proposition 15.3);
aliasing of every permitted combination of arguments (output = first input, output = second input, output =
both, for the binary operations); `C <= 0` (zero and negative, integer and fractional) giving `ADF_DOMAIN` with
every output field-by-field untouched; a value shaped as local backend giving `ADF_UNSUPPORTED` with the
output untouched (structural shape only, as `tests/test_fball.c` function `make_local_shaped` does, since the
context is never dereferenced by this file); huge operands (thousands of bits) for both the ball and `C`. Since
no vector file combines a tight operation with the cap, generate one from the Python reference with a script
kept in this directory (`lanes/m1-cap/gen_vectors.py`), writing `tests/ref/vectors/m1-cap/cap_ops.jsonl`
(`fball.add`/`sub`/`mul`/`scale` then `policies.absolute_cap`, composed in Python, never read back into C
except as the vector file).

Mutation: `make mutate FILES=src/cap.c`, at most 2 jobs; report killed/survived/excused counts. Excuse a
survivor in `tools/mutate/equivalent.txt` only with a reason that the mutant computes the same set for every
input.
