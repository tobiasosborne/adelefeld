# Lane ux-notebook: the first buildable slice of brief 1, Adelica, the notebook

Read first: `CLAUDE.md`, `lanes/COMMON.md`, `lanes/UX-COMMON.md`, `docs/ux/README.md`,
`docs/ux/brief-1-adelica-notebook.md` (all; the slice is its last section), `docs/ux/mock-1-adelica-notebook.html`
(the place strip, status cards, branch chips: reuse its markup and CSS), `docs/api-j.md` section 1 and 4,
`docs/api-ja.md` (what the package exports and its findings), `julia/Adelefeld/src/*.jl` and
`julia/Adelefeld/test/runtests.jl` (how values are made and printed), `lanes/j-slice4/gate.sh` and
`lanes/j-slice4/run-julia.sh` (how Julia is started here), `tools/adf/README.md` (the commands `roots`,
`project`, `reconstruct`, `recover` and their forms: lines 144-272), `docs/conventions.md` 9,
`include/adelefeld/status.h`.

You own: `ux/notebook/**` and `lanes/ux-notebook/**`. Nothing else; the package `julia/Adelefeld/` is read-only.

There is no Pluto on this machine and Julia runs offline (D-UX2). The slice therefore builds the notebook as a
Julia module `Adelica` that evaluates a list of cells in order and writes ONE static HTML page per notebook,
with the records sidecar; the interaction the brief gives to Pluto (a click on a chip edits the cell text) is
done on the static page by inline JavaScript that rewrites the shown cell text and marks the cell "re-evaluate
to keep" (the human's kernel recomputes before the cell is kept: the page cannot compute, so it says so).
Record this in the README as the slice's reading of the brief.

Build, in this order:

1. `ux/notebook/src/Adelica.jl`: a module loaded with `--project=julia/Adelefeld` and
   `push!(LOAD_PATH, "ux/notebook/src")` (or an equivalent that runs offline; say which in the README), using
   `Adelefeld`. The place strip as `show(io, MIME"text/html"(), x)` for `Rat`, `FBall`, `Adele` (and `CAdele`
   if cheap): one column `R` (the real ball printed by the package, or `exact` for a `Rat`), one column per
   interesting prime (the primes below 1000 dividing the modulus `N` of the finite part, found by trial
   division in Julia; an unfactored cofactor is shown as `cofactor <n>`, D-UX3 item 5 of the brief), with the
   valuation and the base-p digits from the centre of `a mod p^k` (for a `Rat`, the pre-period and period of
   its p-adic expansion, computed in Julia from the reduced fraction with a cap of 64 digits and a `...`), fog
   for the digits beyond the precision, and a last column "every other p" with its count (`infinitely many`
   for an exact rational, the brief's wording). The numbers of the strip come from the package's value text,
   parsed in Julia (the finite part `a mod N`); write a parser for exactly the grammar of conventions 9 that
   the package prints and test it on the five SPEC 4.3 row texts.
2. `roots(x, n; at=p, prec=N)` returning a branch fan: the driver's `roots` command on the polynomial `X^n - x`
   at the prime (the slice runs `build/adf` as a subprocess and parses its lines; the package has no local
   roots yet), one chip per root, labelled by its seed (the residue of the root modulo p, or what the driver
   prints as the seed: read `tools/adf/README.md:480-514` for the branch list form); `root(x, n; at, seed)`
   selects one. In the page a click on a chip rewrites the cell text to `root(x, n; at=p, seed=s)`.
3. Status cards: every status the wrapped calls can return (`AdfError`/`AdfResult` of the package, `error:
   STATUS` lines of the driver) renders as a card with the status name, its class and shape (D-UX4), the
   place when known (the driver does not print the failing place: the card says `place: not given`), and
   the actions the brief lists for that status (as text, no buttons in the slice).
4. `Adelica.run(cells::Vector{String}, out::String)`: evaluates the cells (each a Julia expression in the
   notebook's small language: the constructors of the package, `+ - *`, `roots`, `root`, `reconstruct`,
   `project` through the driver), writes `out.html` (cells with input, output strip or card or fan) and the
   sidecar `out.records.jsonl` with one record per output: cell index, input text, kind, value text, status,
   dump text (when the package has it), and the driver line that reproduces the value where one exists.
   `Adelica.replay(records, adf)`: runs the driver line of each record and compares the value text, printing
   `same` or `differs` per record and exiting 1 on a difference.
5. The two scenarios as notebooks: `ux/notebook/scenarios/a_roots.jl` (Scenario A: the square roots of 9, then
   of -1, at 5) and `b_which_rational.jl` (Scenario B: which rational is this, by `reconstruct`); the pages
   `ux/notebook/out/a_roots.html`, `b_which_rational.html` and their records committed; a `make`-free runner
   `ux/notebook/run.sh` that builds the driver if missing and regenerates both.
6. `ux/notebook/README.md` (D-UX6) and `ux/notebook/test/runtests.jl`: the strip parser on the five SPEC 4.3
   rows; the digits of `1/3` at 2, 3, 5 and of `22/7` at 7 against values you compute by hand in the test
   (show the computation in a comment); a fan with the right number of chips for `roots(9, 2; at=5)` (two)
   and `roots(-1, 2; at=5)` (two) and for `roots(2, 2; at=5)` (a status card, no root); a records file with
   as many records as cells; `replay` saying `same` for every record of both scenarios.

Checks to report with numbers: the test counts; the two pages regenerated and `diff` empty; `replay` on both
records files; the run time of each scenario; what the page shows at each step of Scenario A and B.

Out of the slice: ghost cells, plots, help pages, an editor. The agent window is the records file only.
