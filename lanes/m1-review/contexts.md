# Reviewer `contexts`: modulus contexts and the scaled policy

Read `lanes/m1-review/COMMON.md`; it binds you. Authors: pi models (deepseek-flash, mimo-v2.6-pro).
Files under review: `src/modctx.c` (the function `adf_modctx_new_from_dump` is being extended by a running
lane: review what is on master), `src/modctx_internal.h`, `src/scaled.c`. Headers: `modctx.h`, `scaled.h`.
Proofs: `docs/proofs/policies.md` sections 1 and 2; `docs/proofs/precision.md` Propositions 5, 6.
`docs/conventions.md` 4.5, 4.6, 5.4, 5.14, 10. `docs/SPEC.md` 4.4.

Look in particular at: every constructor with hostile arguments (`k` negative, `k` huge, blocks 0, 1,
2^64 - 1, products that overflow, `n` of the factorial and primorial families near the limit and far above
it: the lane reported unbounded work for `adf_modctx_new_primorial_pow`); `*out` untouched and nothing
leaked on every failure; the parser of the dump body `modctx` with input that is not NUL-terminated, the
two excused mutants at lines 698 and 710 of the lane (its report says they are out-of-bounds reads hidden
by the optimiser: are there such reads in the code as it stands? build with `-O0` and the sanitizers and
place the input at the end of a page); thread safety of reduce and recombine; the scaled policy: every
result contains the tight result (Theorem 3) for random and for extreme inputs, the default product's loss
factor, `mul_tight` against the tight product of the converted operands, `set_context` and `set_fball`
reporting `lost` exactly when the set changes, exact values keeping the tag, the shared-pointer rule
checked before any write also when the output aliases an input, contexts without blocks (modulus above a
word), `adf_scaled_set_fball` with a local input (changed today by the orchestrator, commit on master).
