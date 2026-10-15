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
end
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

function _call(op::String, lines::Vector{String})
    out = driver_line(lines)
    startswith(out, "error: ") && throw(DriverError(op, _status_of(out), lines))
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
    lines = ["prec $(prec)", "roots_at $(_operand(x)) with $(at) with $(n)"]
    out = _call("roots_at", lines)
    return Fan(_operand(x), n, at, prec, false, _branches(out), lines, out)
end

"""
    root(x, n; at, seed, prec = 8) -> Fan

The one branch of `roots(x, n; at)` with the identifier `seed` (driver `root_at`). At 2 the seed is
1 or -1 (the driver prints `+1`).
"""
function root(x, n::Integer; at::Integer, seed, prec::Integer = 8)
    s = string(seed)
    s = s == "+1" ? "1" : s
    lines = ["prec $(prec)", "root_at $(_operand(x)) with $(at) with $(n) with $(s)"]
    out = _call("root_at", lines)
    return Fan(_operand(x), n, at, prec, true, _branches(out), lines, out)
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
    lines::Vector{String}
    line::String
end

"""
    project(x, places...; prec = 64) -> Projection

The partial ball of x over the places (`:real` or a prime), through the driver's `project`.
"""
function project(x, places...; prec::Integer = 64)
    ps = join((_place(p) for p in places), " ")
    lines = ["prec $(prec)", "project $(_operand(x)) with $(ps)"]
    out = _call("project", lines)
    es = Tuple{String,String}[]
    for part in split(out, "; ")
        i = findfirst(": ", part)
        push!(es, (part[1:i.start-1], part[i.stop+1:end]))
    end
    return Projection(es, lines, out)
end

"""
    adf"text"

The value text read by the package (`readvalue`, precision 64 bits for a real coordinate).
"""
macro adf_str(s)
    return :($(Adelefeld.readvalue)($s))
end
