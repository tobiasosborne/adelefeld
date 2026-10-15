# Rules shared by the four UX lanes (ux-notebook, ux-atlas, ux-ledger, ux-bench), 2026-10-15

The four visions of `docs/ux/README.md` are built as their "first buildable slice" (the last section of each
brief), one lane each, under a new top-level directory `ux/`. Decisions of the orchestrator (recorded in
`docs/ux/README.md`, section "Decisions"):

- D-UX1. Each slice is static: files on disk, no server, no database, no network. Pages are single HTML files
  with inline CSS and JavaScript, or a page plus files next to it; they open from `file://` in Firefox.
- D-UX2. No new dependencies. Python 3 standard library; Node 24 (`node`) for JavaScript tests, no `npm
  install`; Julia 1.12 with what `~/.julia` already holds (`HypertextLiteral`, `JSON`; there is NO Pluto and
  the machine's Julia runs offline). No system packages. Nothing is published.
- D-UX3. The C library is NOT changed by a UX lane. Where a brief says the library must provide something
  (the result record with the failing place, dump forms of ideles, digits), the slice parses the text of the
  driver `build/adf` and lives with what the driver gives; what is missing is written down in the report
  under "Library demands met by the slice" as a numbered list against the brief's list, so that the C work
  can be planned. Do not fake a value the driver does not give: a cell, column or card then says `not given`.
- D-UX4. One status language for all four (README, "What could be combined"): the four classes with shapes:
  open, dashed (`NOT_DETERMINED`, `UNIT_NOT_CERTIFIED`, `NEEDS_SPLIT`, `NOT_UNIQUE`); proved no, solid
  (`DOMAIN`, `NO_SOLUTION`, `NOT_UNIT`); budget, dotted (`LIMIT`); not mathematics, plain (`PARSE`,
  `UNSUPPORTED`). The status names are those of `include/adelefeld/status.h`; read it and list every name
  in your class table, with `OK` as its own class.
- D-UX5. Every number shown on a screen was computed by the driver or the Julia package in the lane, never
  typed from memory; the fixture files that produced it are committed next to the page. Amended 2026-10-16
  after the atlas drive (H1): a number DERIVED from the driver's output by a finite computation written in
  the slice (base-p digits of a printed centre, a count, a residue) is allowed when the page or its README
  says so and the driver line it came from is kept with it; a number the driver could print but the slice
  computes instead is a library demand to list under D-UX3.
- D-UX6. A test drive by another model follows each slice (codex `gpt-6-astra`, acting as a user of the
  brief's two scenarios, without your report). So the slice must be usable from its own `README.md`
  alone: how to build, how to run each command, what each screen shows, in under 80 lines.

The driver. `make -j2` builds `build/libadelefeld.a`; `make -C tools/adf` builds `build/adf`. It reads
commands from a file or standard input, one result line per command (`tools/adf/README.md`: the whole
file, the table of operations at lines 52-89, the solver commands at 144-213, the place commands at
214-272, ideles at 346-378, output at 379-392). The acceptance fixtures `tests/driver/NN_name.cmd` with
`.out` (the first line `#!exit N` is the expected exit status) show every command with its expected output;
`tests/test_driver.sh` runs them. The value forms are `docs/conventions.md` section 9; the status names
section 11.1 and `include/adelefeld/status.h`.

The Julia package `julia/Adelefeld/` (read `docs/api-j.md` section 1 and `docs/api-ja.md` for what exists):
`Rat`, `FBall`, `Adele`, `CAdele`, `RealBall`, `ComplexBall`, `readvalue`, `value_text`, `dump_text`,
`load_dump`, `reconstruct`. No local balls, no contexts, no roots yet (slices J-b to J-d). Julia is started
through `julia/Adelefeld/bin/adelefeld-julia` (the libgmp preload) with
`JULIA_DEPOT_PATH="$PWD/lanes/<lane>/depot:$HOME/.julia:"` (the TRAILING colon), `JULIA_PKG_OFFLINE=true`,
`JULIA_NUM_THREADS=1`, `JULIA_NUM_PRECOMPILE_TASKS=1`; `lanes/j-slice4/gate.sh` is the model.

Checks. A slice is done when: its README's commands run as written from a fresh shell in the repository
root; its fixtures reproduce (the records or scene or seal or wall files are regenerated and `diff` is
empty); its unit tests pass (`python3 -m unittest`, `node --test`, or a Julia test file, under `timeout`);
and the two scenarios of the brief can be walked by a person with the page open. A screenshot is optional:
`firefox --headless --profile "$(mktemp -d)" --screenshot lanes/<lane>/shot.png file://$PWD/ux/<dir>/<page>`
works on this machine when no other Firefox holds the profile; do not fight it if it does not.

Report (`lanes/COMMON.md` rule 8; `result.md` for a Claude lane): what was built; the files; every check
with its command and numbers; the library demands met and unmet (D-UX3); what the test driver should try;
findings against the brief (where the brief's design did not survive contact with the driver's output).
