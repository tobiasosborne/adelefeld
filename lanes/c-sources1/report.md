# Lane c-sources1: FLINT 3.0.1 Dirichlet documentation and sources on disk

Date: 2026-10-15. No code, no build. Network fetches only through the `get` helper of `refs/fetch_sources.sh`,
at the tag v3.0.1; the first attempt worked, no retry was needed.

## 1. Files fetched (18; sha256 in `refs/manifest.sha256`, lines 2175 and 2204 to 2220)

Size is in bytes. Hashes of the C files are in the manifest (full form); the two that matter most:

- `refs/src/flint-3.0.1/dirichlet.rst` (296 lines, 11807):
  f4dd71b734fc72f705c4013e3a63e6d38d4c40a39343a19fbde8c9365ae3d381
- `refs/src/flint-src-3.0.1/dirichlet.h` (176 lines, 6394):
  54d40ab90ea8c7e06595a8325d5f7cf8fd101f21793b6938562f83b75e88355f
- under `refs/src/flint-src-3.0.1/dirichlet/` (lines, bytes): chi.c (33, 800), pairing.c (34, 843),
  pairing_char.c (26, 724), char_exp.c (22, 640), char_log.c (51, 1396), char_index.c (40, 1359),
  index_char.c (38, 1313), group_init.c (232, 5456), group_clear.c (21, 571), group_dlog_precompute.c (46, 1214),
  char.c (23, 631), char_mul.c (21, 558), char_pow.c (21, 654), char_one.c (21, 558), char_order.c (24, 620),
  inlines.c (14, 432). The brief's `dirichlet_char_conrey` and a conjugation function do not exist (section 4).

The directory was listed first: `curl https://api.github.com/repos/flintlib/flint/contents/src/dirichlet?ref=v3.0.1`
(35 names, among them `char_*.c`, `chi*.c`, `group_*.c`, `index_char.c`, `pairing*.c`, `ui_*.c`).

## 2. Commands

- Edited `refs/fetch_sources.sh`: one block before "text extractions" (13 lines added): `dirichlet.rst` and the
  list `FLINTSRC_DIR` in the form of the existing loops.
- `./refs/fetch_sources.sh` (fetched the 18 missing files, rewrote the manifests).
- `git diff --stat refs/manifest.sha256`: 18 insertions, no deletion (no existing line changed).
- `./refs/fetch_sources.sh --check`: every line OK.

## 3. Side effect, to be decided by the orchestrator

The script also rewrites `refs/manifest-extra.sha256`; it gained 3 lines (`kerber-sagraloff/tex/qir.txt`,
`sagraloff-mehlhorn/tex/obreshkoff.txt`, `sagraloff-mehlhorn/tex/tree.txt`): text extractions already on the shared
disk from another lane, not from this one. I do not own that file and a restore was refused by the harness, so the
change is left; revert it before the commit if it is not wanted (`git diff refs/manifest-extra.sha256`).

## 4. Rows added to `docs/sources.md`: 15

Table 1: 3 rows (the `.rst`, the header, the `dirichlet/` group). Table 3: 12 rows labelled "S.3 (lane c-sources1)",
after the last `s2-slice4` row, each with file, lines and the quoted words.

## 5. The five questions

a. SETTLED. `dirichlet.rst:257, 261`: "The returned value is the numerator of the actual value exponent mod the
   group exponent *G->expo*." / "Compute the value `\chi(n)` as the exponent modulo *G->expo*." Code
   `chi.c:28`: `v = dirichlet_pairing_char(G, chi, x);` returned unchanged; `pairing_char.c:20, 23` adds modulo
   `G->expo`. So `chi(n) = exp(2 pi i k / G->expo)`, not `k / ord(chi)`; null is `DIRICHLET_CHI_NULL` for
   `gcd(q, n) > 1` (`chi.c:17, 19`).
b. SETTLED for the formula and the symmetry. `dirichlet.rst:29`: `chi_q(m,n) = exp(2 i pi sum a_i b_i / phi_i)`.
   `pairing_char.c:23`: `PHI[k] * nmod_mul(chi->log[k], x->log[k], G->P[k].phi)` with `PHI[k] = expo / phi_k`
   (`group_init.c:88`). The expression is symmetric in its two arguments (a derivation from the code; no sentence of
   the source says so). `chi(n) = pairing(log chi, log n)` (`chi.c:26, 28`; `pairing.c:25, 26, 28`). The generators
   and their order are in `group_init.c:46 to 61`, `char_log.c:25`. Left open: nothing in the source names the
   character labelled `m` as "`chi_m`" in words; the pairing of the logs is all it says.
c. SETTLED. `dirichlet_char_exp` (`dirichlet.h:116 to 120`) is an inline returning the stored `x->n`; it caches and
   recomputes nothing. `_dirichlet_char_exp` (`char_exp.c:18 to 21`) recomputes the product of
   `generators[k]^log[k]` modulo `q` and stores it in `x->n`; `dirichlet_char_index` calls it
   (`char_index.c:39`), `dirichlet_char_log` stores the input (`char_log.c:50`).
d. SETTLED for the return value. `dirichlet.rst:73, 74`: "returns 1 on success and 0 if a factor is too large",
   bound 10^16 on the largest prime factor (`:71, 72`). Code `group_init.c:115 to 127, 153`: the power of 2 is
   removed, the odd part factored, and on a 64-bit build any odd factor above 10^16 gives `return 0` before the
   allocations; else `return 1`. A power of 2 never gives 0. Open: the source is silent on `q = 0` (and `q = 1`);
   `nmod_init(&G->mod, q)` is called at `:112` before any test. Also: the group fields set before the early return
   (`q`, `mod`, `q_even`, `neven`) are left in `G` and nothing is allocated, and
   `group_clear.c:17, 18, 19` is `flint_free(G->P); flint_free(G->generators); flint_free(G->PHI);` without a
   test, so a clear after a return of 0 frees pointers that were never set. Do not clear a group whose init gave 0.
e. SETTLED as absent. No function with `conj` in the module (the directory listing, `dirichlet.h`, `dirichlet.rst`).
   `dirichlet_char_pow` (`char_pow.c:19, 20`) multiplies the logs by `n` modulo `phi_k` and the number to the power
   `n`; with `n = expo - 1` the logs are negated and the number is the inverse of `a->n` modulo `q`, since each
   `phi_k` divides `expo` (inference from `group_init.c:76, 81`). The source never calls this the conjugate.

## 6. Contradictions with `docs/api-3c.md` or `src/char.c`

None found. The sources agree with: `api-3c.md:157` ("its non-null k means E(k/G->expo), NOT E(k/ord(chi))"),
`api-3c.md:280 to 283` (inversion negates the cyclic exponents, B(n,a) = sum log_j(n) log_j(a) / d_j) and the
comment `src/char.c:123 to 124` ("k/G->expo"). One refinement for the lane that closes the marks: the pairing of
`api-3c.md:280` is exactly `dirichlet_pairing_char`, with `d_j = phi_j` the orders of the Conrey components, and
in FLINT 3.0.1 the component for `4 \| q` has order 2 with generator `-1` and the one for `8 \| q` has order
`2^(e-2)` with generator 5 (`group_init.c:46 to 52`), which is the usual Conrey choice.

## 7. Not done

No edit of `docs/api-*.md` or `src/` (the pending marks at `docs/api-3c.md:161, 284, 390`, `docs/api-5a.md:127`,
`src/char.c:3, 125, 323` are closed by a later lane). Whether the pointers of a failed `G` are zero is not
stated by the source (the struct is not zeroed in `group_init.c`), so the free is on unset memory (section 5 d).
