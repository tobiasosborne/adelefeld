# Lane c-sources1: FLINT 3.0.1 Dirichlet documentation and sources on disk

The character slices of milestone 3 (`docs/api-3c.md`, `src/char.c`) left three statements marked
`[source pending: ...]` because FLINT's `dirichlet` module is not under `refs/`: the exponent convention of
`dirichlet_chi` (`docs/api-3c.md:161`, `src/char.c:125`), the identification of the Conrey pairing
(`docs/api-3c.md:284`, `:390`, `src/char.c:3`, `:323`, `docs/api-5a.md:127`) and the failure semantics of
`dirichlet_group_init` for a word-sized modulus (`docs/api-3c.md:390`). This lane puts the sources on disk and
registers them. No code, no build, no edit of `docs/api-*.md` or `src/`: the pending marks are closed by a
later lane from what you register.

Read first: `refs/README.md`, `refs/fetch_sources.sh` (the `get` helper and the two FLINT loops at lines
about 90 and 159), `refs/manifest.sha256`, `docs/sources.md` (the head, the FLINT rows of table 1 at lines
47 to 54 and 100 to 103, and the form of the quote rows of table 3), `lanes/s2-sources/brief.md` and
`lanes/s2-sources/report.md` as the pattern, `docs/api-3c.md` sections P3 and 161, `src/char.c:1-20`,
`:115-135`, `:315-330`.

**You own:** `refs/fetch_sources.sh` and `refs/manifest.sha256` (additions only), `docs/sources.md` (new
rows only, appended to the tables in the existing form), `lanes/c-sources1/`. `refs/src/` is not in git; you
add files under `refs/src/flint-3.0.1/` and `refs/src/flint-src-3.0.1/dirichlet/` through the fetch script
only. Everything else is read-only. No git command that changes state, no `bd`.

1. Add to the fetch script, at the SAME tag v3.0.1 and in the same way as the existing loops:
   `doc/source/dirichlet.rst` into `src/flint-3.0.1/dirichlet.rst`; and the C sources under `src/dirichlet/`
   that define `dirichlet_chi`, `dirichlet_pairing`, `dirichlet_pairing_char`, `dirichlet_char_exp`,
   `_dirichlet_char_exp`, `dirichlet_group_init`, `dirichlet_char_log`, `dirichlet_char_index`,
   `dirichlet_index_char`, `dirichlet_char_conrey`... List the directory first (the GitHub tree of
   `src/dirichlet` at v3.0.1, through the API or the raw listing) and fetch every `.c` file whose name
   matches those functions plus `dirichlet.h` from `src/`. Run the script so that it fetches only what is
   missing, then append the new checksums to `refs/manifest.sha256` in the form that is there (the script
   may rewrite the manifest; check with `git diff --stat refs/manifest.sha256` that no existing line changed,
   and `./refs/fetch_sources.sh --check` passes). If the network does not answer, try three times a minute
   apart, then report it; do not take the files from another place on this machine and write nothing from
   memory.
2. From the files on disk, with file and line and the quoted words, add rows to `docs/sources.md` table 1
   (one row per fetched file group, the form of lines 52 and 100) and to table 3 (quote rows, the form of
   the `S.2 (lane s2-slice4)` rows) that settle, as far as the source does:
   a. what `dirichlet_chi(G, chi, n)` returns: the exponent `k` with `chi(n) = exp(2 pi i k / G->expo)`, or
      some other normalisation; quote the doc line and the code line;
   b. the Conrey pairing: what `dirichlet_pairing(G, a, b)` and `dirichlet_pairing_char` compute, the
      symmetry, and the relation to `chi_a(b)`; quote the doc and the code;
   c. `dirichlet_char_exp` against `_dirichlet_char_exp`: which caches, which recomputes;
   d. `dirichlet_group_init(G, q)`: its return value and when it is 0 (the doc says what about `q`; the code
      says what about the factorisation and the word size);
   e. whether the conjugate character has the label with negated exponents (the code of `dirichlet_char_conj`
      or its equivalent, if it exists; otherwise say that no such function exists in the module).
   Each row quotes the words of the file at the cited line. Do not infer beyond the text: where the source
   is silent, the row says so.
3. Write `lanes/c-sources1/report.md` at the end (not before): the files fetched with their sizes and hashes,
   the commands run, the rows added (count), which of the five questions the sources settle and which they
   leave open, and anything in the sources that contradicts `docs/api-3c.md` P3 or `src/char.c` comments
   (quote both sides; do not edit them). Lines at most 116 characters, files end with a newline, plain sober
   prose, no LaTeX. Aim to finish within about forty minutes.
