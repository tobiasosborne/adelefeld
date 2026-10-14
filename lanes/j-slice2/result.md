# Lane j-slice2: result

Step 2 of J-a in `docs/api-j.md` section 9 ("FBall and scalar mixed arithmetic") is built end to end and its
checks pass. The gate went from 904 of 904 (step J-a.1 only) to 904 of 904 plus 1015 of 1015 (step J-a.2). The
session line `using Adelefeld; parse(FBall, "3 mod 12") + parse(FBall, "5 mod 18")` prints `(* ; 2 mod 6)`.
Nothing wider was built: no Adele, CAdele, contexts, local values or dump form.

## What was built

- `julia/Adelefeld/src/fball.jl` (new, 387 lines):
  - the handle `FBall <: Number` on aligned C heap storage with the checked row 48/8; init, finalizer, cached
    clear, `close`, a closed handle refused before C, as for `Rat`;
  - `copy` (`adf_fball_set`), `identical`, and `is_canonical` (a Bool);
  - constructors: `FBall(q::Rat)` (`set_rat`); `FBall(::Integer)` and `FBall(::Rational)` by explicit Rat
    conversion; `try_fball(c, N)` and `FBall(c, N)` (`set_center_radius`, negative N is DOMAIN);
    `try_parse(FBall, s; limits)` and `parse` (`set_str`, both forms of the grammar);
  - text: `try_value_text`, `value_text`, `show` (`adf_fball_get_str`, `adf_str_free` in a `finally`, the C text
    with its `(* ; F)` frame unchanged, as the design prints it);
  - arithmetic: `try_add`, `try_sub`, `try_mul`, `try_neg` and `+ - *`, unary `-` between balls; Rat, Integer and
    Rational scalars on both sides of `+ -` (radius-zero conversion) and `*` (`mul_rat`); `x / q` and `try_div`
    (`div_rat`, NOT_UNIT for 0);
  - predicates: `equal_set`, `overlaps`, `inside` (FIRST INSIDE SECOND = `adf_fball_contains(x, y)` in the same
    order, as the header comment at `fball.h:248` says); `compare_points` returns a `PointComparison`;
  - accessors: `center` and `radius` return fresh Rats;
  - `==` on balls (and between a ball and a scalar) throws an `ArgumentError` naming `equal_set` and
    `compare_points`; `isequal` and `hash` are identity-based.
- `src/status.jl`: `@enum PointComparison::Int32` (EQUAL, DIFFERENT, UNDECIDED; literal from `status.h:35-37`,
  because no C name function exists); `to_point_comparison` and `to_bool` (a raw value out of range is
  `AdfAbiError`).
- `src/load.jl`: the layout row `fball` 48/8 (`docs/api-m1.md:36,45`, size and alignment) and 23 symbols in
  `REQUIRED`. The two layout queries come after `adf_status_count`, so the pre-J-status fixture still reports
  that symbol. `src/Adelefeld.jl`: the include and 10 exports.
- `test/runtests.jl`: the testset "Adelefeld step J-a.2" was appended (18 testsets, 1015 checks).
  `test/fixtures/fault.c` has two more fixture faults, FAULT_FBALL_SIZE and FAULT_FBALL_ALIGN.
- `docs/api-ja.md`: the section "Step J-a.2" was appended, with statements S11-S20, DESIGN-FINDINGs 12-21 and the
  Julia sources.
- `lanes/j-slice2/`: `gate.sh`, `faults.py`, `redgreen.md`, `help.log` (installed Julia help), and the logs
  `exports.log`, `before.log`, `red.log`, `green1.log`, `final.log`, `session.log`, `faults.log`, `fault-F*.log`.

## Commands and results

| Command | Result |
|---|---|
| `timeout 600 sh tests/test_exports.sh` | passed: 625 of 625 declared functions exported, 0 extra, 0 variadic |
| `sh lanes/j-slice2/gate.sh`, before any change (before.log) | exit 0; J-a.1 904 of 904 |
| gate, red (red.log): tests and fixtures only | exit 1; J-a.1 904/904; J-a.2 5 pass, 20 fail, 45 error of 70 |
| gate, green (green1.log), first attempt after the code | exit 0; J-a.1 904/904; J-a.2 1015/1015 |
| gate, final (final.log), after the docs | exit 0; J-a.1 904/904; J-a.2 1015/1015; "tests passed" |
| session through `bin/adelefeld-julia` with no preload set (session.log) | `display(...)` prints `(* ; 2 mod 6)` |
| mallinfo2 margin, 3 x 400 prints of a 50015-byte text (session.log) | bytes in use change by 0, 0, 0 |
| `python3 lanes/j-slice2/faults.py F1 ... F7`, then `F8 ... F14` | 12 of 14 caught (below); 4:45 + 7:08 min |

`gate.sh` is the command at `docs/api-j.md:409-410`. It runs under `timeout 180` with one thread, one precompile
task, the GMP preload, `JULIA_DEPOT_PATH=<lane>/depot:$HOME/.julia:` (with the trailing colon),
`JULIA_PKG_OFFLINE=true` and TMPDIR in the lane. A gate run takes about 35 s.

`tests/test_julia.sh` was not run. Its Pkg.test block is the same `Pkg.test` as the gate, and no block was
added.

## Faults

Each fault was planted in a scratch copy of the package, then the gate was run on that copy. The sources in
`julia/Adelefeld` were never edited by the script.

| Fault | Result (failed / errored checks of J-a.2) |
|---|---|
| F1 `inside`: argument order reversed | caught: 5 failed |
| F2 `mul_rat`: radius not scaled (q a + N Zhat) | caught: 58 failed |
| F3 `sub` implemented as `add` | caught: 64 failed |
| F4 `FBall(c, N)`: radius sign dropped | caught: 6 failed |
| F5 `compare_points` cast to a status | caught: 8 failed |
| F6 `equal_set` by `identical` | NOT caught: an equivalent mutant (finding 13) |
| F7 `GC.@preserve` omitted (ball add/sub/mul) | NOT caught (finding 14) |
| F8 `adf_str_free` skipped in the printer | caught: 1 failed (mallinfo2: 26326352 bytes against < 4000000) |
| F9 fresh output of a failing parse/try not cleared | caught: 2 failed (live count) |
| F10 fball layout row not gated | caught: 17 failed |
| F11 `q - x` computed as `x - q` | caught: 34 failed |
| F12 `div_rat` status ignored | caught: 14 failed |
| F13 `==` on balls as `equal_set` | caught: 3 failed, 1 errored |
| F14 `isequal` by `equal_set`, not identity | caught: 1 failed |

The brief's fault "`adf_str_free` skipped on a failing parse" cannot exist as written, because a parser
allocates no text for its caller. F8 and F9 were planted in its place (finding 15).

## DESIGN-FINDINGs (details in docs/api-ja.md, 12-21)

12. The C printer writes the frame `(* ; F)`, and the design prints `(* ; 2 mod 6)`
    (`docs/api-j.md:313-314, 418`). The text is returned unchanged. The frame removal at `:237` applies only to
    the RealBall/ComplexBall carriers.
13. `equal_set` and `identical` agree on every global canonical value, because the canonical triple is unique.
    F6 is therefore an equivalent mutant; telling the two apart needs local values (J-b).
14. No test can force a GC between a pointer read and its ccall, so a missing `GC.@preserve` is a review
    obligation, not a test result.
15. The fault "free skipped on a failing parse" has no object; it was replaced by F8 and F9. The mallinfo2 test
    needs glibc 2.33 or later (this machine has 2.39) and FLINT allocating with malloc.
16. The constructors take Integer and Rational by explicit Rat conversion, as the brief asks; the design lists
    Rat only. `FBall(c, N)` takes Rats only.
17. `q * x` and `x * q` both call `mul_rat(z, x, q)`. `+` and `-` with a Rat first convert it to a radius-zero
    ball. A closed ball is refused before the scalar conversion allocates.
18. `==` is refused for the ball/scalar pairs as well. As a result `x in [x]` throws, while `x in Set([x])` is
    true. `isequal` of a ball with a scalar is false.
19. Because `FBall <: Number`, Base's generic `^` makes `x^2` equal to `x * x`, the product of two independent
    balls: `(1 mod 2)^2` prints `1 mod 2`, not the tight `1 mod 8`. This is an enclosure but undocumented. Whether
    to refuse `^` until `pow` lands is for the orchestrator to decide.
20. `PointComparison` has no load-time name check, because there is no C name function. The FBall predicates'
    int must be 0 or 1, otherwise `AdfAbiError`. The Rat predicates of step 1 still test `!= 0`.
21. `FBall / FBall`, `Rat / FBall` and `1 / FBall` fail through Julia's promotion fallback (`ErrorException`),
    not through a `MethodError`. The tests check that they throw and that the method is not the package's.

Findings against the specification: none.

## Not done

- The design's "Rat inputs become points" for the set predicates (`docs/api-j.md:223`): there are no
  `inside(q::Rat, x)` or `contains_rat` methods; the brief did not ask for them.
- No `pow`/`^` method (finding 19), no `prec_at`, `haar_volume`, `get_fmpz3`, `is_exact` or `swap`; the brief did
  not ask for them.
- No per-symbol inventory file. `REQUIRED` in `load.jl` lists the 47 symbols in use.
- No two-thread test, and no fuzzing. The 50-ball round trip and the 36-pair scalar matrix are bounded
  differential checks against Julia `Rational{BigInt}`; the 10^4-iteration GC loop is a smoke test.
- `tests/test_julia.sh` was not run (see above).

## Sources pending

- None new. Julia documentation is quoted from the installed help of Julia 1.12.5 in `lanes/j-slice2/help.log`
  (`isequal`, `hash`, `==`, `in`, `objectid`, `@enum`, captured with `@doc` and the text/plain display) and in
  `lanes/j-slice1/help.log`. `intfuncs.jl:416` for `^` comes from `which` in this Julia, not from a document. The
  pending glibc `dlsym` item of step 1 still applies to the two new fixture cases.

## Files

Changed: `julia/Adelefeld/src/{Adelefeld.jl,load.jl,status.jl}`, `julia/Adelefeld/test/runtests.jl` (+476 lines,
appended), `julia/Adelefeld/test/fixtures/fault.c` (+4), `docs/api-ja.md` (+134, appended). New:
`julia/Adelefeld/src/fball.jl`, `lanes/j-slice2/*`. Deleted at the end: `build/`, the lane depot and tmp, the
scratch copies, `julia/Adelefeld/Manifest.toml`. No C, header, `tests/test_julia.sh` or `docs/api-j.md` change;
no git command that changes state; no bd.
