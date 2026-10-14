# Lane j-slice3: step 3 of J-a of docs/api-j.md: Adele, CAdele and the numerical carriers RealBall, ComplexBall

Milestone J, the Julia binding (epic adf-c5e; design `docs/api-j.md`; statements of steps 1 and 2 in
`docs/api-ja.md`, lanes j-slice1 and j-slice2, merged). The package `julia/Adelefeld/` has the load gates, the
statuses, the handle mechanism (`src/handle.jl`), `Rat` (`src/rat.jl`) and `FBall` (`src/fball.jl`), 1919
tests through `Pkg.test`. This lane builds the third item of section 9 of the design, J-a "Adele/CAdele and
numerical carriers", end to end, and nothing wider: no contexts, no places beyond what `adf_adele_get_arb_at`
needs (read it: it takes an `adf_place_t`; if `Place` is needed for that one getter, build the minimal bits type
of design section 5 with `adf_place_inf` only, and say so), no dump form, no text beyond the value forms.

Read first: `CLAUDE.md`, `lanes/COMMON.md`, `docs/api-j.md` (all; the contract; sections 3 and 4 and the
carrier paragraph at lines 145-150, 233-240), `docs/api-ja.md` (all 21 DESIGN-FINDINGs of steps 1 and 2: keep
their decisions), `julia/Adelefeld/src/*.jl` and `test/*.jl` (the mechanism and style you extend),
`lanes/j-slice2/result.md` and `gate.sh` (the gate command with the depot and its trailing colon),
`include/adelefeld/adele.h` (all 43 declarations and their comment blocks), `include/adelefeld/text.h`
(`adf_adele_set_str`, `_get_str`, the `adf_cadele_*` text forms), `docs/SPEC.md` 4.1 to 4.4 (the adele
`(I ; F)`, the complex adele, the precision policies 4.4), `docs/conventions.md` 9 (the value forms `(R ; F)`,
`(Z ; F)`, and the real ball text `m +/- r`), `docs/api-m1.md` (`adf_adele_struct` 96, `adf_cadele_struct` 144,
alignment 8; the first member is the `arb` or `acb`), `tests/julia/tate_fe.jl` (how the fixtures already use a
cadele as an `acb` carrier).

Build (design sections 3 and 4):
- the handles `Adele` (96/8) and `CAdele` (144/8) with their layout rows (literal, size and alignment), init,
  cached clear, finalizer, close, `copy`, `identical`, `is_canonical`, as `FBall`.
- the carriers `RealBall` and `ComplexBall` of design J7 and lines 145-150: each owns an initialized `Adele` or
  `CAdele` whose finite coordinate is the exact 0; the first-member `arb` or `acb` address is used only inside a
  `GC.@preserve` block as the numerical argument of a C call; the whole carrier is cleared exactly once; they are
  NOT adeles for the user (no `finitepart`, no mixed arithmetic with `FBall`), and their `show` prints the C
  carrier text with its fixed outer frame ` ; 0` removed (design line 237: say exactly which characters you
  remove and test the inverse, the text constructor adds the same frame). Constructors from text
  (`parse(RealBall, "1.5 +/- 2^-10")`: read the real ball grammar of conventions 9 and text.h and use the
  forms it accepts), from `Rat` and `Integer` at a `prec` (exact when representable), and `ComplexBall` from two
  `RealBall`s or from text `(a) + (b)*i` as the fixtures write it. Arithmetic `+ - *` and unary `-` of carriers
  through the carrier `adf_adele_add` etc. with `prec` (design line 238), zero finite parts preserved: assert
  after every operation that the finite coordinate is still exact 0 (`adf_adele_get_fball` or the getter the
  header offers, then `radius == 0` and `center == 0`).
- `Adele(q::Rat; prec = 64)` through `adf_adele_set_rat`; `Adele(r::RealBall, f::FBall)` through
  `adf_adele_set_arb_fball` (its status: read `adele.h:112-117`); `CAdele(q; prec)`, `CAdele(a::Adele)` through
  `adf_cadele_set_adele`, `CAdele(z::ComplexBall, f::FBall)` through `adf_cadele_set_acb_fball`;
  `parse(Adele, s; prec, limits)` and `parse(CAdele, ...)` through the text setters (both take `prec`: read
  `text.h:169` onwards), `try_parse`; `string`/`show` through the getters with `adf_str_free` in a `finally`.
- arithmetic (design lines 208-216): `Adele + Adele` etc. with `prec = 64` keyword on the `try_`/named forms
  and the design's default for operators; `Rat` mixed with `Adele`/`CAdele` through the `_rat` calls; `Adele`
  mixed with `CAdele` by `adf_cadele_set_adele` embedding; `x / q::Rat` through the `div_rat` calls (zero
  divisor `NOT_UNIT`); every other mixed pair refused (`MethodError` or `ArgumentError`, say which and why);
  `==` refused as for `FBall`.
- accessors: `finitepart(x::Adele) -> FBall` (a copy), `realpart(x::Adele) -> RealBall` (through
  `adf_adele_get_arb_at` at `adf_place_inf()` or the getter without a place if one exists: read `adele.h:118-128`),
  the same two for `CAdele` with `complexpart -> ComplexBall`; the design names `finitepart` only: name the
  others as the C functions suggest and record a DESIGN-FINDING.
- the decision of the orchestrator on finding 19 of `docs/api-ja.md` (Base's generic `^` on `FBall` gives a
  looser enclosure than the tight power): `^` is REFUSED on `FBall`, `Adele` and `CAdele` with an
  `ArgumentError` naming the future `pow`, until `pow` lands; one method per type, one test each; record it
  in `docs/api-ja.md` as the resolution of finding 19.
- tests (`test/runtests.jl`, one appended testset "Adelefeld step J-a.3"): the design's J-a line of section 8
  as far as it concerns adeles (the SPEC 4.3 rows through `Adele` with `prec` 64, their finite parts
  `equal_set` to the expected balls, their real parts containing the exact rational); the precision policies
  of SPEC 4.4 that the headers state (read them; one test per stated rule); exactness of a rational at `prec`
  where representable and a nonzero radius where not (`1/3` at prec 64); carrier arithmetic against
  `BigFloat` at 256 bits for 50 random pairs (the result contains the exact value: decode the C text or use
  the radius getter); the embedding `CAdele(Adele(q))` has imaginary part exactly 0; mixed-type refusals; the
  round trip of 30 adele texts; closed handles; the GC run with 10^4 handles of each new type and 0 live;
  inputs unchanged; `^` refused; `prec` out of range or non-integer refused before C.
- the gate (as `lanes/j-slice2/gate.sh`): `Pkg.test` green with the counts before (1919) and after; the session
  line of the step: `using Adelefeld; Adele(Rat(1, 3); prec = 64) + Adele(parse(FBall, "5 mod 18"); prec = 64)`
  prints the C value form (give the printed line). No change to `tests/test_julia.sh`.
- faults (as `lanes/j-slice2/faults.py`, scratch copies, restored): at least eight, including the carrier
  frame removed by a wrong width, `prec` not passed, `set_adele` embedding skipped, the finite part of a
  carrier not asserted zero, the `arb` address taken outside the preserve block (state why it cannot be caught
  if it cannot), `div_rat` status ignored, a layout row not gated, the real part getter at the wrong place.

**You own:** `julia/Adelefeld/` (new `src/adele.jl` and, if you separate them, `src/carrier.jl`; edits of
`src/Adelefeld.jl`, `src/load.jl`, `src/fball.jl` ONLY for the `^` refusal; `test/runtests.jl` appended;
`test/fixtures/fault.c` appended), `docs/api-ja.md` (a section "Step J-a.3" appended; the resolution line under
finding 19), `lanes/j-slice3/`. Everything else is read-only: no C or header change, no `tests/test_julia.sh`
change, the design is not edited (DESIGN-FINDING instead). No git command that changes state, no `bd`. At most
2 cores, one Julia thread, one precompile task, every command under `timeout`; the shared object from
`timeout 600 sh tests/test_exports.sh` (delete `build/` at the end); Julia starts with
`LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10` or through `julia/Adelefeld/bin/adelefeld-julia`; the depot under
your lane directory with the trailing colon and `JULIA_PKG_OFFLINE=true` (delete it and
`julia/Adelefeld/Manifest.toml` at the end). Nothing in `/tmp`. Red-green with the runs in
`lanes/j-slice3/redgreen.md`. Lines at most 116 characters; files end with a newline; plain sober prose; no
LaTeX; Julia documentation quoted from the installed `?help` with the version (as steps 1 and 2 did).

Report `lanes/j-slice3/report.md`: what was built, every command with its numbers (the gate's counts before
and after), the red and green runs, the fault table, the DESIGN-FINDINGs, what is not done, sources pending.
Aim to finish within about ninety minutes.
