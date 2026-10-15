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

# Step J-a.4: text classification, dump forms and reconstruction

This step finishes the fourth item of J-a in docs/api-j.md section 9. Earlier decisions are kept,
including 5 (no digits keyword for exact printers), 23, 27, 28 and 32. No context, scaled/cap or local
value surface is added. The specification and design are unchanged.

## What was built

- TextKind::Int32 transcribes the thirteen constants of text.h:122-135, stripping only ADF_. The
  literal 4/4 row is gated against adf_sizeof_text_kind and adf_alignof_text_kind. Julia's enum layout
  is checked too. No name function exists; names are compared against header literals in the tests.
- try_readvalue/readvalue call adf_text_classify first. The kind is read only on OK. Supported kinds
  dispatch to Rat, FBall, Adele or CAdele typed C readers, retaining their statuses and typed result.
  Other kinds return UNSUPPORTED with details.kind. Classifier failures have no kind in details.
- The existing numerical printers accept digits in 1..1000000 before C. show reads :adf_digits from
  IOContext, default 20, for Adele, CAdele, RealBall and ComplexBall. The numerical frame inverse of
  finding 28 is unchanged. Rat and FBall still print exactly, with no digits keyword.
- try_dump_text/dump_text and try_load_dump/load_dump wrap the four typed C pairs. Strings are copied
  at their byte length and freed in a finally. Loaders use ctx=NULL with caller TextLimits; they
  construct no context. Failure temporaries are closed. No bindings or precision keyword is exposed.
- try_reconstruct/reconstruct wrap adf_fball_reconstruct(x,lo,hi) and adf_adele_reconstruct(x), returning
  a fresh Rat only on OK. Closed accepted inputs are checked before allocation. All non-OK results
  have value=nothing, where=nothing and empty details, because recon.h writes no auxiliary output.
- load.jl adds thirteen required symbols to the earlier 89, for 102 total, and one row to the earlier
  five, for six total. Three source files are included: text.jl, dump.jl and recon.jl.
- One testset, "Adelefeld step J-a.4", is appended. The C test fixture adds two enum layout faults,
  counted calls and defensive dump failures. Production C and tests/test_julia.sh are unchanged.

## Statements checked

S28. Classification. One text of each of the thirteen kinds follows its header start symbol. All four
supported kinds return the corresponding type and the same value as the typed parser. Nine unsupported
kinds return UNSUPPORTED and their TextKind, even when later semantic/literal checks would fail.
Length precedes bytes, bytes/grammar precede typed literal limits, and semantic zero denominators remain
DOMAIN. The five SPEC 4.3 result texts dispatch to FBall. Precision is forwarded for archimedean readers;
invalid precision precedes C defaults and classification, and below 2 follows the C clamp.

S29. Printing. The four numerical types use digits through value_text, show and text/plain show in an
IOContext. Invalid digits produce ArgumentError before C, measured by a counted printer. End points 1
and 1000000 are accepted. Rat/FBall retain their exact, keyword-free printers and ignore :adf_digits.

S30. Dumps. Thirty fixed-generator values of each of Rat, FBall, Adele and CAdele load identically to
their originals and dump to the same bytes. Inputs stay identical to their copies. Five handwritten
oracles are checked against tools/adf: rational 7/3; global 2 mod 6; (1 ; 0); the complex fixture with
imaginary 0.5; and (0.5 ; 0). The first four are tests/driver/13_dump.cmd:28-33; the fifth reverses its
exact load at :41. Each rational field is the canonical numerator/denominator or A/H/d. The dyadic
midpoints 1 and 0.5 have mantissa/exponent (1,0) and (1,-1), with zero radii (0,0).
The serialization convention is on disk: refs/src/flint-3.0.1/arb.rst:288-293.

S31. Load refusal. Every type refuses its dump above max_len with LIMIT and malformed dump syntax with
PARSE. Unsupported version/field is UNSUPPORTED. A non-canonical body of each requested type is DOMAIN.
The same accepted dump loads at its exact byte limit. Local finite dumps, including nested finite
coordinates, get DOMAIN from the NULL context; an earlier count limit remains LIMIT. Two counted
layout fault libraries are rejected before Rat init. Dump finally paths free every returned string,
including non-NULL zero-length and non-NUL-terminated fixture strings, as well as NULL.

S32. Reconstruction. For finite balls, singleton closed end points, fractional centres, negative
progression points, zero radius, empty intervals, none and several follow the exact progression rule
of SPEC 9.2. The driver's full-adele fixtures return 2, 1 and NO_SOLUTION. Adele LIMIT is asserted on
positive and negative extreme midpoint and radius exponents, loaded exactly from tiny dumps. Output
is absent on every failure, and no failed temporary remains live. The release-only non-finite courtesy
returns DOMAIN; it is not a claim for canonical inputs or a build with ADF_CHECK_INVARIANTS.
NOT_DETERMINED is not possible in either header function. No limited search API is added.

S33. Lifetime. Closed dump, printer and reconstruction inputs refuse without result allocation. All
accepted and failing reconstruction inputs stay identical to copies. A fresh automatic-GC subprocess
makes 10000 dump/load round trips across the four types. Every pair is identical; four forced GCs leave
0 live owners and 0 finalizer errors. This is an ownership smoke test, not a FLINT leak proof.

S34. Oracle separation. Adele(1/3;prec=192) and Adele(1/3;prec=256) have equal value text at 5 and 20
digits, different stored balls, and different dump bytes. The test's round-trip identity oracle calls
identical and is checked to reject this pair. Replacing it with text comparison is a caught test mutant.

## DESIGN-FINDINGs

33. Enum names. There is no adf_text_kind_str in text.h or the export inventory. TextKind keeps the
    header's TEXT_ prefix and numbers 0..12, with ADF_ removed like AdfStatus. No invented C name gate.

34. Classified results. Supported dispatch returns AdfResult of the typed value, with details.kind on
    success and on a typed-parser refusal. Unsupported/classifier failures use AdfResult{AdfValue},
    where AdfValue is the union of the four global value types. A classifier failure has empty details.
    The untyped prec keyword is validated even for exact or unsupported syntax before any C call,
    following finding 32. Typed Rat/FBall parsers remain precision-free.

35. Exact printing. Finding 5 and the FBall step's no-digits decision remain in force. The common
    digits/IOContext surface applies where the C printer has the argument (conventions 8.1;
    text.h:54-57,152-154,163-165,178-183,216-217). Exact printers have no numerical coordinate to round.
    An initial wider Julia keyword was removed before the final gate to keep these earlier decisions.

36. Failure reconstruction output. recon.h:24-29 certifies no output on non-OK. There is therefore no
    candidate, certificate, count or failing place to put in details. NOT_DETERMINED belongs only to
    partial data, expressly excluded by recon.h:25-27; FBall reconstruction has no LIMIT search case.
    LIMIT belongs to Adele's exponent guard. DOMAIN is only the release-build courtesy described there.

37. The requested example. In the full ball 146 + 1000 Zhat, the rational candidates are 146+1000k.
    For [0,50], ceil(-146/1000)=0 and floor((50-146)/1000)=-1, so there is none. For [100,200], both
    bounds are 0, so the unique result is 146. 22/7 differs from 146 by -1000/7, whose quotient by 1000
    is -1/7, not an integer. The UX sentence docs/ux/README.md:7 is a partial-residue example:
    7*146=1022=22 mod 1000. It is not a full-ball reconstruction example. SPEC 9.2 distinguishes them.
    The lane session uses [100,200] and prints 146; no concrete positive full-ball example is in SPEC 9.2.

38. Dumper refusal. The brief attributes NULL or zero-length dump refusal to the driver. In fact,
    tools/adf/adf.c:694-697 writes the dumper's result unconditionally, relying on dump.h:49-52
    ("never fails"). The driver's NULL->LIMIT check at :632-633 belongs to value printing; it has no
    zero-length test there. Julia implements both defensive refusals from the design's string rule
    (api-j.md:186-188), without claiming that C dumpers normally refuse or that the driver checks length.

39. Identity and contexts. A global FBall and the finite coordinate of a global Adele/CAdele have no
    context occurrence (dump.h:38-47,116,144-162). Their dumps need no context, at any size admitted by
    the load limits. A local occurrence with NULL is DOMAIN (dump.h:41-42), unlike the driver's earlier
    UNSUPPORTED prefilter. The N-D25 identity qualification is retained. The D1 function-array caps of
    api-4.md:104-108/dump.h:288-303 do not apply to these four types; caller limits still do.

## Julia sources and scope

Installed Julia 1.12.5 help is captured in lanes/j-slice4/help.log with @doc and text/plain display.
IOContext says "use get(io, key, default) to retrieve the most recent value" (:57-59).
The @enum help documents an Enum subtype and converted member values (:4-8,33-38).
Earlier lifetime/pointer/string help remains applicable. No Julia package was installed.

Gate history, commands, counts, planted faults and cleanup are in lanes/j-slice4/report.md and redgreen.md.
The final solo gate passes 4881/4881: J-a.1 904, J-a.2 1015, J-a.3 1649, J-a.4 1313. Baseline: 3568.
Its counted-call subprocess passes another 43 checks, not added to the outer count. Ten planted faults
are caught. One prior final attempt timed out without a completed summary; the unchanged solo rerun passes.
No contexts, bindings, scaled/cap, local constructors, partial reconstruction, ideles/classes/characters,
functions, two-thread support, Nemo or long differential fuzzing is built here.
Findings against the specification: none. The UX example and the brief's status/driver claims differ from
the headers, not from SPEC. No new mathematical source is pending. The earlier glibc dlsym dependency
search-order source, linked FLINT threadsafe check, and FLINT malloc-failure documentation remain pending.

# Step J-b.1: modulus contexts, leases and the local backend

This step builds the first item of J-b in docs/api-j.md section 9, "contexts/leases/local backend", and
nothing wider: no Scaled, cap, Place, LBall, SBall, context dumps or bindings. Lane j-slice5. The earlier 39
DESIGN-FINDINGs are kept. The specification, the design and the C library are unchanged.

## What was built

- src/context.jl (new): the private CtxState (C pointer, atomic token count) and the ModCtx handle (the
  state and an atomic owner flag). Token operations: _acquire! (compare-and-swap, never from 0); _drop!
  (the decrement that sees the old count 1 calls adf_modctx_free); the ModCtx finalizer _release_owner!;
  _lease! and _drop_roots! for values. The file header writes out the context proof of
  docs/api-j.md:126-141 step by step.
- src/modctx.jl (new): try_modctx/ModCtx with exactly one of five construction families; modulus, nblocks,
  blocks; show; try_to_local/to_local, try_enclose_local/enclose_local, try_to_global/to_global, is_local,
  context; private context_pointer.
- src/fball.jl, src/adele.jl, src/carrier.jl: FBall, Adele and CAdele get a roots field. Every operation
  whose output can borrow an input's context leases the union of its inputs' roots before the C call. Every
  release drops the tokens after the clear and after the free of the storage.
- src/load.jl: 14 new required symbols (modctx.h:57-112, 145-174), 116 in all. No new layout row: the
  context is opaque (docs/api-j.md:63). src/handle.jl: diagnostics() adds contexts and context_frees.
  src/Adelefeld.jl: two includes and 14 exports.
- test/runtests.jl: one appended testset "Adelefeld step J-b.1" (10 testsets, 488 checks).

## Statements the tests check

S35. Families (modctx.h:57-87). try_modctx takes exactly one of blocks=[q...], prime_powers=[p => e, ...],
modulus=K, factorial=n, primorial_power=(n, e). None, or two or more, is ArgumentError before C. Arrays are
copied into Julia-owned Vector{UInt64} and preserved across the call. A later change of the caller's array
does not change the context. Each word must fit a C ulong. ArgumentError, with no context made, for: -1,
2^64, 1.5, a negative exponent, a tuple instead of a pair, factorial -1, primorial (5, -1), modulus 1.0.
The pointer slot starts NULL and is read only on OK. Every result has details = (family = <keyword>,).

S36. Valid input. For six block lists, among them [2^64-1, 2^61-1] and ten primes, modulus(ctx) equals the
BigInt product of the blocks; blocks(ctx) returns them in order and nblocks gives their number.
prime_powers keeps the order ([3 => 2, 2 => 3] gives [9, 8]); [2 => 63] gives 2^63. modulus=72 and 2^64-1
give one block; 1, 2^64 and 10^30 give none, and modulus(ctx) equals K. factorial 5 gives [8, 3, 5]; 65
gives 65! (v_2 = 63); 0 and 1 give none. primorial (5, 2) gives [4, 9, 25]; (2, 63) gives 2^63; (1, 3) and
(5, 0) give none. blocks=[] is OK: modulus 1 without blocks (modctx.h:57-58). modulus(ctx) is a Rat
(finding 44). show prints ModCtx(modulus=72, blocks=[8, 9]).

S37. Refusals.
DOMAIN: blocks [1], [0], [6, 4], [8, 8], [8, 9, 1], [35, 77]; prime powers [4 => 1], [2 => 0],
[2 => 1, 2 => 2], [1 => 1], [4 => 64] and [2 => 64, 4 => 1] (DOMAIN before UNSUPPORTED, modctx.h:31-32);
modulus 0, -5, -2^70.
UNSUPPORTED: prime powers [2 => 64], [3 => 41]; factorial 66, 10^6, 2^64-1; primorial (2, 64), (3, 41),
(821647, 1); 65537 distinct primes as blocks and as prime powers. That is more than
ADF_MODCTX_MAX_BLOCKS = 65536 (modctx.h:23-28, 48); the 65537th prime is 821647.
In every case value is nothing and no context is made. ModCtx(blocks=[1]) throws AdfError with DOMAIN and
the same details.

S38. Lease basics, checked before any later use. A new context has 1 token. One conversion gives 2. A
failing conversion leaves 2. finalize(ctx) gives 1, with the context still live and no free. The value
then prints. Closing it gives 0 tokens and exactly one free. The value is used only while its context has
not been freed, so that a broken lease is reported by these assertions, not by a use after free (finding
51).

S39. Local conversion (modctx.h:145-169). The context of blocks [16, 27, 5] has K = 2160, and all nine
inexact balls of SPEC 4.3 are local in it. For each, try_to_local gives:
- OK with details (lost = false,);
- is_local, context(y) === ctx, and the value's C context pointer equal to the context's;
- is_canonical; equal_set to x but not identical, since the backend differs (fball.h:102-106);
- value_text(y) equal to the global text of x, for example "(* ; 3 mod 12)": the value form does not
  record the backend (conventions 9.8 A11; text.h:37-38).
to_global(to_local(x)) is global and identical to x; each input is identical to its copy afterwards. On
these balls the enclosing variant gives lost false and a value identical to the exact conversion.
The five rows computed on local operands equal_set the stated results and print them; their to_global is
identical to the parsed result. The sum of two values of one context is local in it (fball.h:196-197).
The exact values 12, 1/3 and 0 are DOMAIN in both conversions, with no value and empty details; the
throwing forms throw AdfError.

S40. lost (modctx.h:155-164). The context of blocks [8, 27, 5] has K = 1080. 1/2 mod 8 is not local in it
(K/R = 135, c K/R = 135/2), so to_local is DOMAIN. enclose_local is OK with details (lost = true,), a Bool.
With d0 = lcm(135, 2) = 270 the result is (135 + 1080 Zhat)/270 = 1/2 mod 4. That value is local,
contains x, is not equal_set to it, prints "(* ; 1/2 mod 4)", and its to_global is identical to
parse(FBall, "1/2 mod 4"). Four contexts without blocks (blocks=[], modulus 1, modulus 2^64, factorial 1)
give UNSUPPORTED in both conversions (modctx.h:151, 160), with empty details. modulus 2^64-1 gives a
one-block context in which 1 mod 3 is local.

S41. Same modulus, distinct contexts (SPEC 184-214, 285-298; conventions 4.6, last paragraph). Two
contexts of blocks [8, 9] and one of prime powers [2 => 3, 3 => 2] have equal moduli and blocks and three
different C pointers: there is no interning. Make 5 mod 72 local in the first two:
- context(a) === c1 and context(b) === c2;
- their C pointers (adf_fball_context, modctx.h:171-174) differ;
- identical(a, b) is false and equal_set(a, b) is true.
a + b falls back to global (fball.h:17-20), has no context, and is identical to x + x. a + a2, both in c1,
is local in c1, and its to_global is identical to the global sum.

S42. Leases (docs/api-j.md:126-141). to_local adds one token (roots(y) == [ctx]). copy adds one, and close
gives it back. y + y adds ONE token: one per distinct state. These all root ctx: negation; subtraction;
multiplication; multiplication by a Rat on both sides; addition of a Rat and of an Integer; division by a
Rat. to_global keeps a conservative root until it is cleared. These hold ctx in their roots too:
Adele(RealBall(1), y), its finitepart (identical to y), CAdele of it, Adele sums, copy, and
CAdele(ComplexBall, y). The failing calls leave the token count unchanged: division by 0 of an FBall and of
an Adele (NOT_UNIT); to_local of an exact ball and of a non-local ball (DOMAIN). Closed inputs to
try_to_local, try_enclose_local, to_global and is_local throw InvalidStateException before C, with no
token and no allocation. After every value is closed the count is back to 1, with 0 finalizer errors.

S43. Both finalizer orders, explicitly.
Order 1: finalize(ctx) while a local value lives drops only the owner token: count 1, the context still
live, 0 frees. The value still prints, adds and converts back. The finalized ctx is refused, with
InvalidStateException, as an argument of both conversions and of modulus, blocks and nblocks; it shows
"ModCtx(finalized)". A second finalize is a no-op. Closing the last value (count 0) frees the context
exactly once; further close or finalize of either is a no-op.
Order 2: the value closed first (count 1), then finalize(ctx): one free. Two contexts, two frees.

S44. GC, in a separate process with automatic GC. 10^4 iterations each make a context, a local value and
its global copy, which is identical to the input. In one third of them the context is finalized first and
the value left to the collector. In one third the value is closed and then the context finalized. In the
rest both are left to the collector. After four GC.gc(): 0 live handles, 0 live contexts, 10000 frees, 0
finalizer errors. A value whose only reference to its context is its root survives two forced collections,
with the context live. After it is dropped and four more collections run: 0 live, 0 contexts, 10001 frees,
0 errors.

S45. Dumps and the session. The dump of a local value differs from the dump of the global value with the
same set: the dump records the backend (conventions 9.8 A11). The NULL-context loader refuses it with
DOMAIN (dump.h:41-42) and constructs no context. Test.detect_ambiguities(Adelefeld) is empty. The session
line prints (* ; 5 mod 72).

## DESIGN-FINDINGs

40. Block count before DOMAIN. modctx.h:31-32 says "The order of the checks is: ADF_DOMAIN first, then
    ADF_UNSUPPORTED". src/modctx.c:215-218 decides k > ADF_MODCTX_MAX_BLOCKS before it reads the blocks
    ("a refused call must not touch an array that is shorter than k"), and modctx.h:23-26 says the bound is
    decided from k before a table is built. So 65537 blocks whose first block is 1 give UNSUPPORTED, not
    DOMAIN (lanes/j-slice5/probe.log line 4); prime powers behave the same (src/modctx.c:252-255). The two
    header sentences disagree for that input, and the code's choice is the safer one. The tests assert
    only the unambiguous cases (65537 valid primes). This is a header wording question for the C owner,
    not a binding fault.

41. Empty array. The brief lists "an empty array" among the refusals. modctx.h:57-58 gives k = 0 the
    context of modulus 1 without blocks, with status OK, so the binding does not refuse it. The refusal
    comes at the conversion, UNSUPPORTED (modctx.h:151, 160), tested with four such contexts.

42. A block above 2^64. The C argument is a ulong (modctx.h:62), so such a block cannot reach C. The
    binding gives ArgumentError before C (docs/api-j.md:154-155), as for every other word. The header's way
    to name a modulus K >= 2^64 is modulus=K: OK, without blocks.

43. LIMIT. The brief names LIMIT cases, but modctx.h:57-87 gives no constructor a LIMIT status. Only
    adf_modctx_new_from_dump (:93) has one, and it is not wrapped in this step. The only LIMIT path of
    try_modctx is the Rat conversion of a modulus whose decimal text exceeds the default text limits. It is
    returned as that status, with details.family. That path is not tested.

44. modulus(ctx) is a Rat. adf_modctx_get_modulus writes an fmpz initialized by the caller
    (modctx.h:105-106). The binding writes it into the numerator of a fresh Rat whose denominator stays 1,
    as docs/api-j.md:148-149 prescribes for fmpz data (refs/src/flint-3.0.1/fmpq.rst:9-11, 21-26, 39-41).
    K >= 1 keeps that value canonical. A BigInt would need either a reinterpretation of GMP storage, which
    is forbidden (docs/api-j.md:196), or a text round trip. modulus=K passes the numerator of Rat(K) the
    same way. nblocks and blocks wrap the two other accessors.

45. Argument forms. The design names the five keywords only. prime_powers is a vector of pairs p => e;
    primorial_power is a tuple (n, e); modulus and factorial are Integers. details is (family = keyword,)
    on every status. The C functions report nothing beyond their status, so there is no stage field.

46. Leases on the earlier operations. fball.h:16-20 keeps a result local when all local inputs share one
    context pointer, and adele.h:85, 130-131 copy the context pointer of the finite coordinate. So a local
    value can now reach these operations: copy, the ring operations, negation, the scalar operations,
    division by a Rat, finitepart, CAdele(::Adele), Adele(::RealBall, ::FBall), CAdele(::ComplexBall,
    ::FBall), and the Adele and CAdele arithmetic. Each of them now leases the union of its inputs' roots
    (context proof step 3). Without that, copy(to_local(x, ctx)) would borrow a context it holds no token
    on. Constructors from Rat, from text and from NULL-context dumps take no lease, and neither do the
    numerical getters: their finite coordinate is global (text.h:27-28; dump.h:41-42; adele.h:119-121
    writes only the arb).

47. Conservative roots. Every result keeps the union of its inputs' roots until it is cleared, also when
    it fell back to global; to_global keeps the roots of its input (docs/api-j.md:133-134). For an FBall,
    adf_fball_context would be a verified accessor for pruning, but the design defers pruning (:140), and
    this step does not prune.

48. lost of the exact conversion. docs/api-j.md:258 puts lost in details only on OK. The exact conversion
    has no lost output in C. On OK it reports (lost = false,), since by definition it does not change the
    set (modctx.h:145-148). The enclosing variant passes an invalid sentinel -1 and reads it only on OK. A
    value other than 0 or 1 is AdfAbiError, after the fresh output is released.

49. Membership predicate. The C surface gives adf_fball_context (modctx.h:171-174) and
    adf_fball_identical (fball.h:102-106, "the same context pointer"). context(x) returns the ModCtx among
    x's roots whose pointer equals adf_fball_context(x), and nothing for a global value. It throws if no
    root matches, which would mean a broken lease.

50. Early release. There is no public close of ModCtx (docs/api-j.md:137). Base.finalize(ctx) runs the
    registered finalizer at once, and it is the deliberate early release of the owner token. Repeating it
    is a no-op. Afterwards ctx is refused as an argument, while existing values stay valid. try_to_global
    is added beside to_global for the try_ family (statusless, OK synthesized).

51. A broken lease shows up as a crash, not as a failed test. If a lease fault lets the context be freed
    early, the next use of a value is a use after free in C: FLINT aborts or segfaults (fmpz_mul in
    adf_modctx_recombine, src/modctx.c:595-598; adf_modctx_free at :427). Two consequences for the tests.
    (a) The token and free counters are asserted before the first later use of a value (S38), and the
    value is used only while its context has not been freed. Then the fault fails assertions before it
    can crash. (b) Julia's libuv stdout and stderr queue writes, and an abort discards them, also when the
    output goes to a regular file (lanes/j-slice5 abort probe: neither a println nor a Test Failed line
    survives). The fault harness lanes/j-slice5/jb1_only.jl therefore rebinds Base.stdout and
    Base.stderr to synchronous raw descriptors (Base.Filesystem.File(RawFD(1)), RawFD(2)). The package's
    own tests are unchanged. Planted faults must not leave cores on the shared disk. The core pattern
    pipes to systemd-coredump, which ignores RLIMIT_CORE, so the harness sets
    /proc/self/coredump_filter to 0, which the Julia children inherit. One earlier run filled the
    disk, with a 1.4 GB core per crash.

## Gate, session and planted faults

Commands are run from the worktree root. The gate is lanes/j-slice5/gate.sh: Pkg.test, one thread,
offline, the depot with the trailing colon.
- Before (lanes/j-slice5/before.log): 4881/4881 (J-a.1 904, J-a.2 1015, J-a.3 1649, J-a.4 1313), plus
  the counted-call subprocess's 43.
- Red (red.log): the J-b.1 testset gives 9 errors and 0 passes (FieldError: diagnostics() has no
  contexts field). The other four sections are unchanged.
- Green (gate-final.log): 5369/5369, of which J-b.1 is 488, plus the 43 counted calls. Pkg.test says
  "Adelefeld tests passed".
- Session (session.jl, session.log): ctx = ModCtx(blocks=[8, 9]); to_local(FBall(Rat(5), Rat(72)), ctx)
  prints (* ; 5 mod 72). ctx shows ModCtx(modulus=72, blocks=[8, 9]). is_local is true, context(y) ===
  ctx, modulus 72, and to_global(y) is identical to the global input.

Sixteen faults were planted, each in a fresh copy of the package (lanes/j-slice5/faults.py). The J-b.1
section was run alone on each (jb1_only.jl); the rows are in fault-summary-final.log. 14 are caught by
failed assertions:
- F2, value tokens never dropped: 30 failed.
- F5, duplicate tokens per state: 3.
- F6, lost written on failure: 7.
- F9, two families accepted: 6.
- F10, copy without a lease: 1 failed, 1 errored.
- F11, an adele without a lease: 6 failed, 1 errored.
- F13, a finalized ctx accepted: 14.
- F14, a block wrapped mod 2^64: 8.
- F15, finitepart without a lease: 1 failed, 1 errored.
- F16: 8.
- F1, no token taken: 8 failed, 7 errored, then a segfault.
- F3, the owner frees directly: 6 failed, then an abort.
- F4, off by one, frees at count 2: 2 failed, then an abort.
- F7, the conversion takes no lease on ctx: 34 failed, 14 errored, then an abort.
For F1, F3 and F4 the first failure is in the lease-basics testset (S38).

Two are not caught, and neither can be seen in this build:
- F8 returns a context on a refusal. Every write of the output pointer in src/modctx.c (:210, 226, 247,
  277, 294, 298, 345, 359, 378, 400) is on an ADF_OK path, so the slot stays NULL on a refusal.
- F12 drops a value's tokens before its C clear. adf_fball_clear (src/fball.c:249-259) reads the context
  only through ADF_INV_RELEASE, and that is ((void) 0) without -DADF_CHECK_INVARIANTS
  (src/invariants.h:107). An INV=1 build under ASan would be the oracle. The order of context proof
  step 4 is kept in the code anyway.

Not done: Scaled, cap, Place, LBall, SBall, context dumps, pruning of conservative roots (finding 47),
two-thread support and long differential fuzzing. Findings against the specification: none. Findings 40
to 43 are against the brief or the header wording. No source is pending for this step.
