# Lane f-review10: bug hunt through `Log` on ideles (lane f-slice11, `src/gfunc_log.c`)

Your task is to REFUTE, not to confirm. Lane f-slice11 (codex `gpt-6.1-sol`; you are of another model family on
purpose) landed today, not reviewed: `src/gfunc_log.c` (373 lines) and the last block of
`include/adelefeld/gfunc.h`: `adf_idele_Log`, `adf_idele_log_abs`, `adf_idele_Log_at`, `adf_idele_log_abs_at`,
`adf_idele_Log_refine`, `adf_idele_log_abs_refine`; the driver commands `Log`, `log_abs`, `Log_at`,
`log_abs_at`, `Log_refine`, `log_abs_refine` (`tools/adf/adf.c`, `tools/adf/README.md`); statements G7 to G12
(`docs/api-1f8.md`); decision N-D17 (`docs/SPEC.md` 15.4). The design it implements, written by the same model
family and not reviewed either: `docs/design/idele-log.md` (IL1 to IL8) with the oracle `proto/idlog_checks.py`.
The lane's report is `lanes/f-slice11/report.md`.

The mathematics, in one paragraph (check it, do not trust it): an idele is `(I, r, c U(M))`: a real ball `I`,
a content `r > 0` rational, a unit coset. At a prime `p` with `k = v_p(M) >= 1` its component is the ball
`r c + p^(v_p(r) + k) Z_p`; at a prime not dividing `M` it is the shell `r Z_p^x`. The Iwasawa `Log` kills `p`
and the roots of unity: `Log(p^m w u) = log u`. So the image of the component is `Log(r' c) + p^k Z_p` at a
restricted odd prime (`r'` = `r` without its power of `p`), at 2 for `k >= 2`; `Log(r') + p Z_p` at an
unrestricted odd prime and `Log(r') + 4 Z_2` at 2 (and for `k = 0, 1` at 2); the all-places finite image lies in
`4 Zhat`. Is each of these the EXACT image (the smallest ball), and does the code return it at
`K = min(N, E)`?

**You own:** `lanes/f-review10/` only. Everything else is read-only. No git command that changes state, no
`bd`. At most 2 cores. Build the archive once: `timeout 600 make -j2 BUILD=lanes/f-review10/build
lanes/f-review10/build/libadelefeld.a`, link your programs against it (`-Iinclude -lflint -lgmp -lm`). Do not
run the repository's test suites. Every program under `timeout`, none over 120 s.

Hunt, in this order, and stop a line of attack when it has run a few thousand cases without a finding:
1. **Your own oracle, written from the definition, not from `proto/idlog_checks.py`** (do not import it): for
   `p` in 2, 3, 5, 7 enumerate the units `u` modulo `p^H` (H = 5 or 6) in the coset, compute `Log(r u)` modulo
   `p^H` in exact integers (for example `Log(x) = log(x^(p-1)) / (p - 1)` for a unit `x`, at 2
   `Log(x) = log(x^2) / 2`, with the series on `1 + p Z_p` truncated with a bound you state), and compare the
   SET with the ball `adf_idele_Log_at` returns: a point of the image outside the ball is a BLOCKER; a ball
   strictly larger than the image where `N >= E` is a MAJOR (the header promises the exact image there).
   Inputs: contents with positive and negative valuation at `p` and other prime factors; moduli with
   `v_p(M)` from 0 to 4 and other factors; exact units `[1]`, `[-1]`; the non-normal stored unit `[1 mod 2]`;
   `N` below, at and above `E`; negative `N`.
2. **The refinement by CRT** (`Log_refine`): the returned finite ball `a + R Zhat` must contain the image at
   EVERY prime (project it to each listed prime and to two unlisted primes and compare with step 1; an unlisted
   prime must still see `4 Zhat`'s factor); lists with repeated primes, unsorted lists, the empty list, a prime
   given as a composite, 65536 and 65537 entries, the CRT bound.
3. **Statuses and outputs**: negative and zero-crossing real balls for `Log` against `log_abs`; `prec` at and
   above the cap; `where` (written on `OK`? right place on each failure? the order of precedence the header
   states, case by case); outputs untouched on every status other than `OK`; `log_abs_at` at a prime.
4. **Memory**: build two of your reproducers with `-fsanitize=address,undefined` and run them with
   `ASAN_OPTIONS=detect_leaks=1` (the author's sandbox could not run the leak check at all).
5. **The driver**: twenty hostile command lines (wrong operand types, missing and extra operands, a prime that
   is not prime, huge primes, `Log_refine` with no primes); the printed values against the library's.

Report: `lanes/f-review10/result.md`, written once, at the end (the harness refuses the name `report.md` for a
Claude subagent); notes in `lanes/f-review10/progress.md` as you go. For each finding: severity (BLOCKER: a
point of the image outside the result, a memory fault, undefined behaviour; MAJOR; MINOR), the input, what the
code returns, what is true and why, the command that reproduces it. Then what you attacked without result, with
counts and what would have made a case fail. No praise, no summary. Give the same text as your final message.
