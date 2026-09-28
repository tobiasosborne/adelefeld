# Reviewer `text`: the value form (parser, printer, classifier) and the public headers

Read `lanes/m1-review/COMMON.md`; it binds you. Author: Claude opus. You are of another model family.
Files under review: `src/text.c`, `tests/fuzz/fuzz_text.c`, the four `tests/test_text_*.c`; and the public
headers `include/adelefeld/*.h` with `docs/api-m1.md` (its 15 choices where the conventions were silent).
`docs/conventions.md` sections 8, 9, 11, 12. Reference: `proto/text_grammar.py`. Golden: `tests/golden/`.

Look in particular at: reading a real ball: is the `arb` an enclosure of the decimal `m +/- r` for every
accepted text, at every `prec` from 2 up, for exponents at the limits, for midpoints that are halfway
cases, for radius 0 and for a radius much larger or much smaller than the midpoint (write your own oracle
with exact rationals and test at least 10^5 random texts and the boundary cases); printing: does the
printed text, read back, contain the ball that was printed (conventions 9.6), for every `digits`; the cost
of printing in the binary exponent (the lane reports that an exponent like 2^30 exhausts memory, issue
adf-b8l: confirm, measure where it becomes unusable, and propose the rule); the order of checks of
conventions 8.5 with two faults in one text; every limit at the limit and one above; accepted texts that
the reference refuses and refused texts that the reference accepts (differential test against
`proto/text_grammar.py`, at least 10^5 texts from a grammar-aware generator of your own); the ten mutants
of `src/text.c` that the lane proposes as equivalent (`lanes/m1-text/report.md`): judge each; the headers:
a declaration whose comment promises what no implementation can keep (the lanes found `arb_add_fmpq` named
in `adele.h`, "never aborts" in `fball.h`, "a printer never fails" in `text.h`): list every such promise.
