# u-review1 progress

- Built libadelefeld.a plain and SAN=1 INV=1 (build/, build-san/).
- w/ref.py: my own reference (conventions 10.1, 10.2, 5.6 to 5.9, 8.4, 8.5). w/fz.c: harness, one fork per
  case. w/gen.py: grammar-directed generator + comparison. w/sb.py: structured sball edits.
- FINDING 1 (BLOCKER) dp_arb_sign (src/dump.c:625-660): `am.p++` without `am.n--` for a negative mid mantissa;
  reads one byte too far (the following space, or the next token), so the ball is misjudged: abort in
  adf_idele_load_str (flint_abort at dump.c:2229) on `adf1 Q idele 1 -1 0 1 0 1 1 1 0`.
- FINDING 2 (BLOCKER) same function, same-binade case with bm > br: `res = top > rt` should be `>=`
  (mid odd, so a tie of the leading bits means |mid| > rad): `adf1 Q idele 1 3 0 1 1 1 1 1 0` is DOMAIN, true OK.
