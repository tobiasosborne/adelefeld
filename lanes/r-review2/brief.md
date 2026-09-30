# Lane r-review2: bug hunt through the second slice of the real roots

A hunt for defects, with your own oracle. The code was written by codex gpt-6.1-sol in lane r-slice2
(`lanes/r-slice2/result.md`): in `src/roots_real.c` and in `adf_roots_real` of `src/roots.c`: (1) the
count of FLINT steers the isolation (count 0: no isolation; the descent stops when points and one-root
cells account for the count); (2) endpoint contraction toward clusters; (3) the count is taken on a
translated and scaled polynomial; (4) the refinement evaluates a fixed local polynomial, with `arb`
filters that return a sign only when certain, else the exact integer calculation; the final
certificate evaluates exact integers, with its own dyadic translate. Design and proofs:
`docs/design/real-roots.md`, R6 to R9. Contract: `include/adelefeld/roots.h`. Review of the first slice:
`docs/reviews/r1/review-real-isolation.md` (its programs in `lanes/r-review1/`).

The author's claim to break: "the final certificate is unchanged in what it proves and uses exact
integers only, so a wrong filter or a wrong early stop can cost time and cannot produce a wrong list".

**You own:** `lanes/r-review2/` only. Everything else is read-only. No git command that changes state,
no `bd`. At most 2 cores. Every program under `timeout`, none longer than 3 minutes. Build with
`make -j2 BUILD=lanes/r-review2/build` and link your programs against that archive.

Hunt, in this order:
1. Read the final certificate (`real_finish` and what it calls now) line by line: does every ball that
   is output get an exact sign test at both end points on the polynomial `g` itself (or on a
   translate that you can prove equal), and is the number of balls compared with a count that does not
   come from the isolation? Is there a path on which a value computed with `arb` decides a sign, an
   end point, an order, or a status in the OUTPUT? Name file and line for each step of your argument.
2. The scaled count (R9): inputs where translate and scale change the polynomial in a way that the
   proof does not cover (scale by a negative number, a translate that is a root, a polynomial that is
   not squarefree before normalisation, content). Compare the count with your own Sturm chain in exact
   rational arithmetic (Python `fractions`), 2000 random and constructed polynomials.
3. The early stop: polynomials where the count is reached while a cell that is already on the stack
   holds a root too (it cannot, if the count is true: try to make the cells that were counted as
   "one root" hold none or two; multiple roots in `f`; roots at cell end points; exact dyadic roots).
4. The filters: polynomials of degree 8 to 60 whose values at the refinement points are tiny relative
   to the coefficients (roots that are nearly dyadic; `(2^k X - 1)^d - 2^-m`-like constructions scaled
   to integers; Wilkinson 30), at `prec` 2, 64, 1000. Every list against your oracle: planted roots
   inside the balls, balls disjoint and ordered, accuracy of S-D19, both verifiers accept.
5. Nesting: for 500 inputs and `prec1 < prec2`, the ball at `prec2` lies inside the ball at `prec1`.
6. Memory and time: valgrind (`~/.local/bin/valgrind`) on 20 inputs; any input of degree at most 50 with
   coefficients of at most 10^4 bits that takes more than 5 s.

Result: `lanes/r-review2/result.md` and the same text as your final message: each defect with severity
(BLOCKER: a wrong list with `OK`, a memory fault; MAJOR; MINOR), its input, what the code returns, what is
true, and the command that shows it; your argument for item 1 with file and line; then what you tried
without result, with counts and what would have made a case fail. No praise, no summary of the code.
Leave no compiled binary in your lane directory.
