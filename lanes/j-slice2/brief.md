# Lane j-slice2: step 2 of J-a of docs/api-j.md: FBall and the scalar mixed arithmetic

Milestone J, the Julia binding (epic adf-c5e; design `docs/api-j.md`; statements of step 1 in
`docs/api-ja.md`, lane j-slice1, merged). Step 1 built `julia/Adelefeld/` with the load gates, the statuses,
the handle mechanism (`src/handle.jl`: aligned C heap storage, finalizers, close) and `Rat` (`src/rat.jl`),
904 tests through `Pkg.test`, run by one block of `tests/test_julia.sh`. This lane builds the second item of
section 9 of the design, J-a "FBall and scalar mixed arithmetic", end to end, and nothing wider: no `Adele`,
no `CAdele`, no contexts, no dump form.

Read first: `CLAUDE.md`, `lanes/COMMON.md`, `docs/api-j.md` (all; the contract), `docs/api-ja.md` (what exists
and the eleven DESIGN-FINDINGs of step 1: keep its decisions, for instance the `Rat` quotient operator `/`, no
`prec` keyword where the C call has none), `julia/Adelefeld/src/*.jl` and `test/*.jl` (the mechanism you
extend: follow its style), `lanes/j-slice1/result.md` (the gate command with the depot and the trailing colon,
the fixture build flags), `include/adelefeld/fball.h` (all declarations and their comment blocks: statuses,
aliasing, costs), `include/adelefeld/text.h:158-170` (`adf_fball_set_str`, `adf_fball_get_str`),
`docs/SPEC.md` 4.1 to 4.4 (the finite ball `a + N Zhat`, the five arithmetic rows at 242-248, the precision
policies), `docs/conventions.md` 9 (text forms: `a mod N`, `(* ; a mod N)`), `docs/api-m1.md` (`adf_fball_struct`
48 bytes, alignment 8).

Build (design sections 3 and 4, `docs/api-j.md:196-241`):
- the handle `FBall` on `adf_fball_struct` storage (layout row 48/8 added to the load gate's table, both size
  and alignment, literal from `docs/api-m1.md:36`); init, the cached clear, finalizer and close as `Rat`;
  `copy` through `adf_fball_set`; `identical` through `adf_fball_identical`; `is_canonical` as a predicate
  (Bool, never a status).
- constructors: `FBall(q::Rat)` through `adf_fball_set_rat` (radius 0; `Integer` and `Rational` by explicit
  `Rat` conversion as `Rat` does), `try_fball(c::Rat, N::Rat)` and `FBall(c, N)` through
  `adf_fball_set_center_radius` (a negative `N` is `DOMAIN`: assert it), `parse(FBall, s)` and `try_parse`
  through `adf_fball_set_str` with `TextLimits` (both forms of the grammar: `3 mod 12` and `(* ; 3 mod 12)`;
  a non-sentence is `PARSE`; the limits are honoured: assert `LIMIT` on a too-long text).
- text: `string` and `show` through `adf_fball_get_str` with `adf_str_free` always run (the frame the C
  printer writes is what the design says to print: read `docs/api-j.md` section 4 and decide; record the
  decision in `docs/api-ja.md`).
- arithmetic: `FBall + FBall`, `-`, `*`, unary `-` through `adf_fball_add`, `_sub`, `_mul`, `_neg`; `Rat`
  mixed with `FBall` by radius-zero conversion for `+` and `-` and by `adf_fball_mul_rat` for `*` (both orders);
  `FBall / Rat` through `adf_fball_div_rat` (zero divisor `NOT_UNIT`); the `try_` forms return `AdfResult`, the
  operators throw `AdfError`; no `FBall / FBall`, no `==` on balls (an `ArgumentError` naming `equal_set` and
  `compare_points`, as the design says); `Integer` and `Rational` scalars by explicit `Rat` conversion only.
- predicates and accessors: `equal_set`, `overlaps`, `inside` (FIRST INSIDE SECOND, `fball.h` set predicates;
  check the argument order against the header and say so in a comment), `compare_points` returning the
  `PointComparison` enum (EQUAL, DIFFERENT, UNDECIDED from `status.h:35-37`, with its own load-time name check
  if a name function exists; otherwise literal with a header citation), `center` and `radius` returning fresh
  `Rat`s through `adf_fball_get_center` and `_get_radius`.
- tests (`test/runtests.jl`, appended testsets; design section 8, the J-a line): the five rows of `docs/SPEC.md`
  4.3 (lines 242-248) EXACTLY, through `parse` and the operators, compared with `equal_set` against the parsed
  expected text (`(3 mod 12) + (5 mod 18)` is `2 mod 6`, and the other four); fractional radius (`0 mod 1/6`)
  and zero radius; `q - x` against `x - q` signs; `x - x` as independent balls (not 0); `den = 0` in a text is
  `DOMAIN` or `PARSE` as the C header says (read it; assert the one it says); division by zero `NOT_UNIT`;
  inputs unchanged by every operation (compare `string` before and after); copies clear separately; the
  closed-handle error; the GC run with 10^4 `FBall` handles and the live count returning to 0; a mixed-scalar
  matrix (`Int`, `BigInt`, `Rational{Int}`, `Rational{BigInt}`, `Rat` on both sides of each operator); the
  text round trip of 50 random balls (`string(parse(FBall, string(x)))` equal, and `equal_set`).
- the gate: the section 9 command (as `lanes/j-slice1/gate.sh`, with the trailing colon on the depot path),
  `Pkg.test` green, and the session line of the step: `using Adelefeld; parse(FBall, "3 mod 12") + parse(FBall,
  "5 mod 18")` prints the text form of `2 mod 6`. The `tests/test_julia.sh` block of step 1 already runs
  `Pkg.test`; do not add a second block.
- faults (as step 1's `faults.py`): at least eight, planted one at a time in scratch copies and restored: the
  argument order of `inside` reversed; `mul_rat` with the radius not scaled; `sub` implemented as `add`;
  radius sign dropped in `FBall(c, N)`; `compare_points` cast to a status; `equal_set` by `identical`; a
  `GC.@preserve` omitted; the `adf_str_free` skipped on a failing parse. Record which the tests catch.

**You own:** `julia/Adelefeld/` (new file `src/fball.jl`, edits of `src/Adelefeld.jl` for the include and the
exports, of `src/load.jl` for the layout row and the new symbols in `REQUIRED`, of `src/status.jl` only if the
`PointComparison` enum belongs there; `test/runtests.jl` appended), `docs/api-ja.md` (a new section "Step
J-a.2" appended: what was built, statements, DESIGN-FINDINGs), `lanes/j-slice2/`. Everything else is
read-only: no C change, no header change, no change to `tests/test_julia.sh`; the design is not edited. No git
command that changes state, no `bd`. At most 2 cores, one Julia thread, one precompile task, every command
under `timeout`; the shared object from `timeout 600 sh tests/test_exports.sh` (delete `build/` at the end);
the depot under your lane directory (delete it at the end; delete `julia/Adelefeld/Manifest.toml`, which is
gitignored). Lines at most 116 characters, files end with a newline, plain sober prose, no LaTeX. Red-green
with the runs recorded in `lanes/j-slice2/redgreen.md`.

Report `lanes/j-slice2/result.md` (a Claude subagent cannot write `report.md`): what was built, every command
with its numbers (the gate's pass count before and after), the red and green runs, the fault table, the
DESIGN-FINDINGs, what is not done, sources pending (Julia help quoted with the version as step 1 did). Aim to
finish within about seventy-five minutes.
