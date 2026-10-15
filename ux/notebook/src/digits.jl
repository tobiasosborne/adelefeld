# Valuations, p-adic digits and trial division, in Julia BigInt arithmetic on the parsed value text.
# Brief item 1 (lanes/ux-notebook/brief.md) and library demand 4 of docs/ux/brief-1-adelica-notebook.md:
# the library has no digit accessor, so the slice computes the digits here.
#
# Digits are written low digit on the right, as in docs/ux/brief-1-adelica-notebook.md ("...4442").
# The text form, used in the records and the tests: the period in parentheses, then the pre-period,
# '.' the p-adic point (between the digit of p^0 and of p^-1), '~' the fog (digits not known),
# '...' a cut at the cap of 64 digits. For p > 10 the digits are decimal numbers separated by spaces.

const DIGIT_CAP = 64
const DIGITS_UP_TO = 97   # above this prime a column shows the residue form only (brief 1, "Too many digits")

"v_p(n) for a non-zero integer n."
function vp(n::Integer, p::Integer)
    n == 0 && throw(ArgumentError("v_p(0) is not an integer"))
    v = 0
    n = big(n)
    while rem(n, p) == 0
        n = div(n, p); v += 1
    end
    return v
end
vp(q::Rational, p::Integer) = vp(numerator(q), p) - vp(denominator(q), p)

_ppow(p, v) = v >= 0 ? big(p)^v // 1 : 1 // big(p)^(-v)

"""
    rat_digits(q, p; cap = 64) -> (v, pre, per, capped)

The p-adic expansion of the unit part u = q / p^v of a non-zero rational: `pre` and `per` are the
pre-period and the period, low digit first. The expansion of u = n/d (p not dividing d) is
x_0 = n, digit_i = x_i / d mod p, x_(i+1) = (x_i - digit_i d) / p; it is periodic from the first repeated
state x_i. If no state repeats within `cap` digits, `pre` holds the `cap` digits, `per` is empty and
`capped` is true.
"""
function rat_digits(q::Rational, p::Integer; cap::Int = DIGIT_CAP)
    q = Rational{BigInt}(q)
    q == 0 && throw(ArgumentError("0 has no unit part"))
    v = vp(q, p)
    u = q / _ppow(p, v)
    n, d = numerator(u), denominator(u)
    dinv = invmod(d, big(p))
    seen = Dict{BigInt,Int}()
    ds = Int[]
    x = n
    while true
        if haskey(seen, x)
            s = seen[x]
            return (v = v, pre = ds[1:s-1], per = ds[s:end], capped = false)
        end
        length(ds) == cap && return (v = v, pre = ds, per = Int[], capped = true)
        seen[x] = length(ds) + 1
        dg = Int(mod(x * dinv, p))
        push!(ds, dg)
        x = div(x - dg * d, p)
    end
end

_dstr(ds::AbstractVector{Int}, p) = p <= 10 ? join(reverse(ds)) : join(reverse(ds), " ")

# Tokens of a digit display, high digits first: (:ell,""), (:fog,""), (:per,s), (:dig,s), (:point,"").
"""
    rat_digit_tokens(q, p)

The display of the expansion of q at p, with the valuation applied: v > 0 puts v zeros at the low end,
v < 0 puts the point v places from the right (the period is unrolled until the pre-period is long enough).
"""
function rat_digit_tokens(q::Rational, p::Integer)
    q == 0 && return [(:dig, "0")]
    e = rat_digits(q, p)
    pre, per = copy(e.pre), copy(e.per)
    if e.v > 0
        pre = vcat(zeros(Int, e.v), pre)
    end
    if e.v < 0
        while length(pre) < -e.v
            if isempty(per)
                break
            end
            push!(pre, per[1]); per = vcat(per[2:end], per[1:1])
        end
    end
    toks = Tuple{Symbol,String}[]
    if e.capped
        push!(toks, (:ell, ""))
    else
        push!(toks, (:per, _dstr(per, p)))
    end
    k = e.v < 0 ? -e.v : 0     # digits right of the point
    hi, lo = pre[k+1:end], pre[1:k]
    p > 10 && !e.capped && (!isempty(hi) || k > 0) && push!(toks, (:sep, " "))   # digits of p > 10 are numbers
    isempty(hi) || push!(toks, (:dig, _dstr(hi, p)))
    if k > 0
        push!(toks, (:point, ""))
        push!(toks, (:dig, _dstr(lo, p)))
    end
    return toks
end

"""
    ball_digit_tokens(a, p, k)

The known digits of the local ball a + p^k Z_p: the positions from min(v, 0) to k - 1, v = v_p(a) when
a is not in p^k Z_p; when it is (a = 0 included), the positions from min(k, 0) to k - 1, all 0. Fog for
the digits from p^k on.
"""
function ball_digit_tokens(a::Rational, p::Integer, k::Integer)
    a = Rational{BigInt}(a)
    v = (a == 0 || vp(a, p) >= k) ? k : vp(a, p)
    lo = min(v, 0)
    m = k - lo                              # the number of known digit positions
    toks = Tuple{Symbol,String}[(:fog, "")]
    m <= 0 && return toks
    A = a * _ppow(p, -lo)                   # p-integral, since lo <= v
    pm = big(p)^m
    r = mod(numerator(A) * invmod(denominator(A), pm), pm)
    ds = Int[]
    for _ in 1:m
        push!(ds, Int(mod(r, p))); r = div(r, p)
    end
    nfrac = -lo                             # digits right of the point
    if nfrac == 0
        push!(toks, (:dig, _dstr(ds, p)))
    elseif k <= 0
        push!(toks, (:point, ""))
        k < 0 && push!(toks, (:fog, ""))
        push!(toks, (:dig, _dstr(ds, p)))
    else
        push!(toks, (:dig, _dstr(ds[nfrac+1:end], p)))
        push!(toks, (:point, ""))
        push!(toks, (:dig, _dstr(ds[1:nfrac], p)))
    end
    return toks
end

function digit_text(toks)
    io = IOBuffer()
    for (t, s) in toks
        t === :ell && print(io, "...")
        t === :fog && print(io, "~")
        t === :per && print(io, "(", s, ")")
        t === :dig && print(io, s)
        t === :point && print(io, ".")
        t === :sep && print(io, " ")
    end
    return String(take!(io))
end

rat_digit_text(q::Rational, p::Integer) = digit_text(rat_digit_tokens(q, p))
ball_digit_text(a::Rational, p::Integer, k::Integer) = digit_text(ball_digit_tokens(a, p, k))

const SMALL_PRIMES = filter(n -> n > 1 && all(d -> n % d != 0, 2:isqrt(n)), 2:999)

"""
    trial_division(n) -> (primes, cofactor)

The primes below 1000 that divide |n| and the cofactor left after dividing them out (1 if none).
No other factoring is done (SPEC 2, "no factorisation"; brief 1, library demand 5).
"""
function trial_division(n::Integer)
    n = abs(big(n))
    n == 0 && return (Int[], big(0))
    ps = Int[]
    for p in SMALL_PRIMES
        if rem(n, p) == 0
            push!(ps, p)
            while rem(n, p) == 0
                n = div(n, p)
            end
        end
    end
    return (ps, n)
end
