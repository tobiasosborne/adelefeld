# lane q-review2 progress

Adversarial review of the first slice of milestone 3 (lane q-slice1), 2026-10-06.

## Builds

- `timeout 600 make -j2 BUILD=lanes/q-review2/build lanes/q-review2/build/libadelefeld.a` -> exit 0.
- `timeout 600 make -j2 SAN=1 INV=1 BUILD=lanes/q-review2/build-san lanes/q-review2/build-san/libadelefeld.a`
  -> exit 0.

## Attack 1: is_canonical, clause by clause

- Harness `can_eval.c` builds PIECES values by hand from a description on stdin (global backend,
  four local contexts, raw field faults) and prints the verdict of `adf_qclass_is_canonical` plus
  the canonical triple read back from C.
- Oracle `can_oracle.py`: exact dyadic arithmetic (no huge powers of two are ever materialised),
  my own statement of the clauses of conventions 5.10 with CV-45 and docs/api-3.md section 1.
- Runs: seeds 1 (300), 7, 11, 23 with 20000 values each: 0 disagreements.
  Clause coverage per 20000 values: d != 1 (about 5800), midpoint (2800), order (1900),
  member not canonical (1500), len (800), lift len != 1 (600), null array (560), tag (580),
  and 5500 values the oracle calls canonical.
- Planted fault check of the harness itself: midpoint `< 0` made `<= 0` in a scratch copy of
  src/qclass.c -> 127 disagreements in 2000 values.
- `ASAN_OPTIONS=detect_leaks=1 ./can_eval_san` over the 20000-value file: exit 0, no finding,
  output byte-identical to the plain build.
- Hand cases at binary exponents +-2^60 and with a 10^6-bit mantissa (exact four-term
  cancellation): `hand_cases.txt`, `hand_cases2.txt`, all as derived by hand.

## Attack 2: the text of the lift form

- Harness `text_eval.c` drives `adf_qclass_set_str` and `adf_qclass_get_str`, prints the status,
  whether the destination changed (adf_qclass_identical against a copy), canonicality, the printed
  text and the exact stored ball.
- `text_oracle.py`: my own reader (conventions 9.1 to 9.3) and my own printer (9.4, 9.5) in exact
  rationals, a table of 72 texts with the status expected from conventions 8.2 to 8.5, every cut of
  a valid text, and 500 random valid lifts. Compared with proto/text_grammar.py where it runs.
- One difference is expected and documented by the lane: C prints 1.1e-5 where the exact-rational
  reference prints 1e-5 (conventions 9.6, C may widen).

## Attack 3: the represented set

- `member_eval.c` + `member_oracle.py`: membership of rational points of A/Q, exact:
  (t ; w) is in the class of (I x (a + N Zhat)) iff there is q in Q with t + q in I and
  w + q in a + N Zhat; decided with one integer-existence test on the scaled interval.
- 2000 lifts, 200 points each, 1.2 million decisions, before and after `add_rat` by rationals of
  1 to 2000 bits: 0 problems. Aliasing (set, swap, add_rat with one object), the lifetime promise
  (the source adele cleared right after set_adele), get_piece at -1, len, LONG_MAX, LONG_MIN.

## Attack 4: memory

- `cycle.c`: 1000 init / hand-built PIECES / set / swap / clear cycles with a borrowed context.
  Plain build: 0 problems. Sanitised build with detect_leaks=1: exit 0, no report.
  Valgrind --leak-check=full: exit 0, no error. LeakSanitizer does run in this environment.
