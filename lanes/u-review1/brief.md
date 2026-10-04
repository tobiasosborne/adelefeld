# Lane u-review1: adversarial review of the dump forms of five types (lane u-dump1) and of the driver's ball kinds (lane drv-ball)

Your task is to REFUTE, not to confirm. Two lanes of the free pi agent (`stealth/space-bunny-alpha`; code by
this model has not been reviewed before in this area) landed on 2026-10-04/05:
- lane u-dump1 (`lanes/u-dump1/report.md`, commit `5ae35c9` and its parent): the dump form of conventions 10
  for `adf_ucoset`, `adf_idele`, `adf_idclass`, `adf_lball`, `adf_sball`: twenty functions (`load_str`,
  `load_str_binds`, `dump_str`, `dump_inspect` per type) in `include/adelefeld/dump.h` and `src/dump.c` (about
  700 added lines: `git diff 840e150~3 840e150 -- src/dump.c` or read the functions named in the report);
  tests `tests/test_dump_units.c`, `tests/test_dump_local.c`; the driver's `dump` and `load` for these kinds.
- lane drv-ball (`lanes/drv-ball/report.md`, commit `ca26a21`): `tools/adf/adf.c`: `lball` and `sball` as
  value kinds; `adf_drv_sball_restrict`; a stored partial ball as operand of the commands at places.
Contract: `docs/conventions.md` section 10 (grammar 10.1, rules 10.2: strict canonical loaders, "the loader
validates the whole text before any FLINT load function sees any of it", CV-52), 5.6 to 5.9 (the canonical
predicates), 8.4, 8.5 (limits; the order of the checks), 12.4; the comment blocks of `dump.h`; the headers
`ucoset.h`, `idele.h`, `idclass.h`, `lball.h`, `sball.h` (the predicates `adf_x_is_canonical`).
`proto/text_grammar.py` is the project's Python reference; the author checked 162 texts against it.

**You own:** `lanes/u-review1/` only. Everything else is read-only. No git command that changes state, no
`bd`. At most 2 cores. Build the archive once: `timeout 600 make -j2 BUILD=lanes/u-review1/build
lanes/u-review1/build/libadelefeld.a` (and once more with `SAN=1 INV=1` into `lanes/u-review1/build-san` for
the memory work), link your programs against it (`-Iinclude -lflint -lgmp -lm`). Do not run the repository's
suites as a whole. Every program under `timeout`, none over 170 s.

A loader reads untrusted text. BLOCKER: a memory fault or undefined behaviour on any input; `OK` with a value
that is not canonical (`adf_x_is_canonical` returns 0, or a later library call aborts under `INV`); `OK` for
a text outside the grammar or the predicates; two different texts loading to identical values; a value whose
dump does not load back identical; an output touched on a status other than `OK`.

Hunt, in this order; stop a line of attack after some thousands of cases without a finding:
1. **Your own reference** for the five bodies, written from conventions 10.1, 10.2 and 5.6 to 5.9 (Python,
   exact integers; do not import `proto/text_grammar.py` for the verdict, but DO compare your reference with it
   and with the C loader three ways: a disagreement between the two references is a finding against one of
   them, with the sentence of the conventions that decides).
2. **Grammar-directed fuzzing, differential**: start from valid dumps of random canonical values of each type
   (built through the public constructors; integers of 1 to 4000 bits; primes 2, 3, `2^64 - 59`; partial balls
   with 0 to 40 primes and each archimedean tag `n`, `r`, `c`); mutate: delete, duplicate, swap a token; flip a
   byte; upper-case a hex digit; leading zeros; `-0`; a sign on an unsigned field; two spaces, tabs, a trailing
   space or newline, an embedded NUL (the readers take a length); a count one too small or too large; a
   mantissa made even; a radius mantissa of 30 and 31 bits; exponents of 63, 64, 65 and 5000 bits; `p`
   composite, 0, 1, `2^64 - 1`, `2^64`; `v`, `N` at `LONG_MAX`, `LONG_MIN`, one beyond; primes out of order or
   repeated; a body name of another type; truncated at every byte. At least 200000 mutated texts per type:
   status of the C loader against your reference; on a status other than `OK` the output is untouched (compare
   the representation); on `OK`: `is_canonical`, dump back equals the text byte for byte, and
   `dump_inspect` gives the same status and `*nctx = 0`. The limits struct: each field at 0, 1, the exact size
   of the text, one less; `load_str_binds` with `nbinds` 0, 1 and a NULL array.
3. **The hand-filled structs.** The loaders of `adf_lball` and `adf_sball` write struct fields directly (report
   section 5). For 20000 loaded values of each: every public function of `lball.h` and `sball.h` that takes
   one (arithmetic, `decompose`, `get_center`, `project`, the functions at places, the text printers) under
   the `INV` build: an abort is a BLOCKER (the loader accepted what the library calls not canonical). The
   complex tag of a partial ball: what do the functions at places do with it? Compare with a value of the
   same set built by the public constructors: `identical`?
4. **Memory**: the fuzz programs of 2 under `-fsanitize=address,undefined` (`build-san`); failure paths after
   an allocation (the `loc` array of a partial ball when entry `k` fails; a NULL-returning printer); leaks:
   your sandbox may not run LeakSanitizer (ptrace); try `valgrind --leak-check=full --error-exitcode=77` on a
   reduced run (5000 texts per type); say what you could run.
5. **The driver** (`build/adf` built by `timeout 300 make -C tools/adf BUILD=...`; read `tools/adf/README.md`):
   `dump` and `load` of the five kinds, 100 hostile lines; `load` of a dump followed by arithmetic and by the
   commands at places; `adf_drv_sball_restrict` (lane drv-ball): a place list with repeats, places not in
   the ball, the real place of a complex-tagged ball, 1000 places, the empty partial ball `{}`; every command
   of the driver with an `lball` or `sball` operand was swept by the orchestrator under the sanitizers (37369
   lines, no report): do not repeat that; look for what a sweep of single lines cannot see (state carried
   between lines: `prec` and `digits` changed between the reading and the printing of a stored ball).
6. **Tests that cannot fail**: plant twelve faults of your own in a scratch copy of the added code of
   `src/dump.c` (the positivity check of the class's real ball dropped; `v` and `N` exchanged in the ball
   form; the primality test skipped; strict increase of primes made non-strict; the count compared with `<`;
   the archimedean count accepted when 0; `max_items` off by one; the exact form `x` accepting `den(u)`
   divisible by `p`; a unit modulus canonicalised on load; output written before the last check; ...), build
   `test_dump_units` and `test_dump_local` against each and record which are detected. A fault that both pass
   is a finding (MAJOR if it gives a wrong value or accepts a forbidden text), with the smallest text.

Report: `lanes/u-review1/report.md`, written once, at the end; notes in `lanes/u-review1/progress.md` as you
go. For each finding: severity (BLOCKER as above; MAJOR; MINOR), the exact text or input, what the code
returns, what is true and the sentence of the conventions that says so, the command that reproduces it. Then
what you attacked without result, with counts and what would have made a case fail; the fault table. No
praise, no summary. Delete your build trees and executables at the end.
