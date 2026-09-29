# Lane i-review1: bug hunt through unit cosets and ideles (milestone 2, slice 1)

A quick hunt for defects, not a full review. The code was written by lane i-slice1:
`include/adelefeld/ucoset.h`, `idele.h`, `src/ucoset.c`, `src/idele.c`, `tests/test_ucoset.c`,
`tests/test_idele.c`. Contract: the headers; `docs/api-2.md` section 1 (statements A to E);
`docs/proofs/ideles.md`; `docs/conventions.md` 5.6, 5.7. Report of the author:
`lanes/i-slice1/result.md`.

**You own:** `lanes/i-review1/` only. Everything else is read-only. No git, no `bd`. At most 2 cores.
Every program under `timeout`, none longer than 3 minutes. Build with `make -j2`, link your programs
against `build/libadelefeld.a`.

Hunt, in this order, with small C programs and your own oracle in exact arithmetic (Python `fractions`):
1. The real kernel (`mul`, `inv`, `set_rat` of ideles): a result whose real ball does not contain an
   end point of the true product set or contains 0; a sign lost; `OK` where the header says
   `NOT_DETERMINED` or the reverse; precisions 2, 3, 64, 4096; end points with exponents near
   `+-2^60`; exact inputs; inputs with radius larger than the midpoint in absolute value (refused?).
2. Unit cosets: `mul`, `inv`, `normalise`, `contains`, `equal_set`, `overlaps` against enumeration in
   `Z/M` for all moduli up to 64 and both exact units; `N` = 1, 2, 4; `c` negative or above `N`; moduli
   of thousands of bits.
3. Memory and states: aliasing of every combination; outputs on every status other than `OK`; a leak
   (valgrind at `~/.local/bin/valgrind`); an abort reachable from valid input.
4. Sentences of the headers that the code does not keep; tests that cannot fail.

Result: `lanes/i-review1/result.md` and the same text as your final message: each defect with its input,
what the code returns, what is true, and the command that shows it; then what you tried without result,
with counts. No praise, no summary of the code.
