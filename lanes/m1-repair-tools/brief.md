# Lane m1-repair-tools: mutation tool and memory checker (review surface R1 to R6; arith R4, R5;
# contexts R5; issues adf-tz1, adf-pf5)

Read `lanes/COMMON.md`. Then `docs/reviews/m1/surface/review.md` findings R1 to R6 with their reproducers
under `docs/reviews/m1/surface/checks/` (`memcheck/`, `mini/`, `equiv_keys.py`, `stale_excuse.py`,
`mutate_sigterm.py`); `docs/reviews/m1/arith/review.md` R4, R5; `docs/reviews/m1/contexts/review.md` R5;
`lanes/tools-mutate/report.md`; `tools/mutate/`, `tools/memcheck/`, `tests/README.md`, the `mutate` targets
of the `Makefile` (read-only). The files `lanes/m1-local/equivalent-added.txt`,
`lanes/m1-scaled/equivalent-added.txt`, `lanes/m1-dump/equivalent-added.txt` and the ten entries that lane
m1-modctx-b proposed (`git -C ../m1-modctx-b diff` is not available to you; they are quoted in
`lanes/m1-modctx-b/report.md` finding 6 and section "Checks run", and two of them are not equivalent).

**You own:** `tools/mutate/`, `tools/memcheck/`, `tests/README.md` (the parts on mutation testing and the
memory check), `lanes/m1-repair-tools/`. Python standard library only. Red-green for every change, in
`tools/mutate/selftest.py` and in a new `tools/memcheck/selftest.py`.

1. **Keys that survive edits (surface R3).** A key of `equivalent.txt` is
   `<file> | <kind> | <the text of the line the mutant changes, stripped> | <old> -> <new> | <reason>`,
   with an optional occurrence number when the same line text and mutation occur more than once in the
   file. No line number. Convert the file: for every present entry find, in the source as it stands, the
   mutant that its reason describes (the reason quotes the call); an entry whose mutant no longer exists
   is dropped and listed in the report; an entry that now matches another statement (the review names
   `src/fball.c:142:drop_call`) is dropped. Add the entries of the three `equivalent-added.txt` files in
   the same way. Entries whose stated reason is "the same value is written" for a swap of the operands of
   `arb_mul` or `acb_mul` get the true reason: both orders give an enclosure, and the library promises an
   enclosure, not a particular ball (arith R4).
2. **`--keep` (surface R2)** judges as the normal run does. **Test names (surface R5):** a compile error
   is recognised by the compiler's own format at the start of a diagnostic line (`<file>:<line>:<col>:
   error:` or `fatal error:`, and the linker's `undefined reference`), not by `error:` anywhere.
   **SIGTERM and SIGINT (surface R6):** the tool kills the process groups of its running mutants and
   removes its scratch directory.
3. **A sanitizer mode (issue adf-pf5):** `--san` builds and runs the mutants with `SAN=1`, so that a mutant
   that only reads or writes out of bounds, or leaks, is killed. The `Makefile` target cannot be changed by
   you; document the direct call in `tests/README.md`.
4. **Memory checker (surface R1, R4):** a use before the first init in text order is reported; an init on
   one branch only, an array element, and a `goto` over the init are reported or named as limits in the
   README with a test that pins what the tool does. The README's statement on valgrind is out of date
   (valgrind is installed in `~/.local/bin`).
5. Then run, one after the other, never two at once, for every file of `src/`:
   `python3 tools/mutate/mutate.py --root . --files src/<f>.c --jobs 2 --seed 20260928 --limit 300 --san`
   (if one run exceeds 40 minutes stop it and lower the limit for that file; say so). Report the table of
   counts per file and every survivor with the missing test. Do not change `src/` or `tests/`.
