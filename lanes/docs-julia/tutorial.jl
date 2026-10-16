# lanes/docs-julia/tutorial.jl: the eight-step REPL tutorial of julia/Adelefeld/README.md, run as a script.
#
# Each block is a list of one-line inputs. For each input the script prints `julia> input`, evaluates it in
# Main, and prints what the Julia REPL displays: nothing for `nothing` or a line that ends in `;`, the value
# through show(io, MIME"text/plain"(), v) otherwise, and `ERROR: ` followed by showerror(e) for an exception
# (the REPL also prints a stack trace; it is left out here and in the README).
# Blocks are separated by lines `#### block NAME`; lanes/docs-julia/check_readme.py compares each fenced
# ```julia block of the README that starts with `julia> ` against the block of the same position here.
#
# Run from the worktree root (lanes/docs-julia/numbers.md has the command and the output).

const BLOCKS = Pair{String,Vector{String}}[
"1-rationals" => [
    "using Adelefeld",
    "Rat(6, 35) + Rat(1, 7)",
    "Rat(1) / 3",
    "Rat(2, -4)",
    "Rat(1, 3) * 3 == 1",
    "Rat(6, 35) == 6//35",
    "Rat(1, 0)",
],
"2-finite-balls" => [
    "x = FBall(Rat(146), Rat(1000))",
    "center(x), radius(x)",
    "parse(FBall, \"3 mod 12\") + parse(FBall, \"5 mod 18\")",
    "x - x",
    "FBall(Rat(1, 3)) * 3",
    "a = parse(FBall, \"1 mod 6\"); b = parse(FBall, \"1 mod 3\");",
    "inside(a, b), inside(b, a)",
    "overlaps(a, parse(FBall, \"4 mod 9\")), equal_set(a, parse(FBall, \"7 mod 6\"))",
    "compare_points(a, b)",
    "compare_points(a, parse(FBall, \"0 mod 2\"))",
    "compare_points(FBall(5), FBall(5))",
],
"3-adeles" => [
    "u = Adele(Rat(1, 3))",
    "realpart(u)",
    "finitepart(u)",
    "v = parse(Adele, \"(1.5 +/- 0.5 ; 0 mod 2)\")",
    "u + v",
    "u * 3",
    "RealBall(Rat(1, 3); prec = 200)",
    "z = parse(ComplexBall, \"(1.5) + (-2)*i\")",
    "CAdele(z, FBall(Rat(1), Rat(4)))",
    "CAdele(u) * Rat(2)",
    "Adele(RealBall(Rat(2)), parse(FBall, \"1 mod 4\"))",
    "parse(RealBall, \"1.5 +/- 0.0009765625\")",
],
"4-reconstruction" => [
    "reconstruct(parse(Adele, \"(1.5 +/- 0.5 ; 0 mod 2)\"))",
    "reconstruct(parse(FBall, \"2 mod 101\"), Rat(-50), Rat(50))",
    "reconstruct(parse(FBall, \"52 mod 101\"), Rat(-50), Rat(50))",
    "try_reconstruct(parse(Adele, \"(1.5 +/- 2 ; 0 mod 2)\")).status",
    "try_reconstruct(parse(Adele, \"(1.5 +/- 0.25 ; 0 mod 2)\")).status",
    "try_reconstruct(FBall(Rat(1, 3)), Rat(0), Rat(1)).value",
],
"5-statuses" => [
    "r = try_div(Rat(1), Rat(0))",
    "r.status, r.value",
    "unwrap(r)",
    "Rat(1) / 0",
    "try_parse(Rat, \"1/0\").status, try_parse(Rat, \"one\").status",
    "try_fball(Rat(1), Rat(-2)).status",
    "unwrap(try_rat(7, 21))",
    "collect(instances(AdfStatus))",
],
"6-text-and-dumps" => [
    "u = Adele(Rat(1, 3));",
    "value_text(u; digits = 5)",
    "show(IOContext(stdout, :adf_digits => 8), u)",
    "d = dump_text(u)",
    "w = load_dump(Adele, d); identical(u, w)",
    "s = value_text(u); identical(u, parse(Adele, s))",
    "dump_text(parse(Adele, s))",
    "readvalue(\"3 mod 12\"), readvalue(\"7/3\")",
    "try_readvalue(\"(1 ; 1/2 mod 3)\").details",
    "r = try_readvalue(\"[5 mod 6]\"); r.status, r.details",
    "dump_text(parse(FBall, \"3 mod 12\"))",
],
"7-modctx" => [
    "ModCtx(blocks = [8, 9, 5])",
    "ModCtx(prime_powers = [2 => 3, 3 => 2])",
    "ModCtx(modulus = 360)",
    "ctx = ModCtx(factorial = 6)",
    "ModCtx(primorial_power = (7, 2))",
    "modulus(ctx), nblocks(ctx), Int.(blocks(ctx))",
    "y = to_local(parse(FBall, \"1 mod 5\"), ctx)",
    "is_local(y), context(y) === ctx",
    "g = to_global(y); is_local(g), context(g)",
    "try_to_local(parse(FBall, \"1 mod 7\"), ctx).status",
    "enclose_local(readvalue(\"1 mod 5\"), ModCtx(blocks = [8, 9]))",
    "r = try_enclose_local(parse(FBall, \"1 mod 32\"), ModCtx(blocks = [8, 9])); r.value, r.details",
    "try_modctx(blocks = [4, 6]).status",
],
"8-housekeeping" => [
    "t = Rat(5); close(t); close(t)",
    "t + 1",
    "Adelefeld.diagnostics().live >= 0, Adelefeld.diagnostics().finalizer_errors",
    "keys(Adelefeld.diagnostics())",
    "c5 = ModCtx(blocks = [5]); y5 = to_local(parse(FBall, \"2 mod 5\"), c5); finalize(c5); c5",
    "y5 + 1",
    "to_local(parse(FBall, \"2 mod 5\"), c5)",
],
]

# The package is loaded here once, so that the loop below runs in a world that has its methods (a `using` inside
# the loop would add methods too new for the loop's calls); the block's `using Adelefeld` line is then a no-op.
using Adelefeld

function show_repl(v)
    io = IOContext(stdout, :limit => true, :module => Main)
    Base.invokelatest(show, io, MIME"text/plain"(), v)
    println()
end

for (name, lines) in BLOCKS
    println("#### block ", name)
    for line in lines
        println("julia> ", line)
        ex = Meta.parseall(line)
        try
            v = Base.invokelatest(Core.eval, Main, ex)
            if endswith(rstrip(line), ";") || v === nothing
                # A show(...) call writes its text without a newline; end the line as the REPL does.
                startswith(line, "show(") && println()
            else
                show_repl(v)
            end
        catch e
            println("ERROR: ", Base.invokelatest(sprint, showerror, e))
        end
        println()
    end
end
