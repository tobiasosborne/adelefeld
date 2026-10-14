# Lane c-pending1: report

Marks closed: 3 of 4 (marks 1, 2, 3). Mark 4 stands. No executable line changed.

## Files written
`src/char.c` (comments only), `docs/api-3c.md`, `docs/api-3d.md`, `docs/api-5a.md`, this report.

## Marks, before and after
1. dirichlet_chi convention. Before: api-3c.md:161 "The full exponent convention remains [source pending: FLINT
   3.0.1 dirichlet_chi source under refs/]."; char.c:125 "[source pending: FLINT 3.0.1 dirichlet_chi
   implementation under refs/]". After: both cite flint-3.0.1:dirichlet.rst:257, 261 (quoted), flint-src-3.0.1:
   dirichlet/chi.c:28, pairing_char.c:20, 23: chi(n) = exp(2 pi i k / G->expo); null is DIRICHLET_CHI_NULL for
   gcd(q, n) > 1 (chi.c:17, 19).
2. Pairing identification. Before: api-3c.md:284 "Step 4's identification with FLINT is [source pending: dirichlet
   pairing implementation under refs/]", :390 "P3's universal FLINT pairing identification ... remain source
   pending", char.c:3 and :323 "[source pending: FLINT pairing identification]", api-3d.md:102 and :287,
   api-5a.md:127. After: api-3c P3 step 5 is rewritten as steps 5 to 7:
   - step 5: pairing_char.c:23 gives sum_k PHI[k] (a_k b_k mod phi_k) mod expo; group_init.c:88 gives
     PHI[k] = expo / phi_k; so k / expo = sum a_k b_k / phi_k mod 1, which is B(n, a) of step 4 with d_j = phi_j;
     dirichlet.rst:29 states the same; logs and generators from group_init.c:46-61, char_log.c:25-47, :50,
     dirichlet.rst:32-33; chi = pairing(log chi, log n) from chi.c:26, 28 and pairing.c:25-28.
     Not in the source: no sentence calls label m "chi_m", none states symmetry. The gap does not matter: the
     rule is about the logs, which the source defines.
   - step 6: phi_k | expo, proved from group_init.c:72, 76, 81 (even phis divide the first expo; at an odd step
     expo_new = (expo / g) phi_k with g = gcd(expo, p-1) dividing phi_k).
   - step 7: conjugate label by char_pow.c:19-20 with n = expo - 1: logs negated, number inverted. Unconditional.
   Closure sentences in api-3c.md:390 area, api-3d.md (two places), api-5a.md:127, char.c:3 and :323 (HEADER-FINDING
   sentence about cost and void ABI kept).
3. Group-init failure. Before: api-3d.md:24 "The group-init failure semantics are source pending." After:
   dirichlet.rst:71-74 and group_init.c:115-127, 153 quoted; with ADF_CHAR_MOD_MAX = 65536
   (include/adelefeld/char.h:15) a return of 0 is unreachable for an accepted modulus. The comment at char.c:51
   (fail-stop kept) says so and also says that this predicate applies no cap. The "must not be cleared" fact
   (group_clear.c:17-19) is recorded in api-3d.md.
4. api-3c.md:304 (signed real quadratic Gauss evaluation, analysis:580-584): the FLINT sources do not settle it.
   The mark and the text "leaves that source pending" stand (moved to line 344 by the added lines).

## Nuance found (not a spec contradiction)
The "unreachable" sentence holds for the capped constructors and evaluators. `adf_char_is_canonical` (char.c:51) and
`adf_char_conj` (char.c:331) apply no cap themselves and read q from the object. An object with a prime factor
above 10^16 could only come from a path that does not apply D1 (none found in char.c). The fail-stop there stays
and the text says so. N-D25 item 10 words it the same way ("if a group of such a modulus ever reaches it").

## Call sites of dirichlet_group_init in src/char.c (clear after failed init?)
- 51 (is_canonical): fail -> flint_abort, no clear. No cap before it. OK.
- 73 (set_conrey, G): fail -> return ADF_UNSUPPORTED, no clear; cap at :69 before. OK.
- 78 (set_conrey, H): fail -> clears c and G (the successful group), not H. OK.
- 117 (order): fail -> return, no clear; cap at :114. OK.
- 139 (chi_phase): fail -> return, no clear; cap at :136. OK.
- 206 (a Gauss-sum/evaluator function, cap :203): fail -> return, no clear. OK.
- 270 (ucoset eval, cap :261): fail -> return, no clear. OK.
- 331 (conj): fail -> flint_abort, no clear. OK.
No site clears a failed group. No finding.

## Checks
- grep "source pending" before: api-5a.md:127; api-3c.md:161, 284, 304, 390; api-3d.md:24, 102, 287;
  char.c:3, 125, 323 (11 lines).
- grep after: api-3c.md:344 only (the mark 4 sentence).
- `git diff src/char.c`: 4 hunks, only comment lines changed (+ and - lines all inside /* */).
- Line length: awk 'length>116' on the four files: one new line of mine was 118 and was wrapped; the pre-existing
  api-3c line 344 (the mark 4 sentence, 118) is not mine and is left.
- `make -j2 BUILD=lanes/c-pending1/build` of test_char, test_char_class, test_char_eval, each under timeout:
  test_char 129725 checks, test_char_class 1208883 checks, test_char_eval 1738298 checks, all passed,
  0 live FLINT blocks. (No `check-char` target exists; run only the three character tests.) Build tree deleted.

## Not done / pending
Mark 4 stands. No new test, no header change. Not verified by test: the divisibility proof (step 6) was not run
against group_init on moduli; it is a reading of the lines cited.
