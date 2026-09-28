# Rules for every lane that writes C for adelefeld (in addition to COMMON.md)

You work in your own git worktree; other lanes work in theirs. The public headers under `include/` are fixed for
you: you implement what they declare. Read the comment block of each declaration: it names the set statement,
the proof or convention, the aliasing rule and the statuses. Read the cited proof statement before you write the
function (CLAUDE.md rules 3 and 4), and cite it in the code as `docs/proofs/<file>.md:<line>` or
`docs/conventions.md` section, in a comment at the function.

1. **Red-green.** For each function: first the test, run it and see it fail (a link error counts only for the
   first test of a file; after that the failure must be an assertion), then the code. Keep a short log of the red
   and green runs in your lane directory and summarise it in the report.
2. **Oracles.** The Python reference `tests/ref/` is the oracle; its vectors are under `tests/ref/vectors/*.jsonl`
   and are read in C with `tests/support/jsonl.h` (see `tests/README.md`, `tests/test_support.c`). Every vector
   file that concerns your functions must be run completely by your tests. Add tests for what the vectors do not
   cover: aliasing of every permitted combination of arguments (output equal to first input, to second, to
   both), exact zero, radius zero, negative and huge operands (thousands of bits), and every status with the
   state of the outputs that the header promises. If you need vectors that do not exist, generate them with a
   script in your lane directory that uses the Python reference, and put them under
   `tests/ref/vectors/<your-lane>/`.
3. **Enclosure first.** A result must contain the true value for every input the header admits. Where a function
   cannot certify its result it returns the status the header names; it never guesses.
4. **FLINT idiom.** Use FLINT's functions and conventions (`fmpz`, `fmpq`, `arb`); init and clear every
   temporary; no leaks and no undefined behaviour: `make check SAN=1` must pass. Build with `make -j2`. No
   global state. No allocation beyond what FLINT does, unless the header says the function allocates.
5. **Mutation testing** (CLAUDE.md rule 2): run `make mutate FILES=src/<your file>.c` (read `tests/README.md`;
   use at most 2 jobs; if the run would take more than 10 minutes, use the tool's limit and seed options and say
   so). Every surviving mutant is either killed by a new test or listed in `tools/mutate/equivalent.txt` with the
   reason why it cannot change any result. Report the counts.
6. **The header is not yours.** If a declaration cannot be implemented as written, is ambiguous, or contradicts
   its proof, do not edit the header: implement the most defensible reading, mark the place with a comment
   `HEADER-FINDING`, and list it in your report.
7. **Performance.** Write the simple correct version. Do not optimise. Note in the report where you see an
   avoidable cost (a gcd that could be skipped, a temporary, a canonicalisation done twice).
