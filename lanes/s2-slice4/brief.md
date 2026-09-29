# Lane s2-slice4: roots modulo `p` for the primes above `2^20` (issue adf-e0n)

`adf_roots_padic` and `adf_roots_padic_partial` (`src/roots.c`) find the roots modulo `p` by evaluation at
every residue and return a TEMPORARY `ADF_UNSUPPORTED` for `p > ADF_ROOTS_P_EVAL_MAX = 2^20`. Decision
S-D10 (`docs/SPEC.md` 15.3): the library accepts every prime of a place (below `2^64`, proved prime by
`adf_place_prime`); above a bound the roots modulo `p` are found by a routine of FLINT, each is tested by
evaluation, and completeness is certified by `deg gcd(g, X^p - X)` (`docs/proofs/solvers.md` Proposition
3.7(2)). This lane builds that route and removes the status.

Read first: `lanes/COMMON.md` (rule 3: every test under `timeout`), `lanes/COMMON-C.md` (rule 6 does not
hold for `roots.h`; rule 5 is replaced by item 5 below); `docs/proofs/solvers.md` Proposition 3.7 and
Algorithm P; `docs/sources.md` (the rows of lane s2-sources about `nmod_poly`, and its list "Sources
pending"); `lanes/s2-sources/report.md`; `lanes/s2-slice2/result.md`; `include/adelefeld/roots.h`.

**You own:** `include/adelefeld/roots.h` and `src/roots.c` (the search of the roots modulo `p` and the
sentences about the temporary status; nothing else), `tests/test_roots_bigp.c` (new),
`tests/test_roots_padic.c` and `tests/test_roots_forged.c` (ONLY the checks that expect `UNSUPPORTED`
above `2^20`), `tests/fuzz/diff_roots_padic.py`, `bench/bench_roots_modp.c` (new),
`refs/fetch_sources.sh`, `refs/manifest.sha256` (additions), `docs/sources.md` (new rows),
`lanes/s2-slice4/`. Everything else is read-only.

1. Sources first (CLAUDE.md rules 3, 4). The root finding of `nmod_poly` is not documented in FLINT 3.0.1.
   Fetch the C sources at the tag v3.0.1 that you will rely on (`nmod_poly_factor/roots.c`, the powering
   and gcd routines you call) with `refs/fetch_sources.sh`, in the way the script fetches the other C
   sources; read them; write rows of `docs/sources.md` with file, line and the quoted words: what the
   routine requires (squarefree, monic, prime modulus), what it returns, whether it is randomised (then:
   can it fail, and how does it say so), what it does for degree 0 and for the zero polynomial.
2. The route: reduce `g` modulo `p`; `d = gcd(g mod p, X^p - X)` with `X^p` computed modulo `g` by
   powering; the candidates from FLINT's root routine on `d` (or on `g mod p`); EVERY candidate tested by
   evaluation of `g` modulo `p`; the list accepted only if the number of distinct roots that passed equals
   `deg d`; if not, the library's own fallback (say which; a wrong or short list from FLINT must never
   become a wrong answer: it becomes `NOT_DETERMINED` or an abort by decision S-D20, choose and justify).
   A polynomial that is 0 modulo `p` after the content is removed cannot occur (Lemma 3.1): assert it.
3. The bound: below `ADF_ROOTS_P_EVAL_MAX` the evaluation at every residue stays. Measure both routes with
   `bench/bench_roots_modp.c` for degree 2, 8, 32 and `p` near `2^8, 2^12, 2^16, 2^20`, give the table, and
   set the constant to the crossover you measure (a power of two). The machine is busy with other lanes:
   say so next to the numbers.
4. Tests first, red then green (`lanes/s2-slice4/redgreen.log`), in `tests/test_roots_bigp.c`:
   - For primes just below the bound both routes are run (through a hidden function that forces the
     route, declared in the test as the other hidden functions of `src/roots.c` are) and give identical
     lists of roots modulo `p`, for random polynomials, products with planted roots, polynomials with no
     root, with every residue a root (`X^p - X` times a constant plus `p h`), degree 0 and 1, degree above
     100.
   - For primes of 21, 32, 48, 63 and 64 bits (`2^64 - 59`): products with planted roots in `Z`; the
     complete list of `adf_roots_padic` has exactly the planted roots (as balls), by an oracle that is
     your own argument (the only roots in `Z_p` of a product of linear factors with distinct roots modulo
     `p` are the planted ones; choose the roots so), and passes both verifiers.
   - A deliberately wrong candidate list (through the hidden function, a root removed, a non-root added)
     is refused.
5. Show that the tests bite, in a scratch copy under `build/`: the test of each candidate by evaluation
   removed; the comparison with `deg d` removed; `X^p` replaced by `X^(p-1)`; the reduction modulo `g`
   dropped from the powering.
6. `tests/fuzz/diff_roots_padic.py`: primes above the bound added (the reference finds roots modulo `p`
   by its own method; if the reference cannot, extend `proto/solvers_checks.py` is NOT yours: use planted
   roots in the script). Run it 180 seconds under `timeout`: a smoke test.
7. `make clean && make check-all`, `make clean && make -j2 check SAN=1`, `make clean && make -j2 check
   CC=clang`, `sh lanes/m1-headers/check_headers.sh` pass. Give the last line of each.
