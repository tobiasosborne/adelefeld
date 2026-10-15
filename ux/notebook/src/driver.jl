# The calls that run the driver build/adf (tools/adf/README.md) as a subprocess: the package has no local
# roots, no residue reconstruction and no partial balls yet (docs/api-j.md section 9: J-b to J-d).
#
#   roots_at X with PRIME with DEGREE            tools/adf/README.md:482-514, every branch, in identifier order
#   root_at X with PRIME with DEGREE with SEED   the same section, one branch
#   recover C mod M with A with B                tools/adf/README.md:184-194, search limit 1000
#   project X with PLACES                        tools/adf/README.md:214-271
#
# The driver prints one line per command and `error: STATUS` on a failure, without the failing place
# (tools/adf/README.md:253, :579-580); D-UX3: a card then says "place: not given".

driver_path() = get(ENV, "ADF_DRIVER", joinpath(REPO, "build", "adf"))

"""
    DriverError(op, status, lines)

The driver answered `error: STATUS` to `lines` (the settings and the one command sent).
"""
struct DriverError <: Exception
    op::String
    status::Adelefeld.AdfStatus
    lines::Vector{String}
    args::Dict{Symbol,Any}      # what the call asked for (:at, :n, :seed), for the card's reason
end
DriverError(op, status, lines) = DriverError(op, status, lines, Dict{Symbol,Any}())
Base.showerror(io::IO, e::DriverError) = print(io, "DriverError: ", e.op, " answered error: ", e.status)

"""
    driver_line(lines; adf = driver_path()) -> String

Runs the driver on the lines (settings, then exactly one command) under `timeout 60` and returns its one
output line.
"""
function driver_line(lines::Vector{String}; adf::AbstractString = driver_path())
    isfile(adf) || error("the driver $(adf) is missing: run `make -j2 && make -C tools/adf`")
    script = join(lines, "\n") * "\n"
    out = read(pipeline(ignorestatus(`timeout 60 $adf`); stdin = IOBuffer(script)), String)
    ls = split(chomp(out), '\n')
    length(ls) == 1 || error("the driver gave $(length(ls)) lines for one command: " * repr(out))
    return String(ls[1])
end

function _status_of(line::AbstractString)
    name = line[length("error: ")+1:end]
    for s in instances(Adelefeld.AdfStatus)
        string(s) == name && return s
    end
    error("not a status name: " * repr(line))
end

function _call(op::String, lines::Vector{String}; args = Dict{Symbol,Any}())
    out = driver_line(lines)
    startswith(out, "error: ") && throw(DriverError(op, _status_of(out), lines, args))
    return out
end

_operand(x::Union{Integer,Rational}) = value_text(Rat(x))
_operand(x::ADELIC) = value_text(x)
_place(p::Integer) = string(p)
_place(p::Union{Symbol,AbstractString}) = string(p) == "real" ? "real" :
    throw(ArgumentError("a place is a prime or :real, not " * repr(p)))

"""
    Branch(place, seed, value)

One branch of a fan: the identifier the driver prints in brackets (the residue of the root modulo an odd
prime; `+1` or `-1` at 2; `0` for the zero) and the value text (a rational, or `C + O(p^N)`).
"""
struct Branch
    place::Int
    seed::String
    value::String
end

"""
    Fan

The result of `roots` (every branch) or `root` (one branch, `single = true`): the driver's line and
the branches parsed from it, with the lines that reproduce it.
"""
struct Fan
    x::String
    n::Int
    p::Int
    prec::Int
    single::Bool
    branches::Vector{Branch}
    lines::Vector{String}
    line::String
    computed_by::String
end

# ---- Driver operands (repair D2 of the test drive) ----
# The driver reads no dump as an operand (`project adf1 Q rat 1 1 with real` is error: PARSE), and a printed
# real ball read again may be wider than the ball printed (docs/conventions.md 9.6, :1331-1340). So a value is
# sent to the driver only as a text that denotes exactly that value: the text of a Rat or an FBall (exact),
# the source text of a literal (CURRENT_PROV, set by `evaluate`), or a value text that reads back to the same
# dump. A computed adele whose text widens has no such operand: `lossless_operand` gives `nothing`.

const CURRENT_PROV = Ref{Union{Nothing,IdDict{Any,String}}}(nothing)

function lossless_operand(x)
    prov = CURRENT_PROV[]
    prov !== nothing && haskey(prov, x) && return prov[x]
    x isa Union{Integer,Rational,Rat,FBall} && return _operand(x)
    x isa ADELIC || return _operand(x)
    t = value_text(x)
    y = try readvalue(t) catch; nothing end
    return (y isa typeof(x) && dump_text(y) == dump_text(x)) ? t : nothing
end

const FINITE_PART_NOTE = "on the exact finite part of a computed adele: at a prime only the finite part " *
    "counts (tools/adf/README.md:485, the commands project to the named prime)"

# The operand of a command at a prime: the value itself if it has a lossless text, else its finite part.
function local_operand(x)
    t = lossless_operand(x)
    t !== nothing && return t, ""
    x isa Union{Adele,CAdele} && return value_text(finitepart(x)), " " * FINITE_PART_NOTE
    throw(ArgumentError("no exact driver operand for a value of type $(typeof(x))"))
end

"""
    local_dump(p, value) -> (dump, reason)

The driver's dump (conventions 10.1, body lball) of the local ball `[p=P: value]` that a branch or a
component at a prime prints (`value` is a rational or `C + O(p^N)`, exact data: the dump is lossless).
"""
function local_dump(p::Integer, value::AbstractString; adf::AbstractString = driver_path())
    d = driver_line(["dump [p=$(p): $(value)]"]; adf)
    startswith(d, "error: ") && return nothing, "the driver's dump of [p=$(p): $(value)] answered " * d
    return d, nothing
end

function _branches(line::AbstractString)
    bs = Branch[]
    for part in split(line, "; ")
        m = match(r"^([0-9]+) \[([^\]]+)\]: (.+)$", part)
        m === nothing && error("not a branch of the driver: " * repr(part))
        push!(bs, Branch(parse(Int, m[1]), m[2], m[3]))
    end
    return bs
end

"""
    roots(x, n; at, prec = 8) -> Fan

The roots of degree n of x at the prime `at`: every branch, as the driver's `roots_at` lists them.
`prec` is the absolute p-adic precision of an irrational root (the driver's setting `prec`).
"""
function roots(x, n::Integer; at::Integer, prec::Integer = 8)
    t, note = local_operand(x)
    lines = ["prec $(prec)", "roots_at $(t) with $(at) with $(n)"]
    out = _call("roots_at", lines; args = Dict{Symbol,Any}(:at => at, :n => n))
    return Fan(t, n, at, prec, false, _branches(out), lines, out, "driver roots_at" * note)
end

"""
    root(x, n; at, seed, prec = 8) -> Fan

The one branch of `roots(x, n; at)` with the identifier `seed` (driver `root_at`). At 2 the seed is
1 or -1 (the driver prints `+1`).
"""
function root(x, n::Integer; at::Integer, seed, prec::Integer = 8)
    s = string(seed)
    s = s == "+1" ? "1" : s
    t, note = local_operand(x)
    lines = ["prec $(prec)", "root_at $(t) with $(at) with $(n) with $(s)"]
    out = _call("root_at", lines; args = Dict{Symbol,Any}(:at => at, :n => n, :seed => s))
    return Fan(t, n, at, prec, true, _branches(out), lines, out, "driver root_at" * note)
end

"The rewrite of a chip: the seed that `root` accepts for an identifier printed by the driver."
seed_arg(b::Branch) = b.seed == "+1" ? "1" : b.seed

"""
    branch_tokens(b, p)

The digit display of a branch value: an exact rational, or the ball `C + O(p^N)`.
"""
function branch_tokens(b::Branch, p::Integer)
    p > DIGITS_UP_TO && return Tuple{Symbol,String}[]
    m = match(r"^(.+) \+ O\(([0-9]+)\^(-?[0-9]+)\)$", b.value)
    if m === nothing
        return rat_digit_tokens(_rational(b.value), p)
    end
    return ball_digit_tokens(_rational(m[1]), p, parse(Int, m[3]))
end
branch_digit_text(b::Branch, p::Integer) = digit_text(branch_tokens(b, p))

"The number of digits known of a ball branch `C + O(p^N)`: the positions min(v, 0) to N - 1."
function known_digits(b::Branch)
    m = match(r"^(.+) \+ O\(([0-9]+)\^(-?[0-9]+)\)$", b.value)
    m === nothing && return nothing
    c, p, k = _rational(m[1]), parse(Int, m[2]), parse(Int, m[3])
    v = (c != 0 && vp(c, p) < k) ? vp(c, p) : k
    return k - min(v, 0)
end

"""
    recover_call(x; num, den) -> (Rat, lines)
    recover(x; num, den) -> Rat

The rational n/d with n = c d mod m, |n| <= num, 0 < d <= den, for the finite ball x = c mod m (SPEC 9.2,
"From partial data"; driver `recover`, search limit 1000). Throws DriverError on NO_SOLUTION, NOT_UNIQUE,
NOT_DETERMINED (the search limit), DOMAIN.
"""
function recover_call(x::FBall; num::Integer, den::Integer)
    lines = ["recover $(_operand(x)) with $(num) with $(den)"]
    out = _call("recover", lines)
    return readvalue(out), lines
end
recover(x::FBall; num::Integer, den::Integer) = recover_call(x; num, den)[1]

"""
    Projection

The partial ball of `project(x, places...)`: the driver's line (tools/adf/README.md:255-262, a text of the
driver, not a value form) and its entries `(place, text)`.
"""
struct Projection
    entries::Vector{Tuple{String,String}}
    lines::Union{Nothing,Vector{String}}   # nothing: no driver line reproduces the whole line
    line::String
    computed_by::String
end

function _entries(out::AbstractString)
    es = Tuple{String,String}[]
    for part in split(out, "; ")
        i = findfirst(": ", part)
        push!(es, (part[1:i.start-1], part[i.stop+1:end]))
    end
    return es
end

"""
    project(x, places...; prec = 64) -> Projection

The partial ball of x over the places (`:real` or a prime), through the driver's `project`. A computed adele
whose printed text would widen (no `lossless_operand`) is not sent as that text: its real component is the
package's `realpart` (the stored ball, at the precision of the value, so only `prec = 64` is accepted), and its
components at the primes are the driver's projection of its exact finite part (tools/adf/README.md:485).
"""
function project(x, places...; prec::Integer = 64)
    pls = [_place(p) for p in places]
    t = lossless_operand(x)
    if t !== nothing
        lines = ["prec $(prec)", "project $(t) with $(join(pls, " "))"]
        out = _call("project", lines)
        return Projection(_entries(out), lines, out, "driver project")
    end
    x isa Adele || throw(ArgumentError("project of a computed $(typeof(x)): no exact driver operand"))
    prec == 64 || throw(ArgumentError("project of a computed adele: the real component is the stored ball at " *
                                      "64 bits; prec = $(prec) is not applied by this slice"))
    allunique(pls) || throw(ArgumentError("a place is named twice"))
    es = Tuple{String,String}[]
    if "real" in pls
        rb = realpart(x)
        push!(es, ("real", value_text(rb)))
        close(rb)
    end
    primes = filter(!=("real"), pls)
    if !isempty(primes)
        lines = ["prec $(prec)", "project $(value_text(finitepart(x))) with $(join(primes, " "))"]
        append!(es, _entries(_call("project", lines)))
    end
    by = "package realpart for the real place" *
         (isempty(primes) ? "" : "; driver project " * FINITE_PART_NOTE * ": " * lines[end])
    return Projection(es, nothing, join(("$(p): $(v)" for (p, v) in es), "; "), by)
end

"""
    adf"text"

The value text read by the package (`readvalue`, precision 64 bits for a real coordinate).
"""
macro adf_str(s)
    return :($(Adelefeld.readvalue)($s))
end
