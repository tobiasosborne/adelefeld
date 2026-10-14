# Lane c-pending1: close the source-pending marks of the character slices from the FLINT sources now on disk

Lane c-sources1 (merged, `lanes/c-sources1/report.md`, 15 rows of `docs/sources.md` labelled "S.3 (lane
c-sources1)") put FLINT 3.0.1's `dirichlet.rst`, `dirichlet.h` and the sources of `src/dirichlet/` under
`refs/src/`. Seven marks `[source pending: ...]` in the character slices waited for exactly these files. You
close each mark that the sources settle, by a citation with file and line and the quoted words, and you
complete the one proof that was conditional. Comments and statements only: no change to any executable
line of C, no header change, no new test, no build beyond `make -j2 check` of the one test named below.

Read first: `CLAUDE.md`, `lanes/COMMON.md`, `lanes/c-sources1/report.md` (all of it), the 15 new rows of
`docs/sources.md` (search "lane c-sources1"), `refs/src/flint-3.0.1/dirichlet.rst`,
`refs/src/flint-src-3.0.1/dirichlet.h`, `refs/src/flint-src-3.0.1/dirichlet/{chi,pairing,pairing_char,
char_exp,char_log,char_pow,group_init,group_clear}.c`, `docs/api-3c.md` sections 1 to 3 and P3 (lines about
150 to 165 and 270 to 310) and the "Findings" near line 390, `docs/api-3d.md` lines 20 to 30,
`docs/api-5a.md` lines 120 to 129, `src/char.c` (all; the marks are at lines about 3, 125, 323).

**You own:** `src/char.c` (COMMENTS only: no change to any line that is not a comment; `git diff` must show
no non-comment change), `docs/api-3c.md`, `docs/api-3d.md`, `docs/api-5a.md`, `lanes/c-pending1/`.
Everything else is read-only. No git command that changes state, no `bd`.

The marks and what to do with each:

1. `docs/api-3c.md:161` and `src/char.c:125` ("dirichlet_chi implementation"): settled. `dirichlet.rst:257,
   261` and `chi.c:28`, `pairing_char.c:20, 23`: the returned `k` satisfies `chi(n) = exp(2 pi i k /
   G->expo)`; null is `DIRICHLET_CHI_NULL` for `gcd(q, n) > 1` (`chi.c:17, 19`). Replace each mark by the
   citation (the form `flint-src-3.0.1:dirichlet/chi.c:28` as `docs/sources.md` prescribes).
2. `docs/api-3c.md:284` (P3 step 5, "Step 4's identification with FLINT"), `:390`, `src/char.c:3`, `:323`,
   `docs/api-5a.md:127` ("universal FLINT Conrey pairing identification"): the source gives the pairing as
   `sum_k PHI[k] * (a_k b_k mod phi_k) mod expo` with `PHI[k] = expo / phi_k` (`pairing_char.c:23`,
   `group_init.c:88`; `dirichlet.rst:29` gives `exp(2 i pi sum a_i b_i / phi_i)`), and `chi(n) =
   pairing(log chi, log n)` (`chi.c:26, 28`; `pairing.c:25-28`). Write the identification as a proof step in
   P3 (after step 4): the bilinear form `B(n, a) = sum_j log_j(n) log_j(a) / d_j mod 1` of step 4 with `d_j =
   phi_j` IS FLINT's pairing divided by `expo`, because `PHI[k] / expo = 1 / phi_k`; the logs are the Conrey
   logs on FLINT's generators (`group_init.c:46-61`, `char_log.c:25`), and a `chi` of label `m` has the logs of
   `m` (`char_log.c:50`, `dirichlet.rst` the lines that define the character of a number: find them). State
   what the source does NOT say (c-sources1 report, question b: no sentence calls the label `m` "chi_m"; the
   pairing of the logs is all it says) and whether that gap matters for the inverse-label rule: it does not,
   because the rule is about the logs, which the source defines. Then the conjugate label by exponent
   negation (`char_pow` with `expo - 1`, `char_pow.c`; c-sources1 question e; each `phi_k` divides `expo`,
   `group_init.c:76, 81`: quote the lines and prove the divisibility from them in one or two sentences) is
   proved unconditionally; say so at `:390` and in the `src/char.c` comments at `:3` and `:323` (keep the
   HEADER-FINDING sentence at `:323` about the cost and the void ABI; only the source-pending clause changes).
   `docs/api-5a.md:127` gets the same closure in one sentence.
3. `docs/api-3d.md:24-25` ("group-init failure semantics are source pending"): settled. `dirichlet.rst:71-74`:
   returns 1 on success, 0 if a factor is too large (the largest prime factor above `10^16`);
   `group_init.c:115-127, 153`: only the odd part is factored and the 0 comes before any allocation. Under the
   modulus cap `ADF_CHAR_MOD_MAX = 65536` (N-D22, `include/adelefeld/char.h`: find the line) every prime
   factor is below `2^16 < 10^16`, so a return of 0 is UNREACHABLE for an accepted modulus: write that
   sentence with both citations; the fail-stop `flint_abort` at `src/char.c:51` stays and its comment gets the
   same sentence. Also record, from c-sources1 question d and `group_clear.c:17-19`, that a group whose init
   returned 0 must not be cleared, and check every `dirichlet_group_init` call site in `src/char.c` (lines
   about 51, 73, 78, 117, 139, 206) for a clear after a failed init: write the result of that check (one line
   per site) in your report; if a site clears a failed group, that is a FINDING (report it with the line; do
   not change the executable line).
4. `docs/api-3c.md:304` (the signed real quadratic Gauss evaluation, analysis:580-584): NOT settled by these
   sources; leave the mark and say in the report that it stands.

Checks: `grep -n "source pending" src/char.c docs/api-3c.md docs/api-3d.md docs/api-5a.md` before and after
(report both lists); `git diff src/char.c` shows comment lines only; `timeout 600 make -j2
BUILD=lanes/c-pending1/build check-char` or, if no such target exists, `timeout 600 make -j2
BUILD=lanes/c-pending1/build lanes/c-pending1/build/test_char && timeout 300 lanes/c-pending1/build/test_char`
(read `Makefile` for the test target names; run only the character tests) passes; delete the build tree at the
end. Lines at most 116 characters; files end with a newline; plain sober prose, no LaTeX.

Report: `lanes/c-pending1/report.md` at the end: each mark before and after (quoted), the proof step added,
the call-site table of item 3, the checks with their output, what stands pending. Aim for about forty minutes.
