# tests/ref: Python reference for the ring

The oracle for the C implementation (work package 1.1). It is written from the proofs in
`docs/proofs/precision.md` and the tables of `docs/SPEC.md` sections 4.1 to 4.4, for clarity, not
speed. It uses `fractions.Fraction` and Python integers only.

```
adfref/rat.py         qgcd, qlcm, is_integer, v_p
adfref/fball.py       the finite ball (A + H Zhat)/d; tight add, sub, neg, mul, scale;
                      equal_set, overlaps, contains, compare; contains_rational
adfref/policies.py    scaled residue (s (u + K Zhat)) and absolute cap; convert_from_tight
adfref/recon.py       rational reconstruction from a full ball and a closed real interval
adfref/membership.py  membership of a rational, from the definition, without ball arithmetic
tests/                unittest suite (run with the command below)
mutants.py            mutation harness: prints mutants killed and surviving
gen_vectors.py        writes the JSON lines in vectors/
vectors/*.jsonl       deterministic test vectors for the C tests
```

Run the suite:

    python3 -m unittest discover -s tests/ref/tests

Run the mutation harness alone:

    python3 tests/ref/mutants.py

Regenerate the vectors (deterministic, seed 20260927):

    python3 tests/ref/gen_vectors.py

## The value

A finite ball is the set `(A + H Zhat)/d`, `A`, `H`, `d` integers, `H >= 0`, `d > 0`. Its centre
is `a = A/d`, its radius is `N = H/d`. Radius 0 is the single point `A/d`. Canonical form (SPEC
4.1): for `H > 0`, `0 <= A < H` and `gcd(A, H, d) = 1`; for `H = 0`, `A/d` in lowest terms.

The tight rules (SPEC 4.3, precision.md Propositions 1 and 2):

    (a + N Zhat) + (b + M Zhat) = (a + b) + gcd(N, M) Zhat
    (a + N Zhat) * (b + M Zhat) is contained in a b + gcd(a M, b N, N M) Zhat
    q (a + N Zhat) = q a + |q| N Zhat

The predicates (SPEC 4.2, Proposition 3), with radius 0 as a point: `equal_set` is `N = M` and
`(a - b)/N` an integer; `overlaps` is `(a - b)/gcd(N, M)` an integer; `contains` (first inside
second) is `N/M` an integer and `(a - b)/M` an integer.

## Vector format

Each file is JSON lines. Every line is one object. Integers are JSON integers; a rational is
`{"num": n, "den": d}` in lowest terms with `den > 0`; a ball is `{"A": a, "H": h, "d": d}` in
canonical form.

| File | `op` | Fields | Result |
|---|---|---|---|
| `canonical.jsonl` | `canonical` | `input`: `[A, H, d]` | `result`: `[A, H, d]` canonical |
| `add.jsonl` | `add` | `a`, `b` | `result` |
| `sub.jsonl` | `sub` | `a`, `b` | `result` |
| `mul.jsonl` | `mul` | `a`, `b` | `result` |
| `neg.jsonl` | `neg` | `a` | `result` |
| `scale.jsonl` | `scale` | `a`, `q` | `result` |
| `predicates.jsonl` | `equal_set`, `overlaps`, `contains` | `a`, `b` | `result`: boolean |
| `compare.jsonl` | `compare` | `a`, `b` | `result`: one of the three comparison strings |
| `membership.jsonl` | `membership` | `ball`, `x` | `result`: boolean |
| `recon.jsonl` | `reconstruct` | `ball`, `lo`, `hi` | `status`: `none`, `one` or `several`; `solutions`: list |
| `policies.jsonl` | `convert_from_tight` | `ball`, `K` | `value`: exact or scaled; `lost`: boolean |
| | `scaled_add`, `scaled_mul` | `a`, `b` (scaled values) | `result` (scaled value) |
| | `absolute_cap` | `ball`, `C` | `result` |

A scaled value is `{"s": q, "u": n, "K": n}` with `s > 0` and `0 <= u < K`. `scaled_add`, `scaled_sub` and
`scaled_mul` require the two operands to share `K`; a mismatch raises `ValueError` (G9 of the gate review). To
combine different contexts, convert each operand into a caller-selected common modulus (for example `lcm(K, K')`)
with `convert_from_tight`, which is lossless when the old `K` divides the target, and then call the same-context
operation. A `value` that is `{"exact": q}` is an exact rational (radius 0).

The C tests read a line, build the inputs, run the operation and compare with the recorded result.
They must not read `adfref`; they use the vectors as a fixture.
