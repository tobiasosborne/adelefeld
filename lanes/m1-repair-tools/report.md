# Lane m1-repair-tools: report

Written by the second model of the lane (Claude Sonnet 5.5, effort medium) as its final message; saved to
this file by the orchestrator, who added the last section. The first model (pi space-bunny-alpha) was stopped
because its provider failed; its notes are in `progress.md`, which is stale.

## What was done, and by whom

**First model (space-bunny-alpha), in `tools/mutate/mutate.py` and `tools/mutate/selftest.py`:**
- surface R2: `--keep` judges as the normal run.
- surface R5: a compile error is recognised by the compiler's own format.
- surface R6: SIGTERM and SIGINT kill the mutants' process groups and remove the scratch directory.
- `--san` (adf-pf5).
- Item 6 (adf-4lj): survivors are printed as they are found.
- Item 7 (adf-4lj): `--make`, and a mutated file outside `src/`.
- The text-key format of `equivalent.txt` in `mutate.py`.

Its migration scripts (`migrate_equiv.py`, `pair_match.py`) were not used, because they matched by heuristic.
Every entry was resolved by hand against the sources as they stand in the worktree.

**Second model (Sonnet):**
1. `python3 tools/mutate/selftest.py` passed before any change (61 s, `selftest-start.log`).
2. **Bug repaired in `mutate.py`.** An entry written `#1` never matched the first of several identical
   mutants, because `entry_key` dropped the number 1. Red: `selftest-red-occ.log` (2 FAIL). Green:
   `selftest-green-occ.log`.
3. **New `tools/mutate/check_equivalent.py`.** It checks that every entry matches exactly one mutant. It
   reports stale, doubled and malformed lines and exits 1 on any of them. Selftest `test_check_equivalent`:
   red `selftest-red-check-equiv.log` (5 FAIL, tool missing), green `selftest-green-check-equiv.log`.
4. **New `mutate.py --keys`.** It prints the line of `equivalent.txt` of every mutant, so a key is copied and
   not typed. Red `selftest-red-keys.log`, green `selftest-green-keys.log`. A docstring in `mutate.py` that
   said the tool had no sanitizer mode was corrected.
5. `tools/mutate/equivalent.txt` rewritten from a table, `lanes/m1-repair-tools/build_equiv.py`. It has 108
   entries and each is resolved to exactly one mutant.
6. Memory checker (below).
7. `tests/README.md`:
   - The section on mutation now covers the key format, `--list` and `--keys`, `--san`, `--keep`, `--make`,
     and mutating `tools/adf/adf.c` with `sh tests/test_driver.sh` as the judge (with `--copy`). It also
     covers `check_equivalent.py`.
   - A new section on the memory check.

## equivalent.txt (item 1)

Old file: 76 entries (adele 14, cap 3, common 1, fball 18, fball_local 5, rat 2, recon 5, scaled 28). New
file: 108 entries.
- Carried to a text key: 72. **The reason of all 72 was rewritten.** That means 0 kept verbatim, 72 changed.
- Dropped: 4.
- Added: 36.
- 72 + 36 = 108.

**Dropped.** The mutant no longer exists:
- `src/fball.c:610`: `fmpq_mul(c, a, qq)` in the old `adf_fball_div_rat`.
- `src/fball.c:612`: `fmpq_mul(r, r, N)`. The old file named the same statement twice. One mutant is left,
  and it is kept once.
- `src/adele.c:285`: `arb_mul(z->inf, x->inf, t, prec)`.
- `src/adele.c:514`: `acb_mul(z->inf, x->inf, t, prec)`.

The stale `fball.c:142` now keyed a different statement. That entry was carried to the mutant its reason
describes, `fmpz_zero(x->H)` in `adf_fball_init`.

**Reasons of the six `arb_add`/`acb_add`/`arb_mul`/`acb_mul` swaps in `adele.c` (arith R4, R5).**
- The old "same value is written" is gone.
- New reason: both orders give an enclosure, which is all the library promises (arb.rst:6-12).
- The radius of a product depends on the order. `docs/reviews/m1/arith/checks/swap_equiv.out`: arb_mul
  differs in 38216 and acb_mul in 55563 of 200000 pairs, arb_add and acb_add in 0.
- The four `adf_fball_add`/`adf_fball_mul` swaps in `adele.c` and the two in `cap.c`: the set sum and product
  are symmetric in every branch (precision.md Propositions 1 and 2).
- The other swaps cite FLINT documentation by line, for symmetry and for aliasing.

**Added (36).**
- `adele_prec` `<` to `<=`.
- 13 in `dump.c`: `dp_fmpz` `-` to `+`, gcd swaps at lines 436 and 478, `q`/`r` initial values at line 495
  (#1 and #2), `arf_add` swap, `fmpz_mul(D, D, M)` swap, `len < 3` to `<= 3`, the two-spaces `+` to `-`,
  `q = 0` in `dp_bind_matches`, `dp_sb_room` `>` to `>=` (if and while), `dp_sb_finish` `1` to `0`.
- In `fball.c`: the lcm swap, the `h_is_one` gcd swap, `fmpz_mul(de, ...)`, `fb_local_res` `==` to `!=` (#1),
  the second `fmpq_sub` of `fb_contains_g`.
- In `modctx.c`: the `n_gcd` swap, `k < 0` to `k < 1` (line 339 only, `#3`), `fmpz_multi_CRT_precomp` sign
  `0` to `1`.
- In `recon.c`: the two `fmpz_one` drops and `fmpq_cmp > 0` to `> 1`.
- In `scaled.c`: 7 that the old file lacked.
- In `text.c`: 5 swaps.

The entries of `lanes/m1-local`, `m1-scaled` and `m1-dump` are included where their mutant still exists.

**Verification.**
- Every one of the 108 mutants was built and judged by `lanes/m1-repair-tools/judge.py`, five per call
  (`judge_all.sh`, log `judge-all.log`). It uses the tool's own `check_mutant` and `make -s -j2 check`.
- Result: 108 survived, 0 killed, 0 not compiled, 0 timed out, about 22 s each.
- The mutant was judged with the tests and the reason by reading the code. Each reason is the lane's reading
  of the source as it stands.
- Exit of item 1: `python3 tools/mutate/check_equivalent.py` printed `check_equivalent: passed: 108 entries,
  each matches exactly one mutant of 14 file(s)` (`check-equivalent.log`).
- `python3 tools/mutate/mutate.py --root . --files src/<f>.c --list --limit 0` runs for all 14 files
  (`list-14.log`, 14 lines).

**Deliberately not added.** A survivor that could not be shown equivalent stays a survivor for the sweep.
- `recon.c` `fmpz_cmp_si(m, max_exp) <= 0` to `<= 1`. Its reason rests on a measurement of
  `arb_get_interval_fmpz_2exp`; source pending, not on disk.
- `modctx.c` drop of `fmpz_fdiv_r(out, out, ctx->K)`. fmpz.rst:1362-1365 does not say what the `sign`
  argument of `fmpz_multi_CRT_precomp` does, so the line is not redundant by the ground truth.
- `dump.c` `b > 0x7e` to `>= 0x7e`. It is not equivalent: `adf2 ~` is UNSUPPORTED in the original and PARSE
  in the mutant.
- `dump.c` `*pos < len` to `<=`. It needs an audit of every caller.
- `fball.c` `h_is_one` (`1` to `0` and `== 1` to `== 0`). These rest on Proposition 19 and Lemma 17.2 and
  were not verified again.
- **The ten entries proposed by lane m1-modctx-b.** They are not on disk: they are uncommitted in that
  worktree, which the lane may not read. The two out-of-bounds ones (`:698`, `:710` logic) are not in the
  file, as required. The other eight could not be identified and were not guessed.
- `text.c`: no list of survivors exists, because the mutation run of m1-repair-text did not finish. Only the
  5 swaps are in.

## Memory checker (item 4; surface R1, R4)

- `check_uninit.py`: a use before the first init in text order is now reported, at the line of the use.
- A `memset(x, ...)` is neither use nor init. Without this the tree gave 11 false findings, in
  `tests/test_dump*.c` (red `memcheck-red2.log`).
- Red `memcheck-red.log` (3 FAIL: s1, s1 exit status, s4). Green `memcheck-green2.log`.
- Final: `python3 tools/memcheck/selftest.py` passed (`memcheck-final.log`).
  - valgrind exits 77 on snippets s0, s1, s2, s3 and s5.
  - `run.sh --valgrind-path` finds `~/.local/bin/valgrind` with PATH reduced to `/usr/bin:/bin`.
  - The tree `src/*.c tests/*.c tools/adf/adf.c` (55 files) has 0 findings.
- **Not handled.** An init on one branch, an array element, and a `goto` over the init are still not seen by
  the checker. They are named as limits in `tools/memcheck/README.md`. Snippets s2, s3 and s5 (copied to
  `tools/memcheck/snippets/`) pin that the checker gives 0 findings on them, while valgrind shows they are
  real defects.
- `run.sh`: looks for valgrind on PATH, then `~/.local/bin/valgrind`. New option `--valgrind-path`.
- The statement of the README that valgrind is not installed is replaced.

## Checks run at the end

- `python3 tools/mutate/selftest.py`: `selftest: passed: the weak test leaves a survivor, the strong test
  leaves none, and no process of a mutant is left` (56 s, `selftest-final.log`).
- `python3 tools/memcheck/selftest.py`: `selftest: passed`.
- `python3 tools/mutate/mutate.py --root . --files tools/adf/adf.c --copy Makefile include src tests tools
  --make 'sh tests/test_driver.sh' --limit 1 --jobs 1 --timeout 300`: ran. 1 mutant, 5.4 s, survived (a
  dropped `adf_str_free`, a leak). This only checks that the command of the README works (`adf-mutant.log`).

## Not done

- The sweep (item 5) was not run. It is the orchestrator's.
- No `--san` mutation was run over a source file. The statements on `--san` and `--keep` in
  `tests/README.md` rest on the selftests of the first model.
- No survivor beyond the 108 was judged.

## Findings against the specification

None. One note for the arith review: `adf_adele_mul` and `adf_cadele_mul` do not store the same real ball
for swapped operands (arith R4). The specification promises an enclosure only, so this is consistent with it.

## Checks of the orchestrator (2026-09-28 23:28 and 2026-09-29 01:55)

- `python3 tools/mutate/selftest.py` after the work of the first model: passed (62 s).
- After the work of the second model, in the worktree: `python3 tools/memcheck/selftest.py` passed;
  `python3 tools/mutate/check_equivalent.py`: 108 entries, each matches exactly one mutant.
- The lane changed files it owns and nothing else (`git status`).
- NOT checked by the orchestrator: the truth of the 108 reasons. 36 entries are new and all 72 old reasons
  were rewritten by one model that judged its own reading; they go to the closure check of `surface`
  (codex), which is asked to attack them. The worktree stands at the master of 2026-09-28 23:00: the check
  of the entries is repeated on master after the merge, where `src/` carries the lines of lane m1-invariants.
