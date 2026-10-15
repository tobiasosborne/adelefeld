# ux/notebook/test/runtests.jl: the tests of the notebook slice (lanes/ux-notebook/brief.md, item 6).
# Run from the repository root: sh ux/notebook/test.sh (it starts Julia as ux/notebook/README.md says).
using Test
const ROOT = normpath(joinpath(@__DIR__, "..", "..", ".."))
push!(LOAD_PATH, joinpath(ROOT, "ux", "notebook", "src"))
using Adelefeld
using Adelica
const A = Adelica
const ADF = joinpath(ROOT, "build", "adf")
_cellout(cells) = (m = A.new_notebook_module(); [A.evaluate(m, c, "c$(i)") for (i, c) in enumerate(cells)])
function _run(cells)
    out = joinpath(mktempdir(), "nb")
    Adelica.run(cells, out)
    return out, A.read_records(out * ".records.jsonl")
end

@testset "Adelica" verbose = true begin

@testset "value-text parser on the five rows of SPEC 4.3" begin
    # docs/SPEC.md:240-246. Each row is computed by the package and its printed text is parsed back.
    a, b = readvalue("3 mod 12"), readvalue("5 mod 18")
    rows = [(a + b, 2//1, 6//1),                                         # (3 mod 12) + (5 mod 18) = 2 mod 6
            (a * b, 3//1, 6//1),                                         # (3 mod 12) * (5 mod 18) = 3 mod 6
            (Rat(12) * b, 60//1, 216//1),                                # 12 * (5 mod 18) = 60 mod 216
            (Rat(1, 3) * b, 5//3, 6//1),                                 # (1/3) * (5 mod 18) = 5/3 mod 6
            (readvalue("1/2 mod 8") * readvalue("2/3 mod 9"), 0//1, 1//6)]  # = 0 mod 1/6
    texts = ["(* ; 2 mod 6)", "(* ; 3 mod 6)", "(* ; 60 mod 216)", "(* ; 5/3 mod 6)", "(* ; 0 mod 1/6)"]
    for (i, (x, c, n)) in enumerate(rows)
        t = value_text(x)
        @test t == texts[i]
        v = A.parse_value_text(t)
        @test v.kind === :fball
        @test v.real === nothing
        @test v.centre == c
        @test v.modulus == n
    end
    # The operand texts of the rows: the bare form `a mod N` of fball_v, a rational, a fraction modulus.
    @test A.parse_value_text("3 mod 12").centre == 3
    @test A.parse_value_text("1/2 mod 8").modulus == 8
    v = A.parse_value_text("2/3 mod 9")
    @test (v.kind, v.centre, v.modulus) == (:fball, 2//3, 9//1)
    v = A.parse_value_text("-22/7")
    @test (v.kind, v.centre, v.modulus) == (:rat, -22//7, 0//1)
    v = A.parse_value_text("(3.14 +/- 0.011 ; 1/7 mod 1)")
    @test (v.kind, v.real, v.centre, v.modulus) == (:adele, "3.14 +/- 0.011", 1//7, 1//1)
    v = A.parse_value_text("((1 +/- 0.5) + (2)*i ; 5)")
    @test (v.kind, v.real, v.centre, v.modulus) == (:cadele, "(1 +/- 0.5) + (2)*i", 5//1, 0//1)
    @test A.parse_value_text(" ( * ;  2  mod 6 ) ").modulus == 6          # whitespace between tokens
    for bad in ("", "1/", "+3", ".5", "3 mod", "(* ; 1.5)", "(1/2 ; 3)", "3 mod -2", "(* ; 2 mod 6", "x",
                "1 2", "(* ; 2 mod 6) 7", "3 mod 6 mod 2")
        @test_throws ArgumentError A.parse_value_text(bad)
    end
end

@testset "p-adic digits of exact rationals" begin
    # Low digit on the right; the period in parentheses; '.' the p-adic point.
    # 1/3 at 2: x0 = 1/3, digit 1/3 mod 2 = 1; x1 = (1/3 - 1)/2 = -1/3, digit 1; x2 = (-1/3 - 1)/2 = -2/3,
    # digit 0; x3 = (-2/3 - 0)/2 = -1/3 = x1. Pre-period [1], period [1, 0] (low first): ...0101 1.
    # Check: 1 + 2 (1 + 4 + 16 + ...) = 1 + 2/(1 - 4) = 1/3.
    @test A.rat_digit_text(1//3, 2) == "(01)1"
    # 1/3 at 3: 1/3 = 3^-1 * 1; the digits of 1 are 1, 0, 0, ...; the point one place from the right.
    @test A.rat_digit_text(1//3, 3) == "(0).1"
    # 1/3 at 5: 3^-1 mod 5 = 2, digit 2; x1 = (1/3 - 2)/5 = -1/3, digit -2 mod 5 = 3; x2 = (-1/3 - 3)/5 =
    # -2/3, digit -4 mod 5 = 1; x3 = (-2/3 - 1)/5 = -1/3 = x1. Pre-period [2], period [3, 1].
    # Check: 2 + 5 (3 + 1*5)/(1 - 25) = 2 - 5/3 = 1/3.
    @test A.rat_digit_text(1//3, 5) == "(13)2"
    # 22/7 at 7: 22/7 = 7^-1 * 22 and 22 = 1 + 3*7, so the digits of 22 are 1, 3, 0, 0, ...;
    # the point one place from the right: ...0 3 . 1. Check: 3 + 1/7 = 22/7.
    @test A.rat_digit_text(22//7, 7) == "(0)3.1"
    # The brief's two chips at 5: -3 = 2 + 4*5 + 4*25 + ... (the period 4) and 3.
    @test A.rat_digit_text(-3//1, 5) == "(4)2"
    @test A.rat_digit_text(3//1, 5) == "(0)3"
    # 9 at 3 = 3^2 * 1: two zeros at the low end. 1/9 at 3: the point two places from the right.
    @test A.rat_digit_text(9//1, 3) == "(0)100"
    @test A.rat_digit_text(1//9, 3) == "(0).01"
    # 22/7 at 11 = 11 * 2/7; 7^-1 mod 11 = 8. x0 = 2, digit 2*8 mod 11 = 5; x1 = (2 - 5*7)/11 = -3, digit
    # -24 mod 11 = 9; x2 = (-3 - 63)/11 = -6, digit -48 mod 11 = 7; x3 = (-6 - 49)/11 = -5, digit -40 mod 11
    # = 4; x4 = (-5 - 28)/11 = -3 = x1. Pre-period [5], period [9, 7, 4]; one zero at the low end for v = 1.
    # Digits of p > 10 are decimal numbers separated by spaces.
    @test A.rat_digit_text(22//7, 11) == "(4 7 9) 5 0"
    # The cap of 64 digits: the period of 1/1021 at 2 is the order of 2 mod 1021, 340 (computed by
    # repeated doubling mod 1021 for this test), so no period is found within 64 digits.
    @test startswith(A.rat_digit_text(1//1021, 2), "...")
    @test length(A.rat_digit_text(1//1021, 2)) == 3 + 64
end

@testset "digits of a ball" begin
    # 280182 + O(5^8), the brief's chip [2] of the square root of -1: 280182 = 3*5^7 + 2*5^6 + 4*5^5
    # + 3*5^4 + 1*5^3 + 2*5^2 + 1*5 + 2 (234375 + 31250 + 12500 + 1875 + 125 + 50 + 5 + 2 = 280182).
    @test A.ball_digit_text(280182//1, 5, 8) == "~32431212"
    # 146 + 1000 Zhat at 2: 146 mod 8 = 2 = 010 in base 2; at 5: 146 mod 125 = 21 = 0*25 + 4*5 + 1.
    @test A.ball_digit_text(146//1, 2, 3) == "~010"
    @test A.ball_digit_text(146//1, 5, 3) == "~041"
    # 1/7 + Z_7: one digit known, at 7^-1. 0 + 2^-1 Z_2 (row 5 of SPEC 4.3): no digit known.
    @test A.ball_digit_text(1//7, 7, 0) == "~.1"
    @test A.ball_digit_text(0//1, 2, -1) == "~"
    # 5/3 + 3 Z_3: 5/3 = 2*3^-1 + 1, digits at 3^0 and 3^-1 known.
    @test A.ball_digit_text(5//3, 3, 1) == "~1.2"
end

@testset "places of a value (trial division below 1000)" begin
    ps = A.places(Rat(6, 35))
    @test [c.p for c in ps.columns] == [2, 3, 5, 7]
    @test [c.v for c in ps.columns] == [1, 1, -1, -1]
    @test isempty(ps.cofactors)
    @test ps.rest == "unit, |x|_p = 1, at infinitely many primes"
    ps = A.places(readvalue("146 mod 1000"))
    @test [c.p for c in ps.columns] == [2, 5]
    @test [c.k for c in ps.columns] == [3, 3]
    @test [c.v for c in ps.columns] == [1, 0]
    @test ps.rest == "Z_p (no information), at infinitely many primes"
    # 1009 * 1013 has no prime factor below 1000: the cofactor is shown, not factored.
    ps = A.places(FBall(Rat(1), Rat(8 * 1009 * 1013)))
    @test [c.p for c in ps.columns] == [2]
    @test ps.cofactors == [big(1009) * 1013]
    ps = A.places(readvalue("(3.14 +/- 0.01 ; 1/7 mod 1)"))
    @test [c.p for c in ps.columns] == [7]
    @test ps.real == "3.14 +/- 0.011"
    h = sprint(show, MIME"text/html"(), Rat(6, 35))
    for s in ("class=\"strip\"", ">R<", ">2<", ">3<", ">5<", ">7<", "every other p")
        @test occursin(s, h)
    end
    h = sprint(show, MIME"text/html"(), FBall(Rat(1), Rat(8 * 1009 * 1013)))
    @test occursin("cofactor 1022117", h)
end

@testset "branch fans through the driver" begin
    f = roots(9, 2; at = 5)
    @test length(f.branches) == 2
    @test [b.seed for b in f.branches] == ["2", "3"]
    @test [b.value for b in f.branches] == ["-3", "3"]
    f = roots(-1, 2; at = 5)
    @test length(f.branches) == 2
    @test [b.value for b in f.branches] == ["280182 + O(5^8)", "110443 + O(5^8)"]
    @test A.branch_digit_text(f.branches[2], 5) == "~12013233"
    r = root(-1, 2; at = 5, seed = 2, prec = 20)
    @test length(r.branches) == 1 && r.branches[1].value == "79753541295807 + O(5^20)"
    e = try roots(2, 2; at = 5); nothing catch err; err end
    @test e isa A.DriverError
    @test e.status === Adelefeld.DOMAIN
    @test recover(readvalue("146 mod 1000"); num = 30, den = 15) == Rat(22, 7)
    # At 2 the driver prints the identifier +1, and reads only 1 or -1 (tools/adf/README.md:489-490):
    # the seed +1 of a chip is passed on as 1.
    f = roots(9, 2; at = 2)
    @test [b.seed for b in f.branches] == ["+1", "-1"]
    @test A.seed_arg(f.branches[1]) == "1"
    @test root(9, 2; at = 2, seed = "+1").branches[1].value == "-3"
end

@testset "status classes (D-UX4)" begin
    @test A.status_class("OK") == ("ok", "none")
    for s in ("NOT_DETERMINED", "UNIT_NOT_CERTIFIED", "NEEDS_SPLIT", "NOT_UNIQUE")
        @test A.status_class(s) == ("open", "dashed")
    end
    for s in ("DOMAIN", "NO_SOLUTION", "NOT_UNIT")
        @test A.status_class(s) == ("proved no", "solid")
    end
    @test A.status_class("LIMIT") == ("budget", "dotted")
    for s in ("PARSE", "UNSUPPORTED")
        @test A.status_class(s) == ("not mathematics", "plain")
    end
    @test length(A.STATUS_NAMES) == 11
end

@testset "notebooks: page, records, replay" begin
    tmp = mktempdir()   # under TMPDIR, which julia.sh sets to the lane directory
    cells = ["roots(9, 2; at = 5)", "x = Rat(6, 35)", "roots(2, 2; at = 5)", "x + Rat(1, 35)", "x / 0"]
    out = joinpath(tmp, "t")
    Adelica.run(cells, out)
    recs = A.read_records(out * ".records.jsonl")
    @test length(recs) == length(cells)
    @test [r["kind"] for r in recs] == ["fan", "rat", "status", "rat", "status"]
    @test recs[5]["status"] == "NOT_UNIT" && recs[5]["driver"] == ["div 6/35 with 0"]
    @test recs[3]["status"] == "DOMAIN"
    @test recs[4]["value"] == "1/5"
    @test recs[4]["driver"] == ["add 6/35 with 1/35"]
    @test recs[2]["dump"] == dump_text(Rat(6, 35))
    # A value with no command of its own is reproduced by `load` of its dump (conventions 9.6: the dump is
    # the identity-preserving form; a printed real ball may widen when it is read again).
    @test recs[2]["driver"] == ["load " * dump_text(Rat(6, 35))]
    page = read(out * ".html", String)
    @test occursin("data-rewrite=\"root(9, 2; at = 5, seed = 3)\"", page)
    @test occursin("place: not given", page)
    @test A.replay(out * ".records.jsonl", joinpath(ROOT, "build", "adf"); io = devnull) == 0
    # A record whose value was changed is caught.
    lines = readlines(out * ".records.jsonl")
    lines[4] = replace(lines[4], "\"value\":\"1/5\"" => "\"value\":\"1/6\"")
    write(out * ".bad.jsonl", join(lines, "\n") * "\n")
    @test A.replay(out * ".bad.jsonl", joinpath(ROOT, "build", "adf"); io = devnull) == 1
    # A literal operand keeps its source text in the driver line: the printed text of this adele,
    # (3.14 +/- 0.011 ; 1/7 mod 1), read again at 64 bits prints as (3.14 +/- 0.012 ; 1/7 mod 1).
    out2 = joinpath(tmp, "u")
    Adelica.run(["x = adf\"(3.14 +/- 0.01 ; 1/7 mod 1)\"", "reconstruct(x)", "x + 1"], out2)
    recs = A.read_records(out2 * ".records.jsonl")
    @test recs[1]["driver"] == ["load " * recs[1]["dump"]]
    @test recs[2]["driver"] == ["reconstruct (3.14 +/- 0.01 ; 1/7 mod 1)"]
    @test recs[3]["driver"] == ["add (3.14 +/- 0.01 ; 1/7 mod 1) with 1"]
    @test A.replay(out2 * ".records.jsonl", joinpath(ROOT, "build", "adf"); io = devnull) == 0
    for name in ("a_roots", "b_which_rational")
        src = joinpath(ROOT, "ux", "notebook", "scenarios", name * ".jl")
        cells = A.read_cells(src)
        rec = joinpath(ROOT, "ux", "notebook", "out", name * ".records.jsonl")
        @test length(A.read_records(rec)) == length(cells)
        buf = IOBuffer()
        @test A.replay(rec, joinpath(ROOT, "build", "adf"); io = buf) == 0
        @test count("same", String(take!(buf))) == length(cells)
    end
end

# ---- The repair after the test drive (lanes/ux-notebook-r1/brief.md, items 1 to 6) ----


@testset "repair 1: the README's example runs, the README names what is not done" begin
    readme = read(joinpath(ROOT, "ux", "notebook", "README.md"), String)
    ls = split(chomp(readme), '\n')
    @test length(ls) < 80
    @test !occursin("FBall(c, N)", readme)                 # the drive's MethodError path (FBall(0, 25))
    # The example notebook of the README: the indented block that begins with an indented `# %%` line.
    i = findfirst(l -> startswith(l, "    # %%"), ls)
    @test i !== nothing
    ex = String[]
    if i !== nothing
        for l in ls[i:end]
            startswith(l, "    ") || break
            push!(ex, l[5:end])
        end
    end
    f = joinpath(mktempdir(), "readme.jl")
    write(f, join(ex, "\n") * "\n")
    cells = A.read_cells(f)
    @test length(cells) >= 3
    outs = Adelica.run_file(f, joinpath(dirname(f), "readme"))
    @test all(o -> o.kind != "julia_error", outs)
    @test any(o -> o.kind == "fball", outs)
    for w in ("not do", "equal", "compare", "teichmuller", "certificate")
        @test occursin(w, readme)
    end
end

@testset "repair 2 (D3): the DOMAIN card names possible causes, not one action" begin
    o = _cellout(["root(9, 2; at = 4, seed = 3)", "roots(9, 2; at = 4)", "root(9, 2; at = 5, seed = 4)",
                  "roots(2, 2; at = 5)"])
    @test all(x -> x.status == "DOMAIN", o)
    for x in o
        @test !occursin("another seed", x.html)
        @test !occursin("a seed that roots(x, n; at) lists", x.html)
    end
    # The place 4 is composite: the slice checks it in Julia and says so.
    for x in o[1:2]
        @test occursin("the place 4 is not prime", x.html)
        @test occursin("checked in Julia", x.html)
    end
    # At the prime 5 the driver gives no sub-code: the card lists the causes and recommends none of them.
    for x in o[3:4]
        @test !occursin("not prime", x.html)
        @test occursin("possible causes", x.html)
        @test occursin("existence criterion", x.html)
        @test occursin("the driver does not say which", x.html)
    end
    @test occursin("seed", o[3].html) && occursin("degree", o[3].html)
    _, recs = _run(["root(9, 2; at = 4, seed = 3)"])
    @test occursin("the place 4 is not prime", recs[1]["reason"])
    @test recs[1]["actions"] isa Vector
end

@testset "repair 3 (D4): above 97 the refinement is a control that works like the fog" begin
    o = _cellout(["root(-1, 2; at = 101, seed = 10, prec = 8)", "root(-1, 2; at = 5, seed = 2)"])
    h = o[1].html
    @test occursin("data-rewrite=\"root(-1, 2; at = 101, seed = 10, prec = 20)\"", h)
    @test !occursin("click the fog", h)
    @test occursin("data-rewrite=\"root(-1, 2; at = 5, seed = 2, prec = 20)\"", o[2].html)
    # Every instruction to click the fog stands next to a fog that is a control.
    for x in o
        occursin("click the fog", x.html) && @test occursin("class=\"fog\" data-cell=", x.html)
    end
end

@testset "repair 4 (D1): replay compares status, dump, digits and input, and counts unchecked" begin
    src = joinpath(ROOT, "ux", "notebook", "scenarios", "b_which_rational.jl")
    out = joinpath(mktempdir(), "b")
    Adelica.run_file(src, out)
    rec = out * ".records.jsonl"
    s = A.replay_summary(rec, ADF; io = devnull)
    @test (s.same, s.differs, s.unchecked) == (8, 0, 0)
    lines = readlines(rec)
    variants = ["\"status\":\"OK\"" => "\"status\":\"NO_SOLUTION\"",
                "\"dump\":\"adf1 Q rat 16 7\"" => "\"dump\":\"adf1 Q rat 17 7\"",
                "\"digits\":\"(011)1010\"" => "\"digits\":\"(0)0\"",
                "recover(c; num = 30, den = 15)" => "recover(c; num = 0, den = 1)"]
    for v in variants
        l2 = copy(lines)
        @test occursin(v.first, l2[2])
        l2[2] = replace(l2[2], v)
        f = out * ".variant.jsonl"
        write(f, join(l2, "\n") * "\n")
        buf = IOBuffer()
        s = A.replay_summary(f, ADF; io = buf)
        txt = String(take!(buf))
        @test (s.same, s.differs, s.unchecked) == (7, 1, 0)
        @test occursin("cell 2: differs", txt)
    end
    # The drive's a_flow notebook: two cells without a driver line are unchecked, not the same.
    out2, recs = _run(["root(-1, 2; at = 5, seed = 2)", "root(-1, 2; at = 5, seed = 2, prec = 20)",
        "equal(teichmuller(2; at = 5), root(-1, 2; at = 5, seed = 2, prec = 20))",
        "compare(teichmuller(2; at = 5), root(-1, 2; at = 5, seed = 2, prec = 20))"])
    s = A.replay_summary(out2 * ".records.jsonl", ADF; io = devnull)
    @test (s.same, s.differs, s.unchecked) == (2, 0, 2)
    @test A.replay_sentence(s) == "replay: 2 same, 0 differs, 2 unchecked"
    @test !occursin("every record", A.replay_sentence(s))
    p = pipeline(Cmd(`sh ux/notebook/replay.sh $(out2 * ".records.jsonl")`; dir = ROOT, ignorestatus = true))
    r = read(p, String)
    @test endswith(chomp(r), "replay: 2 same, 0 differs, 2 unchecked")
    pr = run(Cmd(`sh ux/notebook/replay.sh $(out * ".variant.jsonl")`; dir = ROOT, ignorestatus = true),
             devnull, devnull, devnull)
    @test pr.exitcode == 1
end

@testset "repair 5 (D2): a computed value is never sent to the driver as its widened text" begin
    lit = "(0.99 +/- 0.0099 ; 0 mod 1)"
    out, recs = _run(["x = adf\"$(lit)\"", "y = x + Rat(0)", "project(y, :real)", "reconstruct(y)",
                      "z = y * y", "w = adf\"(3.14 +/- 0.01 ; 9 mod 625)\" + Rat(0)", "roots(w, 2; at = 5)",
                      "project(w, :real, 5)"])
    # The package's answer on the stored enclosure [0.9801, 0.9999], which holds no integer.
    @test recs[4]["status"] == "NO_SOLUTION"
    @test recs[4]["driver"] === nothing
    @test occursin("package", recs[4]["computed_by"])
    @test occursin("computed value", recs[4]["unchecked"])
    # The real component of y is the stored one: the driver's projection of the literal itself.
    @test recs[3]["value"] == A.driver_line(["prec 64", "project $(lit) with real"])
    @test recs[3]["driver"] === nothing && occursin("realpart", recs[3]["computed_by"])
    @test recs[5]["driver"] === nothing && occursin("computed value", recs[5]["unchecked"])
    # At a prime only the finite part counts (tools/adf/README.md:488): its text is exact.
    @test recs[7]["driver"] == ["prec 8", "roots_at (* ; 9 mod 625) with 5 with 2"]     # the finite part of w
    @test recs[6]["driver"] == ["add (3.14 +/- 0.01 ; 9 mod 625) with 0"]                # an inline literal
    @test recs[7]["value"] == A.driver_line(["prec 8", "roots_at (3.14 +/- 0.01 ; 9 mod 625) with 5 with 2"])
    @test recs[8]["value"] == A.driver_line(["prec 64", "project (3.14 +/- 0.01 ; 9 mod 625) with real 5"])
    # No driver line holds the printed, widened text of a computed real ball.
    for r in recs, l in something(r["driver"], String[])
        @test !occursin("0.99 +/- 0.01 ", l) && !occursin("0.99 +/- 0.01;", l)
        @test !occursin("3.14 +/- 0.011", l)
    end
    s = A.replay_summary(out * ".records.jsonl", ADF; io = devnull)
    @test s.differs == 0
    @test s.unchecked == 4
end

@testset "repair 6: a dump or its reason on every record, versions, the set on hover" begin
    for name in ("a_roots", "b_which_rational")
        src = joinpath(ROOT, "ux", "notebook", "scenarios", name * ".jl")
        out = joinpath(mktempdir(), name)
        Adelica.run_file(src, out)
        recs = A.read_records(out * ".records.jsonl")
        for r in recs
            @test r["dump"] !== nothing || (get(r, "dump_reason", nothing) isa String)
            @test haskey(r, "stamp") && occursin("Adelefeld.jl", r["stamp"])
            if r["kind"] in ("fan", "branch")
                for b in r["branches"]
                    @test startswith(b["dump"], "adf1 Q lball 5 ")
                end
            end
        end
        page = read(out * ".html", String)
        @test occursin("Adelefeld.jl " * string(pkgversion(Adelefeld)), page)
        @test occursin("not exposed by the package", page)
        @test occursin("commit ", page)
        nchips = count("class=\"chip", page)
        @test count(r"class=\"chip[^\"]*\"[^>]*title=\"the set ", page) == nchips
        @test count("<details class=\"dump\">", page) >= count(r -> r["status"] == "OK", recs)
    end
    # The branch dump is the driver's dump of the local ball, which load reads back to the same ball.
    _, recs = _run(["root(-1, 2; at = 5, seed = 2)"])
    b = recs[1]["branches"][1]
    @test A.driver_line(["load " * b["dump"]]) == "[p=5: " * b["value"] * "]"
    @test recs[1]["dump"] == b["dump"]
end

end
