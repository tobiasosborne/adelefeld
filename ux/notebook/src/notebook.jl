# The notebook: cells evaluated in order in a fresh module, one static page, one records sidecar, replay.
#
# A record (one JSON object per line of `<out>.records.jsonl`, one per cell output; the agent's window):
#   cell     the index of the cell, from 1        input   the cell text
#   kind     rat, fball, adele, cadele (the driver's `type` names), fan, branch, projection, status, text,
#            julia_error
#   value    the value text (package printer), or the driver's line, or `error: STATUS`
#   status   OK or the name of the status; null for text and julia_error
#   dump     the package's dump form (conventions 10.1) for rat, fball, adele, cadele; else null
#   driver   the lines (settings, then one command) whose output line reproduces `value`; null if none
#   and per kind: places, real, cofactors, rest (a strip); branches (a fan); entries (a projection);
#   place, op, source (a status).

const BINOPS = Dict(:+ => ("add", +), :- => ("sub", -), :* => ("mul", *), :/ => ("div", /))
const DRIVABLE = Union{Integer,Rational,Rat,FBall,Adele,CAdele}

_kind(::Rat) = "rat"
_kind(::FBall) = "fball"
_kind(::Adele) = "adele"
_kind(::CAdele) = "cadele"

function _call_parts(ex::Expr)
    pos, kws = Any[], Pair{Symbol,Any}[]
    for a in ex.args[2:end]
        if a isa Expr && a.head === :parameters
            for k in a.args
                k isa Expr && k.head === :kw || throw(ArgumentError("a keyword argument needs `= value`"))
                push!(kws, k.args[1] => k.args[2])
            end
        elseif a isa Expr && a.head === :kw
            push!(kws, a.args[1] => a.args[2])
        else
            push!(pos, a)
        end
    end
    return pos, kws
end

# The source text of each literal value of a notebook, by notebook module (see plan).
const PROVENANCE = IdDict{Module,IdDict{Any,String}}()

# The source text of a literal value: adf"..." or readvalue("...") with no keyword. The value it reads is
# the value the driver reads from the same text at its default precision (64 bits, both).
function literal_text(ex)
    if ex isa Expr && ex.head === :macrocall && ex.args[1] === Symbol("@adf_str") && ex.args[end] isa String
        return ex.args[end]
    elseif ex isa Expr && ex.head === :call && ex.args[1] === :readvalue && length(ex.args) == 2 &&
           ex.args[2] isa String
        return ex.args[2]
    end
    return nothing
end

# The plan of a cell's right-hand side: a thunk that computes the value and the driver lines that
# reproduce it, when the top-level call is one whose driver command is known. An operand is written as
# the source text of the literal that made it when there is one (PROVENANCE), else
# as its value text, which is lossless for Rat and FBall and an enclosure for a real coordinate
# (conventions 9.6: a printed real ball read again may widen; replay then says `differs`).
function plan(m::Module, ex)
    prov = PROVENANCE[m]
    _operand(v) = v isa ADELIC ? get(prov, v, Adelica._operand(v)) : Adelica._operand(v)
    recover_lines(x, num, den) = ["recover $(_operand(x)) with $(num) with $(den)"]
    if ex isa Expr && ex.head === :call && ex.args[1] isa Symbol
        f = ex.args[1]
        pos, kws = _call_parts(ex)
        if haskey(BINOPS, f) && length(pos) == 2 && isempty(kws)
            a, b = Core.eval(m, pos[1]), Core.eval(m, pos[2])
            if a isa DRIVABLE && b isa DRIVABLE && (a isa ADELIC || b isa ADELIC)
                name, op = BINOPS[f]
                return (() -> op(a, b)), ["$(name) $(_operand(a)) with $(_operand(b))"], name
            end
            return (() -> BINOPS[f][2](a, b)), nothing, ""
        elseif f === :- && length(pos) == 1 && isempty(kws)
            a = Core.eval(m, pos[1])
            a isa ADELIC && return (() -> -a), ["neg $(_operand(a))"], "neg"
        elseif f === :reconstruct && isempty(kws)
            v = [Core.eval(m, a) for a in pos]
            if length(v) == 1 && v[1] isa Adele
                return (() -> Adelefeld.reconstruct(v[1])), ["reconstruct $(_operand(v[1]))"], "reconstruct"
            elseif length(v) == 3 && v[1] isa FBall && all(x -> x isa Union{Rat,Integer,Rational}, v[2:3])
                lo, hi = (x isa Rat ? x : Rat(x) for x in v[2:3])
                return (() -> Adelefeld.reconstruct(v[1], lo, hi)),
                       ["reconstruct $(_operand(v[1])) with $(_operand(lo)) with $(_operand(hi))"], "reconstruct"
            end
        elseif f === :recover && length(pos) == 1
            x = Core.eval(m, pos[1])
            kv = Dict(k => Core.eval(m, v) for (k, v) in kws)
            if x isa FBall && haskey(kv, :num) && haskey(kv, :den)
                return (() -> recover(x; num = kv[:num], den = kv[:den])),
                       recover_lines(x, kv[:num], kv[:den]), "recover"
            end
        end
    end
    return (() -> Core.eval(m, ex)), nothing, ""
end

struct Output
    kind::String
    value::String
    status::Union{Nothing,String}
    dump::Union{Nothing,String}
    driver::Union{Nothing,Vector{String}}
    html::String
    extra::Vector{Pair{String,Any}}
end

# The source texts of a roots/root cell, for the rewrites of the page.
struct Rewrite
    prefix::String
    x::String
    n::String
    at::String
    seed::Union{Nothing,String}
    prec::Union{Nothing,String}
end

function rewrite_info(rhs, target)
    rhs isa Expr && rhs.head === :call && rhs.args[1] in (:roots, :root) || return nothing
    pos, kws = _call_parts(rhs)
    length(pos) == 2 || return nothing
    kv = Dict(k => string(v) for (k, v) in kws)
    haskey(kv, :at) || return nothing
    return Rewrite(target === nothing ? "" : string(target) * " = ", string(pos[1]), string(pos[2]), kv[:at],
                   get(kv, :seed, nothing), get(kv, :prec, nothing))
end

function _rootcall(rw::Rewrite, seed, prec)
    s = rw.prefix * "root(" * rw.x * ", " * rw.n * "; at = " * rw.at * ", seed = " * seed
    prec === nothing || (s *= ", prec = " * string(prec))
    return s * ")"
end

function _attr(cellid, rewrite)
    return " data-cell=\"" * cellid * "\" data-rewrite=\"" * esc(rewrite) * "\""
end

function fan_html(f::Fan, rw, cellid)
    io = IOBuffer()
    nb = length(f.branches)
    if !f.single
        print(io, "<div class=\"fanhead\"><b>", nb, nb == 1 ? " branch" : " branches", "</b> at ", f.p,
              " · degree ", f.n, " · every branch, as the driver's roots_at lists them",
              rw === nothing ? "" : " · click a branch to pin its seed in the cell", "</div>")
    end
    print(io, "<div class=\"fan\">")
    for b in f.branches
        toks = branch_tokens(b, f.p)
        inexact = occursin(" + O(", b.value)
        if f.single
            known = inexact ? string(known_digits(b), " digits known") : "exact"
            fogattr = ""
            if inexact && rw !== nothing
                rew = _rootcall(rw, rw.seed === nothing ? seed_arg(b) : rw.seed, f.prec + 12)
                fogattr = _attr(cellid, rew) * " tabindex=\"0\" role=\"button\" aria-label=\"12 more digits\"" *
                          " title=\"12 more digits: " * esc(rew) * "\""
            end
            print(io, "<div class=\"chip picked\"><span class=\"seed\">", b.place, " [", esc(b.seed), "] · ", known,
                  "</span>", digits_html(toks; fogattr), "<span class=\"v\">", esc(b.value), "</span></div>")
        else
            attr = rw === nothing ? "" : _attr(cellid, _rootcall(rw, seed_arg(b), rw.prec))
            print(io, "<button class=\"chip\" type=\"button\"", attr, "><span class=\"seed\">", b.place, " [",
                  esc(b.seed), "]</span><span class=\"v\">", esc(b.value), "</span>", digits_html(toks),
                      "</button>")
        end
    end
    print(io, "</div>")
    if f.single && any(b -> occursin(" + O(", b.value), f.branches)
        print(io, "<p class=\"note\">fog: the digits from ", f.p, "<sup>", f.prec, "</sup> on are not known",
              rw === nothing ? "" : " · click the fog for 12 more digits (prec = $(f.prec + 12))", "</p>")
    end
    return String(take!(io))
end

function projection_html(pr::Projection)
    io = IOBuffer()
    print(io, "<div class=\"scroll\"><div class=\"strip\" role=\"table\">")
    for (pl, t) in pr.entries
        if pl == "real"
            print(io, "<div class=\"pl real\"><span class=\"name\">R</span><span class=\"val\">", esc(t),
                  "</span><span class=\"sub\">the driver's real-ball text</span></div>")
        else
            p = parse(Int, pl)
            toks = branch_tokens(Branch(p, "", t), p)
            print(io, "<div class=\"pl\"><span class=\"name\">", p, "</span><span class=\"val\">", esc(t),
                "</span>",
                  digits_html(toks), "</div>")
        end
    end
    print(io, "<div class=\"pl rest\"><span class=\"name\">every other place</span><span class=\"val\">",
          "not in the partial ball</span><span class=\"sub\">project keeps only the places asked for</span></div>")
    print(io, "</div></div>")
    return String(take!(io))
end

function output_of(val, lines, rw, cellid)
    if val isa ADELIC
        t = value_text(val)
        ps = places(val)
        d = dump_text(val)
        html = strip_html(val) * "<details class=\"dump\"><summary>dump</summary><code>" * esc(d) *
               "</code></details>"
        cols = [Pair{String,Any}["p" => c.p, "v" => c.v, "k" => c.k, "digits" => c.digits] for c in ps.columns]
        extra = Pair{String,Any}["real" => ps.real, "places" => cols,
            "cofactors" => [string(c) for c in ps.cofactors],
                                 "rest" => ps.rest]
        # No command of its own: `load` of the dump, the identity-preserving form (conventions 9.6).
        return Output(_kind(val), t, "OK", d, lines === nothing ? ["load $(d)"] : lines, html, extra)
    elseif val isa Fan
        bs = Any[]
        for b in val.branches
            e = Pair{String,Any}["seed" => b.seed, "value" => b.value, "digits" => branch_digit_text(b, val.p)]
            if rw !== nothing && !val.single
                push!(e, "rewrite" => _rootcall(rw, seed_arg(b), rw.prec))
            end
            push!(bs, e)
        end
        return Output(val.single ? "branch" : "fan", val.line, "OK", nothing, val.lines, fan_html(val, rw, cellid),
                      Pair{String,Any}["place" => val.p, "prec" => val.prec, "branches" => bs])
    elseif val isa Projection
        es = Any[Pair{String,Any}["place" => p, "text" => t] for (p, t) in val.entries]
        return Output("projection", val.line, "OK", nothing, val.lines, projection_html(val),
                      Pair{String,Any}["entries" => es])
    else
        t = val isa AbstractString ? String(val) : repr(val)
        return Output("text", t, nothing, nothing, lines, "<pre class=\"plain\">" * esc(t) * "</pre>",
            Pair{String,Any}[])
    end
end

function status_output(status::String, op::String, lines, source::String)
    html = card_html(status, op; source)
    return Output("status", "error: " * status, status, nothing, lines, html,
                  Pair{String,Any}["place" => "not given", "op" => op, "source" => source])
end

"""
    evaluate(m, text, cellid) -> Output

Evaluates one cell (one Julia expression, optionally `name = expression`) in the module m.
"""
function evaluate(m::Module, text::String, cellid::String)
    ex = try
        Meta.parse(text)
    catch err
        msg = sprint(showerror, err)
        return Output("julia_error", msg, nothing, nothing, nothing, "<pre class=\"jerr\">" * esc(msg) * "</pre>",
                      Pair{String,Any}[])
    end
    target, rhs = nothing, ex
    if ex isa Expr && ex.head === :(=) && ex.args[1] isa Symbol
        target, rhs = ex.args[1], ex.args[2]
    end
    lines, op = nothing, ""
    try
        thunk, lines, op = plan(m, rhs)
        val = thunk()
        lit = literal_text(rhs)
        lit !== nothing && val isa ADELIC && (PROVENANCE[m][val] = lit)
        target === nothing || Core.eval(m, Expr(:(=), target, QuoteNode(val)))
        return output_of(val, lines, rewrite_info(rhs, target), cellid)
    catch err
        if err isa DriverError
            return status_output(string(err.status), err.op, err.lines, "driver")
        elseif err isa Adelefeld.AdfError
            return status_output(string(err.status), isempty(op) ? string(err.op) : op, lines, "package")
        end
        msg = first(split(sprint(showerror, err), '\n'))
        return Output("julia_error", msg, nothing, nothing, nothing, "<pre class=\"jerr\">" * esc(msg) * "</pre>",
                      Pair{String,Any}[])
    end
end

function new_notebook_module()
    m = Module(:Notebook)
    Core.eval(m, :(using Adelefeld))
    PROVENANCE[m] = IdDict{Any,String}()
    Core.eval(m, :(const Adelica = $(Adelica)))
    Core.eval(m, :(using .Adelica))
    return m
end

record(i, text, o::Output) = Pair{String,Any}["cell" => i, "input" => text, "kind" => o.kind, "value" => o.value,
    "status" => o.status, "dump" => o.dump, "driver" => o.driver, o.extra...]

"""
    run(cells, out; notes = nothing, source = nothing) -> Vector{Output}

Evaluates the cells in order in a fresh module and writes `out.html` and `out.records.jsonl`.
"""
function run(cells::Vector{String}, out::AbstractString; notes = nothing, source = nothing)
    m = new_notebook_module()
    outs = Output[]
    for (i, c) in enumerate(cells)
        push!(outs, evaluate(m, c, "c$(i)"))
    end
    mkpath(dirname(abspath(out)))
    open(out * ".records.jsonl", "w") do io
        for (i, (c, o)) in enumerate(zip(cells, outs))
            json(io, record(i, c, o)); print(io, '\n')
        end
    end
    write(out * ".html", page_html(cells, outs, basename(out); notes, source))
    return outs
end

"""
    read_notebook(path) -> (cells, notes)

A notebook file: Julia, cells separated by lines that begin `# %%`; the rest of such a line is the note
shown above the cell. Comment lines inside a cell are notes too; the text before the first marker (the
preamble that makes the file a plain script) is not a cell.
"""
function read_notebook(path::AbstractString)
    cells, notes = String[], String[]
    cur, note, started = String[], String[], false
    endcell() = started && (push!(cells, strip(join(cur, "\n"))); push!(notes, join(note, " ")))
    for l in eachline(path)
        if startswith(l, "# %%")
            endcell()
            cur, note, started = String[], String[], true
            t = strip(l[5:end]); isempty(t) || push!(note, t)
        elseif started && startswith(strip(l), "#")
            push!(note, strip(lstrip(strip(l), '#')))
        elseif started
            push!(cur, l)
        end
    end
    endcell()
    return String.(cells), notes
end
read_cells(path::AbstractString) = read_notebook(path)[1]

"Runs a notebook file; `out` is the path without extension."
function run_file(path::AbstractString, out::AbstractString)
    cells, notes = read_notebook(path)
    return run(cells, out; notes, source = relpath(abspath(path), REPO))
end

read_records(path::AbstractString) = [json_parse(l) for l in eachline(path) if !isempty(strip(l))]

"""
    replay(records, adf; io = stdout) -> number of differences

Runs the driver lines of each record (one driver process per record) and compares the output line with
the record's value text: prints `cell i: same`, `cell i: differs` or `cell i: no driver line`.
"""
function replay(records::AbstractString, adf::AbstractString; io::IO = stdout)
    n = 0
    for r in read_records(records)
        i = r["cell"]
        if r["driver"] === nothing
            println(io, "cell ", i, ": no driver line")
            continue
        end
        got = driver_line(String.(r["driver"]); adf)
        if got == r["value"]
            println(io, "cell ", i, ": same: ", got)
        else
            n += 1
            println(io, "cell ", i, ": differs: record ", repr(r["value"]), ", driver ", repr(got))
        end
    end
    return n
end

include("page.jl")
