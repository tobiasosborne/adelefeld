# Step J-a.1: status count, package skeleton, load gates, Rat

This step builds the first two items of `docs/api-j.md` section 9: the C prerequisite J-status and the first
step of J-a (package skeleton, load gates, statuses, the `Rat` handle). Nothing wider: no `FBall`, `Adele`,
`Place`, contexts or numerical carriers. Lane j-slice1. The design `docs/api-j.md` is the contract; where this
step departs from it or where it is silent, the place is marked DESIGN-FINDING below.

## What was built

- C: `int adf_status_count(void)` in `include/adelefeld/status.h`, header-inline and exported (conventions
  12.1), returning `ADF_STATUS_COUNT` = 11 (`docs/api-j.md:72-76`). Test: `tests/test_status.c`, test
  `the_status_count_accessor_returns_the_macro`. `tests/test_exports.sh`: 625 of 625 declared functions exported.
- `julia/Adelefeld/Project.toml`: name Adelefeld, version 0.1.0, deps Libdl only, extras and test target Test.
- `julia/Adelefeld/src/Adelefeld.jl` (module, exports), `status.jl` (statuses, results, errors), `load.jl`
  (the gates of `docs/api-j.md` section 2), `handle.jl` (aligned C heap storage, counters), `rat.jl`
  (`TextLimits`, `Rat`, constructors, text, arithmetic, equality).
- `julia/Adelefeld/bin/adelefeld-julia`: the launcher of `docs/api-j.md:85-87`.
- `julia/Adelefeld/test/runtests.jl`, `test/load_fault.jl` (run in a separate process per load-rejection case),
  `test/fixtures/fault.c` (fixture libraries that falsify one query each; built by the tests with `cc`).
- `tests/test_julia.sh`: one block that runs `Pkg.test()` through the launcher.

## Statements the tests check

S1. Load order (`load.jl`, `__init__`): words, library path and dlopen (RTLD_NOW | RTLD_LOCAL), every symbol of
`REQUIRED` from that one handle, FLINT version, layouts, statuses. A failing gate throws `AdfLoadError` with the
gate, the expected value, the observed value and the library path; `using Adelefeld` then fails with an
`InitError` around it. After a failed gate no allocation and no adf init runs: every owning allocation calls
`require_loaded()` first, and in the observed runs the module is not even bound after the failed `using`.
Tests: six fixture libraries in separate processes, each checked for the gate's words, the library path,
`INIT_CALLS=0` (the fixture counts its `adf_rat_init` calls) and no allocation afterwards:

| Fixture | Falsified query | Gate that rejects it |
|---|---|---|
| FAULT_SIZE | `adf_sizeof_rat` returns 24 | layout: expected 16 (`docs/api-m1.md:35`) |
| FAULT_ALIGN | `adf_alignof_rat` returns 16 | layout: expected 8 (`docs/api-m1.md:45`) |
| FAULT_NAME | `adf_status_str(7)` returns `DOMAIN_ERROR` | status name: expected `DOMAIN` |
| FAULT_COUNT | `adf_status_count` returns 12 | status count: expected 11 |
| FAULT_VERSION | `adf_version_check` returns 8 | FLINT version; the message shows `3.0.1` |
| FAULT_MISSING | no `adf_status_count` (the library before J-status) | symbol: names `adf_status_count` |

Two more separate processes: the real library without the GMP preload (rejected at dlopen naming
`__gmpn_modexact_1_odd`, with the exact command `LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 julia ...` and
the launcher path), and a library path that does not exist.

S2. Layout table (`LAYOUT_ROWS`): literal rows `rat` 16/8 (`docs/api-m1.md:35,45`) and `text_limits` 32/8
(`docs/api-m1.md:41,45`), size AND alignment compared; the Julia `TextLimits` is checked to be 32 bytes with
fields at 0, 8, 16, 24 (`include/adelefeld/text.h:89-90`). `Rat` storage is allocated with the row's size 16 and
alignment 8, not a newly read size.

S3. Statuses: `@enum AdfStatus::Int32` as `docs/api-j.md:161-164`; at load every `adf_status_str(i)`, i = 0..10,
equals the enum name, `adf_status_str(-1)` and `adf_status_str(11)` are `UNKNOWN`, and
`adf_status_count() == 11 == length(instances(AdfStatus))`. A raw C int outside 0..10 is `AdfAbiError` with the
raw value, never a status. `AdfResult{T}(status, value, where, details)`; `value` is `nothing` on every non-OK
status; `unwrap` and every throwing form throw `AdfError` carrying the operation, the status, a copy of the
status name and the whole result.

S4. Storage (`handle.jl`, `aligned_alloc`): `n + a - 1` bytes after checked addition; `p = (b + a - 1) & ~(a - 1)`;
the proof that `p % a == 0` and `b <= p`, `p + n <= b + n + a - 1` is the docstring of `aligned_alloc`, and both
facts are checked at run time; the base is freed, never `p`. Tests: alignments 1, 2, 8, 64, 4096 with sizes 1,
16, 33; a non-power-of-two alignment and a zero size are `ArgumentError`; an overflowing size `OverflowError`.

S5. Life cycle (`rat.jl`): `adf_rat_init` once per handle, then the finalizer; the release claims LIVE by
`@atomicreplace`, calls the cached `adf_rat_clear`, frees the base, decrements the live count; repeated `close`
and a later `finalize` are no-ops; a closed handle throws `InvalidStateException` before any C call and before
any output is allocated (the live count is unchanged by the refused calls); `copy` uses `adf_rat_set` into fresh
storage and the copy clears separately. Every ccall that passes a handle is inside `GC.@preserve` of the owners.

S6. Constructors and statuses: `Rat(n)` with n in Int64 uses `adf_rat_set_si`; otherwise, and for `Rat(n, d)`
and `Rat(::Rational)`, the pair is made to have a positive denominator with BigInt sign changes and read from
"n/d" by `adf_rat_set_str` with `TextLimits()` from `adf_text_limits_default`. `den = 0` is DOMAIN (`Rat(5, 0)`,
`Rat(0, 0)`, `Rat(1//0)`, `parse(Rat, "5/0")`; text.h:147-150, rat.h:74-79); division and inversion by zero are
NOT_UNIT (rat.h:109-116), as results and as `AdfError`. Parser statuses: PARSE for "", "1/", "1/-2", "abc",
"1\0", "é"; LIMIT for "1234" at `max_len = 3` and for "é1" at `max_len = 2` (3 bytes, 2 characters: the byte
count is `ncodeunits`); LIMIT for 10^2000 at `max_len = 100`, OK with the default limits.

S7. Arithmetic and equality: `+ - * /`, unary `-` and `inv` between Rats and with Integer and Rational scalars on
either side, by explicit Rat conversion; `==` by `adf_rat_equal`, and with Integer and Rational scalars; a Julia
Rational with zero denominator equals no Rat; `hash` is value-based and equal to the hash of the same Julia
Rational (so `isequal`, `Set` and `hash(Rat(5)) == hash(5)` agree with `==`). Float64 and Complex operands are
refused. No ambiguity (`Test.detect_ambiguities(Adelefeld)` is empty). Oracle: Julia `Rational{BigInt}` over 10
values (up to 151-bit numerators) in all 100 ordered pairs, for `+ - * /`, `==` and negation.

S8. Exactness and the session: `Rat(1)/3` prints `1/3`; three of it sum to 1; `Rat(1)/3 + Rat(1, 6)` prints
`1/2` (in the package and through the launcher in a separate process). Inputs are unchanged by every operation,
failing ones included, and the same handle may be both operands.

S9. Text: `string`, `show` and `repr` use `adf_rat_get_str`; exactly `len` bytes are copied, `s[len] = 0` is
checked, and `adf_str_free` runs in a `finally`; the round trip `parse(Rat, string(x)) == x` with equal text holds
for six values up to 300 digits.

S10. Handles under GC: 10^4 handles made and dropped in a loop, four forced `GC.gc()`, the live count returns to
its value before; finalizer errors 0.

## DESIGN-FINDINGs and decisions where the design is silent

1. The brief asks for "the five SPEC arithmetic rows that are rational (docs/SPEC.md 4.1)". SPEC 4.1 has no
   arithmetic rows; the five rows are the table of SPEC 4.3 (`docs/SPEC.md:242-248`), and they are finite-ball
   rows (radii by the gcd rules). This step checks their rational content only: the centre computed with Rats
   (8, 15, 60, 5/3, 1/3) and that it differs from the stated result's centre by an integer multiple of the stated
   radius. The rows themselves belong to the FBall step of J-a.
2. The design is silent on `//` for Rat. The session line uses `/`: `Rat(1)/3 + Rat(1, 6)` prints `1/2`.
   `//` is not defined.
3. The brief names only the `rat` layout row. The parsers of this step pass `adf_text_limits_t` by pointer, so
   its row (`docs/api-j.md:55`, 32/8) is checked too, with the Julia field offsets.
4. `docs/api-j.md:229` gives `==` for `(Rat, Rat)` only; `:232` lets the operators take Integer and Rational. The
   brief asks for `==` with them as well; it is implemented, and a Julia Rational with zero denominator (an
   infinity) compares unequal to every Rat instead of throwing.
5. `docs/api-j.md:213` writes `try_add(x, y; prec=64)` for the family. Rat arithmetic has no precision; the Rat
   methods take no `prec` keyword (passing one is a `MethodError`, not a silently ignored value). Likewise
   `try_value_text(x::Rat)` has no `digits` keyword: `adf_rat_get_str` has none.
6. `AdfResult.where` is typed `Any` and is always `nothing` in this step: no function here reports a place, and
   `Place` (`docs/api-j.md:248`) lands in J-b, which should narrow the field type to `Union{Place,Nothing}`.
7. `Base.close(::Rat)` is the public explicit release (`docs/api-j.md:107-109` speaks of an explicit close
   without naming it). `Adelefeld.diagnostics()` is the explicit diagnostic of `:111-112` (live handles,
   finalizer errors, load state, library path); it is not exported.
8. A failed gate is thrown from `__init__`; Julia wraps it in `InitError`, so a user sees
   `InitError: Adelefeld load gate "..." failed for library ...`. Observed with Julia 1.12.5: after the failed
   `using`, `Adelefeld` is not bound in `Main`.
9. The fixture approach relies on `dlsym` on a handle searching the object and then its dependencies, so that a
   fixture linked to the real library overrides one query. Observed on this machine (all six fixture cases);
   [source pending: glibc `dlsym` documentation of the search order through a handle's dependencies]. The
   fixtures must be linked with `-Wl,--no-as-needed`: they call nothing of the library, and the default
   `--as-needed` drops the dependency (observed: the first run reported `adf_version_check` missing).
10. Gate environment: with `JULIA_DEPOT_PATH=lanes/j-slice1/depot:$HOME/.julia` (no trailing colon) Julia does
    not append its bundled depots, recompiles Pkg and two dozen standard libraries, and the gate's 180 s ran
    out. With a trailing colon (`...:$HOME/.julia:`) the bundled caches are used and `Pkg.test` takes about
    40-60 s here. `Pkg.test` writes `julia/Adelefeld/Manifest.toml` (a generated file naming the Julia version);
    the lane deleted it; an ignore entry for it is for the orchestrator.
11. Julia's `dlopen` defaults on Linux are `RTLD_LAZY|RTLD_DEEPBIND|RTLD_LOCAL` (installed help, Julia 1.12.5,
    `lanes/j-slice1/help.log:28`); the package passes `RTLD_NOW | RTLD_LOCAL` explicitly, as the design says.

## Julia sources

Julia's documentation is not under `refs/`. The installed help of Julia 1.12.5 is quoted in
`lanes/j-slice1/help.log` (captured with `@doc`): `dlopen` flags and defaults (lines 1-35); `finalizer` "must not
cause a task switch" (59-94); `GC.@preserve` covers "Passing a pointer to x to ccall" and "Using resources of x
which would be cleaned up in the finalizer" (95-135); `InvalidStateException` (136-157); `@atomicreplace`
returns "(old, success::Bool)" (158-230); `Libc.free` "Only use this on memory obtained from malloc" (238-244);
`ncodeunits` (245-276); `unsafe_string(p, length)` "the string does not have to be NUL-terminated" (277-285).

# Step J-a.2: FBall and the scalar mixed arithmetic

This step builds the second item of J-a in `docs/api-j.md` section 9: the `FBall` handle, its constructors, text,
arithmetic with balls and with exact scalars, set predicates, point comparison and accessors. Nothing wider: no
`Adele`, `CAdele`, contexts, local values or dump form. Lane j-slice2. The decisions of step J-a.1 are kept (the
quotient operator `/`, no `prec` keyword where the C call has none, `AdfResult.where` typed `Any`).

## What was built

- `julia/Adelefeld/src/fball.jl` (new): `FBall <: Number` on aligned C heap storage of the checked row 48/8; init,
  finalizer, cached clear, `close`; `copy` (`adf_fball_set`), `identical`, `is_canonical`; `FBall(q::Rat)`,
  `FBall(::Integer)`, `FBall(::Rational)`, `try_fball(c, N)`, `FBall(c, N)`; `try_parse(FBall, s; limits)`,
  `parse`; `try_value_text`, `value_text`, `show`; `try_add`, `try_sub`, `try_mul`, `try_neg`, `try_div` and the
  operators `+ - *`, unary `-`, `/` (by a scalar only); `equal_set`, `overlaps`, `inside`, `compare_points`,
  `center`, `radius`; `==` on balls throws `ArgumentError`; identity-based `isequal` and `hash`.
- `src/status.jl`: `@enum PointComparison::Int32` (EQUAL=0, DIFFERENT=1, UNDECIDED=2, `status.h:35-37`),
  `to_point_comparison` and `to_bool` (a raw value outside the stated range is `AdfAbiError`).
- `src/load.jl`: the layout row `fball` 48/8 (`docs/api-m1.md:36,45`), size and alignment, and 23 new symbols in
  `REQUIRED` (the two layout queries placed after `adf_status_count`, so that the library before J-status still
  reports that symbol first). `src/Adelefeld.jl`: the include and 10 new exports.
- `test/runtests.jl`: the testset "Adelefeld step J-a.2" (18 testsets, 1015 checks); `test/fixtures/fault.c`:
  two more fixture faults, FAULT_FBALL_SIZE (`adf_sizeof_fball` returns 56) and FAULT_FBALL_ALIGN (16).

## Statements the tests check

S11. Layout and load: the row `fball` 48/8 is in `LAYOUT_ROWS`, and storage is allocated with that row's size and
alignment. A fixture library with `adf_sizeof_fball` = 56, or `adf_alignof_fball` = 16, is rejected by the layout
gate in a separate process; the message names the query, 48 or 8, the observed value and `docs/api-m1.md:36,45`;
no adf init runs. `Test.detect_ambiguities(Adelefeld)` is empty.

S12. The five rows of SPEC 4.3 (`docs/SPEC.md:244-248`), through `parse` and the operators, each `equal_set` to
the parsed expected text and printing it: `(3 mod 12) + (5 mod 18)` is `(* ; 2 mod 6)`, `(3 mod 12) * (5 mod 18)`
is `(* ; 3 mod 6)`, `12 * (5 mod 18)` is `(* ; 60 mod 216)`, `(1/3) * (5 mod 18)` is `(* ; 5/3 mod 6)`,
`(1/2 mod 8) * (2/3 mod 9)` is `(* ; 0 mod 1/6)`. Rows 3 and 4 also by the other routes: the scalar as an exact
ball (`adf_fball_mul`), the other order, a Julia Rational, and `div_rat` by 3 and by 1/12.

S13. The session line: `parse(FBall, "3 mod 12") + parse(FBall, "5 mod 18")` shows `(* ; 2 mod 6)` through
`show`, `show` with `text/plain`, `repr` and `value_text`, in the package and in a fresh process started by
`bin/adelefeld-julia` without a preload set.

S14. Radii: a fractional radius (`0 mod 1/6`, radius 1/6; `(1/4 mod 1/2) + (1/6 mod 1/4)` is `1/6 mod 1/4`), a
zero radius (`FBall(Rat(5), Rat(0))` is `(* ; 5)`; `(* ; 14/6 mod 0)` is `(* ; 7/3)`); `FBall(Rat(1)/3)` added
three times is `(* ; 1)`. `q - x` against `x - q`: `2 - (1 mod 6)` is `1 mod 6`, `(1 mod 6) - 2` is `5 mod 6`,
and the same with Rat, Integer and Rational scalars and fractional centres. `x - x` is `0 mod 12` for
`x = 3 mod 12`: not `equal_set` to 0, overlapping it, containing it, compared UNDECIDED with it.

S15. Statuses: a negative radius in `try_fball` is DOMAIN (`fball.h:133-134`) with no value and no live handle
left; a zero denominator in a text is DOMAIN in both forms (`(* ; 1/0)`, `(* ; 1 mod 1/0)`, `1/0 mod 6`,
`1 mod 6/0`; `text.h:159-160` lists DOMAIN for a zero denominator); 13 non-sentences are PARSE (among them `7/3`,
`1 mod -6`, a NUL byte and a non-ASCII byte); LIMIT for `3 mod 12` at `max_len = 7` (OK at 8), for `é mod 1` at
`max_len = 7` (8 bytes, 7 characters) and for a text one megabyte long under the default limits. Division by a
zero Rat, Integer or Rational is NOT_UNIT (`fball.h:228-232`); a scalar whose Rat conversion fails (`1//0`)
returns or throws that conversion's DOMAIN. The `try_` forms return `AdfResult{FBall}`; the operators throw
`AdfError` with the same status.

S16. Mixed-scalar matrix: four balls (`3 mod 12`, `1/2 mod 8`, `0 mod 1/6`, the point -5/7) against nine scalars
(Int 2, -3, 0, Bool true, BigInt 2^70, Rational{Int} -2/3, Rational{BigInt} 5/2^65, Rat 7/4, Rat 0), each on
both sides of `+ - *` and on the right of `/`. Oracle: Julia `Rational{BigInt}` arithmetic on the centre and
radius, `x + q = (a + q) + N Zhat`, `q - x = (q - a) + N Zhat`, `q x = q a + |q| N Zhat` (`fball.h:219`),
`x / q = a/q + N/|q| Zhat`, read back through `FBall(c, N)`; both `equal_set` and the same text.

S17. Predicates (`fball.h:234-265`): `inside` is FIRST INSIDE SECOND, `inside(2 mod 6, 0 mod 2)` true and the
reverse false; `inside(x, y)` passes `x, y` to `adf_fball_contains` in that order, whose comment reads "1 if the
set x is inside the set y" (`fball.h:248`). `equal_set`, `overlaps`, `inside`, `identical`, `is_canonical`
return `Bool`; `compare_points` returns a `PointComparison` (EQUAL, DIFFERENT, UNDECIDED in the stated cases),
never an `AdfStatus`. `center` and `radius` return fresh `Rat`s; closing one leaves the ball unchanged.

S18. Equality: `==` between balls, and between a ball and a Rat, Integer or Rational in either order, throws
`ArgumentError` naming `equal_set` and `compare_points`. `isequal` is identity (`isequal(x, copy(x))` is false),
`hash` is `objectid`-based, a `Set` of balls is a set of handles. `x < y` is a `MethodError`. No package method
exists for `FBall / FBall` or `Rat / FBall`; Julia's promotion fallback throws for them.

S19. Life cycle: inputs are unchanged by 26 operations (failing ones included); the same handle may be both
operands (`x + x`, `x * x`, `x - x` computed as independent balls). A copy is `identical` and clears separately;
repeated `close` and a later `finalize` are no-ops; a closed ball or a closed Rat operand throws
`InvalidStateException` in 24 operations before any C call, with the live count unchanged. 10^4 loop iterations
(about 4 * 10^4 handles), four forced `GC.gc()`: the live count returns, finalizer errors 0.

S20. Text: 50 balls from a fixed 64-bit generator (centres up to 30 digits, radii up to 25 digits, every fifth
radius 0) round trip: `string(parse(FBall, string(x))) == string(x)`, `equal_set`, `identical`; the bare form of
the text parses to the same text; the stored centre lies in [0, N) and differs from the given centre by an
integer multiple of N (`fball.h:163-165`). The printer's text is freed: 400 prints of a 50015-byte text change
glibc's bytes in use (`mallinfo2`, `uordblks + hblkhd`) by less than 4 MB (observed 0; with the free removed,
26.3 MB).

## DESIGN-FINDINGs and decisions where the design is silent

12. Printed frame. The C printer writes `(* ; F)` (`text.h:163-165`), and the design says the session line prints
    `(* ; 2 mod 6)` (`docs/api-j.md:313-314, 418`). `show`, `string` and `value_text` give the C text unchanged.
    The frame removal of `docs/api-j.md:237` is for the RealBall/ComplexBall carriers only, not for FBall.
13. `equal_set` and `identical` agree on every value this step can make: all are global and canonical, and the
    canonical triple of a set is unique (conventions Lemma 5.2, cited at `fball.h:3-4`). The planted fault
    "equal_set by identical" is therefore an equivalent mutant here; a test that separates them needs a local value
    (J-b), whose fields differ from the global triple of the same set.
14. The fault "GC.@preserve omitted" is not caught: no test can force a collection between the pointer read and
    the ccall, and in the observed runs the operands stay rooted by the calling frame. The preserve blocks remain a
    review obligation (`docs/api-j.md:116-118`), as the design's fault list implies.
15. The brief's fault "the `adf_str_free` skipped on a failing parse" has no object: a parser allocates no text for
    the caller. Two faults were planted in its place: the printer's `adf_str_free` removed (caught by the
    `mallinfo2` test) and the fresh output of a failing parse or constructor not cleared (caught by the live count).
    The `mallinfo2` test depends on glibc (2.33 or later; this machine has 2.39) and on FLINT allocating with
    malloc.
16. Constructors. The design lists `FBall(q::Rat)` and `FBall(c::Rat, N::Rat)` (`docs/api-j.md:203-204`). As the
    brief asks, `FBall(::Integer)` and `FBall(::Rational)` convert explicitly to a Rat first; `FBall(c, N)` and
    `try_fball` take Rats only. `try_fball` reports its failures under the operation name `:fball`.
17. Scalar products. `q * x` and `x * q` both call `adf_fball_mul_rat(z, x, q)` (scaling a set by a rational
    commutes). `+` and `-` with a Rat make the radius-zero ball first (`adf_fball_set_rat`), then call the ball
    operation (`docs/api-j.md:209`). A closed ball operand is refused before the scalar conversion allocates.
18. `==` refusal covers the scalar pairs too: `docs/api-j.md:234` says "ball `==`" without naming operand types; a
    ball compared with a Rat, Integer or Rational throws the same `ArgumentError`. One consequence: `x in [x]`
    throws, because `in` on a vector compares with `==` (installed help, Julia 1.12.5, `lanes/j-slice2/help.log:
    151-166`); `x in Set([x])` is true, since a Set uses `isequal`. `isequal` of a ball with a scalar is false, not
    an error: Julia's default `isequal` would call `==` (`help.log:26-27`).
19. Since `FBall <: Number`, Base's generic `^(x::Number, p::Integer)` (intfuncs.jl:416, observed with `which`)
    gives `x^2 == x * x`, the product of two independent balls: an enclosure, not the tight power of one unknown
    point (`(1 mod 2)^2` prints `(* ; 1 mod 2)`, while every odd square is 1 mod 8). The design names `pow(x, k;
    tight=false)` for a later step (`docs/api-j.md:296`). This step neither defines nor refuses `^`; a refusing
    method until `pow` lands is for the orchestrator to decide.
    Resolution from the orchestrator, step J-a.3: refuse `^` on FBall, Adele and CAdele until `pow` lands.
    Each type has one method for an Integer exponent, throwing ArgumentError and naming the future `pow`.
    One shared literal_pow method forwards literal powers to ^; otherwise negative literals call inv first.
20. `PointComparison` has no load-time name check: the C library has no name function for the `ADF_CMP_*` values.
    The numbers are transcribed from `status.h:35-37`; a raw value outside 0..2 is `AdfAbiError`. Likewise the int
    of every FBall predicate must be 0 or 1 (`fball.h:234-237`), else `AdfAbiError` (the Rat predicates of step
    J-a.1 still read `!= 0`).
21. The `/` of a scalar by a ball and of a ball by a ball fall to Julia's promotion fallback, which throws an
    `ErrorException` ("promotion of types ... failed", or "/ not defined for FBall"), not a `MethodError`. The tests
    assert only that the call throws and that the method found is not the package's.

## Julia sources (step J-a.2)

The installed help of Julia 1.12.5 for `isequal`, `hash`, `==`, `in`, `objectid` and `@enum` is in
`lanes/j-slice2/help.log` (captured with `@doc` and the text/plain display): `isequal` "The default implementation
of isequal calls ==" (26-27) and "isequal(x,y) must imply that hash(x) == hash(y)" (29-30); `in` "in the sense
that it is == to one of the values", Sets use `isequal` (151-166); `objectid` "Get a hash value for x based on
object identity" (231-237). The other Julia facts used are those of step J-a.1 (`lanes/j-slice1/help.log`).

# Step J-a.3: Adele, CAdele and numerical carriers

This step builds the third item of J-a in docs/api-j.md section 9. The earlier 21 DESIGN-FINDINGs are kept.
Finding 19 is resolved by the orchestrator's refusal of ^ until pow lands. No context, place type, dump
form or text classification is added. The specification and design are unchanged.

## What was built

- src/adele.jl: owning Adele/CAdele handles, literal layout rows 96/8 and 144/8, init, cached clear,
  finalizers, close, copy, identical and is_canonical. Constructors from Rat, Integer and Rational; exact
  embedding of Adele into CAdele; typed value parsers with prec and TextLimits; C printers with digits,
  checked NUL and adf_str_free in a finally; finitepart copies.
- src/carrier.jl: RealBall and ComplexBall own an initialized private Adele/CAdele with exact finite 0.
  Numerical addresses are borrowed inside preserve only. Constructors from Rat, Integer and Rational at
  prec; parsers with the fixed frame; an exact two-RealBall complex constructor; realpart and complexpart;
  copy, close, show, and numerical +, -, *, unary -. Every returned carrier checks its finite coordinate.
- Adelically typed +, -, *, unary -, and division by Rat. Rat mixed arithmetic uses C *_rat functions;
  subtraction uses exact negation and add_rat because the header offers no sub_rat. Integer and Rational
  scalars convert explicitly to Rat. Adele mixed with CAdele embeds through set_adele. Named/try forms take
  prec=64; operators use 64. Failures retain C statuses, and temporary owners are cleared on failure.
- load.jl: 42 new required symbols (41 core functions and one FLINT numerical copy from the core handle's
  dependency), for 89 total, and two layout rows, for five total. No new dlopen. Adelefeld.jl exports seven
  new names and includes both source files. fball.jl has only the ^ refusal added.
- test/runtests.jl: one appended testset, "Adelefeld step J-a.3". Four layout fault functions are appended
  to test/fixtures/fault.c. Their fixture builds also define FAULT_NAME; the layout gate rejects them
  before the unreachable status-name fault.

## Statements checked

S21. Layout and lifecycle. Both literal rows are gated for size and alignment. Four fixture libraries are
rejected before any counted Rat init. copy calls typed C set into fresh storage. Repeated close and later
finalize are no-ops. Closed accepted inputs are refused before C and before result allocation.

S22. Coordinates. finitepart returns an independent FBall. realpart uses adf_adele_get_real; complexpart
uses adf_cadele_get_complex. Each numerical getter copies into the first member of a fresh initialized
carrier whose finite coordinate remains exact 0. Projections survive closing their source. Constructors
from numerical and finite coordinates preserve both inputs and retain DOMAIN for non-finite numerical
data. CAdele(Adele(q)) preserves the real ball and finite data exactly, and its imaginary ball is exact 0.

S23. Precision. prec must be an Integer in the Int64 range before any C call or owning allocation.
Representable negative/zero precisions are passed unchanged: C clamps below 2 (adele.h:21-23). Constructors,
parsers, ring and scalar arithmetic all pass prec; tests distinguish 16/64 from 128 bits and compare
below-2 calls with 2. The finite coordinate is independent of prec. Negation is exact and has no prec
keyword. A dyadic with enough precision is exact, including a 91-bit odd integer at prec 128; 1/3 at
prec 64 has positive numerical radius and exact finite coordinate 1/3. The text reader's dyadic midpoint
and dyadic radius rule is checked with 1.5 +/- 1/1024. Adele implements only SPEC 4.4's tight policy:
(1 mod 2) times 1/2 has finite part 1/2 mod 1, and never the excluding fixed radius 2.

S24. Arithmetic. All five SPEC 4.3 rows run through Adele at prec 64. Their finite coordinates are equal_set
to the specified balls; actual numerical coordinates contain the exact rational result. x-x has finite
radius 12 for x=(3 ; 3 mod 12), while its real coordinate contains 0. Rat on either side has the specified
sign and exact finite data. Both embedding orders run through set_adele. Division by exact 0 returns
NOT_UNIT and value=nothing, or throws AdfError with NOT_UNIT; its output is cleared immediately.
Inputs stay identical after accepted and failing operations.

S25. Numerical arithmetic and text. Fifty random real and complex pairs run through +, -, *, unary -
against BigFloat at 256 bits. Their dyadic coordinates and reference results are exactly representable at
that precision. Test-only FLINT queries check containment in the actual stored ball, rather than its
potentially wider printed text. Every result has exact finite 0. The frame inverse is checked for both
carriers. Thirty real and thirty complex adele value round trips preserve finite sets and enclose the
original numerical balls; no fixed-point decimal text promise is made.

S26. Refusals. Unsupported adelic pairs with FBall, Float64, BigFloat and Complex raise ArgumentError on
both sides of +, -, *. Division by an adelic value or finite ball raises ArgumentError. Carriers have
only same-type arithmetic and are not Number subtypes; mixing them with FBall, adeles or scalars raises
MethodError. Adelically typed ==, including scalar comparisons, raises ArgumentError naming the finite
set predicates. isequal/hash use ownership identity. ^ with an Integer exponent is refused on FBall,
Adele and CAdele, with ArgumentError naming future pow. Test.detect_ambiguities(Adelefeld) is empty.

S27. GC. A fresh Julia subprocess has 0 live owners initially. For each of Adele, CAdele, RealBall and
ComplexBall it runs 10000 iterations making an input, copy, sum and negative, then four collections.
Every type ends with 0 live handles and 0 finalizer errors. The wrapper has no additional finalizer;
its initialized carrier clears exactly once. This is a bounded ownership smoke test, not a FLINT leak proof.
The final gate prints all four LIVE=0 ERRORS=0 child counters into lanes/j-slice3/limits-green2.log.

The gate, red-green runs, planted faults and session are recorded in lanes/j-slice3/report.md and
redgreen.md. The earlier gate count is 1919. The first green J-a.3 count is 1557, for 3476 total.
Further closed-input, NULL-printer and negative-literal checks bring J-a.3 to 1645, for 3564 total.
The default-limit counting fixture brings the final J-a.3 count to 1649, for 3568 total.
The parent testset freezes automatic GC for exact live-count comparisons and restores it in a finally;
the separate GC acceptance process runs with automatic GC enabled.

## DESIGN-FINDINGs

22. Accessor names. The design names only finitepart. This step names realpart and complexpart after
    adf_adele_get_real (adele.h:119-121) and adf_cadele_get_complex (:200). The convenience real getter exists,
    so no Place type or adf_place_inf wrapper is needed.

23. Real text grammar. The brief gives "1.5 +/- 2^-10", but conventions 9.1-9.2 admits decimal udec for
    the radius, not powers. C returns PARSE for that example. This step uses "1.5 +/- 0.0009765625".
    The package delegates the grammar to the C reader.

24. FBall injection. The brief's session calls Adele(f::FBall; prec). Neither the design nor the header
    gives a one-FBall adele constructor. This explicit constructor copies f with numerical exact 0,
    through set_arb_fball; it does not interpret the stored finite centre as a real value. prec is
    validated, and cannot affect exact numerical 0. FBall mixed arithmetic is still refused.

25. Two-ball complex copy. No adf constructor takes two arb inputs. ComplexBall(re, im) calls documented
    FLINT acb_set_arb_arb (refs/src/flint-3.0.1/acb.rst:124-126), resolved and cached from the same
    libadelefeld handle's dependency. Both source owners and the whole output carrier are preserved.
    This uses no raw FLINT init/clear and no additional library. Tests compare both stored balls
    identically at 192 bits; decimal reframing could not provide that copy contract.

26. Mixed pair refusals. Explicit unsupported Number-family operator methods raise ArgumentError, since
    Base's promotion fallback can raise ErrorException (finding 21). Unsupported try_ pairs have no
    applicable methods and raise MethodError. Carriers are separate types with same-type methods only;
    no Number subtype or scalar promotion is supplied for them in this step.

27. Precision validation. The header allows every slong, clamping below 2. This step checks Julia Integer
    and Int64 representability, without inventing a positive minimum or another upper bound. The C
    printer's digits precondition is checked as 1..1000000 (text.h:54-57). No keyword is silently ignored.

28. Carrier text. Exactly six ASCII characters are removed: one leading "(" and the five-character
    suffix " ; 0)". Parsing adds exactly those characters back. Internal parentheses of complex text
    are retained. TextLimits bounds the whole framed C input, including those six bytes.

29. ^ resolution. Finding 19 now has the orchestrator's resolution beside its original statement.
    Each of FBall, Adele and CAdele has one refusing method for an Integer exponent. Base.literal_pow
    uses ^ by default (installed Julia 1.12.5 help in lanes/j-slice3/help.log), so x^2 is refused too.

30. Numerical pointer fault. A planted arb address read before preserve is not caught by the bounded
    suite. The owner stays rooted in the observed calling frame, and there is no forced collection at
    that pointer read. Test success cannot establish the preserve discipline. Production code makes
    early lifecycle checks without producing pointers, and borrows every numerical address inside
    preserve. This remains a review obligation, as in finding 14.

31. Negative literal powers. Installed Julia 1.12.5 rewrites negative literal exponents through inv
    before calling ^ (help.log:272-279). The initial one-method-per-type refusal therefore gave the
    wrong error for x^-1 and x^-2. A separate red-green test had 9 pass and 6 fail, then 15/15 after
    one shared literal_pow method forwarded all three ball families to their existing ^ refusals.
    There remains exactly one ^ method per type; zero, positive and negative literals name future pow.

32. Default text limits and precision. A keyword default limits=TextLimits() invokes C before the parser
    body validates prec. A counting fixture observed 24 default C calls on 24 invalid-precision inputs.
    Parsers now use a nothing sentinel and obtain C defaults only after the precision guard. Explicit
    limits=nothing also selects the C defaults. A dedicated subprocess checks 24 ArgumentErrors,
    0 default-limit calls and 0 live handles. C defaults are still queried, not copied into production.

## Julia sources

Installed Julia 1.12.5 help is captured with @doc and text/plain display in lanes/j-slice3/help.log.
Earlier lifetime, atomic, string and identity documentation remains in the first two lanes.
The preserve help includes "Passing a pointer to x to ccall" (14) and resources cleaned by a finalizer (15).
Finalizer callbacks "must not cause a task switch" (56). setprecision changes arithmetic precision for the
duration of its function (90-101). The ^ help gives Base.literal_pow's default fallback to ^ (272-275).
No additional package was installed. Numerical copy and containment conventions are quoted from
refs/src/flint-3.0.1/{arb,acb}.rst at the lines cited in the source and tests.

Findings against the specification: none. Sources pending: none new; earlier pending glibc dlsym
search-order documentation still applies to fixture libraries and dependency symbol lookup.
