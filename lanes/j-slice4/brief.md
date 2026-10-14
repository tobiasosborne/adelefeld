# Lane j-slice4: step 4 of J-a of docs/api-j.md: text classification, dump forms and reconstruction

Milestone J, the Julia binding (epic adf-c5e; design `docs/api-j.md`; statements of steps 1 to 3 in
`docs/api-ja.md`, lanes j-slice1 to j-slice3, merged). The package `julia/Adelefeld/` has the load gates, the
statuses, the handle mechanism, `Rat`, `FBall`, `Adele`, `CAdele`, `RealBall`, `ComplexBall`, 3568 tests through
`Pkg.test`. This lane builds the fourth and last item of J-a in section 9 of the design, "text/dump/
reconstruction", end to end, and nothing wider: no contexts (`ModCtx`), no `Scaled`, no local values; the dump
loaders are used with `ctx = NULL` and no bindings only (read `dump.h` for what a global value's dump needs: if
a dump of a global `FBall` or `Adele` never needs a context, say so with the citation; a dump that needs one is
`UNSUPPORTED` or `DOMAIN` as the header says, and the test asserts that status).

Read first: `CLAUDE.md`, `lanes/COMMON.md`, `docs/api-j.md` (all; sections 4 at lines 176-195 for `readvalue`,
`try_value_text`, `digits`, and 6 at 281-333 for the driver map rows "reconstruct, cap, dump, load"),
`docs/api-ja.md` (all 32 DESIGN-FINDINGs: keep their decisions, in particular 23, 27, 28, 32),
`julia/Adelefeld/src/*.jl` and `test/*.jl`, `lanes/j-slice3/report.md` and `gate.sh`, `include/adelefeld/text.h`
(`adf_text_classify` at 137-143 and the enum `adf_text_kind` at about 120-135 with `adf_text_kind_str` if it
exists, the `_get_str_digits` or digits variants if they exist: read the whole header), `include/adelefeld/dump.h`
(the `_dump_str` and `_load_str` pairs of `rat`, `fball`, `adele`, `cadele`; the limits and statuses; the
dump/load identity statement and its caps: `docs/api-4.md` section 2 as amended by N-D25), `include/adelefeld/
recon.h` (`adf_fball_reconstruct` at 52, `adf_adele_reconstruct` at 57, their statuses NOT_UNIQUE, NO_SOLUTION,
NOT_DETERMINED, LIMIT and what is written on each), `docs/SPEC.md` 4.5 or the section on reconstruction and
`conventions.md` 9 (text forms) and 10 (dump forms), `tools/adf/README.md:92-137` (the driver's `reconstruct`,
`dump`, `load` lines: the Julia user must be able to do the same), `tests/driver/*recon*` and `*dump*` fixtures
(expected values you can reuse as test oracles, with their derivations).

Build (design section 4, lines 176-195, and section 6 rows):
- `TextKind` as an `@enum` transcribed from `adf_text_kind` (values 0 to 12, size 4, alignment 4: the layout
  row exists in the design's table at `docs/api-j.md:55`; gate it with `adf_sizeof_text_kind` and
  `adf_alignof_text_kind`, and compare each name with `adf_text_kind_str(i)` if that function exists, else
  literal with a header citation as `PointComparison` did).
- `try_readvalue(s; prec, limits)` and `readvalue(s)`: `adf_text_classify` first, the kind read only on OK, then
  dispatch to the typed parser of the four value types of this package; a kind of a type not yet in the package
  (local values, ideles, classes, characters, functions) is `UNSUPPORTED` with the kind in `details` (design:
  "supported kind dispatches to its typed parser"); the statuses and stages exactly `text.h:137-143`.
- `try_value_text(x; digits)` and `value_text`, `show` with `:adf_digits` from `IOContext` (design 186-193): the
  `digits` variant of the C printers if the headers have one (read `text.h`; the carriers print at `digits` as
  conventions 9.5 says); `digits` validated in `1..10^6` before C (finding 27 already states the range).
- `dump_text(x) -> String` through `adf_<type>_dump_str` (lossless; `adf_str_free` in a `finally`; NULL or
  length 0 is `LIMIT` as the driver treats it: cite the driver source line) and `try_load_dump(::Type{T}, s;
  limits)` / `load_dump` through `adf_<type>_load_str` with `ctx = NULL`, for `Rat`, `FBall`, `Adele`, `CAdele`;
  the identity `load_dump(T, dump_text(x))` `identical` to `x` for every value the caps admit (design: the
  identity holds when the load limits and D1 caps admit the dumped representation; test the positive and one
  refused case).
- `try_reconstruct(x::FBall, lo::Rat, hi::Rat) -> AdfResult{Rat}` and `reconstruct` through
  `adf_fball_reconstruct`; `try_reconstruct(a::Adele)` through `adf_adele_reconstruct`; every status of the
  headers asserted on an input that produces it (`NOT_UNIQUE`, `NO_SOLUTION`, `NOT_DETERMINED`, `LIMIT`,
  `DOMAIN`), with `details` carrying what the header says is written on that status (read `recon.h`; "an output
  written on a failure status" is a design question: follow the header, record a DESIGN-FINDING if the design's
  "value only when the C header certifies that output on this status" needs a decision).
- tests (`test/runtests.jl`, one appended testset "Adelefeld step J-a.4"): classification of one text of each
  of the 13 kinds (the unsupported ones return `UNSUPPORTED` with the kind); the four typed dumps round-trip
  on 30 random values each, and the dump text equals the C driver's `dump` output for the fixture values of
  `tests/driver/` you can derive by hand (at least five lines); a dump above the load limits refused with
  `LIMIT`; a malformed dump `PARSE`; reconstruction: the SPEC's reconstruction example(s) (find them: the
  `146 mod 1000 = 22/7`-style cases of `docs/ux/README.md` and `tests/driver/*recon*`), `NOT_UNIQUE` on a ball
  too wide for the bounds, `NO_SOLUTION` when proved, `NOT_DETERMINED` when the search is cut (read the header
  for the limited-search semantics of `adf_resid`-style statuses if `adf_fball_reconstruct` has them);
  `readvalue` on the five SPEC 4.3 row texts dispatching to `FBall`; `digits` in `show` through `IOContext`;
  closed handles; GC run with 10^4 round trips and 0 live; inputs unchanged.
- the gate (as `lanes/j-slice3/gate.sh`): `Pkg.test` green, counts before (3568) and after; the session line
  of the step: `using Adelefeld; reconstruct(parse(FBall, "146 mod 1000"), Rat(0), Rat(50))` prints the
  rational the C library returns (derive it by hand and from the driver first; if the example is not
  reconstructible with these bounds, choose the SPEC's own example and say so). No change to
  `tests/test_julia.sh`.
- faults (as before, scratch copies, restored): at least eight: the kind read on a failure status; dispatch
  to the wrong typed parser; `adf_str_free` skipped on the dump; the load limits not passed; `ctx` not NULL;
  `NOT_UNIQUE` turned into a value; `digits` validation after C; the identity test comparing text instead of
  `identical`.

**You own:** `julia/Adelefeld/` (new `src/text.jl` or `src/dump.jl` and `src/recon.jl`; edits of
`src/Adelefeld.jl`, `src/load.jl`, `src/status.jl` for the enum; `test/runtests.jl` appended;
`test/fixtures/fault.c` appended), `docs/api-ja.md` (a section "Step J-a.4" appended), `lanes/j-slice4/`.
Everything else is read-only: no C or header change, no `tests/test_julia.sh` change, the design is not edited
(DESIGN-FINDING instead). No git command that changes state, no `bd`. At most 2 cores, one Julia thread, one
precompile task, every command under `timeout`; the shared object from `timeout 600 sh tests/test_exports.sh`
(delete `build/` at the end); Julia with `LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10` or through the launcher;
the depot under your lane directory with the trailing colon and `JULIA_PKG_OFFLINE=true` (delete it and
`julia/Adelefeld/Manifest.toml` at the end). Nothing in `/tmp`. Red-green with the runs in
`lanes/j-slice4/redgreen.md`. Lines at most 116 characters; files end with a newline; plain sober prose; no
LaTeX; Julia documentation quoted from the installed `?help` with the version.

Report `lanes/j-slice4/report.md`: what was built, every command with its numbers (the gate's counts before
and after), the red and green runs, the fault table, the DESIGN-FINDINGs, what is not done, sources pending.
Aim to finish within about ninety minutes.
