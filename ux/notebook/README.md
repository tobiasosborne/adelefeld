# Adelica, the notebook slice

The first slice of `docs/ux/brief-1-adelica-notebook.md`: a notebook in which every value is drawn as a place strip.
A notebook is a Julia file of cells. `Adelica.run` evaluates the cells in order and writes one static HTML page and
a records file `NAME.records.jsonl` (one JSON object per output: the agent's window). `replay` runs the driver
line of every record and compares. The page is static (no Pluto, Julia offline): a click on a branch chip or on
the fog writes the new call into the cell's text and marks the cell "re-evaluate to keep" (`restore` undoes it).
The cell is kept only when you put that text into the notebook file and run it again.

## Build and run (from the repository root)

    sh ux/notebook/run.sh            builds build/adf and build/libadelefeld.so if missing, then writes
                                     ux/notebook/out/a_roots.html and b_which_rational.html with their records
    firefox ux/notebook/out/a_roots.html
    sh ux/notebook/replay.sh ux/notebook/out/a_roots.records.jsonl    one line per record, then
                                     "replay: N same, M differs, K unchecked"; exit status 1 if M > 0
    sh ux/notebook/test.sh           the Julia tests (ux/notebook/test/runtests.jl)
    node ux/notebook/test/page.test.js                                the page's click script

Julia is started by `ux/notebook/julia.sh` (the package `julia/Adelefeld` is the project, the libgmp preload of
`julia/Adelefeld/bin/adelefeld-julia`, offline, one thread, the depot under `$ADELICA_LANE`, by default
`lanes/ux-notebook`). A notebook of your own, for example `my.jl` (each `# %%` line begins a cell):

    # %% A finite ball 146 + 1000 Zhat: FBall(centre, modulus), both of them Rat.
    c = FBall(Rat(146), Rat(1000))
    # %% The same ball from its text.
    adf"146 mod 1000"
    # %% 6/35 as a strip: its primes 2, 3, 5, 7.
    Rat(6, 35)
    # %% The square roots of -1 at 5.
    roots(-1, 2; at = 5)

    sh ux/notebook/julia.sh -e 'push!(LOAD_PATH, "ux/notebook/src"); using Adelica;
        Adelica.run_file("my.jl", "/tmp/my")'        writes /tmp/my.html and /tmp/my.records.jsonl

## The calls of a cell (one Julia expression, optionally `name = expression`)

| Call | Computed by | Shows |
|---|---|---|
| `Rat(6, 35)`, `FBall(Rat(c), Rat(N))`, `adf"3 mod 12"`, `readvalue("...")`, `+ - * /` | the package | a strip |
| `reconstruct(x)` (an Adele), `reconstruct(f, lo, hi)` | the package | a strip, or a card |
| `roots(x, n; at = p, prec = 8)`, `root(x, n; at = p, seed = s, prec = 8)` | driver `roots_at`, `root_at` | a fan |
| `recover(f; num = A, den = B)` | driver `recover` (search limit 1000) | a strip, or a card |
| `project(x, :real, 2, 7; prec = 64)` | driver `project` | the partial ball |

## What a screen shows

- **Strip.** Column R: the real ball as the package prints it (20 digits), `exact` for a rational, `*` for a
  finite ball. One column per prime below 1000 dividing the modulus or a denominator (for an exact value its
  numerator or denominator): `v`, `|x|_p` and the p-adic digits, low digit on the right, the period overlined,
  fog for the digits not known, `.` the p-adic point; above 97 no digits. A cofactor with no prime factor below
  1000 is a column `cofactor n`. The last column, `every other p`, says `unit` or `Z_p`. Hover: the set in words.
- **Fan.** One chip per branch, labelled `p [seed]` as the driver prints it; a click writes `root(...; seed = s)`
  into the cell. A single root asks for `prec + 12` by a click on its fog, or on `12 more digits` above p = 97.
- **Card.** The status name and its class: dashed open (NOT_DETERMINED, UNIT_NOT_CERTIFIED, NEEDS_SPLIT,
  NOT_UNIQUE), solid proved no (DOMAIN, NO_SOLUTION, NOT_UNIT), dotted budget (LIMIT), plain not mathematics
  (PARSE, UNSUPPORTED); OK has no card. `place: not given` (the driver does not print the failing place). When
  the driver gives no reason code, the card lists the possible causes and says which ones the slice checked.
- **Dump.** Under every value: the package's dump, or the driver's `dump` of a local ball `[p=5: ...]`, or why
  there is none. The footer: the package version, the library commit, and what the package does not expose.

## Records and replay

Each record has `cell`, `input`, `kind`, `value`, `status`, `dump` (or `dump_reason`), `driver` (the lines whose
output is `value`), `computed_by`, `stamp`, and the strip columns or the branches. A value without its own
command is reproduced by `load <dump>`. A computed adele is never sent to the driver as its printed text (a
printed real ball, read again, may widen: docs/conventions.md 9.6): at a prime its exact finite part is sent; at
the real place and for `reconstruct` the package computes, and the record says `unchecked` with the reason.
`replay` compares the value, the status, the dump and digits derived from the driver's value, and the literal
arguments of the input against the driver line. A record with no driver line is `unchecked`, never `same`.

## What the slice does not do

- Scenario A step 5: there is no `equal`, `compare` or `teichmuller` in the notebook (an UndefVarError).
- Scenario B steps 3 and 4: no certificates are kept or compared, and there is no acceptance state.
- No pinned branch or moving fog without a rerun; no failing place, reason code or per-place map from the driver.
