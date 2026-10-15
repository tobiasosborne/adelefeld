# The value text that the package prints for Rat, FBall, Adele and CAdele, parsed in Julia for the strip.
# Grammar: docs/conventions.md:1159-1171 (lexical, no whitespace inside a token) and :1175-1185 (syntactic,
# whitespace between tokens): rat_v = rat; fball_v = "(" "*" ";" fin ")" | rat "mod" urat;
# adele_v = "(" real ";" fin ")"; cadele_v = "(" complex ";" fin ")"; fin = rat ["mod" urat];
# real = dec ["+/-" udec]; complex = "(" real ")" "+" "(" real ")" "*" "i". Whitespace: conventions 8.2,
# the bytes space, TAB, LF, CR. Meaning (conventions:1199-1200): fin is a + N Zhat, N = 0 if no mod.
# Only these four start symbols are read; every other text is an ArgumentError (not a status: this parser
# is a reader of the screen, not of the library, which has its own, adf_text_classify).

"""
    ValueParts(kind, real, centre, modulus)

The parsed value text: `kind` is `:rat`, `:fball`, `:adele` or `:cadele`; `real` the text of the real
(or complex) coordinate rebuilt with single spaces, `nothing` for a rational and for `*`; the finite part
is `centre + modulus Zhat` with `modulus = 0` for an exact finite part.
"""
struct ValueParts
    kind::Symbol
    real::Union{Nothing,String}
    centre::Rational{BigInt}
    modulus::Rational{BigInt}
end

const RE_NUM = r"^-?[0-9]+(\.[0-9]+)?([eE][+-]?[0-9]+)?(/[0-9]+)?"
const RE_RAT = r"^-?[0-9]+(/[0-9]+)?$"
const RE_URAT = r"^[0-9]+(/[0-9]+)?$"
const RE_DEC = r"^-?[0-9]+(\.[0-9]+)?([eE][+-]?[0-9]+)?$"
const RE_UDEC = r"^[0-9]+(\.[0-9]+)?([eE][+-]?[0-9]+)?$"

_bad(s,
    why) = throw(ArgumentError("not a value text of Rat, FBall, Adele or CAdele: " * repr(s) * " (" * why * ")"))

function _tokens(s::String)
    isascii(s) || _bad(s, "a byte outside ASCII")
    toks = Tuple{Symbol,String}[]
    i = 1
    n = ncodeunits(s)
    while i <= n
        c = s[i]
        rest = SubString(s, i)
        if c in (' ', '\t', '\n', '\r')
            i += 1
        elseif startswith(rest, "+/-")
            push!(toks, (:pm, "+/-")); i += 3
        elseif c in ('(', ')', ';', '*', '+')
            push!(toks, (Symbol(c), string(c))); i += 1
        elseif c == '-' || isdigit(c)
            m = match(RE_NUM, rest)
            m === nothing && _bad(s, "a sign without digits")
            push!(toks, (:num, m.match)); i += ncodeunits(m.match)
        elseif isletter(c)
            m = match(r"^[A-Za-z]+", rest)
            w = m.match
            w == "mod" || w == "i" || _bad(s, "the word " * repr(w))
            push!(toks, (Symbol(w), w)); i += ncodeunits(w)
        else
            _bad(s, "the byte " * repr(c))
        end
    end
    return toks
end

function _rational(t::AbstractString)
    parts = split(t, '/')
    num = parse(BigInt, parts[1])
    den = length(parts) == 2 ? parse(BigInt, parts[2]) : big(1)
    den == 0 && throw(ArgumentError("a zero denominator in " * repr(t)))
    return num // den
end

mutable struct _Cursor
    s::String
    toks::Vector{Tuple{Symbol,String}}
    pos::Int
end

_peek(c::_Cursor) = c.pos <= length(c.toks) ? c.toks[c.pos][1] : :end
function _take(c::_Cursor, kind::Symbol)
    _peek(c) === kind || _bad(c.s, "expected " * string(kind) * " at token " * string(c.pos))
    c.pos += 1
    return c.toks[c.pos - 1][2]
end
function _num(c::_Cursor, re::Regex, what)
    t = _take(c, :num)
    occursin(re, t) || _bad(c.s, repr(t) * " is not a " * what)
    return t
end

function _real(c::_Cursor)
    m = _num(c, RE_DEC, "dec")
    if _peek(c) === :pm
        _take(c, :pm)
        return m * " +/- " * _num(c, RE_UDEC, "udec")
    end
    return m
end

function _fin(c::_Cursor)
    a = _rational(_num(c, RE_RAT, "rat"))
    if _peek(c) === :mod
        _take(c, :mod)
        return a, _rational(_num(c, RE_URAT, "urat")), true
    end
    return a, 0 // big(1), false
end

"""
    parse_value_text(s) -> ValueParts

Reads exactly the four start symbols rat_v, fball_v, adele_v, cadele_v of conventions 9.2.
"""
function parse_value_text(s::AbstractString)
    str = String(s)
    c = _Cursor(str, _tokens(str), 1)
    _peek(c) === :end && _bad(str, "empty")
    if _peek(c) === Symbol("(")
        _take(c, Symbol("("))
        if _peek(c) === :*
            _take(c, :*)
            kind, real = :fball, nothing
        elseif _peek(c) === Symbol("(")
            _take(c, Symbol("(")); re = _real(c); _take(c, Symbol(")"))
            _take(c, :+)
            _take(c, Symbol("(")); im = _real(c); _take(c, Symbol(")"))
            _take(c, :*); _take(c, :i)
            kind, real = :cadele, "(" * re * ") + (" * im * ")*i"
        else
            kind, real = :adele, _real(c)
        end
        _take(c, Symbol(";"))
        a, N, _ = _fin(c)
        _take(c, Symbol(")"))
    else
        # `rat` is rat_v; `rat mod urat` is the bare form of fball_v (`a mod 0` is a ball of radius 0).
        a, N, hadmod = _fin(c)
        kind, real = hadmod ? :fball : :rat, nothing
    end
    _peek(c) === :end || _bad(str, "text after the value")
    return ValueParts(kind, real, a, N)
end
