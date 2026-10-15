# The place strip (brief 1, "The place strip"; mock docs/ux/mock-1-adelica-notebook.html:142-151):
# the column R (or C), one column per interesting prime, a column per unfactored cofactor, and the column
# "every other p". Interesting primes: those below 1000 that divide the modulus N (numerator or
# denominator) or the denominator of the centre; for an exact value, those that divide its numerator or
# denominator. Found by trial division (digits.jl), never by factoring.

struct PlaceColumn
    p::Int
    v::Union{Int,Nothing}      # v_p of the value; nothing when the local ball contains 0 (v >= k)
    k::Union{Int,Nothing}      # the local ball is c + p^k Z_p; nothing for an exact value
    tokens::Vector{Tuple{Symbol,String}}   # the digit display; empty above p = 97
    digits::String             # the text form of the digits ("" above p = 97)
    absval::String             # |x|_p as a rational, or a bound
    centre::Rational{BigInt}
end

struct Places
    kind::Symbol
    real::Union{Nothing,String}      # the text of the real or complex coordinate
    exact_real::Bool                 # true for a Rat: the real coordinate is the rational itself
    exact::Bool                      # the finite part is a point
    centre::Rational{BigInt}
    modulus::Rational{BigInt}
    columns::Vector{PlaceColumn}
    cofactors::Vector{BigInt}
    rest::String
end

_q(x::Rational) = denominator(x) == 1 ? string(numerator(x)) : string(numerator(x), "/", denominator(x))
_q(x::Integer) = string(x)

function _column(c::Rational{BigInt}, N::Rational{BigInt}, p::Int)
    if N == 0
        v = vp(c, p)
        toks = p <= DIGITS_UP_TO ? rat_digit_tokens(c, p) : Tuple{Symbol,String}[]
        return PlaceColumn(p, v, nothing, toks, digit_text(toks), _q(_ppow(p, -v)), c)
    end
    k = vp(N, p)
    v = (c != 0 && vp(c, p) < k) ? vp(c, p) : nothing
    toks = p <= DIGITS_UP_TO ? ball_digit_tokens(c, p, k) : Tuple{Symbol,String}[]
    a = v === nothing ? "<= " * _q(_ppow(p, -k)) : _q(_ppow(p, -v))
    return PlaceColumn(p, v, k, toks, digit_text(toks), a, c)
end

const ADELIC = Union{Rat,FBall,Adele,CAdele}

"""
    places(x) -> Places

The columns of the strip of a Rat, FBall, Adele or CAdele, from its value text (package printer,
20 digits for a real coordinate).
"""
function places(x::ADELIC)
    t = value_text(x)
    vt = parse_value_text(t)
    c, N = vt.centre, vt.modulus
    real, exact_real = vt.real, false
    if vt.kind === :rat
        rb = RealBall(x)
        real, exact_real = value_text(rb), true
        close(rb)
    end
    ns = N == 0 ? (c == 0 ? BigInt[] : [numerator(c), denominator(c)]) :
                  [numerator(N), denominator(N), denominator(c)]
    ps, cof = Int[], BigInt[]
    for n in ns
        a, r = trial_division(n)
        append!(ps, a)
        r > 1 && push!(cof, r)
    end
    ps = sort(unique(ps)); cof = sort(unique(cof))
    cols = [_column(c, N, p) for p in ps]
    rest = if N == 0 && c == 0
        "0 at every prime"
    elseif N == 0
        "unit, |x|_p = 1, at infinitely many primes"
    else
        "Z_p (no information), at infinitely many primes"
    end
    isempty(cof) || (rest *= "; the primes of the cofactor are not known")
    return Places(vt.kind, real, exact_real, N == 0, c, N, cols, cof, rest)
end

esc(s) = replace(string(s), "&" => "&amp;", "<" => "&lt;", ">" => "&gt;", "\"" => "&quot;")

# The digit display in HTML: the period overlined once, then repeated plainly until four digits show
# (the mock's "...4442" with the bar over the first 4), the fog as a textured band.
function digits_html(toks; fogattr = "")
    io = IOBuffer()
    print(io, "<span class=\"dig\">")
    shown = sum((length(replace(s, " " => "")) for (t, s) in toks if t === :dig); init = 0)
    for (t, s) in toks
        if t === :ell
            print(io, "<span class=\"ell\">…</span>")
        elseif t === :fog
            print(io, "<span class=\"fog\"",
                isempty(fogattr) ? " role=\"img\" aria-label=\"digits not known\"" : fogattr,
                  "></span>")
        elseif t === :per
            print(io, "<span class=\"ell\">…</span><span class=\"per\">", esc(s), "</span>")
            n = length(replace(s, " " => ""))
            reps = n == 0 ? 0 : max(0, cld(4 - n - shown, n))
            for _ in 1:reps
                print(io, esc(s), occursin(" ", s) ? " " : "")
            end
        elseif t === :dig
            print(io, esc(s))
        elseif t === :point
            print(io, "<span class=\"pt\">.</span>")
        elseif t === :sep
            print(io, " ")
        end
    end
    print(io, "</span>")
    return String(take!(io))
end

function set_words(ps::Places)
    fin = ps.exact ? "the point " * _q(ps.centre) :
          "the set " * _q(ps.centre) * " + " * _q(ps.modulus) * " Zhat"
    ps.kind === :rat && return "the rational " * _q(ps.centre) * ", a point at every place"
    ps.kind === :fball && return "finite part: " * fin * "; no real coordinate"
    return "real coordinate " * ps.real * "; finite part: " * fin
end

function strip_html(x::ADELIC)
    ps = places(x)
    io = IOBuffer()
    print(io, "<div class=\"scroll\"><div class=\"strip\" role=\"table\" title=\"", esc(set_words(ps)), "\">")
    name = ps.kind === :cadele ? "C" : "R"
    if ps.real === nothing
        print(io, "<div class=\"pl real nofo\"><span class=\"name\">R</span><span class=\"val\">*</span>",
              "<span class=\"sub\">no real coordinate</span></div>")
    else
        sub, tip = ps.exact_real ? ("exact: " * _q(ps.centre),
                                    "the decimal is the package's real ball of the rational at 64 bits") :
                                   ("a ball", "the real ball as the package prints it, 20 digits")
        print(io, "<div class=\"pl real\" title=\"", esc(tip), "\"><span class=\"name\">", name,
              "</span><span class=\"val\">", esc(ps.real), "</span><span class=\"sub\">", esc(sub), "</span></div>")
    end
    for col in ps.columns
        p = col.p
        val = col.v === nothing ? "v >= " * string(col.k) : "v = " * string(col.v)
        print(io, "<div class=\"pl\"><span class=\"name\">", p, "</span><span class=\"val\">", esc(val),
              "</span><span class=\"sub\">|x|<sub>", p, "</sub> ", col.v === nothing ? "" : "= ", esc(col.absval),
              "</span>")
        if p <= DIGITS_UP_TO
            print(io, digits_html(col.tokens))
        end
        if col.k === nothing
            print(io, "<span class=\"sub\">exact", p <= DIGITS_UP_TO ? "" : "; digits not shown above 97",
                "</span>")
        else
            print(io, "<span class=\"sub\">", esc(_q(col.centre)), " + O(", p, "<sup>", col.k, "</sup>)</span>")
        end
        print(io, "</div>")
    end
    for c in ps.cofactors
        print(io, "<div class=\"pl cof\"><span class=\"name\">cofactor ", c, "</span>",
              "<span class=\"val\">not factored</span><span class=\"sub\">no prime factor below 1000; ",
              ndigits(c), " digits</span></div>")
    end
    restval, restsub = ps.exact ? (ps.centre == 0 ? ("0", "every prime") : ("unit", "|x|<sub>p</sub> = 1")) :
                                  ("Z<sub>p</sub>", "no information")
    restsub *= isempty(ps.cofactors) ? " · infinitely many primes" : " · and the primes of the cofactor: not known"
    print(io, "<div class=\"pl rest\" title=\"", esc(ps.rest), "\"><span class=\"name\">every other p</span>",
          "<span class=\"val\">", restval, "</span><span class=\"sub\">", restsub, "</span></div>")
    print(io, "</div></div>")
    return String(take!(io))
end

for T in (:Rat, :FBall, :Adele, :CAdele)
    @eval Base.show(io::IO, ::MIME"text/html", x::$T) = print(io, strip_html(x))
end
