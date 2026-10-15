# The notebook: cells evaluated in order in a fresh module, one static page, one records sidecar, replay.
#
# A record (one JSON object per line of `<out>.records.jsonl`, one per cell output; the agent's window):
#   cell     the index of the cell, from 1        input   the cell text
#   kind     rat, fball, adele, cadele (the driver's `type` names), fan, branch, projection, status, text,
#            julia_error
#   value    the value text (package printer), or the driver's line, or `error: STATUS`
#   status   OK or the name of the status; null for text and julia_error
#   dump     the package's dump form (conventions 10.1) for rat, fball, adele, cadele; the driver's dump of
#            the local ball for a branch; the driver's dump of a partial ball with no real component
#   dump_reason  null when there is a dump, else why there is none
#   driver   the lines (settings, then one command) whose output line reproduces `value`; null if none
#   computed_by  what computed the value: the package, or the driver and its command
#   unchecked    null, or why no driver line reproduces the value (replay then says `unchecked`)
#   stamp    the package version and the commit of the library sources (provenance)
#   and per kind: places, real, cofactors, rest (a strip); branches (a fan); entries (a projection);
#   place, op, source, reason, actions (a status).

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

const COMPUTED_NOTE = "not reproducible by the driver: computed value. Its printed real ball may be wider than " *
    "the ball (docs/conventions.md 9.6) and the driver reads no dump as an operand, so the package computed it"

# The plan of a cell's right-hand side: (thunk, lines, op, why). The thunk computes the value; `lines` are the
# driver lines that reproduce it, when the top-level call is one whose driver command is known; `why` is the
# reason when such a call has no driver line. An operand is written as `lossless_operand` gives it (the source
# text of a literal, CURRENT_PROV; or a text that denotes exactly the value). A computed adele whose printed
# text would widen is never written as that text (repair D2): the package computes, and `why` says so.
function plan(m::Module, ex)
    if ex isa Expr && ex.head === :call && ex.args[1] isa Symbol
        f = ex.args[1]
        pos, kws = _call_parts(ex)
        if haskey(BINOPS, f) && length(pos) == 2 && isempty(kws)
            a, b = Core.eval(m, pos[1]), Core.eval(m, pos[2])
            if a isa DRIVABLE && b isa DRIVABLE && (a isa ADELIC || b isa ADELIC)
                name, op = BINOPS[f]
                ta, tb = lossless_operand(a), lossless_operand(b)
                (ta === nothing || tb === nothing) && return (() -> op(a, b)), nothing, name, COMPUTED_NOTE
                return (() -> op(a, b)), ["$(name) $(ta) with $(tb)"], name, nothing
            end
            return (() -> BINOPS[f][2](a, b)), nothing, "", nothing
        elseif f === :- && length(pos) == 1 && isempty(kws)
            a = Core.eval(m, pos[1])
            if a isa ADELIC
                ta = lossless_operand(a)
                ta === nothing && return (() -> -a), nothing, "neg", COMPUTED_NOTE
                return (() -> -a), ["neg $(ta)"], "neg", nothing
            end
        elseif f === :reconstruct && isempty(kws)
            v = [Core.eval(m, a) for a in pos]
            if length(v) == 1 && v[1] isa Adele
                t = lossless_operand(v[1])
                t === nothing && return (() -> Adelefeld.reconstruct(v[1])), nothing, "reconstruct", COMPUTED_NOTE
                return (() -> Adelefeld.reconstruct(v[1])), ["reconstruct $(t)"], "reconstruct", nothing
            elseif length(v) == 3 && v[1] isa FBall && all(x -> x isa Union{Rat,Integer,Rational}, v[2:3])
                lo, hi = (x isa Rat ? x : Rat(x) for x in v[2:3])
                return (() -> Adelefeld.reconstruct(v[1], lo, hi)),
                       ["reconstruct $(lossless_operand(v[1])) with $(_operand(lo)) with $(_operand(hi))"],
                       "reconstruct", nothing
            end
        elseif f === :recover && length(pos) == 1
            x = Core.eval(m, pos[1])
            kv = Dict(k => Core.eval(m, v) for (k, v) in kws)
            if x isa FBall && haskey(kv, :num) && haskey(kv, :den)
                return (() -> recover(x; num = kv[:num], den = kv[:den])),
                       ["recover $(lossless_operand(x)) with $(kv[:num]) with $(kv[:den])"], "recover", nothing
            end
        end
    end
    return (() -> Core.eval(m, ex)), nothing, "", nothing
end

struct Output
    kind::String
    value::String
    status::Union{Nothing,String}
    dump::Union{Nothing,String}
    driver::Union{Nothing,Vector{String}}
    html::String
    extra::Vector{Pair{String,Any}}
    dump_reason::Union{Nothing,String}
    computed_by::String
    unchecked::Union{Nothing,String}
end
Output(kind, value, status, dump, driver, html, extra; dump_reason = nothing, computed_by = "package",
       unchecked = nothing) =
    Output(kind, value, status, dump, driver, html, extra, dump_reason, computed_by, unchecked)

const NO_VALUE_DUMP = "not a value of the package or the driver: no dump"

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

"The set of a branch in words (the hover text, as on a strip)."
function branch_set_words(b::Branch, p::Integer)
    m = match(r"^(.+) \+ O\(([0-9]+)\^(-?[0-9]+)\)$", b.value)
    m === nothing && return "the set {" * b.value * "}: one point of Q_$(p), exact"
    return "the set " * m[1] * " + $(p)^" * m[3] * " Z_$(p): a ball of Q_$(p), the digits from $(p)^" * m[3] *
           " on not known"
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
            fogattr, refine = "", ""
            if inexact && rw !== nothing
                rew = _rootcall(rw, rw.seed === nothing ? seed_arg(b) : rw.seed, f.prec + 12)
                fogattr = _attr(cellid, rew) * " tabindex=\"0\" role=\"button\" aria-label=\"12 more digits\"" *
                          " title=\"12 more digits: " * esc(rew) * "\""
                # Above 97 no digits and no fog are drawn: the refinement is a button (repair D4 of the drive).
                f.p > DIGITS_UP_TO && (refine = "<button class=\"refine\" type=\"button\"" * _attr(cellid, rew) *
                    " title=\"" * esc(rew) * "\">12 more digits (prec = $(f.prec + 12))</button>")
            end
            print(io, "<div class=\"chip picked\" title=\"", esc(branch_set_words(b, f.p)), "\">",
                  "<span class=\"seed\">", b.place, " [", esc(b.seed), "] · ", known, "</span>",
                  digits_html(toks; fogattr), "<span class=\"v\">", esc(b.value), "</span>", refine, "</div>")
        else
            attr = rw === nothing ? "" : _attr(cellid, _rootcall(rw, seed_arg(b), rw.prec))
            print(io, "<button class=\"chip\" type=\"button\" title=\"", esc(branch_set_words(b, f.p)), "\"", attr,
                  "><span class=\"seed\">", b.place, " [",
                  esc(b.seed), "]</span><span class=\"v\">", esc(b.value), "</span>", digits_html(toks),
                      "</button>")
        end
    end
    print(io, "</div>")
    if f.single && any(b -> occursin(" + O(", b.value), f.branches)
        ask = rw === nothing ? "" : f.p > DIGITS_UP_TO ?
              " · no digits above 97: the button asks for 12 more digits (prec = $(f.prec + 12))" :
              " · click the fog for 12 more digits (prec = $(f.prec + 12))"
        print(io, "<p class=\"note\">fog: the digits from ", f.p, "<sup>", f.prec, "</sup> on are not known", ask,
              "</p>")
    end
    return String(take!(io))
end

function projection_set_words(pr::Projection)
    return "the set: the partial ball over the places " * join((p for (p, _) in pr.entries), ", ") *
           "; at each place the component printed in its column, no information at every other place"
end

function projection_html(pr::Projection)
    io = IOBuffer()
    print(io, "<div class=\"scroll\"><div class=\"strip\" role=\"table\" title=\"", esc(projection_set_words(pr)),
          "\">")
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

function dump_html(rows)
    io = IOBuffer()
    print(io, "<details class=\"dump\"><summary>dump</summary>")
    for (label, d) in rows
        print(io, "<div>", isempty(label) ? "" : esc(label) * ": ", "<code>", esc(d), "</code></div>")
    end
    print(io, "</details>")
    return String(take!(io))
end

const COMPONENT_DUMP_REASON = "no dump of the partial ball: its real component is the driver's printed text, an " *
    "enclosure that may be wider than the computed ball (docs/conventions.md 9.6), and the package has no " *
    "partial-ball type. dump_primes is the driver's dump of the components at the primes, which are exact data"

# The partial ball of the components at the primes, in the value form of a partial ball (conventions 9.2,
# `{p=5: ...}`; tools/adf/README.md:255-262 gives the driver's line, whose local texts are the same).
sball_text(es) = "{" * join(("p=$(p): $(t)" for (p, t) in es), "; ") * "}"

function output_of(val, lines, rw, cellid; computed_by = "package", why = nothing)
    if val isa ADELIC
        t = value_text(val)
        ps = places(val)
        d = dump_text(val)
        html = strip_html(val) * dump_html([("", d)])
        cols = [Pair{String,Any}["p" => c.p, "v" => c.v, "k" => c.k, "digits" => c.digits] for c in ps.columns]
        extra = Pair{String,Any}["real" => ps.real, "places" => cols,
            "cofactors" => [string(c) for c in ps.cofactors],
                                 "rest" => ps.rest]
        # No command of its own: `load` of the dump, the identity-preserving form (conventions 9.6). A computed
        # value whose operands have no exact driver text has no driver line at all (`why`).
        drv = why !== nothing ? nothing : lines === nothing ? ["load $(d)"] : lines
        return Output(_kind(val), t, "OK", d, drv, html, extra; computed_by, unchecked = why)
    elseif val isa Fan
        bs = Any[]
        rows = Tuple{String,String}[]
        for b in val.branches
            d, reason = local_dump(val.p, b.value)
            e = Pair{String,Any}["seed" => b.seed, "value" => b.value, "digits" => branch_digit_text(b, val.p),
                                 "dump" => d]
            reason === nothing || push!(e, "dump_reason" => reason)
            d === nothing || push!(rows, ("$(b.place) [$(b.seed)]", d))
            if rw !== nothing && !val.single
                push!(e, "rewrite" => _rootcall(rw, seed_arg(b), rw.prec))
            end
            push!(bs, e)
        end
        single = val.single && length(bs) == 1
        d = single ? bs[1][4].second : nothing
        reason = single ? (d === nothing ? bs[1][5].second : nothing) :
                 "one dump per branch: branches[i].dump, the driver's dump of the local ball [p=$(val.p): value]"
        return Output(val.single ? "branch" : "fan", val.line, "OK", d, val.lines,
                      fan_html(val, rw, cellid) * dump_html(rows),
                      Pair{String,Any}["place" => val.p, "prec" => val.prec, "branches" => bs];
                      dump_reason = reason, computed_by = val.computed_by)
    elseif val isa Projection
        es = Any[Pair{String,Any}["place" => p, "text" => t] for (p, t) in val.entries]
        fin = [(p, t) for (p, t) in val.entries if p != "real"]
        dfin = isempty(fin) ? nothing : driver_line(["dump " * sball_text(fin)])
        dfin !== nothing && startswith(dfin, "error: ") && (dfin = nothing)
        hasreal = any(e -> e[1] == "real", val.entries)
        d = hasreal ? nothing : dfin
        reason = hasreal ? COMPONENT_DUMP_REASON : d === nothing ? "the driver gave no dump of " * sball_text(fin) :
                 nothing
        rows = Tuple{String,String}[]
        hasreal && push!(rows, ("no dump", COMPONENT_DUMP_REASON))
        dfin === nothing || push!(rows, (hasreal ? "the components at the primes" : "", dfin))
        extra = Pair{String,Any}["entries" => es]
        hasreal && push!(extra, "dump_primes" => dfin)
        return Output("projection", val.line, "OK", d, val.lines, projection_html(val) * dump_html(rows), extra;
                      dump_reason = reason, computed_by = val.computed_by,
                      unchecked = val.lines === nothing ? COMPUTED_NOTE : nothing)
    else
        t = val isa AbstractString ? String(val) : repr(val)
        return Output("text", t, nothing, nothing, lines, "<pre class=\"plain\">" * esc(t) * "</pre>",
            Pair{String,Any}[]; dump_reason = NO_VALUE_DUMP, computed_by = "julia",
            unchecked = lines === nothing ? "not a value of the package: no driver line" : nothing)
    end
end

function status_output(status::String, op::String, lines, source::String; args = Dict{Symbol,Any}(),
                       why = nothing)
    html = card_html(status, op; source, args)
    reason, acts = card_reason(status, op, args)
    return Output("status", "error: " * status, status, nothing, why === nothing ? lines : nothing, html,
                  Pair{String,Any}["place" => "not given", "op" => op, "source" => source, "reason" => reason,
                                   "actions" => acts];
                  dump_reason = "a status, not a value: no dump", computed_by = source * " " * op,
                  unchecked = why !== nothing ? why : lines === nothing ? "no driver line for this call" : nothing)
end

julia_error_output(msg) = Output("julia_error", msg, nothing, nothing, nothing,
    "<pre class=\"jerr\">" * esc(msg) * "</pre>", Pair{String,Any}[]; dump_reason = NO_VALUE_DUMP,
    computed_by = "julia", unchecked = "a Julia error: no value and no driver line")

# A literal written as a positional argument of the top-level call (`adf"..." + Rat(0)`, `recover(adf"..."; ...)`)
# is read once here and its source text recorded (PROVENANCE), so that the driver line carries that text and
# not the printed one; the call then holds the value itself.
function hoist_literals(m::Module, ex)
    ex isa Expr && ex.head === :call || return ex
    args = Any[ex.args[1]]
    for a in ex.args[2:end]
        lt = literal_text(a)
        if lt !== nothing
            v = Core.eval(m, a)
            if v isa ADELIC
                PROVENANCE[m][v] = lt
                push!(args, v)
                continue
            end
        end
        push!(args, a)
    end
    return Expr(:call, args...)
end

"""
    evaluate(m, text, cellid) -> Output

Evaluates one cell (one Julia expression, optionally `name = expression`) in the module m.
"""
function evaluate(m::Module, text::String, cellid::String)
    ex = try
        Meta.parse(text)
    catch err
        return julia_error_output(sprint(showerror, err))
    end
    target, rhs = nothing, ex
    if ex isa Expr && ex.head === :(=) && ex.args[1] isa Symbol
        target, rhs = ex.args[1], ex.args[2]
    end
    lines, op, why = nothing, "", nothing
    CURRENT_PROV[] = PROVENANCE[m]
    try
        thunk, lines, op, why = plan(m, hoist_literals(m, rhs))
        val = thunk()
        lit = literal_text(rhs)
        lit !== nothing && val isa ADELIC && (PROVENANCE[m][val] = lit)
        target === nothing || Core.eval(m, Expr(:(=), target, QuoteNode(val)))
        by = op == "recover" ? "driver recover" : op == "" ? "package" : "package " * op
        return output_of(val, lines, rewrite_info(rhs, target), cellid; computed_by = by, why)
    catch err
        if err isa DriverError
            return status_output(string(err.status), err.op, err.lines, "driver"; args = err.args)
        elseif err isa Adelefeld.AdfError
            return status_output(string(err.status), isempty(op) ? string(err.op) : op, lines, "package"; why)
        end
        return julia_error_output(first(split(sprint(showerror, err), '\n')))
    finally
        CURRENT_PROV[] = nothing
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

record(i, text, o::Output, stamp) = Pair{String,Any}["cell" => i, "input" => text, "kind" => o.kind,
    "value" => o.value, "status" => o.status, "dump" => o.dump, "dump_reason" => o.dump_reason,
    "driver" => o.driver, "computed_by" => o.computed_by, "unchecked" => o.unchecked, "stamp" => stamp, o.extra...]

# Provenance (repair item 6). The package exposes its own version (julia/Adelefeld/Project.toml, read by
# Base.pkgversion) but no version of the library or of FLINT: its load gate checks only that the run-time FLINT
# agrees in major and minor version with the compiled one (julia/Adelefeld/src/load.jl:159-166). The commit is
# the last one that changed the library sources src/, include/ and tools/adf (read by `git log`, which changes
# nothing), so that the stamp is the same on every run between two changes of the library.
const LIB_PATHS = ["src", "include", "tools/adf"]
function library_commit()
    c = try
        readchomp(`git -C $(REPO) log -1 --format=%h -- $(LIB_PATHS)`)
    catch
        ""
    end
    isempty(c) && return "commit: not known (no git)"
    dirty = try
        !success(`git -C $(REPO) diff --quiet HEAD -- $(LIB_PATHS)`)
    catch
        false
    end
    return "commit " * c * (dirty ? " with uncommitted changes" : "")
end
stamp_text() = "Adelefeld.jl $(pkgversion(Adelefeld)); library sources (src, include, tools/adf) at " *
    library_commit() * "; library and FLINT versions: not exposed by the package"

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
    stamp = stamp_text()
    mkpath(dirname(abspath(out)))
    open(out * ".records.jsonl", "w") do io
        for (i, (c, o)) in enumerate(zip(cells, outs))
            json(io, record(i, c, o, stamp)); print(io, '\n')
        end
    end
    write(out * ".html", page_html(cells, outs, basename(out); notes, source, stamp))
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

# ---- Replay (repair D1 of the test drive) ----
# A record is `same` only when every field it claims that the driver line can reproduce agrees: the value text,
# the status, and what derives from the driver's value (the dump of a rat or fball, the dump in a `load` line,
# the strip columns and digits, the branches and their local dumps, the entries and dump of a projection), and
# the literal arguments of the input agree with the driver line. A record without a driver line is `unchecked`.

_get(r, k) = get(r, k, nothing)
_neq(what, rec, drv) = "$(what): record $(repr(rec)), from the driver $(repr(drv))"

# The fields of a strip derived from a value text (the driver's output line), as `output_of` writes them.
function strip_fields(t::AbstractString)
    vt = parse_value_text(t)
    ps = if vt.kind in (:rat, :fball)
        places(readvalue(t))      # the text of a Rat or an FBall is exact
    else
        fin = vt.modulus == 0 ? _q(vt.centre) : _q(vt.centre) * " mod " * _q(vt.modulus)
        f = readvalue(fin)
        p = places(f)
        Places(vt.kind, vt.real, false, p.exact, p.centre, p.modulus, p.columns, p.cofactors, p.rest)
    end
    cols = [Dict{String,Any}("p" => c.p, "v" => c.v, "k" => c.k, "digits" => c.digits) for c in ps.columns]
    return Dict{String,Any}("real" => ps.real, "places" => cols, "cofactors" => [string(c) for c in ps.cofactors],
                            "rest" => ps.rest)
end

function compare_derived!(diffs, notes, r, got, adf)
    kind = _get(r, "kind")
    if kind in ("rat", "fball", "adele", "cadele")
        drv = String.(r["driver"])
        if startswith(drv[end], "load ")
            drv[end][6:end] == _get(r, "dump") || push!(diffs, _neq("dump", _get(r, "dump"), drv[end][6:end]))
        elseif kind in ("rat", "fball")
            d = dump_text(readvalue(got))
            d == _get(r, "dump") || push!(diffs, _neq("dump", _get(r, "dump"), d))
        else
            push!(notes, "the dump (the driver prints the real ball as an enclosure, docs/conventions.md 9.6)")
        end
        s = strip_fields(got)
        for k in ("real", "cofactors", "rest")
            haskey(r, k) && r[k] != s[k] && push!(diffs, _neq(k, r[k], s[k]))
        end
        if haskey(r, "places")
            rp, sp = r["places"], s["places"]
            [c["p"] for c in rp] == [c["p"] for c in sp] ||
                push!(diffs, _neq("primes", [c["p"] for c in rp], [c["p"] for c in sp]))
            for (a, b) in zip(rp, sp), f in ("v", "k", "digits")
                a["p"] == b["p"] && get(a, f, nothing) != b[f] &&
                    push!(diffs, _neq("$(f) at $(a["p"])", get(a, f, nothing), b[f]))
            end
        end
    elseif kind in ("fan", "branch")
        p = r["place"]
        bs = _branches(got)
        rb = r["branches"]
        length(rb) == length(bs) || return push!(diffs, _neq("the number of branches", length(rb), length(bs)))
        for (e, b) in zip(rb, bs)
            e["seed"] == b.seed || push!(diffs, _neq("seed", e["seed"], b.seed))
            dg = branch_digit_text(b, p)
            e["digits"] == dg || push!(diffs, _neq("digits of [$(b.seed)]", e["digits"], dg))
            if haskey(e, "dump")
                d = local_dump(p, b.value; adf)[1]
                e["dump"] == d || push!(diffs, _neq("dump of [$(b.seed)]", e["dump"], d))
            end
        end
        if kind == "branch" && length(rb) == 1 && haskey(rb[1], "dump") && _get(r, "dump") != rb[1]["dump"]
            push!(diffs, _neq("dump", _get(r, "dump"), rb[1]["dump"]))
        end
    elseif kind == "projection"
        es = _entries(got)
        rec = [(e["place"], e["text"]) for e in r["entries"]]
        rec == es || push!(diffs, _neq("entries", rec, es))
        fin = [(p, t) for (p, t) in es if p != "real"]
        if _get(r, "dump") !== nothing || _get(r, "dump_primes") !== nothing
            d = driver_line(["dump " * sball_text(fin)]; adf)
            for k in ("dump", "dump_primes")
                _get(r, k) === nothing || _get(r, k) == d || push!(diffs, _neq(k, r[k], d))
            end
        end
    end
end

const _LITERAL_CALLS = (:Rat, :FBall, :readvalue)
_is_literal(x) = x isa Union{Integer,String} ||
    (x isa Expr && x.head === :call && x.args[1] in _LITERAL_CALLS && all(_is_literal, x.args[2:end])) ||
    literal_text(x) !== nothing || (x isa Expr && x.head === :call && x.args[1] === :- && length(x.args) == 2 &&
                                    x.args[2] isa Integer)
_lit_value(x) = x isa Integer ? x : Core.eval(new_notebook_module(), x)
_lit_text(x) = x isa Integer ? string(x) : (lt = literal_text(x); lt !== nothing ? lt :
                                            lossless_operand(_lit_value(x)))

# Two operand texts denote the same value when they are equal, or when the package reads both to the same dump
# (`1 mod 2003` and `(* ; 1 mod 2003)`). A printed real ball that was widened reads to another dump.
function same_denotation(a::AbstractString, b::AbstractString)
    a == b && return true
    da = try dump_text(readvalue(a)) catch; return false end
    db = try dump_text(readvalue(b)) catch; return false end
    return da == db
end

"""
    check_input(r, diffs, notes)

Compares the literal arguments of the record's input with its driver line: the numbers, places, seeds and
precisions, and the operand when it is a literal; an operand that is a variable of the notebook is noted as
not checked (replay runs the driver line, not the cells). A cell that is a literal value is compared with the
dump that its `load` line reads.
"""
function check_input(r, diffs, notes)
    ex = try Meta.parse(r["input"]) catch; return push!(diffs, "input: not a Julia expression") end
    rhs = ex isa Expr && ex.head === :(=) && ex.args[1] isa Symbol ? ex.args[2] : ex
    drv = String.(r["driver"])
    cmd = drv[end]
    sp = findfirst(' ', cmd)
    word = sp === nothing ? cmd : cmd[1:sp-1]
    opnds = sp === nothing ? String[] : String.(split(cmd[sp+1:end], " with "))
    settings = Dict(split(l, ' ')[1] => split(l, ' ')[2] for l in drv[1:end-1] if count(' ', l) == 1)
    want(what, a, b) = string(a) == string(b) || push!(diffs, "input: $(what) $(a), the driver line has $(b)")
    function operand(i, x)
        i <= length(opnds) || return push!(diffs, "input: the driver line has no operand $(i)")
        if _is_literal(x)
            t = _lit_text(x)
            t === nothing ? push!(notes, "the input operand $(x)") :
                same_denotation(t, opnds[i]) || want("operand $(i) is", t, opnds[i])
        else
            push!(notes, "the input operand $(x) (a variable of the notebook)")
        end
    end
    if word == "load" && _is_literal(rhs)
        v = _lit_value(rhs)
        v isa ADELIC ? want("value has the dump", dump_text(v), opnds[1]) : push!(notes, "the input")
        return
    end
    rhs isa Expr && rhs.head === :call && rhs.args[1] isa Symbol || return push!(notes, "the input")
    f = rhs.args[1]
    pos, kws = _call_parts(rhs)
    kv = Dict(kws)
    expect = f === :roots ? "roots_at" : f === :root ? "root_at" : f in (:recover, :project, :reconstruct) ?
             string(f) : (f === :- && length(pos) == 1) ? "neg" : haskey(BINOPS, f) ? BINOPS[f][1] : nothing
    expect === nothing && return push!(notes, "the input")
    want("calls", expect, word)
    word == expect || return
    if f in (:roots, :root)
        operand(1, pos[1])
        for (i, x) in ((2, get(kv, :at, nothing)), (3, pos[2]))
            x isa Integer ? want(i == 2 ? "at =" : "degree", x, get(opnds, i, "")) : push!(notes, "the input $(x)")
        end
        if f === :root
            s = string(get(kv, :seed, ""))
            want("seed =", s == "+1" ? "1" : s, get(opnds, 4, ""))
        end
        want("prec =", get(kv, :prec, 8), get(settings, "prec", ""))
    elseif f === :recover
        operand(1, pos[1])
        for (i, k) in ((2, :num), (3, :den))
            x = get(kv, k, nothing)
            x isa Integer ? want("$(k) =", x, get(opnds, i, "")) : push!(notes, "the input $(k) = $(x)")
        end
    elseif f === :project
        operand(1, pos[1])
        pls = [p isa QuoteNode ? string(p.value) : string(p) for p in pos[2:end]]
        want("places", join(pls, " "), get(opnds, 2, ""))
        want("prec =", get(kv, :prec, 64), get(settings, "prec", ""))
    else
        for (i, x) in enumerate(pos)
            operand(i, x)
        end
    end
end

"""
    replay_summary(records, adf; io = stdout) -> (same, differs, unchecked)

Runs the driver lines of each record (one driver process per record, and one per local dump) and compares
every field the record claims that the driver can reproduce. Prints `cell i: same`, `cell i: differs: ...`
or `cell i: unchecked: <why>`, one line per record.
"""
function replay_summary(records::AbstractString, adf::AbstractString; io::IO = stdout)
    same = differs = unchecked = 0
    for r in read_records(records)
        i = r["cell"]
        if r["driver"] === nothing
            unchecked += 1
            why = _get(r, "unchecked")
            println(io, "cell ", i, ": unchecked: ", why === nothing ? "no driver line" : why)
            continue
        end
        got = driver_line(String.(r["driver"]); adf)
        diffs, notes = String[], String[]
        got == r["value"] || push!(diffs, _neq("value", r["value"], got))
        st = startswith(got, "error: ") ? got[8:end] : "OK"
        _get(r, "status") === nothing || r["status"] == st || push!(diffs, _neq("status", r["status"], st))
        if isempty(diffs)
            try
                compare_derived!(diffs, notes, r, got, adf)
            catch err
                msg = first(split(sprint(showerror, err), '\n'))
                push!(diffs, "the derived fields could not be recomputed: " * msg)
            end
        end
        check_input(r, diffs, notes)
        if isempty(diffs)
            same += 1
            println(io, "cell ", i, ": same: ", got,
                    isempty(notes) ? "" : " (not checked: " * join(notes, "; ") * ")")
        else
            differs += 1
            println(io, "cell ", i, ": differs: ", join(diffs, "; "))
        end
    end
    return (same = same, differs = differs, unchecked = unchecked)
end

replay_sentence(s) = "replay: $(s.same) same, $(s.differs) differs, $(s.unchecked) unchecked"

"""
    replay(records, adf; io = stdout) -> number of records that differ
"""
replay(records::AbstractString, adf::AbstractString; io::IO = stdout) = replay_summary(records, adf; io).differs

include("page.jl")
