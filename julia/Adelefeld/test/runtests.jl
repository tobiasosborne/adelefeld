# julia/Adelefeld/test/runtests.jl: tests of step 1 of J-a (docs/api-j.md section 9): the load gates of
# section 2, the statuses of section 4 and the Rat handle of sections 3 and 4. Acceptance lines of
# section 8 for this step: Rat 1/3 stays exact, den = 0 is DOMAIN against division by zero NOT_UNIT,
# inputs unchanged, copies clear separately, load rejection in separate processes.
# Run from the repository root (docs/api-j.md section 9, with the preload of tests/julia/README.md):
#     env JULIA_NUM_THREADS=1 JULIA_NUM_PRECOMPILE_TASKS=1 \
#         LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 \
#         julia --startup-file=no --project=julia/Adelefeld -e 'using Pkg; Pkg.test()'
using Test
using Adelefeld
using Adelefeld: OK, DOMAIN, NOT_UNIT, PARSE, LIMIT

const PKG = normpath(joinpath(@__DIR__, ".."))

# The oracle: Julia's own Rational{BigInt}, printed in the template q(x) of conventions 9.4
# ("n" or "n/d" in lowest terms, "-" only before a negative numerator).
oracle_text(q::Rational) = denominator(q) == 1 ? string(numerator(q)) :
                           string(numerator(q), "/", denominator(q))

@testset "Adelefeld step J-a.1" begin

@testset "load gates passed" begin
    d = Adelefeld.diagnostics()
    @test d.state === :ok
    @test isfile(d.library)
    @test isabspath(d.library)
    # docs/api-j.md:70-76: names, UNKNOWN at -1 and 11, count 11 == length(instances(AdfStatus)).
    @test length(instances(AdfStatus)) == 11
    @test [Int(s) for s in instances(AdfStatus)] == collect(0:10)
    @test collect(string.(instances(AdfStatus))) == ["OK", "NOT_DETERMINED", "UNIT_NOT_CERTIFIED",
        "NEEDS_SPLIT", "NOT_UNIQUE", "NO_SOLUTION", "NOT_UNIT", "DOMAIN", "UNSUPPORTED", "PARSE", "LIMIT"]
    @test Adelefeld.status_name(-1) == "UNKNOWN"
    @test Adelefeld.status_name(11) == "UNKNOWN"
    @test Adelefeld.status_name(7) == "DOMAIN"
    @test Adelefeld.status_count() == 11
    # The layout table is the literal rows; the rat row is docs/api-m1.md:35.
    @test (:rat, 16, 8) in [(r.suffix, r.size, r.align) for r in Adelefeld.LAYOUT_ROWS]
    @test sizeof(TextLimits) == 32
    @test fieldoffset.(TextLimits, 1:4) == UInt[0, 8, 16, 24]
    @test isempty(Test.detect_ambiguities(Adelefeld))
end

@testset "aligned allocation (docs/api-j.md:100-102)" begin
    for a in (1, 2, 8, 64, 4096), n in (1, 16, 33)
        base, p = Adelefeld.aligned_alloc(n, a)
        b = UInt(base); q = UInt(p)
        @test q % a == 0
        @test b <= q && q + n <= b + n + a - 1
        Libc.free(base)
    end
    @test_throws ArgumentError Adelefeld.aligned_alloc(16, 12)
    @test_throws ArgumentError Adelefeld.aligned_alloc(0, 8)
    @test_throws OverflowError Adelefeld.aligned_alloc(typemax(Int), 8)
end

@testset "statuses and results" begin
    r = try_rat(1, 0)
    @test r isa AdfResult{Rat}
    @test r.status === DOMAIN
    @test r.value === nothing
    @test r.where === nothing
    e = try
        Rat(1, 0)
        nothing
    catch err
        err
    end
    @test e isa AdfError
    @test e.status === DOMAIN
    @test e.message == "DOMAIN"
    @test e.result.status === DOMAIN
    @test occursin("DOMAIN", sprint(showerror, e))
    @test_throws AdfError unwrap(r)
    @test unwrap(try_rat(3)) == Rat(3)
    # An unknown numeric status is an ABI error retaining the raw Cint (docs/api-j.md:170).
    @test_throws Adelefeld.AdfAbiError Adelefeld.to_status(Cint(11), :test)
    @test_throws Adelefeld.AdfAbiError Adelefeld.to_status(Cint(-1), :test)
    @test Adelefeld.to_status(Cint(6), :test) === NOT_UNIT
end

@testset "den = 0 is DOMAIN, division by zero is NOT_UNIT" begin
    # rat.h:74-79 / text.h:147-150: zero denominator DOMAIN; rat.h:109-116: y = 0 is NOT_UNIT.
    @test try_rat(5, 0).status === DOMAIN
    @test try_rat(0, 0).status === DOMAIN
    @test try_rat(1//0).status === DOMAIN
    @test try_parse(Rat, "5/0").status === DOMAIN
    x = Rat(5)
    @test try_div(x, Rat(0)).status === NOT_UNIT
    @test try_div(x, Rat(0)).value === nothing
    @test try_div(x, 0).status === NOT_UNIT
    @test try_inv(Rat(0)).status === NOT_UNIT
    err = try; x / 0; catch e; e; end
    @test err isa AdfError && err.status === NOT_UNIT
    err = try; inv(Rat(0)); catch e; e; end
    @test err isa AdfError && err.status === NOT_UNIT
    err = try; Rat(1, 0); catch e; e; end
    @test err isa AdfError && err.status === DOMAIN
    @test string(x) == "5"
end

@testset "parser statuses" begin
    # text.h:17-27 (order of checks), 147-150 (rat statuses); conventions 9.2 rat = int ["/" uint].
    @test try_parse(Rat, "").status === PARSE
    @test try_parse(Rat, "1/").status === PARSE
    @test try_parse(Rat, "1/-2").status === PARSE
    @test try_parse(Rat, "abc").status === PARSE
    @test try_parse(Rat, "1\0").status === PARSE
    @test try_parse(Rat, "é").status === PARSE
    @test try_parse(Rat, "1234"; limits=TextLimits(max_len=3)).status === LIMIT
    @test try_parse(Rat, "123"; limits=TextLimits(max_len=3)).status === OK
    # ncodeunits, not characters: two characters, three bytes, over a limit of 2 bytes.
    @test try_parse(Rat, "é1"; limits=TextLimits(max_len=2)).status === LIMIT
    @test string(parse(Rat, "-0")) == "0"
    @test string(parse(Rat, "0010/0004")) == "5/2"
    @test string(parse(Rat, SubString("x7/21", 2))) == "1/3"
    @test_throws AdfError parse(Rat, "1/")
    @test TextLimits().max_len == 1048576
    @test TextLimits() == TextLimits(1048576, 100000, 100000, 1048576)
    @test_throws InexactError TextLimits(max_len=-1)
end

@testset "constructors" begin
    @test string(Rat(0)) == "0"
    @test string(Rat(-7)) == "-7"
    @test string(Rat(typemin(Int64))) == string(typemin(Int64))
    @test string(Rat(typemax(UInt64))) == string(typemax(UInt64))
    @test string(Rat(big(2)^200)) == string(big(2)^200)
    @test string(Rat(3, -6)) == "-1/2"
    @test string(Rat(-3, -6)) == "1/2"
    @test string(Rat(4, 2)) == "2"
    @test string(Rat(2//6)) == "1/3"
    @test string(Rat(-big(2)^130 // 3)) == oracle_text(-big(2)^130 // 3)
    @test string(Rat(true)) == "1"
    @test Rat(3, 1) == Rat(3)
    big_n = big(10)^2000
    @test try_rat(big_n; limits=TextLimits(max_len=100)).status === LIMIT
    @test string(Rat(big_n)) == string(big_n)
end

@testset "Rat 1/3 stays exact" begin
    t = Rat(1) / 3
    @test string(t) == "1/3"
    @test t + t + t == 1
    @test t * 3 == Rat(1)
    @test string(t - Rat(1, 2)) == "-1/6"
    # docs/api-j.md section 9 session line for this step, with / for the quotient (design silent on //).
    @test string(Rat(1) / 3 + Rat(1, 6)) == "1/2"
end

@testset "SPEC 4.3 arithmetic rows, rational centres" begin
    # docs/SPEC.md:242-248. Each row is a finite-ball row; the Rat step checks the exact arithmetic of
    # the centres, and that the centre of the stated result differs from it by an integer multiple of
    # the stated radius. The radii (gcd rules of 4.3) are the FBall step's.
    rows = [  # (centre computed with Rats, centre of the result, radius of the result)
        (Rat(3) + Rat(5),             Rat(2),    Rat(6)),
        (Rat(3) * Rat(5),             Rat(3),    Rat(6)),
        (Rat(12) * Rat(5),            Rat(60),   Rat(216)),
        (Rat(1, 3) * Rat(5),          Rat(5, 3), Rat(6)),
        (Rat(1, 2) * Rat(2, 3),       Rat(0),    Rat(1, 6)),
    ]
    expected_centres = ["8", "15", "60", "5/3", "1/3"]
    for ((c, a, N), want) in zip(rows, expected_centres)
        @test string(c) == want
        k = (c - a) / N
        @test !occursin('/', string(k))
    end
end

@testset "arithmetic against Rational{BigInt}" begin
    vals = Rational{BigInt}[0, 1, -1, 1//3, -5//7, big(2)^151 // (big(2)^127 - 1),
                            -(big(3)^90 + 1) // big(10)^20, big(2)^64, -big(2)^63, 7//big(2)^70]
    for p in vals, q in vals
        x = Rat(p); y = Rat(q)
        @test string(x + y) == oracle_text(p + q)
        @test string(x - y) == oracle_text(p - q)
        @test string(x * y) == oracle_text(p * q)
        if q == 0
            @test try_div(x, y).status === NOT_UNIT
        else
            @test string(x / y) == oracle_text(p / q)
        end
        @test (x == y) == (p == q)
        @test string(-x) == oracle_text(-p)
    end
end

@testset "mixed arithmetic with Integer and Rational" begin
    x = Rat(2, 3)
    @test x + 1 == Rat(5, 3)
    @test 1 + x == Rat(5, 3)
    @test x - 1 == Rat(-1, 3)
    @test 1 - x == Rat(1, 3)
    @test x * 3 == 2
    @test 3 * x == 2
    @test x / 2 == Rat(1, 3)
    @test 2 / x == 3
    @test x + 1//3 == 1
    @test 1//3 + x == 1
    @test x * (3//2) == 1
    @test x / (2//3) == 1
    @test (1//2) / x == Rat(3, 4)
    @test true * x == x
    @test x * false == 0
    @test x + big(2)^100 == Rat(big(2)^100 * 3 + 2, 3)
    @test x == 2//3
    @test 2//3 == x
    @test Rat(4) == 4
    @test 4 == Rat(4)
    @test x != 1
    @test Rat(1) != 1//0
    @test isequal(Rat(1, 2), 1//2)
    @test hash(Rat(1, 2)) == hash(1//2)
    @test hash(Rat(5)) == hash(5)
    @test hash(Rat(1, 2)) == hash(Rat(2, 4))
    @test length(Set([Rat(1, 2), Rat(2, 4), Rat(1)])) == 2
    # No implicit Float64 or Complex conversion (docs/api-j.md:232-233): refused, whatever the error.
    @test_throws Exception x + 1.5
    @test_throws Exception x * (1 + 2im)
    @test_throws MethodError x < Rat(1)
end

@testset "inputs unchanged by every operation" begin
    x = Rat(-22, 7); y = Rat(big(2)^80 + 1, 3); z = Rat(0)
    sx, sy = string(x), string(y)
    ops = [() -> x + y, () -> x - y, () -> x * y, () -> x / y, () -> -x, () -> inv(x),
           () -> try_div(x, z), () -> try_inv(z), () -> copy(x), () -> x == y,
           () -> identical(x, y), () -> x + 1, () -> 1 - x, () -> x * (1//5), () -> hash(x)]
    for f in ops
        f()
        @test string(x) == sx
        @test string(y) == sy
        @test string(z) == "0"
    end
    # The same handle on both sides: the C aliasing of rat.h:2-11 permits it.
    @test string(x + x) == "-44/7"
    @test string(x * x) == "484/49"
    @test string(x / x) == "1"
    @test string(x - x) == "0"
    @test string(x) == sx
end

@testset "copies, identity, close" begin
    x = Rat(9, 4)
    c = copy(x)
    @test c !== x
    @test identical(c, x)
    @test c == x
    before = Adelefeld.diagnostics().live
    close(c)
    @test Adelefeld.diagnostics().live == before - 1
    close(c)                              # repeated close is a no-op
    finalize(c)                           # so is a later finalizer run
    @test Adelefeld.diagnostics().live == before - 1
    @test string(x) == "9/4"             # the original has its own storage and its own clear
    @test_throws InvalidStateException string(c)
    @test_throws InvalidStateException c + x
    @test_throws InvalidStateException x + c
    @test_throws InvalidStateException copy(c)
    @test_throws InvalidStateException c == x
    @test_throws InvalidStateException try_div(x, c)
    @test Adelefeld.diagnostics().live == before - 1   # no allocation before the refusal
    @test identical(Rat(1, 2), Rat(2, 4))
    @test !identical(Rat(1, 2), Rat(1, 3))
end

@testset "string round trip" begin
    for q in Rational{BigInt}[0, -1, 1//7, -big(10)^300 // 7, big(2)^200 + 1, -3//big(2)^90]
        x = Rat(q)
        s = string(x)
        @test s == oracle_text(q)
        @test parse(Rat, s) == x
        @test string(parse(Rat, s)) == s
        @test sprint(show, x) == s
        @test repr(x) == s
    end
end

@testset "10^4 handles, forced GC, live count returns" begin
    GC.gc(); GC.gc()
    before = Adelefeld.diagnostics().live
    function churn(n)
        acc = Rat(0)
        for i in 1:n
            acc = acc + Rat(i, i + 1)
        end
        return string(acc)
    end
    want = oracle_text(sum(big(i) // (i + 1) for i in 1:10_000))
    @test churn(10_000) == want
    for _ in 1:4
        GC.gc()
    end
    @test Adelefeld.diagnostics().live == before
    @test Adelefeld.diagnostics().finalizer_errors == 0
end

@testset "load rejection in separate processes" begin
    lib = Adelefeld.diagnostics().library
    script = joinpath(@__DIR__, "load_fault.jl")
    src = joinpath(@__DIR__, "fixtures", "fault.c")
    cc = Sys.which("cc")
    @test cc !== nothing
    dir = mktempdir()
    julia = Base.julia_cmd()
    function run_case(env; arg=nothing)
        cmd = `$julia --startup-file=no --project=$PKG $script`
        arg === nothing || (cmd = `$cmd $arg`)
        out = IOBuffer()
        p = run(pipeline(setenv(cmd, env); stdout=out, stderr=out); wait=false)
        wait(p)
        text = String(take!(out))
        return p.exitcode, text
    end
    base_env = copy(ENV)
    base_env["JULIA_NUM_THREADS"] = "1"
    cases = [  # (macro, linked to the library, words the error must contain)
        ("FAULT_SIZE",    true,  ["layout", "adf_sizeof_rat", "16", "24"]),
        ("FAULT_ALIGN",   true,  ["layout", "adf_alignof_rat", "8", "16"]),
        ("FAULT_NAME",    true,  ["status", "DOMAIN", "DOMAIN_ERROR"]),
        ("FAULT_COUNT",   true,  ["status count", "11", "12"]),
        ("FAULT_VERSION", true,  ["FLINT version", "3.0.1"]),
        ("FAULT_MISSING", false, ["symbol", "adf_status_count"]),
    ]
    for (macro_name, linked, words) in cases
        so = joinpath(dir, "lib" * lowercase(macro_name) * ".so")
        ccmd = linked ?
            `$cc -shared -fPIC -O1 -D$macro_name $src -o $so -Wl,--no-as-needed $lib -Wl,-rpath,$(dirname(lib))` :
            `$cc -shared -fPIC -O1 -D$macro_name $src -o $so`
        run(ccmd)
        env = copy(base_env)
        env["ADELEFELD_LIB"] = so
        code, text = run_case(env; arg=so)
        @testset "$macro_name" begin
            @test code == 0
            @test occursin("LOADED=false", text)
            for w in words
                @test occursin(w, text)
            end
            @test occursin(so, text)
            @test occursin("INIT_CALLS=0", text)
            @test !occursin("AFTER=allocated", text)
        end
    end
    # The missing GMP symbol (docs/api-j.md:80-87): without the preload, this Julia's own libgmp is
    # loaded first; the error must give the exact preload command.
    env = copy(base_env)
    delete!(env, "LD_PRELOAD")
    env["ADELEFELD_LIB"] = lib
    code, text = run_case(env)
    @testset "missing GMP symbol without preload" begin
        @test code == 0
        if occursin("LOADED=false", text)
            @test occursin("__gmpn_modexact_1_odd", text)
            @test occursin("LD_PRELOAD=", text)
            @test occursin("adelefeld-julia", text)
            @test !occursin("AFTER=allocated", text)
        else
            # A Julia whose bundled GMP has the symbol loads without the preload; nothing to reject.
            @test occursin("LOADED=true", text)
            @info "the GMP quirk is absent on this machine; the rejection case was not exercised"
        end
    end
    # A library path that does not exist.
    env = copy(base_env)
    env["ADELEFELD_LIB"] = joinpath(dir, "no-such-library.so")
    code, text = run_case(env)
    @testset "no library" begin
        @test occursin("LOADED=false", text)
        @test occursin("no-such-library.so", text)
    end
    # The launcher bin/adelefeld-julia (docs/api-j.md:85-86): from a process without the preload, it sets it
    # before Julia starts, and a plain `using Adelefeld` works; the session line of this step.
    env = copy(base_env)
    delete!(env, "LD_PRELOAD")
    env["JULIA"] = joinpath(Sys.BINDIR, "julia")
    launcher = joinpath(PKG, "bin", "adelefeld-julia")
    out = IOBuffer()
    err = IOBuffer()
    cmd = `sh $launcher --startup-file=no --project=$PKG -e 'using Adelefeld; print(Rat(1) / 3 + Rat(1, 6))'`
    p = run(pipeline(setenv(cmd, env); stdout=out, stderr=err); wait=false)
    wait(p)
    text = String(take!(out))
    p.exitcode == 0 || print(String(take!(err)))
    @testset "launcher" begin
        @test p.exitcode == 0
        @test text == "1/2"
    end
    rm(dir; recursive=true)
end

end

# ---- Step J-a.2 (docs/api-j.md section 9, second item of J-a): FBall and the scalar mixed arithmetic ----
# Acceptance line of docs/api-j.md:373-374: SPEC's five arithmetic rows, fractional and zero radii, q - x sign,
# x - x as independent balls, den = 0 DOMAIN against division by zero NOT_UNIT, inputs unchanged, copies clear
# separately. C contract: include/adelefeld/fball.h; text: include/adelefeld/text.h:156-165.

fb(s) = parse(FBall, s)

# The exact value of a Rat as a Julia Rational{BigInt} (read from the C text by the package's own reader).
jrat(x::Rat) = Adelefeld._rational(x)

# A deterministic generator for the random balls (no Random dependency): a 64-bit LCG (Knuth MMIX constants).
mutable struct Lcg
    s::UInt64
end
next!(g::Lcg) = (g.s = g.s * 0x5851f42d4c957f2d + 0x14057b7ef767814f; g.s >> 11)

@testset "Adelefeld step J-a.2" begin

@testset "fball layout row, types, ambiguities" begin
    # docs/api-m1.md:36 (48 bytes), :45 (alignment 8); fball.h:53-54.
    @test (:fball, 48, 8) in [(r.suffix, r.size, r.align) for r in Adelefeld.LAYOUT_ROWS]
    @test Adelefeld.FBALL_SIZE == 48
    @test Adelefeld.FBALL_ALIGN == 8
    @test FBall <: Number
    @test isempty(Test.detect_ambiguities(Adelefeld))
    # PointComparison: status.h:35-37, literal; separate from AdfStatus (docs/api-j.md:223, 315).
    @test [Int(c) for c in instances(PointComparison)] == [0, 1, 2]
    @test collect(string.(instances(PointComparison))) == ["EQUAL", "DIFFERENT", "UNDECIDED"]
    @test PointComparison !== AdfStatus
end

@testset "SPEC 4.3 rows exactly (docs/SPEC.md:244-248)" begin
    rows = [  # (the operation through parse and the operators, the expected text of the table)
        ("(3 mod 12) + (5 mod 18)",     fb("3 mod 12") + fb("5 mod 18"),     "2 mod 6"),
        ("(3 mod 12) * (5 mod 18)",     fb("3 mod 12") * fb("5 mod 18"),     "3 mod 6"),
        ("12 * (5 mod 18)",             12 * fb("5 mod 18"),                 "60 mod 216"),
        ("(1/3) * (5 mod 18)",          Rat(1, 3) * fb("5 mod 18"),          "5/3 mod 6"),
        ("(1/2 mod 8) * (2/3 mod 9)",   fb("1/2 mod 8") * fb("2/3 mod 9"),   "0 mod 1/6"),
    ]
    for (name, got, want) in rows
        @testset "$name" begin
            @test got isa FBall
            @test equal_set(got, fb(want))
            @test string(got) == "(* ; " * want * ")"
        end
    end
    # The same rows by the other routes: the scalar as an exact ball (fball_mul, not mul_rat), the other
    # order, a Rational scalar, and division by the scalar's inverse (div_rat).
    @test string(fb("(* ; 12)") * fb("5 mod 18")) == "(* ; 60 mod 216)"
    @test string(fb("5 mod 18") * Rat(12)) == "(* ; 60 mod 216)"
    @test string(fb("5 mod 18") * (1//3)) == "(* ; 5/3 mod 6)"
    @test string(fb("5 mod 18") / 3) == "(* ; 5/3 mod 6)"
    @test string(fb("5 mod 18") / Rat(1, 12)) == "(* ; 60 mod 216)"
    @test string(fb("5 mod 18") + fb("3 mod 12")) == "(* ; 2 mod 6)"
end

@testset "the session line (docs/api-j.md:313-314, 418)" begin
    x = parse(FBall, "3 mod 12") + parse(FBall, "5 mod 18")
    @test sprint(show, x) == "(* ; 2 mod 6)"
    @test sprint(show, MIME"text/plain"(), x) == "(* ; 2 mod 6)"
    @test repr(x) == "(* ; 2 mod 6)"
    @test value_text(x) == "(* ; 2 mod 6)"
    @test try_value_text(x).status === OK
end

@testset "fractional and zero radii" begin
    x = fb("0 mod 1/6")
    @test string(x) == "(* ; 0 mod 1/6)"
    @test radius(x) == Rat(1, 6)
    @test center(x) == 0
    @test string(fb("1/4 mod 1/2") + fb("1/6 mod 1/4")) == "(* ; 1/6 mod 1/4)"
    @test string(FBall(Rat(1, 2), Rat(1, 4))) == "(* ; 0 mod 1/4)"
    @test string(FBall(Rat(5, 6), Rat(2, 3))) == "(* ; 1/6 mod 2/3)"
    z = fb("(* ; 7/3)")
    @test radius(z) == 0
    @test center(z) == Rat(7, 3)
    @test identical(z, FBall(Rat(7, 3)))
    @test string(FBall(Rat(5), Rat(0))) == "(* ; 5)"
    @test string(fb("(* ; 14/6 mod 0)")) == "(* ; 7/3)"
    # Rat 1/3 stays exact inside a ball of radius 0.
    t = FBall(Rat(1) / 3)
    @test string(t) == "(* ; 1/3)"
    @test string(t + t + t) == "(* ; 1)"
    @test string(t * 3) == "(* ; 1)"
    @test string(FBall(Rat(1, 3)) + FBall(Rat(1, 6))) == "(* ; 1/2)"
    @test string(fb("(* ; 1/2)") * fb("(* ; 2/3)")) == "(* ; 1/3)"
end

@testset "constructors" begin
    @test string(FBall(Rat(5, 3))) == "(* ; 5/3)"
    @test string(FBall(7)) == "(* ; 7)"
    @test string(FBall(-3//4)) == "(* ; -3/4)"
    @test string(FBall(big(2)^100)) == "(* ; " * string(big(2)^100) * ")"
    @test string(FBall(Rat(0))) == "(* ; 0)"
    @test identical(FBall(Rat(3), Rat(12)), fb("3 mod 12"))
    @test string(FBall(Rat(-1), Rat(2))) == "(* ; 1 mod 2)"
    @test string(FBall(Rat(1, 2), Rat(0))) == "(* ; 1/2)"
    @test try_fball(Rat(3), Rat(12)) isa AdfResult{FBall}
    @test try_fball(Rat(3), Rat(12)).status === OK
    # A negative radius is DOMAIN (fball.h:133-134), the output untouched and cleared.
    c1 = Rat(1); m6 = Rat(-6)
    before = Adelefeld.diagnostics().live
    r = try_fball(c1, m6)
    @test r.status === DOMAIN
    @test r.value === nothing
    @test Adelefeld.diagnostics().live == before
    @test try_fball(Rat(1), Rat(-1, 2)).status === DOMAIN
    err = try; FBall(Rat(1), Rat(-6)); catch e; e; end
    @test err isa AdfError && err.status === DOMAIN
    # Both orders of the scalar types (Integer and Rational by explicit Rat conversion).
    @test_throws AdfError FBall(1//0)
    err = try; FBall(1//0); catch e; e; end
    @test err.status === DOMAIN
end

@testset "parser: both forms, statuses, limits" begin
    # text.h:158-161: "(* ; F)" and the bare "a mod N" (conventions 9.2, fball_v); golden tests/golden/fball.tsv.
    @test string(fb("3 mod 12")) == "(* ; 3 mod 12)"
    @test string(fb("(* ; 3 mod 12)")) == "(* ; 3 mod 12)"
    @test identical(fb("3 mod 12"), fb("(* ; 3 mod 12)"))
    @test string(fb("(*;2mod6)")) == "(* ; 2 mod 6)"
    @test string(fb("-1 mod 2")) == "(* ; 1 mod 2)"
    @test string(fb("(* ; 20 mod 6)")) == "(* ; 2 mod 6)"
    @test string(fb("(* ; 10/4 mod 6/4)")) == "(* ; 1 mod 3/2)"
    @test string(parse(FBall, SubString("xx3 mod 12", 3))) == "(* ; 3 mod 12)"
    # den = 0 in a text: DOMAIN (text.h:159-160 "ADF_DOMAIN (zero denominator)"; conventions 9.3).
    for s in ("(* ; 1/0)", "(* ; 1 mod 1/0)", "(* ; 1/0 mod 0)", "1/0 mod 6", "1 mod 6/0")
        @test try_parse(FBall, s).status === DOMAIN
    end
    err = try; parse(FBall, "(* ; 1/0)"); catch e; e; end
    @test err isa AdfError && err.status === DOMAIN
    # Not a sentence: PARSE.
    for s in ("", "7/3", "mod 6", "2 mod", "1 mod -6", "(* ; 1 mod 6", "(* ; 1 mod 6))", "(*)",
              "(* ; 1.5 mod 6)", "(1 ; 2 mod 6)", "((((* ; 2 mod 6))))", "(* ; 2 mod 6)\0", "3 mod 12 é")
        r = try_parse(FBall, s)
        @test r.status === PARSE
        @test r.value === nothing
    end
    @test_throws AdfError parse(FBall, "7/3")
    # The limits are honoured (text.h:22-23, stage 1): bytes counted by ncodeunits.
    @test try_parse(FBall, "3 mod 12"; limits=TextLimits(max_len=7)).status === LIMIT
    @test try_parse(FBall, "3 mod 12"; limits=TextLimits(max_len=8)).status === OK
    @test try_parse(FBall, "é mod 1"; limits=TextLimits(max_len=7)).status === LIMIT
    long = "(* ; " * "1"^Int(TextLimits().max_len) * " mod 6)"
    @test try_parse(FBall, long).status === LIMIT
    # A failing parse leaves no live handle.
    GC.gc()
    before = Adelefeld.diagnostics().live
    for s in ("7/3", "(* ; 1/0)", long)
        try_parse(FBall, s)
    end
    @test Adelefeld.diagnostics().live == before
end

@testset "q - x against x - q" begin
    x = fb("1 mod 6")
    @test string(x - Rat(2)) == "(* ; 5 mod 6)"
    @test string(Rat(2) - x) == "(* ; 1 mod 6)"
    @test string(x - 2) == "(* ; 5 mod 6)"
    @test string(2 - x) == "(* ; 1 mod 6)"
    y = fb("1/3 mod 1")
    @test string(y - Rat(1, 2)) == "(* ; 5/6 mod 1)"
    @test string(Rat(1, 2) - y) == "(* ; 1/6 mod 1)"
    @test string(y - 1//2) == "(* ; 5/6 mod 1)"
    @test string(1//2 - y) == "(* ; 1/6 mod 1)"
    @test equal_set(-(y - Rat(1, 2)), Rat(1, 2) - y)
    @test !equal_set(y - Rat(1, 2), Rat(1, 2) - y)
    @test string(FBall(Rat(1, 3)) - Rat(1, 2)) == "(* ; -1/6)"
    @test string(Rat(1, 2) - FBall(Rat(1, 3))) == "(* ; 1/6)"
    @test string(-fb("1 mod 6")) == "(* ; 5 mod 6)"
end

@testset "x - x is a ball around 0 (independent inputs, SPEC 4.3)" begin
    x = fb("3 mod 12")
    d = x - x
    @test string(d) == "(* ; 0 mod 12)"
    @test radius(d) == 12
    @test !equal_set(d, FBall(Rat(0)))
    @test overlaps(d, FBall(Rat(0)))
    @test inside(FBall(Rat(0)), d)
    @test compare_points(d, FBall(Rat(0))) === Adelefeld.UNDECIDED
    @test string(x + (-x)) == "(* ; 0 mod 12)"
    q = FBall(Rat(5, 7))
    @test string(q - q) == "(* ; 0)"
end

@testset "division by zero is NOT_UNIT, den = 0 is DOMAIN" begin
    x = fb("5 mod 18")
    r = try_div(x, Rat(0))
    @test r isa AdfResult{FBall}
    @test r.status === NOT_UNIT
    @test r.value === nothing
    @test try_div(x, 0).status === NOT_UNIT
    @test try_div(x, 0//1).status === NOT_UNIT
    err = try; x / 0; catch e; e; end
    @test err isa AdfError && err.status === NOT_UNIT
    err = try; x / Rat(0); catch e; e; end
    @test err isa AdfError && err.status === NOT_UNIT
    # A scalar whose Rat conversion fails keeps the conversion's status.
    @test try_add(x, 1//0).status === DOMAIN
    @test try_add(x, 1//0) isa AdfResult{FBall}
    @test try_mul(1//0, x).status === DOMAIN
    @test try_div(x, 1//0).status === DOMAIN
    err = try; x + 1//0; catch e; e; end
    @test err isa AdfError && err.status === DOMAIN
    @test string(x * 0) == "(* ; 0)"
    @test string(0 * x) == "(* ; 0)"
    @test string(x) == "(* ; 5 mod 18)"
end

@testset "set predicates and point comparison" begin
    a = fb("2 mod 6"); b = fb("0 mod 2")
    # FIRST INSIDE SECOND (fball.h:248-251; docs/api-j.md:222, 314): 2 + 6 Zhat is inside 0 + 2 Zhat.
    @test inside(a, b) === true
    @test inside(b, a) === false
    @test inside(FBall(Rat(4)), b) === true
    @test inside(b, FBall(Rat(4))) === false
    @test inside(a, a) === true
    @test equal_set(a, fb("(* ; 20 mod 6)")) === true
    @test equal_set(a, fb("2 mod 3")) === false
    @test equal_set(FBall(Rat(1, 3)), FBall(Rat(2, 6))) === true
    @test overlaps(fb("1 mod 2"), fb("0 mod 3")) === true
    @test overlaps(fb("1 mod 2"), fb("0 mod 4")) === false
    @test overlaps(fb("0 mod 4"), fb("1 mod 2")) === false
    @test overlaps(a, b) === true
    @test compare_points(FBall(Rat(1, 3)), FBall(Rat(2, 6))) === Adelefeld.EQUAL
    @test compare_points(fb("1 mod 2"), fb("0 mod 2")) === Adelefeld.DIFFERENT
    @test compare_points(FBall(Rat(1)), FBall(Rat(2))) === Adelefeld.DIFFERENT
    @test compare_points(fb("1 mod 2"), fb("1 mod 4")) === Adelefeld.UNDECIDED
    @test compare_points(a, a) === Adelefeld.UNDECIDED
    @test compare_points(a, b) isa PointComparison
    @test !(compare_points(a, b) isa AdfStatus)
    @test is_canonical(a) === true
    @test is_canonical(fb("(* ; 0)")) === true
    @test identical(a, fb("(* ; 20 mod 6)")) === true
    @test identical(a, b) === false
    # Accessors: fresh Rats (fball.h:163-170).
    c = center(fb("(* ; -7 mod 3)"))
    @test c isa Rat && c == 2
    @test radius(fb("2/3 mod 9")) == 9
    @test center(fb("2/3 mod 9")) == Rat(2, 3)
    x = fb("5/3 mod 6")
    r = radius(x)
    close(r)
    @test string(x) == "(* ; 5/3 mod 6)"
    @test radius(x) == 6
end

@testset "no == on balls, identity isequal and hash, no ordering, no ball division" begin
    x = fb("3 mod 12"); y = fb("3 mod 12")
    err = try; x == y; catch e; e; end
    @test err isa ArgumentError
    @test occursin("equal_set", err.msg) && occursin("compare_points", err.msg)
    @test_throws ArgumentError x == x
    @test_throws ArgumentError x != y
    @test_throws ArgumentError x == 1
    @test_throws ArgumentError 1 == x
    @test_throws ArgumentError x == Rat(1)
    @test_throws ArgumentError Rat(1) == x
    @test_throws ArgumentError x == 1//2
    # Identity-based isequal and hash (docs/api-j.md:235).
    @test isequal(x, x)
    @test !isequal(x, y)
    @test hash(x) == hash(x)
    @test length(Set([x, y, x])) == 2
    @test !isequal(x, Rat(3))
    @test_throws MethodError x < y
    # No division by a ball (docs/api-j.md:217): no method of the package; the call fails.
    @test_throws Exception x / y
    @test_throws Exception Rat(1) / x
    @test_throws Exception 1 / x
    @test which(/, Tuple{FBall,FBall}).module !== Adelefeld
    @test which(/, Tuple{Rat,FBall}).module !== Adelefeld
    # No implicit Float64 or Complex conversion (docs/api-j.md:232-233).
    @test_throws Exception x + 1.5
    @test_throws Exception 1.5 * x
    @test_throws Exception x * (1 + 2im)
end

@testset "mixed-scalar matrix" begin
    # Oracle: Julia Rational{BigInt} arithmetic on the centre a and radius N, read through FBall(c, N)
    # (set_center_radius). x + q = (a + q) + N Zhat; q - x = (q - a) + N Zhat; q x = q a + |q| N Zhat (fball.h:219);
    # x / q = a/q + N/|q| Zhat.
    balls = [fb("3 mod 12"), fb("1/2 mod 8"), fb("0 mod 1/6"), FBall(Rat(-5, 7))]
    scalars = Any[2, -3, 0, true, big(2)^70, -2//3, big(5)//big(2)^65, Rat(7, 4), Rat(0)]
    expect(c, N) = FBall(Rat(c), Rat(abs(N)))
    for x in balls, q in scalars
        a = jrat(center(x)); N = jrat(radius(x))
        Q = q isa Rat ? jrat(q) : Rational{BigInt}(q)
        @testset "$(string(x)) with $(typeof(q)) $q" begin
            @test equal_set(x + q, expect(a + Q, N)) && string(x + q) == string(expect(a + Q, N))
            @test equal_set(q + x, expect(a + Q, N)) && string(q + x) == string(expect(a + Q, N))
            @test equal_set(x - q, expect(a - Q, N)) && string(x - q) == string(expect(a - Q, N))
            @test equal_set(q - x, expect(Q - a, N)) && string(q - x) == string(expect(Q - a, N))
            @test equal_set(x * q, expect(Q * a, Q * N)) && string(x * q) == string(expect(Q * a, Q * N))
            @test equal_set(q * x, expect(Q * a, Q * N)) && string(q * x) == string(expect(Q * a, Q * N))
            if Q == 0
                @test try_div(x, q).status === NOT_UNIT
            else
                @test equal_set(x / q, expect(a / Q, N / Q)) && string(x / q) == string(expect(a / Q, N / Q))
            end
            @test try_add(x, q) isa AdfResult{FBall}
            @test try_sub(q, x) isa AdfResult{FBall}
            @test try_mul(q, x) isa AdfResult{FBall}
        end
    end
end

@testset "inputs unchanged by every operation" begin
    x = fb("1/2 mod 8"); y = fb("2/3 mod 9"); q = Rat(-5, 3); z = Rat(0)
    sx, sy, sq = string(x), string(y), string(q)
    ops = [() -> x + y, () -> x - y, () -> x * y, () -> -x, () -> x + q, () -> q - x, () -> x * q,
           () -> q * x, () -> x / q, () -> try_div(x, z), () -> copy(x), () -> identical(x, y),
           () -> equal_set(x, y), () -> overlaps(x, y), () -> inside(x, y), () -> compare_points(x, y),
           () -> center(x), () -> radius(x), () -> is_canonical(x), () -> x + 1, () -> 2//3 * x,
           () -> try_fball(q, z), () -> FBall(q), () -> try_fball(q, Rat(-1)), () -> hash(x),
           () -> try_add(x, 1//0)]
    for f in ops
        f()
        @test string(x) == sx
        @test string(y) == sy
        @test string(q) == sq
        @test string(z) == "0"
    end
    # The same handle on both sides (fball.h:24-26 permits aliasing): independent balls.
    @test string(x + x) == "(* ; 1 mod 8)"
    @test string(x * x) == "(* ; 1/4 mod 4)"
    @test string(x - x) == "(* ; 0 mod 8)"
    @test string(x) == sx
end

@testset "copies, identity, close" begin
    x = fb("5/3 mod 6")
    q = Rat(2); one_ = Rat(1); close(q)    # operands made before the live count is read
    c = copy(x)
    @test c !== x
    @test identical(c, x)
    @test equal_set(c, x)
    before = Adelefeld.diagnostics().live
    close(c)
    @test Adelefeld.diagnostics().live == before - 1
    close(c)
    finalize(c)
    @test Adelefeld.diagnostics().live == before - 1
    @test string(x) == "(* ; 5/3 mod 6)"
    for f in (() -> string(c), () -> c + x, () -> x + c, () -> c * 2, () -> 2 - c, () -> -c, () -> c / 2,
              () -> try_div(c, one_), () -> copy(c), () -> center(c), () -> radius(c), () -> is_canonical(c),
              () -> equal_set(c, x), () -> overlaps(x, c), () -> inside(c, x), () -> compare_points(x, c),
              () -> identical(c, x), () -> x * c)
        @test_throws InvalidStateException f()
    end
    @test Adelefeld.diagnostics().live == before - 1   # no allocation before the refusal
    # A closed Rat operand is refused before C as well.
    @test_throws InvalidStateException x + q
    @test_throws InvalidStateException q * x
    @test_throws InvalidStateException x / q
    @test_throws InvalidStateException FBall(q)
    @test_throws InvalidStateException try_fball(q, one_)
    @test_throws InvalidStateException try_fball(one_, q)
    @test Adelefeld.diagnostics().live == before - 1
end

@testset "text round trip of 50 random balls" begin
    g = Lcg(0x2545f4914f6cdd1d)
    rnd(k) = big(next!(g)) * big(next!(g)) % big(10)^k
    for i in 1:50
        num = rnd(1 + i % 30) - big(10)^(i % 30) ÷ 2
        den = rnd(1 + i % 12) + 1
        rn = i % 5 == 0 ? big(0) : rnd(1 + i % 25) + 1
        rd = rnd(1 + i % 7) + 1
        x = FBall(Rat(num, den), Rat(rn, rd))
        s = string(x)
        y = parse(FBall, s)
        @test string(y) == s
        @test equal_set(x, y)
        @test identical(x, y)
        # The stored centre is in [0, N) for N > 0 (fball.h:163-165).
        a = jrat(center(x)); N = jrat(radius(x))
        @test N == abs(rn // rd)
        @test N == 0 ? a == num // den : (0 <= a < N && isinteger((a - num // den) / N))
        if N != 0
            bare = s[6:end-1]                  # "(* ; F)" -> "F", the bare form of fball_v
            @test string(parse(FBall, bare)) == s
        end
    end
end

@testset "10^4 FBall handles, forced GC, live count returns" begin
    GC.gc(); GC.gc()
    before = Adelefeld.diagnostics().live
    function churn(n)
        acc = FBall(Rat(0))
        for i in 1:n
            acc = acc + FBall(Rat(i), Rat(6))
        end
        return string(acc)
    end
    @test churn(10_000) == "(* ; $(mod(sum(1:10_000), 6)) mod 6)"
    for _ in 1:4
        GC.gc()
    end
    @test Adelefeld.diagnostics().live == before
    @test Adelefeld.diagnostics().finalizer_errors == 0
end

@testset "printer text is freed (glibc mallinfo2)" begin
    # Every adf_fball_get_str result is released with adf_str_free (text.h:32-35): 400 texts of about 50 kB
    # leave the bytes in use by malloc (uordblks + hblkhd of mallinfo2, glibc >= 2.33) within 4 MB, where a
    # skipped free would leave about 20 MB.
    x = FBall(Rat(1, big(10)^25000 + 1), Rat(big(10)^25000))
    n = ncodeunits(string(x))
    @test n > 50_000
    inuse() = (m = ccall(:mallinfo2, NTuple{10,Csize_t}, ()); Int(m[8]) + Int(m[5]))
    GC.gc(); GC.gc()
    m0 = inuse()
    for _ in 1:400
        string(x)
    end
    GC.gc(); GC.gc()
    @test inuse() - m0 < 4_000_000
end

@testset "fball layout rejection and the session through the launcher" begin
    lib = Adelefeld.diagnostics().library
    script = joinpath(@__DIR__, "load_fault.jl")
    src = joinpath(@__DIR__, "fixtures", "fault.c")
    cc = Sys.which("cc")
    dir = mktempdir()
    julia = Base.julia_cmd()
    base_env = copy(ENV)
    base_env["JULIA_NUM_THREADS"] = "1"
    for (macro_name, words) in (("FAULT_FBALL_SIZE", ["layout", "adf_sizeof_fball", "48", "56"]),
                                ("FAULT_FBALL_ALIGN", ["layout", "adf_alignof_fball", "8", "16"]))
        so = joinpath(dir, "lib" * lowercase(macro_name) * ".so")
        run(`$cc -shared -fPIC -O1 -D$macro_name $src -o $so -Wl,--no-as-needed $lib -Wl,-rpath,$(dirname(lib))`)
        env = copy(base_env)
        env["ADELEFELD_LIB"] = so
        out = IOBuffer()
        p = run(pipeline(setenv(`$julia --startup-file=no --project=$PKG $script $so`, env); stdout=out,
                         stderr=out); wait=false)
        wait(p)
        text = String(take!(out))
        @testset "$macro_name" begin
            @test p.exitcode == 0
            @test occursin("LOADED=false", text)
            for w in words
                @test occursin(w, text)
            end
            @test occursin("docs/api-m1.md:36,45", text)
            @test occursin("INIT_CALLS=0", text)
            @test !occursin("AFTER=allocated", text)
        end
    end
    # The session line of this step (docs/api-j.md:418) in a fresh process started by the launcher.
    env = copy(base_env)
    delete!(env, "LD_PRELOAD")
    env["JULIA"] = joinpath(Sys.BINDIR, "julia")
    launcher = joinpath(PKG, "bin", "adelefeld-julia")
    out = IOBuffer()
    err = IOBuffer()
    line = "using Adelefeld; show(stdout, MIME\"text/plain\"(), " *
           "parse(FBall, \"3 mod 12\") + parse(FBall, \"5 mod 18\"))"
    p = run(pipeline(setenv(`sh $launcher --startup-file=no --project=$PKG -e $line`, env); stdout=out,
                     stderr=err); wait=false)
    wait(p)
    text = String(take!(out))
    p.exitcode == 0 || print(String(take!(err)))
    @testset "launcher session" begin
        @test p.exitcode == 0
        @test text == "(* ; 2 mod 6)"
    end
    rm(dir; recursive=true)
end

end

# ---- Step J-a.3: Adele/CAdele and numerical carriers (docs/api-j.md sections 3, 4 and 9) ----
using Libdl
# Freeze unrelated finalizers during exact live-count comparisons. The GC acceptance runs in a
# separate process with automatic GC enabled. Restore this process even if a test throws.
previous_gc = GC.enable(false)
try
@testset "Adelefeld step J-a.3" begin

@testset "types and layout rows" begin
    rows = [(r.suffix, r.size, r.align) for r in Adelefeld.LAYOUT_ROWS]
    @test (:adele, 96, 8) in rows
    @test (:cadele, 144, 8) in rows
    @test Adele <: Number
    @test CAdele <: Number
    @test RealBall !== Adele
    @test ComplexBall !== CAdele
    @test isempty(Test.detect_ambiguities(Adelefeld))
end

# Test-only queries from the same handle's FLINT dependency, never raw init/clear.
# refs/src/flint-3.0.1/arb.rst:382-388,660-680; acb.rst:302-304,374-380.
function num_contains(x, q)
    r = Rat(q)
    try
        GC.@preserve x r begin
            f = Libdl.dlsym(Adelefeld.LIBHANDLE[], :arb_contains_fmpq)
            return ccall(f, Cint, (Ptr{Cvoid}, Ptr{Cvoid}),
                         Adelefeld._ptr(x._carrier), Adelefeld._ptr(r)) == 1
        end
    finally
        close(r)
    end
end
function ball_contains(x, y)
    GC.@preserve x y begin
        f = Libdl.dlsym(Adelefeld.LIBHANDLE[], x isa RealBall ? :arb_contains : :acb_contains)
        return ccall(f, Cint, (Ptr{Cvoid}, Ptr{Cvoid}),
                     Adelefeld._ptr(x._carrier), Adelefeld._ptr(y._carrier)) == 1
    end
end
function num_radius(x)
    z = RealBall(0)
    GC.@preserve x z begin
        f = Libdl.dlsym(Adelefeld.LIBHANDLE[], :arb_get_rad_arb)
        ccall(f, Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}),
              Adelefeld._ptr(z._carrier), Adelefeld._ptr(x._carrier))
    end
    return z
end
function component_copy(x, name)
    z = RealBall(0)
    GC.@preserve x z begin
        f = Libdl.dlsym(Adelefeld.LIBHANDLE[], name)
        ccall(f, Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}),
              Adelefeld._ptr(z._carrier), Adelefeld._ptr(x._carrier))
    end
    return z
end
function exact_zero_fin(x)
    f = finitepart(x)
    c = center(f); n = radius(f)
    try
        return string(c) == "0" && string(n) == "0"
    finally
        close(c); close(n); close(f)
    end
end

@testset "constructors, copies and numerical frame inverse" begin
    for T in (Adele, CAdele)
        x = T(Rat(3, 2); prec=64)
        @test is_canonical(x)
        @test identical(x, copy(x))
        @test equal_set(finitepart(x), FBall(Rat(3, 2)))
    end
    r = parse(RealBall, "1.5 +/- 0.0009765625")
    z = parse(ComplexBall, "(1.5 +/- 0.0009765625) + (-2)*i")
    @test string(r) == "1.5 +/- 0.00098"
    @test string(z) == "(1.5 +/- 0.00098) + (-2)*i"
    @test identical(num_radius(r), RealBall(Rat(1, 1024)))
    for x in (r, z)
        @test value_text(x._carrier) == "(" * string(x) * " ; 0)"
        @test ball_contains(parse(typeof(x), string(x)), x)
        @test identical(x, copy(x))
        @test is_canonical(x)
        @test exact_zero_fin(x._carrier)
        @test_throws MethodError finitepart(x)
    end
    @test try_parse(RealBall, "1.5 +/- 2^-10").status === PARSE
    rr = RealBall(Rat(1, 3); prec=192); ii = RealBall(Rat(-5, 7); prec=192)
    zz = ComplexBall(rr, ii)
    @test identical(component_copy(zz, :acb_get_real), rr)
    @test identical(component_copy(zz, :acb_get_imag), ii)
    @test exact_zero_fin(zz._carrier)
    f = fb("5 mod 18")
    a = Adele(r, f); c = CAdele(z, f)
    @test identical(realpart(a), r)
    @test identical(complexpart(c), z)
    @test equal_set(finitepart(a), f)
    @test equal_set(finitepart(c), f)
    @test string(Adele(f; prec=128)) == "(0 ; 5 mod 18)"
    aa = Adele(Rat(1, 3); prec=192); cc = CAdele(aa)
    @test equal_set(finitepart(cc), finitepart(aa))
    @test identical(component_copy(complexpart(cc), :acb_get_real), realpart(aa))
    @test string(component_copy(complexpart(cc), :acb_get_imag)) == "0"
    # Private invalid prefixes exercise C setters' DOMAIN and immediate failure cleanup.
    for (B, T, name) in ((RealBall, Adele, :arb_indeterminate), (ComplexBall, CAdele, :acb_indeterminate))
        b = B(0)
        GC.@preserve b begin
            p = Libdl.dlsym(Adelefeld.LIBHANDLE[], name)
            ccall(p, Cvoid, (Ptr{Cvoid},), Adelefeld._ptr(b._carrier))
        end
        live = Adelefeld.diagnostics().live
        e = try; T(b, f); catch err; err; end
        @test e isa AdfError && e.status === DOMAIN
        @test Adelefeld.diagnostics().live == live
        close(b)
    end
end

@testset "SPEC 4.3 rows through Adele at prec 64" begin
    a(c, s) = Adele(RealBall(Rat(c); prec=64), fb(s))
    rows = [
        (a(3, "3 mod 12") + a(5, "5 mod 18"), "2 mod 6", 8//1),
        (a(3, "3 mod 12") * a(5, "5 mod 18"), "3 mod 6", 15//1),
        (Rat(12) * a(5, "5 mod 18"), "60 mod 216", 60//1),
        (Rat(1, 3) * a(5, "5 mod 18"), "5/3 mod 6", 5//3),
        (a(1//2, "1/2 mod 8") * a(2//3, "2/3 mod 9"), "0 mod 1/6", 1//3),
    ]
    for (x, fin, q) in rows
        @test equal_set(finitepart(x), fb(fin))
        @test num_contains(realpart(x), q)
        @test is_canonical(x)
    end
    x = a(3, "3 mod 12")
    @test equal_set(finitepart(x - x), fb("0 mod 12"))
    @test !equal_set(finitepart(x - x), FBall(0))
    @test num_contains(realpart(x - x), 0)
    @test num_contains(realpart(Rat(2) - x), -1)
end

@testset "header precision rules and exactness" begin
    for T in (Adele, CAdele)
        part = T === Adele ? realpart : complexpart
        dy = Rat(big(2)^90 + 1)
        lo = T(dy; prec=64); hi = T(dy; prec=128)
        @test !identical(lo, hi)
        @test equal_set(finitepart(lo), finitepart(hi))
        # At sufficient digits the exactly representable dyadic prints without a radius.
        @test !occursin("+/-", value_text(part(hi); digits=50))
        @test occursin("+/-", value_text(part(lo); digits=50))
        @test identical(T(Rat(1, 3); prec=-100), T(Rat(1, 3); prec=2))
        txt = T === Adele ? "(1208925819614629174706177 ; 2 mod 7)" :
                            "((1208925819614629174706177) + (0)*i ; 2 mod 7)"
        @test !identical(parse(T, txt; prec=64), parse(T, txt; prec=128))
        @test identical(parse(T, txt; prec=0), parse(T, txt; prec=2))
        x = T(Rat(1, 3); prec=192); y = T(Rat(5, 7); prec=192)
        for op in (add, sub, mul)
            low = op(x, y; prec=16); high = op(x, y; prec=128)
            @test !identical(low, high)
            @test equal_set(finitepart(low), finitepart(high))
            @test identical(op(x, y; prec=-1), op(x, y; prec=2))
        end
        for op in (add, sub, mul, divexact)
            @test !identical(op(x, Rat(5, 7); prec=16), op(x, Rat(5, 7); prec=128))
            @test identical(op(x, Rat(5, 7); prec=1), op(x, Rat(5, 7); prec=2))
        end
        @test identical(-(-x), x)
        @test_throws MethodError try_neg(x; prec=128)
        @test equal_set(finitepart(T(Rat(1, 3); prec=64)), FBall(Rat(1, 3)))
    end
    @test string(num_radius(realpart(Adele(Rat(3, 2); prec=64)))) == "0"
    @test num_contains(realpart(Adele(Rat(1, 3); prec=64)), 1//3)
    @test string(num_radius(realpart(Adele(Rat(1, 3); prec=64)))) != "0"
    for B in (RealBall, ComplexBall)
        @test !occursin("+/-", string(B(Rat(3, 2); prec=64)))
        @test occursin("+/-", string(B(Rat(1, 3); prec=64)))
        x = B(Rat(1, 3); prec=192); y = B(Rat(5, 7); prec=192)
        for op in (add, sub, mul)
            @test !identical(op(x, y; prec=16), op(x, y; prec=128))
        end
        @test identical(-(-x), x)
    end
    # Only SPEC 4.4 policy 1, tight, is offered by adele.h:19-23. No scaled/cap surface here.
    x = Adele(RealBall(1), fb("1 mod 2")) * Rat(1, 2)
    @test equal_set(finitepart(x), fb("1/2 mod 1"))
    @test !equal_set(finitepart(x), fb("1/2 mod 2"))
end

@testset "numerical arithmetic: 50 random pairs at BigFloat 256 bits" begin
    g = Lcg(0x897cb013abfe4411)
    setprecision(BigFloat, 256) do
        for _ in 1:50
            qs = [Int(next!(g) % 2_000_001) - 1_000_000 for _ in 1:4]
            p, q, u, v = Rational{BigInt}.(qs) ./ big(2)^20
            x = RealBall(Rat(p)); y = RealBall(Rat(q))
            z = ComplexBall(x, RealBall(Rat(u))); w = ComplexBall(y, RealBall(Rat(v)))
            bx, by = BigFloat(p), BigFloat(q)
            bz, bw = Complex{BigFloat}(bx, BigFloat(u)), Complex{BigFloat}(by, BigFloat(v))
            old = string.((x, y, z, w))
            for (op, bval, cval) in ((add, bx + by, bz + bw), (sub, bx - by, bz - bw),
                                     (mul, bx * by, bz * bw))
                r = op(x, y; prec=64); c = op(z, w; prec=64)
                @test num_contains(r, Rational{BigInt}(bval))
                want = ComplexBall(RealBall(Rat(Rational{BigInt}(real(cval))); prec=256),
                                   RealBall(Rat(Rational{BigInt}(imag(cval))); prec=256))
                @test ball_contains(c, want)
                @test exact_zero_fin(r._carrier)
                @test exact_zero_fin(c._carrier)
            end
            @test num_contains(-x, -p)
            @test exact_zero_fin((-z)._carrier)
            @test string.((x, y, z, w)) == old
        end
    end
end

@testset "scalar matrix, embedding, inputs unchanged and failure status" begin
    for T in (Adele, CAdele), scalar in (Rat(5, 7), 3, -2//3, big(2)^70, true)
        x = T(Rat(1, 3); prec=192); before = copy(x)
        q = scalar isa Rat ? jrat(scalar) : Rational{BigInt}(scalar)
        part = T === Adele ? realpart : complexpart
        for (op, got) in ((+, 1//3 + q), (-, 1//3 - q), (*, q/3))
            out = op(x, scalar)
            @test equal_set(finitepart(out), FBall(Rat(got)))
            if T === Adele
                @test num_contains(part(out), got)
            else
                @test ball_contains(part(out), ComplexBall(Rat(got); prec=256))
            end
        end
        @test equal_set(finitepart(scalar - x), FBall(Rat(q - 1//3)))
        @test equal_set(finitepart(scalar + x), FBall(Rat(q + 1//3)))
        @test equal_set(finitepart(scalar * x), FBall(Rat(q/3)))
        @test equal_set(finitepart(x / scalar), FBall(Rat((1//3)/q)))
        @test identical(x, before)
        zero = Rat(0); live = Adelefeld.diagnostics().live
        result = try_div(x, zero; prec=64)
        @test result.status === NOT_UNIT
        @test result.value === nothing
        @test Adelefeld.diagnostics().live == live
        err = try; x / zero; catch e; e; end
        @test err isa AdfError && err.status === NOT_UNIT
        @test try_add(x, 1//0).status === DOMAIN
        @test identical(x, before)
    end
    a = Adele(RealBall(3), fb("3 mod 12"))
    c = parse(CAdele, "((5) + (2)*i ; 5 mod 18)")
    for op in (add, sub, mul), (x, y) in ((a, c), (c, a))
        @test identical(op(x, y; prec=128),
                        op(x isa Adele ? CAdele(x) : x, y isa Adele ? CAdele(y) : y; prec=128))
    end
    @test string(complexpart(a + c)) == "(8) + (2)*i"
    @test string(complexpart(c - a)) == "(2) + (2)*i"
    @test string(complexpart(a * c)) == "(15) + (6)*i"
end

@testset "refusals and argument bounds before C" begin
    a = Adele(Rat(1)); c = CAdele(a)
    for x in (a, c), y in (FBall(1), 1.5, BigFloat(2), 1 + 2im)
        for op in (+, -, *)
            @test_throws ArgumentError op(x, y)
            @test_throws ArgumentError op(y, x)
        end
    end
    for x in (a, c), y in (a, c, Rat(1), 1, 1//2)
        @test_throws ArgumentError x == y
        @test_throws ArgumentError y == x
    end
    @test !isequal(a, copy(a))
    @test isequal(a, a)
    @test length(Set([a, copy(a), a])) == 2
    for x in (FBall(1), a, c)
        e = try; x^2; catch err; err; end
        @test e isa ArgumentError && occursin("pow", sprint(showerror, e))
        for call in (() -> x^0, () -> x^1, () -> x^-1, () -> x^-2)
            e = try; call(); catch err; err; end
            @test e isa ArgumentError && occursin("pow", sprint(showerror, e))
        end
    end
    for x in (a, c), y in (a, c, FBall(1))
        @test_throws ArgumentError x / y
    end
    for x in (RealBall(1), ComplexBall(1)), y in (FBall(1), a, c, Rat(1), 1, 1.0)
        @test_throws MethodError x + y
        @test_throws MethodError y * x
    end
    @test_throws MethodError RealBall(1) + ComplexBall(1)
    q = Rat(1)
    for prec in (1.5, 64//1, "64", nothing, big(2)^63, -big(2)^63 - 1)
        live = Adelefeld.diagnostics().live
        for T in (Adele, CAdele, RealBall, ComplexBall)
            @test_throws ArgumentError T(q; prec=prec)
            s = T === Adele ? "(1 ; 0)" : T === CAdele ? "((1) + (0)*i ; 0)" :
                T === RealBall ? "1" : "(1) + (0)*i"
            @test_throws ArgumentError parse(T, s; prec=prec)
        end
        @test_throws ArgumentError add(a, a; prec=prec)
        @test_throws ArgumentError mul(a, q; prec=prec)
        @test_throws ArgumentError divexact(a, q; prec=prec)
        @test Adelefeld.diagnostics().live == live
    end
    for digits in (0, 1_000_001, big(2)^63, 2.5)
        @test_throws ArgumentError value_text(a; digits=digits)
    end
    bad = Adele(Rat(1))
    @test_throws AssertionError RealBall(Adelefeld._Raw(), bad)
    @test_throws AssertionError Adelefeld._assert_zero_fin(CAdele(bad))
    close(bad)
end

@testset "text statuses and 30 adele/complex round trips" begin
    g = Lcg(0x489134312210ab01)
    for T in (Adele, CAdele)
        for s in ("", "(nan ; 0)", "(1 ; 1/0)", "(1 ; 2 mod 0/0)")
            text = T === CAdele ? replace(s, "(1 ;" => "((1) + (0)*i ;") : s
            @test try_parse(T, text).status === (occursin("/0", text) ? DOMAIN : PARSE)
        end
        text = T === Adele ? "(1 ; 0)" : "((1) + (0)*i ; 0)"
        @test try_parse(T, text; limits=TextLimits(max_len=ncodeunits(text)-1)).status === LIMIT
        @test try_parse(T, text * "\0").status === PARSE
        @test try_parse(T, text * "é"; limits=TextLimits(max_len=ncodeunits(text)+1)).status === LIMIT
    end
    # text.h:37-42, M1-D6: NULL printer with length 0 above the binary exponent limit.
    q = Rat(big(2)^100001)
    for T in (Adele, CAdele, RealBall, ComplexBall)
        x = T(q)
        live = Adelefeld.diagnostics().live
        r = try_value_text(x)
        @test r.status === LIMIT
        @test r.value === nothing
        e = try; value_text(x); catch err; err; end
        @test e isa AdfError && e.status === LIMIT
        @test Adelefeld.diagnostics().live == live
        @test is_canonical(x)
        close(x)
    end
    close(q)
    for _ in 1:30
        m = Int(next!(g) % 1_000_001) - 500_000
        n = Int(next!(g) % 1_000_001) - 500_000
        fin = "$(Int(next!(g) % 10_000)) mod $(1 + Int(next!(g) % 100_000))"
        for (T, s, part) in ((Adele, "($m.125 +/- 0.0009765625 ; $fin)", realpart),
                            (CAdele, "(($m.125 +/- 0.0009765625) + ($n.25 +/- 0.125)*i ; $fin)",
                             complexpart))
            x = parse(T, s; prec=128); y = parse(T, string(x); prec=128)
            @test is_canonical(y)
            @test equal_set(finitepart(x), finitepart(y))
            @test ball_contains(part(y), part(x))
        end
    end
end

@testset "closed owners, projections survive and GC" begin
    for T in (Adele, CAdele, RealBall, ComplexBall)
        x = T(Rat(1, 3)); y = copy(x)
        @test identical(x, y)
        close(x); close(x); finalize(x)
        live = Adelefeld.diagnostics().live
        for f in (() -> copy(x), () -> string(x), () -> is_canonical(x), () -> identical(x, y),
                  () -> x + y, () -> -x, () -> x * y)
            @test_throws InvalidStateException f()
            @test Adelefeld.diagnostics().live == live
        end
        @test is_canonical(y)
        if T === Adele || T === CAdele
            @test_throws InvalidStateException finitepart(x)
            @test_throws InvalidStateException T === Adele ? realpart(x) : complexpart(x)
            part = T === Adele ? realpart : complexpart
            p = part(y); fin = finitepart(y); close(y)
            @test is_canonical(p)
            @test equal_set(fin, FBall(Rat(1, 3)))
            close(p); close(fin)
        else
            close(y)
        end
    end
    q = Rat(1); close(q)
    for T in (Adele, CAdele, RealBall, ComplexBall)
        live = Adelefeld.diagnostics().live
        @test_throws InvalidStateException T(q)
        @test Adelefeld.diagnostics().live == live
    end
    for T in (Adele, CAdele)
        x = T(1); r = Rat(2); close(x)
        for f in (() -> try_add(x, r), () -> try_add(r, x), () -> try_sub(x, r),
                  () -> try_sub(r, x), () -> try_mul(x, r), () -> try_mul(r, x), () -> try_div(x, r))
            live = Adelefeld.diagnostics().live
            @test_throws InvalidStateException f()
            @test Adelefeld.diagnostics().live == live
        end
        x = T(1); close(r)
        for f in (() -> x + r, () -> r - x, () -> x * r, () -> x / r)
            live = Adelefeld.diagnostics().live
            @test_throws InvalidStateException f()
            @test Adelefeld.diagnostics().live == live
        end
        close(x)
    end
    a = Adele(1); close(a)
    live = Adelefeld.diagnostics().live
    @test_throws InvalidStateException CAdele(a)
    @test Adelefeld.diagnostics().live == live
    r = RealBall(1); z = ComplexBall(1); f = FBall(1)
    close(f)
    for call in (() -> Adele(r, f), () -> CAdele(z, f))
        live = Adelefeld.diagnostics().live
        @test_throws InvalidStateException call()
        @test Adelefeld.diagnostics().live == live
    end
    close(r); close(z)
    f = FBall(1)
    for call in (() -> Adele(r, f), () -> CAdele(z, f), () -> ComplexBall(r, r))
        live = Adelefeld.diagnostics().live
        @test_throws InvalidStateException call()
        @test Adelefeld.diagnostics().live == live
    end
    close(f)
    # The outer testset still roots some earlier inputs. Test zero live owners in a fresh process.
    code = """
        using Adelefeld
        function churn(T)
            for i in 1:10_000
                x = T(i); y = copy(x); x + y; -x
            end
        end
        println("BASELINE=", Adelefeld.diagnostics().live)
        for T in (Adele, CAdele, RealBall, ComplexBall)
            churn(T)
            GC.gc(); GC.gc(); GC.gc(); GC.gc()
            d = Adelefeld.diagnostics()
            println(T, " LIVE=", d.live, " ERRORS=", d.finalizer_errors)
        end
        """
    julia = Base.julia_cmd()
    out = read(`timeout 90 $julia --startup-file=no --project=$PKG -e $code`, String)
    print(out)
    @test occursin("BASELINE=0", out)
    for T in (Adele, CAdele, RealBall, ComplexBall)
        @test occursin("$T LIVE=0", out)
        @test occursin("$T LIVE=0 ERRORS=0", out)
    end

end

@testset "adele layout rejection in separate processes" begin
    lib = Adelefeld.diagnostics().library
    script = joinpath(@__DIR__, "load_fault.jl")
    src = joinpath(@__DIR__, "fixtures", "fault.c")
    cc = Sys.which("cc")
    dir = mktempdir()
    julia = Base.julia_cmd()
    for (macro_name, suffix, observed) in
        (("FAULT_ADELE_SIZE", "adele", 104), ("FAULT_ADELE_ALIGN", "adele", 16),
         ("FAULT_CADELE_SIZE", "cadele", 152), ("FAULT_CADELE_ALIGN", "cadele", 16))
        so = joinpath(dir, lowercase(macro_name) * ".so")
        run(`timeout 30 $cc -shared -fPIC -O1 -DFAULT_NAME -D$macro_name $src -o $so
             -Wl,--no-as-needed $lib -Wl,-rpath,$(dirname(lib))`)
        env = copy(ENV); env["ADELEFELD_LIB"] = so
        out = IOBuffer()
        p = run(pipeline(setenv(`timeout 30 $julia --startup-file=no --project=$PKG $script $so`, env);
                         stdout=out, stderr=out); wait=false)
        wait(p)
        text = String(take!(out))
        @test p.exitcode == 0
        @test occursin("layout", text)
        query = occursin("SIZE", macro_name) ? "adf_sizeof_" : "adf_alignof_"
        @test occursin(query * suffix, text)
        @test occursin(string(observed), text)
        @test occursin("INIT_CALLS=0", text)
        @test occursin("LOADED=false", text)
        @test !occursin("AFTER=allocated", text)
    end
    rm(dir; recursive=true)
end

@testset "invalid prec precedes even the default-limit C call" begin
    lib = Adelefeld.diagnostics().library
    src = joinpath(@__DIR__, "fixtures", "fault.c")
    cc = Sys.which("cc")
    dir = mktempdir()
    so = joinpath(dir, "count-limits.so")
    run(`timeout 30 $cc -shared -fPIC -O1 -DFAULT_MISSING -DCOUNT_LIMITS $src -o $so
         -Wl,--no-as-needed $lib -Wl,-rpath,$(dirname(lib))`)
    code = """
        using Adelefeld, Libdl
        errors = 0
        for T in (Adele, CAdele, RealBall, ComplexBall),
            prec in (1.5, 64//1, "64", nothing, big(2)^63, -big(2)^63-1)
            try
                parse(T, "invalid"; prec)
            catch e
                global errors += e isa ArgumentError
            end
        end
        f = Libdl.dlsym(Adelefeld.LIBHANDLE[], :fixture_limits_calls)
        println("ERRORS=", errors, " LIMIT_CALLS=", ccall(f, Cint, ()),
                " LIVE=", Adelefeld.diagnostics().live)
        """
    env = copy(ENV); env["ADELEFELD_LIB"] = so
    julia = Base.julia_cmd()
    out = IOBuffer()
    p = run(pipeline(setenv(`timeout 30 $julia --startup-file=no --project=$PKG -e $code`, env);
                     stdout=out, stderr=out); wait=false)
    wait(p)
    text = String(take!(out))
    print(text)
    @test p.exitcode == 0
    @test occursin("ERRORS=24", text)
    @test occursin("LIMIT_CALLS=0", text)
    @test occursin("LIVE=0", text)
    rm(dir; recursive=true)
end

end
finally
    GC.enable(previous_gc)
    GC.gc()
end

# ---- Step J-a.4: text classification, dumps and full-ball reconstruction ----
previous_gc4 = GC.enable(false)
try
@testset "Adelefeld step J-a.4" begin

@testset "TextKind literal ABI and closed language dispatch" begin
    @test isdefined(Adelefeld, :TextKind)
    if isdefined(Adelefeld, :TextKind)
        # No adf_text_kind_str exists. Literal names/numbers: text.h:122-135.
        kinds = instances(TextKind)
        @test sizeof(TextKind) == 4
        @test [Int(k) for k in kinds] == collect(0:12)
        @test string.(kinds) == (:TEXT_RAT, :TEXT_FBALL, :TEXT_ADELE, :TEXT_CADELE, :TEXT_UCOSET,
            :TEXT_IDELE, :TEXT_IDCLASS, :TEXT_LBALL, :TEXT_SBALL, :TEXT_QCLASS, :TEXT_FFUN,
            :TEXT_RFUN, :TEXT_CHAR) .|> string
        @test (:text_kind, 4, 4) in [(r.suffix, r.size, r.align) for r in Adelefeld.LAYOUT_ROWS]
        for raw in (-1, 13)
            @test_throws AdfAbiError Adelefeld.to_text_kind(Cint(raw))
        end
    end
    # conventions 9.2 start symbols; unsupported kinds are classified without semantic parsing.
    texts = ["7/3", "2 mod 6", "(1 ; 0)", "((1) + (0.5)*i ; 2 mod 6)", "[5 mod 6]",
             "(1 ; 1 * [1])", "<1 ; [1]>", "[p=5: 3 + O(5^4)]", "{}", "(0.5 ; 0) + Q",
             "ffun(D=1, M=1; (0) + (0)*i)", "rfun()", "char(q=4, n=3, s=(0) + (0)*i)"]
    for (i, s) in enumerate(texts)
        @testset "kind $(i-1)" begin
            r = try_readvalue(s)
            @test r.details.kind === TextKind(i-1)
            @test r.where === nothing
            if i <= 4
                T = (Rat, FBall, Adele, CAdele)[i]
                @test r.status === OK
                @test r isa AdfResult{T}
                @test r.value isa T
                typed = parse(T, s)
                @test identical(r.value, typed)
                @test identical(readvalue(s), typed)
                close(r.value); close(typed)
            else
                @test r.status === Adelefeld.UNSUPPORTED
                @test r.value === nothing
                e = try; readvalue(s); catch err; err; end
                @test e isa AdfError && e.status === Adelefeld.UNSUPPORTED
                @test e.result.details.kind === TextKind(i-1)
            end
        end
    end
    # Classification checks only stages 1-3, then supported typed readers apply later stages.
    for (s, st, lim) in (("", PARSE, TextLimits()), ("1\0", PARSE, TextLimits()),
                         ("é1", LIMIT, TextLimits(max_len=2)), ("1/", PARSE, TextLimits()),
                         ("1/0", DOMAIN, TextLimits()), ("(1e100001 ; 0)", LIMIT, TextLimits()),
                         ("(1e100001 ; 0) junk", PARSE, TextLimits()))
        r = try_readvalue(s; limits=lim)
        @test r.status === st
        @test r.value === nothing
        if st === PARSE || s == "é1"
            @test !haskey(r.details, :kind)  # C did not write kind on a classifier failure.
        else
            @test haskey(r.details, :kind)
        end
    end
    # Syntax-only classification of an unsupported type stops before stage 4 or 6.
    for s in ("[p=4: 1]", "[p=5: 1 + O(5^999999999)]", "char(q=0, n=0, s=(0) + (0)*i)")
        @test try_readvalue(s).status === Adelefeld.UNSUPPORTED
    end
    @test readvalue(SubString("x7/3", 2)) isa Rat
    for prec in (1.5, "64", nothing, big(2)^63, -big(2)^63-1)
        @test_throws ArgumentError try_readvalue("1"; prec)
    end
    for s in ("(1208925819614629174706177 ; 0)", "((1208925819614629174706177) + (0)*i ; 0)")
        x = readvalue(s; prec=16); y = readvalue(s; prec=128)
        @test !identical(x, y)
        @test identical(readvalue(s; prec=0), readvalue(s; prec=2))
        close(x); close(y)
    end
    # The five result texts of SPEC 4.3: docs/SPEC.md:244-248.
    for s in ("2 mod 6", "3 mod 6", "60 mod 216", "5/3 mod 6", "0 mod 1/6")
        x = readvalue(s)
        @test x isa FBall
        @test identical(x, parse(FBall, s))
        close(x)
    end
end

@testset "digits precondition and IOContext" begin
    # Preserve api-ja.md finding 5: digits belongs only to C printers with numerical coordinates.
    for T in (Rat, FBall)
        x = T(1//3)
        @test_throws MethodError try_value_text(x; digits=5)
        @test_throws MethodError value_text(x; digits=5)
        @test sprint(show, x; context=:adf_digits => 5) == value_text(x)
        close(x)
    end
    for T in (Adele, CAdele, RealBall, ComplexBall)
        x = T(1//3)
        for d in (1, 5, 20, 1_000_000)
            s = value_text(x; digits=d)
            @test try_value_text(x; digits=d).status === OK
            io = IOBuffer()
            show(IOContext(io, :adf_digits => d), x)
            @test String(take!(io)) == s
            @test sprint(show, MIME"text/plain"(), x; context=:adf_digits => d) == s
        end
        for d in (0, -1, 1_000_001, big(2)^63, 1.5, 20//1, "20", nothing)
            @test_throws ArgumentError try_value_text(x; digits=d)
            @test_throws ArgumentError show(IOContext(IOBuffer(), :adf_digits => d), x)
        end
        @test value_text(x; digits=1) != value_text(x; digits=20)
        close(x)
    end
end

@testset "dump fixture oracles and exact identity" begin
    # Self-check this round-trip oracle on a same-text, different-bits pair below.
    dump_identity_oracle(x, y) = identical(x, y)
    # First four lines: tests/driver/13_dump.cmd:28-34 and .out:8-11.
    # 7/3 -> num=7,den=3; 2+6 Zhat -> A=2,H=6,d=1; 1=1*2^0; 0.5=1*2^-1.
    # Fifth line is the same fixture's load at :41, derived from its exact dyadic fields.
    cases = [(Rat, "7/3", "adf1 Q rat 7 3"),
             (FBall, "(* ; 2 mod 6)", "adf1 Q fball g 2 6 1"),
             (Adele, "(1 ; 0)", "adf1 Q adele 1 1 0 0 0 g 0 0 1"),
             (CAdele, "((1) + (0.5)*i ; 2 mod 6)", "adf1 Q cadele 1 1 0 0 0 1 -1 0 0 g 2 6 1"),
             (Adele, "(0.5 ; 0)", "adf1 Q adele 1 1 -1 0 0 g 0 0 1")]
    for (T, s, want) in cases
        x = parse(T, s)
        @test dump_text(x) == want
        @test try_dump_text(x).status === OK
        @test identical(load_dump(T, want), x)
        @test dump_text(load_dump(T, want)) == want
        close(x)
    end
    # Fixed-generator random values: 30 for each type, including exact and nonzero-radius arbs.
    g = Lcg(0x548f30e56b98a2c1)
    for i in 1:30
        n = Int(next!(g) % 200001) - 100000
        d = 1 + Int(next!(g) % 1000)
        rn = i % 5 == 0 ? 0 : 1 + Int(next!(g) % 100000)
        rd = 1 + Int(next!(g) % 1000)
        q = Rat(n, d); r = Rat(rn, rd)
        f = FBall(q, r)
        re = RealBall(q; prec=64 + i)
        im = RealBall(r; prec=80 + i)
        z = ComplexBall(re, im)
        a = Adele(re, f); c = CAdele(z, f)
        for x in (q, f, a, c)
            before = copy(x)
            text = dump_text(x)
            loaded = try_load_dump(typeof(x), text)
            @test loaded.status === OK
            @test loaded.value isa typeof(x)
            @test dump_identity_oracle(loaded.value, x)  # Calls identical, not value-text comparison.
            @test dump_text(loaded.value) == text
            @test identical(x, before)
            @test try_load_dump(typeof(x), text; limits=TextLimits(max_len=ncodeunits(text)-1)).status === LIMIT
            close(before); close(loaded.value)
        end
        close.((q, r, f, re, im, z, a, c))
    end
    # A decimal enclosure can hide changes to the stored numerical ball. It cannot test dump identity.
    x = Adele(1//3; prec=192); y = Adele(1//3; prec=256)
    @test value_text(x; digits=5) == value_text(y; digits=5)
    @test value_text(x) == value_text(y)
    @test !dump_identity_oracle(x, y)
    @test dump_text(x) != dump_text(y)
    close(x); close(y)
end

@testset "strict load stages, limits and context refusal" begin
    for T in (Rat, FBall, Adele, CAdele)
        x = T(1)
        s = dump_text(x)
        lim = TextLimits(max_len=ncodeunits(s))
        @test identical(load_dump(T, s; limits=lim), x)
        for (text, st) in ((s * " ", PARSE), (s * "\0", PARSE), (s * "é", PARSE),
                           (replace(s, "adf1" => "adf2"), Adelefeld.UNSUPPORTED),
                           (replace(s, " Q " => " K "), Adelefeld.UNSUPPORTED),
                           (s, LIMIT))
            r = try_load_dump(T, text; limits=text == s ? TextLimits(max_len=1) : TextLimits())
            @test r.status === st
            @test r.value === nothing
            @test isempty(r.details)
            e = try; unwrap(r); catch err; err; end
            @test e isa AdfError && e.status === st
        end
        @test try_load_dump(T, "é"; limits=TextLimits(max_len=0)).status === LIMIT
        @test try_load_dump(T, "adf1 Q rat 07 3").status === PARSE
        # A different body is a typed grammar failure (src/dump.c:1488-1489), before semantics.
        bad = T === Rat ? "adf1 Q rat 2 4" : T === FBall ? "adf1 Q fball g 2 4 2" :
              T === Adele ? "adf1 Q adele 1 1 0 0 0 g 2 4 2" :
              "adf1 Q cadele 1 1 0 0 0 0 0 0 0 g 2 4 2"
        @test try_load_dump(T, bad).status === DOMAIN
        close(x)
    end
    # dump.h:43-51: local occurrences need a live matching context; NULL gives DOMAIN.
    for (T, text) in ((FBall, "adf1 Q fball l 1 6 2 2 3 0 2"),
                      (Adele, "adf1 Q adele 1 1 0 0 0 l 1 6 2 2 3 0 2"),
                      (CAdele, "adf1 Q cadele 1 1 0 0 0 0 0 0 0 l 1 6 2 2 3 0 2"))
        @test try_load_dump(T, text).status === DOMAIN
        @test try_load_dump(T, text; limits=TextLimits(max_items=1)).status === LIMIT
    end
end

@testset "reconstruction: exact closed progression, statuses, inputs unchanged" begin
    # Own oracle: for q=a+N*k, integers k run from ceil((lo-a)/N) to floor((hi-a)/N).
    cases = [("1 mod 2", 1//1, 1//1, OK, "1"), ("1 mod 2", 0//1, 10//1, Adelefeld.NOT_UNIQUE, ""),
             ("1 mod 2", 0//1, 0//1, Adelefeld.NO_SOLUTION, ""),
             ("1 mod 2", 2//1, 0//1, Adelefeld.NO_SOLUTION, ""),
             ("146 mod 1000", 0//1, 50//1, Adelefeld.NO_SOLUTION, ""),
             ("146 mod 1000", 100//1, 200//1, OK, "146"),
             ("5/3 mod 6", 5//3, 5//3, OK, "5/3"),
             ("5/3 mod 6", -13//3, -13//3, OK, "-13/3"),
             ("(* ; 22/7)", 0//1, 50//1, OK, "22/7"),
             ("(* ; 22/7)", 0//1, 3//1, Adelefeld.NO_SOLUTION, "")]
    for (text, l, h, st, want) in cases
        x = fb(text); lo = Rat(l); hi = Rat(h)
        originals = copy.((x, lo, hi))
        live = Adelefeld.diagnostics().live
        r = try_reconstruct(x, lo, hi)
        @test r isa AdfResult{Rat}
        @test r.status === st
        @test r.where === nothing
        @test isempty(r.details)  # recon.h:24-27: no output written on any failing status.
        if st === OK
            @test string(r.value) == want
            close(r.value)
            q = reconstruct(x, lo, hi)
            @test string(q) == want
            close(q)
        else
            @test r.value === nothing
            e = try; reconstruct(x, lo, hi); catch err; err; end
            @test e isa AdfError && e.status === st
            @test e.result.details == r.details
        end
        @test all(identical(a, b) for (a, b) in zip((x, lo, hi), originals))
        @test Adelefeld.diagnostics().live == live
        close.((x, lo, hi, originals...))
    end
    # First three are the reconstruction rows of tests/driver/06_pairs.cmd:131-133.
    for (text, st, want) in (("(2 ; 2 mod 1)", OK, "2"), ("(1 ; 1)", OK, "1"),
                            ("(0 ; 3 mod 12)", Adelefeld.NO_SOLUTION, ""),
                            ("(5 +/- 5 ; 1 mod 2)", Adelefeld.NOT_UNIQUE, ""),
                            ("(1.5 +/- 0.5 ; 0 mod 2)", OK, "2"))
        x = parse(Adele, text); before = copy(x)
        r = try_reconstruct(x)
        @test r.status === st
        @test isempty(r.details)
        @test identical(x, before)
        if st === OK
            @test string(r.value) == want
            @test string(reconstruct(x)) == want
            close(r.value)
        else
            @test r.value === nothing
        end
        close(x); close(before)
    end
    # recon.h:16-24,41-43: bound is on binary exponents; dumps preserve them without huge shifts.
    for arb in ("1 100001 0 0", "1 -100003 0 0", "0 0 1 100001", "0 0 1 -100003")
        x = load_dump(Adele, "adf1 Q adele 1 $arb g 0 1 1")
        before = copy(x); live = Adelefeld.diagnostics().live
        r = try_reconstruct(x)
        @test r.status === LIMIT
        @test r.value === nothing
        @test isempty(r.details)
        @test identical(x, before)
        @test Adelefeld.diagnostics().live == live
        close(x); close(before)
    end
    # Release-build courtesy only: recon.h:27-29, SPEC M1-D11. Not a canonical public input.
    # arb_indeterminate sets NaN midpoint and infinite radius: refs/src/flint-3.0.1/arb.rst:241-243.
    x = Adele(0)
    GC.@preserve x ccall(Libdl.dlsym(Adelefeld.LIBHANDLE[], :arb_indeterminate), Cvoid,
                         (Ptr{Cvoid},), Adelefeld._ptr(x))
    live = Adelefeld.diagnostics().live
    r = try_reconstruct(x)
    @test r.status === DOMAIN
    @test r.value === nothing
    @test isempty(r.details)
    @test Adelefeld.diagnostics().live == live
    close(x)
    # NOT_DETERMINED is not a status of recon.h:25-27. No limited search argument exists here.
    @test_throws MethodError try_reconstruct(CAdele(0))
    @test_throws MethodError try_reconstruct(fb("1 mod 2"), Rat(0), Rat(10); limit=0)
end

@testset "closed handles refuse without allocation" begin
    for T in (Rat, FBall, Adele, CAdele)
        x = T(1); close(x)
        live = Adelefeld.diagnostics().live
        for call in (() -> dump_text(x), () -> try_dump_text(x), () -> value_text(x),
                     () -> show(IOContext(IOBuffer(), :adf_digits => 5), x))
            @test_throws InvalidStateException call()
            @test Adelefeld.diagnostics().live == live
        end
    end
    f = fb("1 mod 2"); lo = Rat(0); hi = Rat(1); a = Adele(1)
    for x in (f, lo, hi)
        args = copy.((f, lo, hi))
        index = x === f ? 1 : x === lo ? 2 : 3
        close(args[index])
        live = Adelefeld.diagnostics().live
        @test_throws InvalidStateException try_reconstruct(args...)
        @test Adelefeld.diagnostics().live == live
        close.(args)
    end
    close(a)
    live = Adelefeld.diagnostics().live
    @test_throws InvalidStateException try_reconstruct(a)
    @test Adelefeld.diagnostics().live == live
    close.((f, lo, hi))
end

@testset "enum layout rejection before init" begin
    lib = Adelefeld.diagnostics().library
    script = joinpath(@__DIR__, "load_fault.jl")
    src = joinpath(@__DIR__, "fixtures", "fault.c")
    cc = Sys.which("cc"); julia = Base.julia_cmd()
    dir = mktempdir()
    for (macro_name, query) in (("FAULT_TEXT_KIND_SIZE", "adf_sizeof_text_kind"),
                                ("FAULT_TEXT_KIND_ALIGN", "adf_alignof_text_kind"))
        so = joinpath(dir, lowercase(macro_name) * ".so")
        run(`timeout 30 $cc -shared -fPIC -O1 -DFAULT_NAME -D$macro_name $src -o $so
             -Wl,--no-as-needed $lib -Wl,-rpath,$(dirname(lib))`)
        env = copy(ENV); env["ADELEFELD_LIB"] = so
        out = IOBuffer()
        p = run(pipeline(setenv(`timeout 30 $julia --startup-file=no --project=$PKG $script $so`, env);
                         stdout=out, stderr=out); wait=false)
        wait(p)
        text = String(take!(out))
        @test p.exitcode == 0
        @test occursin(query, text)
        @test occursin("layout", text)
        @test occursin("4", text) && occursin("8", text)
        @test occursin("LOADED=false", text)
        @test occursin("INIT_CALLS=0", text)
        @test !occursin("AFTER=allocated", text)
    end
    rm(dir; recursive=true)
end

@testset "counted frees, NULL contexts and guards before C" begin
    lib = Adelefeld.diagnostics().library
    src = joinpath(@__DIR__, "fixtures", "fault.c")
    cc = Sys.which("cc"); julia = Base.julia_cmd()
    dir = mktempdir(); so = joinpath(dir, "count-ja4.so")
    run(`timeout 30 $cc -shared -fPIC -O1 -DFAULT_MISSING -DCOUNT_LIMITS -DCOUNT_JA4 $src -o $so
         -Wl,--no-as-needed $lib -Wl,-rpath,$(dirname(lib)) -ldl`)
    code = """
        using Adelefeld, Libdl, Test
        using Adelefeld: OK, LIMIT, PARSE
        count(s) = ccall(Libdl.dlsym(Adelefeld.LIBHANDLE[], s), Cint, ())
        mode(m) = ccall(Libdl.dlsym(Adelefeld.LIBHANDLE[], :fixture_set_dump_mode), Cvoid, (Cint,), m)
        @testset "C call counters" begin
            q = Rat(1); f = FBall(q); a = Adele(q); c = CAdele(q)
            xs = (q, f, a, c)
            texts = String[]
            for m in 0:3
                mode(m)
                for x in xs
                    if m == 3
                        @test_throws ErrorException try_dump_text(x)
                    else
                        r = try_dump_text(x)
                        @test r.status === (m == 0 ? OK : LIMIT)
                        m == 0 && push!(texts, r.value)
                    end
                end
            end
            @test count(:fixture_dump_calls) == 16
            @test count(:fixture_free_calls) == 16
            println("DUMPS=", count(:fixture_dump_calls), " FREES=", count(:fixture_free_calls))
            mode(0)
            for (x, s) in zip(xs, texts)
                y = load_dump(typeof(x), s)
                @test identical(y, x)
                close(y)
                r = try_load_dump(typeof(x), s; limits=TextLimits(max_len=0))
                @test r.status === LIMIT
                @test r.value === nothing
            end
            @test count(:fixture_context_calls) == 0
            println("CONTEXTS=", count(:fixture_context_calls))
            re = RealBall(q); im = ComplexBall(q)
            before = count(:fixture_print_calls)
            errors = 0
            for x in (a, c, re, im), d in (0, -1, 1000001, big(2)^63, 1.5, 20//1, "20", nothing)
                for call in (() -> try_value_text(x; digits=d),
                             () -> show(IOContext(IOBuffer(), :adf_digits => d), x))
                    try
                        call()
                    catch e
                        errors += e isa ArgumentError
                    end
                end
            end
            @test errors == 64
            @test count(:fixture_print_calls) == before
            println("DIGIT_ERRORS=", errors, " PRINT_CALLS=", count(:fixture_print_calls)-before)
            before_lim = count(:fixture_limits_calls)
            before_cls = count(:fixture_classify_calls)
            errors = 0
            for prec in (1.5, "64", nothing, big(2)^63, -big(2)^63-1)
                try
                    try_readvalue("1"; prec)
                catch e
                    errors += e isa ArgumentError
                end
            end
            @test errors == 5
            @test count(:fixture_limits_calls) == before_lim
            @test count(:fixture_classify_calls) == before_cls
            println("PREC_ERRORS=", errors, " LIMIT_CALLS=", count(:fixture_limits_calls)-before_lim,
                    " CLASSIFY_CALLS=", count(:fixture_classify_calls)-before_cls)
            for s in ("", "x", "1\\0")
                r = try_readvalue(s)
                @test r.status === PARSE
                @test !haskey(r.details, :kind)
            end
            close.((xs..., re, im))
            @test Adelefeld.diagnostics().live == 0
            println("LIVE=", Adelefeld.diagnostics().live)
        end
        """
    env = copy(ENV); env["ADELEFELD_LIB"] = so; env["FIXTURE_CORE"] = lib
    out = IOBuffer()
    p = run(pipeline(setenv(`timeout 45 $julia --startup-file=no --project=$PKG -e $code`, env);
                     stdout=out, stderr=out); wait=false)
    wait(p)
    text = String(take!(out)); print(text)
    @test p.exitcode == 0
    @test occursin("DUMPS=16 FREES=16", text)
    @test occursin("CONTEXTS=0", text)
    @test occursin("DIGIT_ERRORS=64 PRINT_CALLS=0", text)
    @test occursin("PREC_ERRORS=5 LIMIT_CALLS=0 CLASSIFY_CALLS=0", text)
    @test occursin("LIVE=0", text)
    rm(dir; recursive=true)
end

@testset "10^4 dump round trips with automatic GC, zero owners" begin
    code = """
        using Adelefeld
        function churn()
            for i in 1:10_000
                T = (Rat, FBall, Adele, CAdele)[1 + i % 4]
                x = T(i//3)
                y = load_dump(T, dump_text(x))
                identical(x, y) || error("dump identity failed")
            end
        end
        println("BASELINE=", Adelefeld.diagnostics().live)
        churn()
        GC.gc(); GC.gc(); GC.gc(); GC.gc()
        d = Adelefeld.diagnostics()
        println("ROUNDTRIPS=10000 LIVE=", d.live, " ERRORS=", d.finalizer_errors)
        """
    julia = Base.julia_cmd()
    out = read(`timeout 90 $julia --startup-file=no --project=$PKG -e $code`, String)
    print(out)
    @test occursin("BASELINE=0", out)
    @test occursin("ROUNDTRIPS=10000 LIVE=0 ERRORS=0", out)
end

end
finally
    GC.enable(previous_gc4)
    GC.gc()
end

# ---- Step J-b.1: contexts, leases and the local backend (docs/api-j.md:126-141, 242-260) ----
# Contracts: include/adelefeld/modctx.h:11-35 (constructor rules), 57-101 (constructors, free), 105-112
# (accessors), 145-174 (local conversions); fball.h:13-21, 102-106, 196-197; docs/conventions.md:342-397;
# docs/SPEC.md:184-214 (different context pointers fall back to global), 285-298.
# Exact live and token counts: automatic GC is frozen here, as in J-a.3/J-a.4; the GC acceptance runs
# in a separate process with automatic GC enabled.
using Adelefeld: UNSUPPORTED, NOT_UNIT
previous_gc5 = GC.enable(false)
try
@testset "Adelefeld step J-b.1" begin

ctx_live() = Adelefeld.diagnostics().contexts
tokens(c) = Adelefeld.ctx_tokens(c)
# The C context pointer of a value, read through adf_fball_context (modctx.h:171-174).
cptr(x::FBall) = Adelefeld.context_pointer(x)
# The first n primes, by a sieve (65537 primes are below 821648, modctx.h:45-49).
function first_primes(n)
    lim = 821_700
    sieve = trues(lim)
    sieve[1] = false
    for i in 2:isqrt(lim)
        sieve[i] && (sieve[i*i:i:lim] .= false)
    end
    return UInt64.(findall(sieve)[1:n])
end

@testset "construction families on valid input (modctx.h:57-87, 105-112)" begin
    c = ModCtx(blocks=[8, 9])
    @test c isa ModCtx
    @test modulus(c) isa Rat
    @test modulus(c) == 72
    @test nblocks(c) == 2
    @test blocks(c) == UInt64[8, 9]
    r = try_modctx(blocks=[8, 9])
    @test r isa AdfResult{ModCtx}
    @test r.status === OK
    @test r.details.family === :blocks
    # The modulus from the accessor equals the product of the blocks, computed in Julia with BigInt.
    for b in ([8, 9], [16, 27, 5], [7], [2, 3, 5, 7, 11, 13, 17, 19, 23, 29],
              [typemax(UInt64), big(2)^61 - 1], UInt64[2^32 + 15, 2^31 - 1])
        c = ModCtx(blocks=b)
        @test modulus(c) == prod(BigInt.(b))
        @test blocks(c) == UInt64.(b)
        @test nblocks(c) == length(b)
    end
    # k = 0 is a context of modulus 1 without blocks (modctx.h:57-58).
    c = ModCtx(blocks=Int[])
    @test nblocks(c) == 0 && blocks(c) == UInt64[] && modulus(c) == 1
    # prime powers, in the order supplied (modctx.h:64-68).
    c = ModCtx(prime_powers=[2 => 3, 3 => 2])
    @test blocks(c) == UInt64[8, 9] && modulus(c) == 72
    @test blocks(ModCtx(prime_powers=[3 => 2, 2 => 3])) == UInt64[9, 8]
    @test blocks(ModCtx(prime_powers=[2 => 63])) == UInt64[UInt64(1) << 63]
    @test try_modctx(prime_powers=[2 => 3]).details.family === :prime_powers
    # modulus K: one block if 2 <= K < 2^64, none if K = 1 or K >= 2^64 (modctx.h:71-74).
    @test blocks(ModCtx(modulus=72)) == UInt64[72]
    @test blocks(ModCtx(modulus=big(2)^64 - 1)) == [typemax(UInt64)]
    for K in (1, big(2)^64, big(10)^30)
        c = ModCtx(modulus=K)
        @test nblocks(c) == 0
        @test modulus(c) == K
    end
    @test modulus(ModCtx(modulus=big(10)^30)) == big(10)^30
    # factorial n: the prime powers of n! in increasing order; n = 0, 1 without blocks (modctx.h:76-81).
    c = ModCtx(factorial=5)
    @test blocks(c) == UInt64[8, 3, 5] && modulus(c) == 120
    @test nblocks(ModCtx(factorial=0)) == 0 && nblocks(ModCtx(factorial=1)) == 0
    c = ModCtx(factorial=65)          # v_2(65!) = 63: every block fits a word
    @test modulus(c) == factorial(big(65))
    @test prod(BigInt.(blocks(c))) == factorial(big(65))
    # primorial power (n, e): blocks p^e, p <= n increasing (modctx.h:83-87).
    c = ModCtx(primorial_power=(5, 2))
    @test blocks(c) == UInt64[4, 9, 25] && modulus(c) == 900
    @test blocks(ModCtx(primorial_power=(2, 63))) == UInt64[UInt64(1) << 63]
    @test nblocks(ModCtx(primorial_power=(1, 3))) == 0
    @test nblocks(ModCtx(primorial_power=(5, 0))) == 0
    @test try_modctx(primorial_power=(5, 2)).details.family === :primorial_power
    @test try_modctx(factorial=5).details.family === :factorial
    @test try_modctx(modulus=6).details.family === :modulus
    # Input arrays are copied (modctx.h:20) and unchanged.
    b = [8, 9]
    c = ModCtx(blocks=b)
    @test b == [8, 9]
    b[1] = 5
    @test blocks(c) == UInt64[8, 9]
    @test sprint(show, ModCtx(blocks=[8, 9])) == "ModCtx(modulus=72, blocks=[8, 9])"
end

@testset "construction refusals (modctx.h:11-35, 57-87)" begin
    n0 = ctx_live()
    # Exactly one construction family (docs/api-j.md:255): ArgumentError before C.
    @test_throws ArgumentError try_modctx()
    @test_throws ArgumentError ModCtx()
    @test_throws ArgumentError try_modctx(blocks=[8], modulus=8)
    @test_throws ArgumentError ModCtx(factorial=3, primorial_power=(3, 1))
    @test_throws ArgumentError try_modctx(blocks=[8], prime_powers=[2 => 3], modulus=8, factorial=4,
                                          primorial_power=(3, 1))
    # Words: a block must fit ulong (modctx.h:62); never a wrapped word (docs/api-j.md:154-155).
    @test_throws ArgumentError try_modctx(blocks=[big(2)^64])
    @test_throws ArgumentError try_modctx(blocks=[-1])
    @test_throws ArgumentError try_modctx(blocks=[1.5])
    @test_throws ArgumentError try_modctx(prime_powers=[2 => -1])
    @test_throws ArgumentError try_modctx(prime_powers=[(2, 3)])
    @test_throws ArgumentError try_modctx(factorial=-1)
    @test_throws ArgumentError try_modctx(primorial_power=5)
    @test_throws ArgumentError try_modctx(primorial_power=(5, -1))
    @test_throws ArgumentError try_modctx(modulus=1.0)
    @test ctx_live() == n0
    # DOMAIN: a block below 2, blocks not pairwise coprime (modctx.h:58-59).
    for b in ([1], [0], [6, 4], [8, 8], [8, 9, 1], [35, 77])
        r = try_modctx(blocks=b)
        @test r.status === DOMAIN
        @test r.value === nothing
        @test r.details == (family = :blocks,)
    end
    # DOMAIN: not prime, repeated prime, exponent 0; UNSUPPORTED: p^e >= 2^64 (modctx.h:64-68);
    # DOMAIN first, then UNSUPPORTED (modctx.h:31-32): a composite with an oversized power is DOMAIN.
    for (pp, st) in (([4 => 1], DOMAIN), ([2 => 0], DOMAIN), ([2 => 1, 2 => 2], DOMAIN), ([1 => 1], DOMAIN),
                     ([2 => 64], UNSUPPORTED), ([3 => 41], UNSUPPORTED), ([4 => 64], DOMAIN),
                     ([2 => 64, 4 => 1], DOMAIN))
        r = try_modctx(prime_powers=pp)
        @test r.status === st
        @test r.value === nothing
        @test r.details == (family = :prime_powers,)
    end
    # modulus K < 1 is DOMAIN (modctx.h:73).
    for K in (0, -5, -big(2)^70)
        @test try_modctx(modulus=K).status === DOMAIN
    end
    # factorial: UNSUPPORTED from n = 66 on (v_2(66!) = 64; modctx.h:78-80).
    @test try_modctx(factorial=66).status === UNSUPPORTED
    @test try_modctx(factorial=10^6).status === UNSUPPORTED
    @test try_modctx(factorial=typemax(UInt64)).status === UNSUPPORTED
    # primorial power: UNSUPPORTED if some p^e >= 2^64 (modctx.h:86), or more than 65536 blocks
    # (n >= ADF_MODCTX_MAX_PRIME = 821647, modctx.h:26-28, 45-49).
    @test try_modctx(primorial_power=(2, 64)).status === UNSUPPORTED
    @test try_modctx(primorial_power=(3, 41)).status === UNSUPPORTED
    @test try_modctx(primorial_power=(821647, 1)).status === UNSUPPORTED
    # More than ADF_MODCTX_MAX_BLOCKS = 65536 blocks is UNSUPPORTED (modctx.h:23-28, 48).
    ps = first_primes(65537)
    @test ps[end] == 821647
    @test try_modctx(blocks=ps).status === UNSUPPORTED
    @test try_modctx(prime_powers=[p => 1 for p in ps]).status === UNSUPPORTED
    # No constructor names LIMIT (modctx.h:57-87): none of the cases above is LIMIT; no context made.
    @test ctx_live() == n0
    # The throwing form carries the status and the whole result (decision J5).
    err = try ModCtx(blocks=[1]); nothing; catch e; e; end
    @test err isa AdfError
    @test err.status === DOMAIN
    @test err.result.details == (family = :blocks,)
    @test ctx_live() == n0
end

@testset "lease basics, before any later use of the value (context proof steps 1-5)" begin
    n0 = ctx_live()
    f0 = Adelefeld.diagnostics().context_frees
    ctx = ModCtx(blocks=[8, 9])
    @test tokens(ctx) == 1                                    # step 1: the owner token
    y = to_local(parse(FBall, "5 mod 72"), ctx)
    @test tokens(ctx) == 2                                    # step 2: one token for the value
    r = try_to_local(FBall(Rat(1)), ctx)                      # a failing call: its temporary's token returns
    @test r.status === DOMAIN
    @test tokens(ctx) == 2
    @test Adelefeld.diagnostics().context_frees == f0
    finalize(ctx)                                             # step 4: the owner token only
    @test tokens(ctx) == 1
    @test ctx_live() == n0 + 1
    @test Adelefeld.diagnostics().context_frees == f0         # step 5: no free while y lives
    # y is used only while its context has not been freed, so that a broken lease is reported by the
    # assertions above instead of a use after free; on a correct package the branch always runs.
    if Adelefeld.diagnostics().context_frees == f0
        @test value_text(y) == "(* ; 5 mod 72)"
        close(y)
        @test tokens(ctx) == 0
        @test Adelefeld.diagnostics().context_frees == f0 + 1   # exactly one free, at the last token
        @test ctx_live() == n0
    end
end

# A context in which all nine inexact balls of SPEC 4.3 are local values: c + R Zhat is local in K
# exactly when K/R and c K/R are integers (modctx.h:145-148). K = 2160 = 16 * 27 * 5.
ROWS_INEXACT = ["3 mod 12", "5 mod 18", "2 mod 6", "3 mod 6", "60 mod 216", "5/3 mod 6",
                      "1/2 mod 8", "2/3 mod 9", "0 mod 1/6"]

@testset "SPEC 4.3 rows to local and back (modctx.h:145-169)" begin
    ctx = ModCtx(blocks=[16, 27, 5])
    for s in ROWS_INEXACT
        x = parse(FBall, s)
        x0 = copy(x)
        r = try_to_local(x, ctx)
        @test r.status === OK
        @test r.details == (lost = false,)
        y = r.value
        @test is_local(y)
        @test !is_local(x)
        @test context(y) === ctx
        @test context(x) === nothing
        @test cptr(y) == Adelefeld.ctx_pointer(ctx)
        @test equal_set(x, y)
        @test !identical(x, y)                     # the backend differs (fball.h:102-106)
        @test is_canonical(y)
        # The value form does not record the backend (conventions 9.8 A11; text.h:37-38).
        @test value_text(y) == value_text(x) == "(* ; " * s * ")"
        g = to_global(y)
        @test !is_local(g)
        @test identical(g, x)                      # exact: to_global(to_local(x)) is x
        @test identical(x, x0)                     # input unchanged
        # The enclosing variant on a value that is local already: lost false, the same value.
        e = try_enclose_local(x, ctx)
        @test e.status === OK
        @test e.details == (lost = false,)
        @test identical(e.value, y)
        @test identical(to_local(x, ctx), y)
        @test identical(enclose_local(x, ctx), y)
    end
    # The five rows computed on local operands; sums of one context stay local (fball.h:196-197).
    L(s) = to_local(parse(FBall, s), ctx)
    rows = [(L("3 mod 12") + L("5 mod 18"), "2 mod 6"), (L("3 mod 12") * L("5 mod 18"), "3 mod 6"),
            (12 * L("5 mod 18"), "60 mod 216"), (Rat(1, 3) * L("5 mod 18"), "5/3 mod 6"),
            (L("1/2 mod 8") * L("2/3 mod 9"), "0 mod 1/6")]
    for (z, want) in rows
        w = parse(FBall, want)
        @test equal_set(z, w)
        @test value_text(z) == "(* ; " * want * ")"
        @test identical(to_global(z), w)
    end
    @test is_local(rows[1][1])
    @test context(rows[1][1]) === ctx
    # Exact values are never local (modctx.h:149-150, 159-160): DOMAIN, no value, empty details.
    for q in (Rat(12), Rat(1, 3), Rat(0))
        x = FBall(q)
        for r in (try_to_local(x, ctx), try_enclose_local(x, ctx))
            @test r.status === DOMAIN
            @test r.value === nothing
            @test r.details == NamedTuple()
        end
        @test_throws AdfError to_local(x, ctx)
        @test_throws AdfError enclose_local(x, ctx)
    end
end

@testset "lost: the enclosing variant (modctx.h:155-164)" begin
    # K = 1080 = 8 * 27 * 5: 1/2 mod 8 is not local (K/R = 135, c K/R = 135/2).
    ctx = ModCtx(blocks=[8, 27, 5])
    x = parse(FBall, "1/2 mod 8")
    r = try_to_local(x, ctx)
    @test r.status === DOMAIN && r.value === nothing && r.details == NamedTuple()
    e = try_enclose_local(x, ctx)
    @test e.status === OK
    @test e.details == (lost = true,)
    @test e.details.lost isa Bool
    y = e.value
    # d0 = lcm(135, 2) = 270; y = (135 + 1080 Zhat)/270 = 1/2 + 4 Zhat.
    @test is_local(y) && context(y) === ctx
    @test inside(x, y)                         # an enclosure of x
    @test !equal_set(x, y)
    @test equal_set(y, parse(FBall, "1/2 mod 4"))
    @test value_text(y) == "(* ; 1/2 mod 4)"
    @test identical(to_global(y), parse(FBall, "1/2 mod 4"))
    @test enclose_local(x, ctx) isa FBall
    @test_throws AdfError to_local(x, ctx)
    # A context without blocks serves only the scaled policy: UNSUPPORTED (modctx.h:151, 160).
    for c in (ModCtx(blocks=Int[]), ModCtx(modulus=1), ModCtx(modulus=big(2)^64), ModCtx(factorial=1))
        for r in (try_to_local(parse(FBall, "3 mod 12"), c), try_enclose_local(parse(FBall, "3 mod 12"), c))
            @test r.status === UNSUPPORTED
            @test r.value === nothing
            @test r.details == NamedTuple()
        end
    end
    # A one-block modulus context of 2^64 - 1 is an ordinary local context.
    c = ModCtx(modulus=big(2)^64 - 1)
    y = to_local(parse(FBall, "1 mod 3"), c)
    @test is_local(y) && value_text(y) == "(* ; 1 mod 3)"
end

@testset "same modulus, distinct contexts (SPEC 184-214, 285-298; conventions 4.6)" begin
    c1 = ModCtx(blocks=[8, 9])
    c2 = ModCtx(blocks=[8, 9])
    c3 = ModCtx(prime_powers=[2 => 3, 3 => 2])
    @test c1 !== c2
    @test modulus(c1) == modulus(c2) == modulus(c3)
    @test blocks(c1) == blocks(c2) == blocks(c3)
    @test Adelefeld.ctx_pointer(c1) != Adelefeld.ctx_pointer(c2)
    @test Adelefeld.ctx_pointer(c1) != Adelefeld.ctx_pointer(c3)
    x = parse(FBall, "5 mod 72")
    a = to_local(x, c1)
    b = to_local(x, c2)
    @test context(a) === c1 && context(b) === c2
    @test cptr(a) == Adelefeld.ctx_pointer(c1)
    @test cptr(b) == Adelefeld.ctx_pointer(c2)
    @test cptr(a) != cptr(b)
    # A value made in one is not a value of the other: a different context pointer, not identical,
    # although the sets are equal (fball.h:102-106; adf_fball_context, modctx.h:171-174).
    @test !identical(a, b)
    @test equal_set(a, b)
    # Two different context pointers: the finite operation falls back to global (fball.h:17-20).
    s = a + b
    @test !is_local(s)
    @test context(s) === nothing
    @test equal_set(s, x + x)
    @test identical(s, x + x)
    # One context pointer: local, in that context.
    a2 = to_local(parse(FBall, "7 mod 36"), c1)
    t = a + a2
    @test is_local(t) && context(t) === c1
    @test identical(to_global(t), x + parse(FBall, "7 mod 36"))
end

@testset "leases: tokens, roots and failure paths (docs/api-j.md:126-141)" begin
    n0 = ctx_live()
    ctx = ModCtx(blocks=[8, 9])
    @test ctx_live() == n0 + 1
    @test tokens(ctx) == 1                       # the owner token
    x = parse(FBall, "5 mod 72")
    y = to_local(x, ctx)
    @test tokens(ctx) == 2
    @test Adelefeld.roots(y) == [ctx]
    @test Adelefeld.roots(x) == ModCtx[]
    yc = copy(y)                                  # same context pointer (fball.h:86-88)
    @test identical(yc, y) && context(yc) === ctx
    @test tokens(ctx) == 3
    close(yc)
    @test tokens(ctx) == 2
    # One token per distinct context state: y + y holds one token, not two.
    s = y + y
    @test is_local(s) && tokens(ctx) == 3
    @test Adelefeld.roots(s) == [ctx]
    for z in (-y, y - y, y * y, y * Rat(3), Rat(2) * y, y + Rat(1), y / Rat(1, 4), y + 2)
        @test ctx in Adelefeld.roots(z)
        close(z)
    end
    close(s)
    @test tokens(ctx) == 2
    # to_global keeps a conservative root (docs/api-j.md:133-134): a token until it is cleared.
    g = to_global(y)
    @test !is_local(g) && tokens(ctx) == 3
    close(g)
    @test tokens(ctx) == 2
    # Values containing a local finite ball (adele.h:85, 112-117, 130-131, 187-188).
    A = Adele(RealBall(1), y)
    @test tokens(ctx) == 3
    f = finitepart(A)
    @test identical(f, y) && context(f) === ctx && tokens(ctx) == 4
    C = CAdele(A)
    fC = finitepart(C)
    @test identical(fC, y)
    close(fC)
    A2 = A + A
    fA2 = finitepart(A2)
    @test is_local(fA2)
    close(fA2)
    Ac = copy(A)
    for v in (A, f, C, A2, Ac)
        @test ctx in Adelefeld.roots(v)
    end
    CB = CAdele(ComplexBall(Rat(1)), y)
    @test ctx in Adelefeld.roots(CB)
    # A failing C call releases its reserved tokens with its temporary (docs/api-j.md:130).
    t0 = tokens(ctx)
    @test try_div(y, Rat(0)).status === NOT_UNIT
    @test try_div(A, Rat(0)).status === NOT_UNIT
    @test tokens(ctx) == t0
    @test try_to_local(FBall(Rat(1)), ctx).status === DOMAIN
    @test try_to_local(parse(FBall, "1/2 mod 8"), ctx).status === DOMAIN
    @test tokens(ctx) == t0
    # Closed inputs are refused before C, with no token and no allocation.
    xc = parse(FBall, "1 mod 8")
    close(xc)
    live1 = Adelefeld.diagnostics().live
    @test_throws InvalidStateException try_to_local(xc, ctx)
    @test_throws InvalidStateException try_enclose_local(xc, ctx)
    @test_throws InvalidStateException to_global(xc)
    @test_throws InvalidStateException is_local(xc)
    @test tokens(ctx) == t0
    @test Adelefeld.diagnostics().live == live1
    for v in (A, f, C, A2, Ac, CB)
        close(v)
    end
    @test tokens(ctx) == 2
    # Inputs unchanged by the conversions.
    x0 = copy(x)
    close(to_local(x, ctx)); close(enclose_local(x, ctx)); close(to_global(y))
    @test identical(x, x0)
    @test tokens(ctx) == 2
    close(y)
    @test tokens(ctx) == 1
    @test ctx_live() == n0 + 1
    @test Adelefeld.diagnostics().finalizer_errors == 0
end

@testset "context finalized before and after its values (docs/api-j.md:126-141)" begin
    n0 = ctx_live()
    f0 = Adelefeld.diagnostics().context_frees
    # Order 1: the context's owner token is dropped first; the value keeps the C context alive.
    ctx = ModCtx(blocks=[8, 9])
    y = to_local(parse(FBall, "5 mod 72"), ctx)
    finalize(ctx)
    @test tokens(ctx) == 1
    @test ctx_live() == n0 + 1
    @test Adelefeld.diagnostics().context_frees == f0
    @test value_text(y) == "(* ; 5 mod 72)"
    @test is_local(y) && context(y) === ctx
    z = y + y
    @test tokens(ctx) == 2
    close(z)
    # A finalized context is refused as an explicit argument (docs/api-j.md:127).
    @test_throws InvalidStateException try_to_local(parse(FBall, "1 mod 8"), ctx)
    @test_throws InvalidStateException try_enclose_local(parse(FBall, "1 mod 8"), ctx)
    @test_throws InvalidStateException modulus(ctx)
    @test_throws InvalidStateException blocks(ctx)
    @test_throws InvalidStateException nblocks(ctx)
    @test sprint(show, ctx) == "ModCtx(finalized)"
    finalize(ctx)                                 # repeated finalize: a no-op
    @test tokens(ctx) == 1
    g = to_global(y)                              # a conservative root: one more token
    @test identical(g, parse(FBall, "5 mod 72"))
    @test tokens(ctx) == 2
    close(g)
    close(y)                                      # the last token: exactly one free
    @test tokens(ctx) == 0
    @test ctx_live() == n0
    @test Adelefeld.diagnostics().context_frees == f0 + 1
    close(y); finalize(y); finalize(ctx)          # repeated close/finalize: no-ops
    @test Adelefeld.diagnostics().context_frees == f0 + 1
    # Order 2: the value is cleared first, then the context's owner token.
    ctx = ModCtx(blocks=[8, 9])
    y = to_local(parse(FBall, "5 mod 72"), ctx)
    close(y)
    @test tokens(ctx) == 1 && ctx_live() == n0 + 1
    finalize(ctx)
    @test tokens(ctx) == 0 && ctx_live() == n0
    @test Adelefeld.diagnostics().context_frees == f0 + 2
    @test Adelefeld.diagnostics().finalizer_errors == 0
end

@testset "GC: both orders, 10^4 round trips, zero contexts and owners" begin
    code = """
        using Adelefeld
        function churn()
            x = parse(FBall, "5 mod 72")
            for i in 1:10_000
                ctx = ModCtx(blocks=[8, 9])
                y = to_local(x, ctx)
                identical(to_global(y), x) || error("round trip failed")
                if i % 3 == 0
                    finalize(ctx)                 # the context first, the value later by GC
                elseif i % 3 == 1
                    close(y); finalize(ctx)       # the value first, then the context
                end                               # else both are left to the collector
            end
        end
        function kept()
            # The only reference to the context is the value's root: forced GC must not free it.
            y = to_local(parse(FBall, "1 mod 8"), ModCtx(blocks=[8]))
            GC.gc(); GC.gc()
            ok = Adelefeld.diagnostics().contexts == 1 && value_text(y) == "(* ; 1 mod 8)"
            return ok
        end
        println("BASELINE=", Adelefeld.diagnostics().live, " CTX=", Adelefeld.diagnostics().contexts)
        churn()
        GC.gc(); GC.gc(); GC.gc(); GC.gc()
        d = Adelefeld.diagnostics()
        println("ROUNDTRIPS=10000 LIVE=", d.live, " CONTEXTS=", d.contexts, " FREES=", d.context_frees,
                " ERRORS=", d.finalizer_errors)
        println("KEPT=", kept())
        GC.gc(); GC.gc(); GC.gc(); GC.gc()
        d = Adelefeld.diagnostics()
        println("AFTER LIVE=", d.live, " CONTEXTS=", d.contexts, " FREES=", d.context_frees,
                " ERRORS=", d.finalizer_errors)
        """
    julia = Base.julia_cmd()
    out = read(`timeout 150 $julia --startup-file=no --project=$PKG -e $code`, String)
    print(out)
    @test occursin("BASELINE=0 CTX=0", out)
    @test occursin("ROUNDTRIPS=10000 LIVE=0 CONTEXTS=0 FREES=10000 ERRORS=0", out)
    @test occursin("KEPT=true", out)
    @test occursin("AFTER LIVE=0 CONTEXTS=0 FREES=10001 ERRORS=0", out)
end

@testset "local dumps and the session line" begin
    ctx = ModCtx(blocks=[8, 9])
    y = to_local(FBall(Rat(5), Rat(72)), ctx)
    @test value_text(y) == "(* ; 5 mod 72)"
    @test sprint(show, y) == "(* ; 5 mod 72)"
    # The dump records the backend (conventions 9.8 A11); the NULL-context loader refuses it with
    # DOMAIN (dump.h:41-42), and constructs no context.
    n0 = ctx_live()
    d = dump_text(y)
    @test d != dump_text(FBall(Rat(5), Rat(72)))
    @test try_load_dump(FBall, d).status === DOMAIN
    @test ctx_live() == n0
    @test isempty(Test.detect_ambiguities(Adelefeld))
end

end
finally
    GC.enable(previous_gc5)
    GC.gc()
end
