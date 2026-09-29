# Lane s2-slice2: all roots at a prime (Algorithm P)

Slice 1 is on master (`include/adelefeld/roots.h`, `src/roots.c`, `tests/test_roots_seed.c`,
`lanes/s2-slice1/result.md`): the type `adf_rootlist`, `adf_root_padic_from_seed`, the entries verifier at
a prime, accessors. This lane adds the search for all roots in `Z_p`. A review of slice 1 runs at the
same time: do not change what the existing functions return; `tests/test_roots_seed.c` must pass
unchanged.

Read first: `lanes/COMMON.md` (rule 3: every test under `timeout`), `lanes/COMMON-C.md` (rule 6 does not
hold for `roots.h`; rule 5 is replaced by item 5 below); `docs/proofs/solvers.md` section 3: Propositions
3.4, 3.5 with Algorithm P, 3.6, 3.7, 3.11, 3.13; `docs/api-s.md` section 4 with its notes; decisions
S-D10, S-D15, S-D17, S-D18 in `docs/SPEC.md` 15.3; `proto/solvers_checks.py` (the reference of Algorithm
P, `PADIC_CASES`, the `check_s2_*` functions).

**You own:** `include/adelefeld/roots.h` and `src/roots.c` (additions), `tests/test_roots_padic.c` (new),
`tests/ref/vectors/s2-slice2/` (new), `tests/julia/roots.jl` (additions),
`tests/fuzz/diff_roots_padic.py` (new), `lanes/s2-slice2/`. Everything else is read-only.

## What is built

1. `adf_roots_padic`, `adf_roots_padic_partial`, `adf_rootlist_get_unresolved`,
   `adf_rootlist_verify_complete` (at a prime; at the real place it returns 0, marked temporary),
   as `docs/api-s.md` section 4 states them.
2. The roots modulo `p` are found by evaluation at every residue (`solvers` P3.7(1)). Decision S-D10: a
   bound on `p` is a property of this slice, not of the library. Above `ADF_ROOTS_P_EVAL_MAX = 2^20`
   the functions return `ADF_UNSUPPORTED`, `L` untouched, and the header says that this status is temporary
   and goes away with the slice that adds the route of P3.7(2). No routine of FLINT for roots or
   factorisation modulo `p` is called.
3. Tests first, red then green (`lanes/s2-slice2/redgreen.log`), in `tests/test_roots_padic.c`:
   - Against an enumeration written in the test: `p` in 2, 3, 5, 7; polynomials of degree up to 4 with
     small coefficients, products with planted integer roots (also repeated, also roots congruent modulo
     high powers of `p`), `prec_p` 1 to 6, depths 0 to 8. For every result: every `x` modulo `p^M`
     (`M` large enough, stated in the test) with `g(x) = 0` modulo `p^M` lies in exactly one listed ball or
     unresolved class; every listed ball contains exactly `p^s` such `x` modulo `p^(K+s+3)` that are
     congruent to the centre modulo `p^(s+1)`; `complete = 1` exactly when no class is unresolved; the
     strict function returns `NOT_DETERMINED` with `L` untouched exactly when the partial one has
     `complete = 0`. Print the counts of complete and incomplete lists, of empty lists, of lists with
     `s > 0`; fail if one is 0.
   - The 31 named cases of `PADIC_CASES` with their numbers of roots and the depth limits 0, 1, 3, 8, as
     vectors from the reference (`lanes/s2-slice2/gen_vectors.py`, import, do not copy); `x^2 + 1` at 2
     (note 1 of section 4); the zero polynomial, constants, degree 1.
   - `adf_rootlist_verify_complete`: every complete result is accepted at the depth that produced it;
     changed lists (a certificate dropped, two balls merged, a ball moved off its root, a class kept with
     `complete = 1`, a false `n`, a depth one below the least depth that completes the run) are refused;
     `f = X (X - 1)` at 3 with an empty list called complete passes the entries verifier and fails this
     one; no list of the seed function passes it. Print the number refused for each kind of change; fail
     if one kind has none.
   - Limits: exponent overflow (`e + j`, `k + s`, `2 k - s`) and `ADF_ROOTS_BITS_MAX` give `LIMIT` before
     allocation; `p` just above `2^20` gives `UNSUPPORTED`; `depth < 0`, `prec_p < 1`, the real place give
     `DOMAIN`; `L` untouched on each.
4. `tests/julia/roots.jl`: all roots of `(X^2 - 2)(X - 3)` in `Z_7`, read through the accessors.
   `tests/fuzz/diff_roots_padic.py --seconds N --seed S`: random polynomials, primes up to 101, precisions
   and depths to the C functions and to the reference: equal status, equal lists (balls and classes). Run
   it for 180 seconds under `timeout`; that run is a smoke test.
5. Show that the tests bite, in a scratch copy under `build/`, one change at a time, recorded in
   `redgreen.log`: the condition `g'(b) != 0` of a simple root removed; `j > w - 2e` changed to `>=`;
   `k > s` changed to `>=`; one residue left out of the evaluation (the loop runs to `p - 1`); a class
   dropped instead of kept as unresolved at the depth limit; `complete` set to 1 with a class unresolved.
   No `make mutate`.
6. `make clean && make check-all`, `make clean && make -j2 check SAN=1`, `make clean && make -j2 check
   CC=clang`, `sh lanes/m1-headers/check_headers.sh` pass. Give the last line of each. (`make check-all`
   contains the memory checker of `tools/memcheck`, which refuses a variable that is used or cast before
   an init.)

Not in this lane: `adf_roots_real`, `get_arb`, the route of P3.7(2) for larger primes, `set`, `swap`,
`identical`, benchmarks.

If a statement of `docs/proofs/solvers.md` section 3 is wrong or cannot be implemented as written, give the
counterexample in the report.
