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
