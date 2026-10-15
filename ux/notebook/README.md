# Adelica, the notebook slice

The first slice of `docs/ux/brief-1-adelica-notebook.md`: a notebook in which every value is drawn as a place strip.
A notebook is a Julia file of cells. `Adelica.run` evaluates the cells in order and writes one static HTML page and
a records file `NAME.records.jsonl` (one JSON object per output: the agent's window). `replay` runs the driver
line of every record and compares.

How the slice reads the brief: this machine has no Pluto and Julia runs offline. So the page is static and cannot
compute. A click on a branch chip, or on the fog of a single root, writes the new call into the cell's text on the
page and marks the cell "re-evaluate to keep" (a `restore` button undoes it). The cell is kept only after you put
that text into the notebook file and run it again. That second run is the human's kernel recomputing.

## Build and run (from the repository root)

    sh ux/notebook/run.sh            builds build/adf and build/libadelefeld.so if missing, then writes
                                     ux/notebook/out/a_roots.html and b_which_rational.html with their records
    firefox ux/notebook/out/a_roots.html
    sh ux/notebook/replay.sh ux/notebook/out/a_roots.records.jsonl     prints `cell i: same` per record;
                                                                        exit status 1 on a difference
    sh ux/notebook/test.sh           the Julia tests (ux/notebook/test/runtests.jl)
    node --test ux/notebook/test/page.test.js                           the page's click script

Julia is started by `ux/notebook/julia.sh`: the package `julia/Adelefeld` is the project, the libgmp preload comes
from `julia/Adelefeld/bin/adelefeld-julia`, it runs offline on one thread, and the depot is under
`lanes/ux-notebook/depot`. `Adelica` is loaded with `push!(LOAD_PATH, "ux/notebook/src"); using Adelica`.
Your own notebook:

    sh ux/notebook/julia.sh -e 'push!(LOAD_PATH, "ux/notebook/src"); using Adelica;
        Adelica.run_file("my.jl", "/tmp/my")'        writes /tmp/my.html and /tmp/my.records.jsonl

## The notebook file

Cells are separated by lines that begin `# %%`. The rest of that line is a note shown above the cell. Each cell
is one Julia expression, optionally `name = expression`. The text before the first `# %%` is not a cell. The
two scenarios are `ux/notebook/scenarios/a_roots.jl` (the square roots of 9, then of -1, at 5) and
`b_which_rational.jl` (which rational is 146 mod 1000, by recover and by reconstruct).

| Call | Computed by | Shows |
|---|---|---|
| `Rat(6, 35)`, `FBall(c, N)`, `adf"146 mod 1000"`, `readvalue("...")`, `+ - * /` | the package | a strip |
| `reconstruct(x)` (an Adele), `reconstruct(f, lo, hi)` | the package | a strip, or a card |
| `roots(x, n; at = p, prec = 8)` | driver `roots_at` | a fan: one chip per branch |
| `root(x, n; at = p, seed = s, prec = 8)` | driver `root_at` | one branch |
| `recover(f; num = A, den = B)` | driver `recover` (search limit 1000) | a strip, or a card |
| `project(x, :real, 2, 7; prec = 64)` | driver `project` | the partial ball |

## What a screen shows

- **Strip.** Column R holds the real ball as the package prints it at 20 digits; for a rational it says `exact`,
  and for a finite ball it is `*` (no real coordinate). Then there is one column per prime below 1000 that divides
  the modulus or the centre's denominator; for an exact value, its numerator or denominator. Each prime column shows
  the valuation `v`, `|x|_p` and the p-adic digits. A cofactor with no prime factor below 1000 gets its own
  column, `cofactor n`, and is not factored. The last column, `every other p`, says `unit` or `Z_p`.
- **Digits.** The low digit is on the right. For a rational the period is overlined; it is eventually periodic,
  with a cap of 64 digits. For a ball, a textured fog stands for the digits from `p^k` on, which are not known. The
  `.` is the p-adic point. Above p = 10 each digit is a decimal number, separated by spaces; above 97 no digits are
  shown. In the records the same text is written `(period)preperiod`, `~` for fog, `...` for the cap.
- **Fan.** One chip per branch, labelled `p [seed]` as the driver prints it. A click writes
  `root(x, n; at = p, seed = s)` into the cell. A click on the fog of a single root asks for `prec + 12`.
- **Card.** It shows the status name and its class. Open statuses have a dashed border (NOT_DETERMINED,
  UNIT_NOT_CERTIFIED, NEEDS_SPLIT, NOT_UNIQUE). Proved answers have a solid border (DOMAIN, NO_SOLUTION, NOT_UNIT).
  The budget status LIMIT has a dotted border. Plain cards are not mathematics (PARSE, UNSUPPORTED). OK has no
  card. The card also gives `place: not given` (the driver does not print the place), one reason and the actions.
- **Records.** Each record has `cell`, `input`, `kind`, `value`, `status`, `dump` and `driver` (the lines that
  reproduce `value`), plus the strip columns or the branches. A value without its own command is reproduced by
  `load <dump>`. A printed real ball, read again, may widen (docs/conventions.md 9.6), so the dump is used instead.
