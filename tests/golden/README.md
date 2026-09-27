# tests/golden: fixed vectors for the text forms

Format and use: `docs/conventions.md` section 11 (lines `input<TAB>expected`, escapes `\\ \t \n \r \xHH`,
`@gen:PREFIX|UNIT|COUNT|SUFFIX` for long inputs, `!STATUS` for an expected status). Written by the lane
`m0-conventions` from the hand-computed vectors in `lanes/m0-conventions/write_golden.py`; checked by
`python3 -m unittest proto/test_text_grammar.py` against the reference `proto/text_grammar.py`.

| File | Vectors | of which a status | What |
|---|---|---|---|
| `rat.tsv` | 51 | 29 | value form of `adf_rat` |
| `fball.tsv` | 68 | 22 | `adf_fball`, with every finite ball of `SPEC.md` 4.2, 4.3, 4.4 and 6 |
| `adele.tsv` | 69 | 28 | `adf_adele`, including the rounding rules of the real ball |
| `cadele.tsv` | 16 | 10 | `adf_cadele` |
| `ucoset.tsv` | 38 | 16 | `adf_ucoset`, normal form (`SPEC.md` 5) |
| `idele.tsv` | 24 | 12 | `adf_idele`, constrained printing |
| `idclass.tsv` | 15 | 8 | `adf_idclass` |
| `lball.tsv` | 52 | 23 | `adf_lball` |
| `sball.tsv` | 22 | 12 | `adf_sball` |
| `qclass.tsv` | 29 | 12 | `adf_qclass` (pieces: PENDING part B) |
| `ffun.tsv` | 18 | 12 | `adf_ffun` |
| `rfun.tsv` | 14 | 8 | `adf_rfun` |
| `char.tsv` | 19 | 8 | `adf_char` |
| `dispatch.tsv` | 29 | 5 | type of a text (`adf_text_classify`) |
| `realball_read.tsv` | 28 | 13 | a real ball read as an exact interval |
| `realball_print.tsv` | 23 | 0 | the printing algorithm of `docs/conventions.md` 9.5 on exact dyadic balls |
| `dump.tsv` | 151 | 99 | dump form, version 1 |
| **total** | **666** | **307** | |

Every example of `PLAN.md` section 5 and every finite ball and unit coset written in `SPEC.md` sections 4.2, 4.3,
4.4, 5 and 6 appears as an input. The hostile inputs include: embedded NUL, non-ASCII bytes (UTF-8 minus sign,
full-width digit, no-break space, `±`), an input of 1048577 bytes and one of exactly 1048576 bytes, decimal and
p-adic exponents beyond the limits, deep nesting, leading zeros, negative moduli, zero denominators, primes at and
beyond 2^64, and the `arb` dump strings on which FLINT 3.0.1's `arb_load_str` aborts.

To regenerate after changing a vector: `python3 lanes/m0-conventions/write_golden.py` from the repository root.
