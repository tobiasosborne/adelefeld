# tools/mutate/example: the example the mutation tool is tested on

A minimal tree with the same interface as the repository, so that `tools/mutate/mutate.py`
works on it without an option: a `Makefile` with a `check` target, one source file and two
tests. `tools/mutate/selftest.py` copies it under `build/mutate/selftest/`, adds
`tests/test_runner.h` from the repository, and runs the tool twice.

| File | What |
|---|---|
| `src/example.c`, `src/example.h` | three small functions of `docs/SPEC.md` 4.2 and 4.3: the equality of two finite balls with denominator 1, the greatest common divisor, and the radius of a sum as a named call |
| `tests/test_weak.c` | a deliberately weak test: the diagonal of the equality, and one value of the sum radius. A mutant that changes the way `h` enters the predicate, the loop of the remainder or the order of the arguments passes it |
| `tests/test_strong.c` | the same claims by enumeration over every small case, against a reference computed in the test |

The self-test requires:

- with the weak test, at least one mutant survives and the tool says so;
- with the strong test, no mutant survives; the one exception is the exchange of the two
  arguments of `adf_example_gcd(n, m)`, which computes the same value because the greatest
  common divisor is symmetric. The self-test writes that mutant into an equivalent file, so the
  run also shows that a listed survivor is excused and does not make the tool fail;
- at least one mutant of the strong run does not compile and at least one times out, so that
  both reports are exercised.

    make mutate-selftest

The run of the tool itself, on the example, with every mutant:

    python3 tools/mutate/mutate.py --root tools/mutate/example --files src/example.c \
        --scratch build/mutate/example --list
    python3 tools/mutate/mutate.py --root tools/mutate/example --files src/example.c \
        --scratch build/mutate/example --timeout 5

`--list` prints the mutants without building anything, which is the quickest way to see what
the tool changes.
