# Status cards (brief 1, "Statuses"; D-UX4 of lanes/UX-COMMON.md). The names are those of
# include/adelefeld/status.h:17-28 (the package's AdfStatus, whose names its load gate checks against
# adf_status_str, julia/Adelefeld/src/status.jl:3-9). Classes and shapes:
#   OK                                                          its own class, no card
#   open (dashed):            NOT_DETERMINED UNIT_NOT_CERTIFIED NEEDS_SPLIT NOT_UNIQUE
#   proved no (solid):        DOMAIN NO_SOLUTION NOT_UNIT
#   budget (dotted):          LIMIT
#   not mathematics (plain):  PARSE UNSUPPORTED

const STATUS_NAMES = [string(s) for s in instances(Adelefeld.AdfStatus)]

function status_class(name::AbstractString)
    name == "OK" && return ("ok", "none")
    name in ("NOT_DETERMINED", "UNIT_NOT_CERTIFIED", "NEEDS_SPLIT", "NOT_UNIQUE") && return ("open", "dashed")
    name in ("DOMAIN", "NO_SOLUTION", "NOT_UNIT") && return ("proved no", "solid")
    name == "LIMIT" && return ("budget", "dotted")
    name in ("PARSE", "UNSUPPORTED") && return ("not mathematics", "plain")
    throw(ArgumentError("not a status name of status.h: " * repr(name)))
end

# The meaning of each code, from the comments of include/adelefeld/status.h:18-28.
const MEANING = Dict(
    "NOT_DETERMINED" => "the inputs do not determine the quantity, or a result invariant is not certified; " *
                        "more precision or a sharper method may help",
    "UNIT_NOT_CERTIFIED" => "an invertible input was required; the enclosure does not prove it",
    "NEEDS_SPLIT" => "the result is a union of pieces and one piece was asked for",
    "NOT_UNIQUE" => "several candidates satisfy the problem",
    "NO_SOLUTION" => "proved: no value satisfies the problem",
    "NOT_UNIT" => "proved: the value is not invertible",
    "DOMAIN" => "proved outside the domain; invalid raw data; context mismatch",
    "UNSUPPORTED" => "valid request that version 1 does not implement",
    "PARSE" => "the text is not a sentence of the grammar",
    "LIMIT" => "a resource limit was reached")

# The actions that can change the answer, by status (brief 1, "Statuses" and Scenario B step 4).
const ACTIONS = Dict(
    "NOT_DETERMINED" => ["raise the precision at the place", "split the ball"],
    "UNIT_NOT_CERTIFIED" => ["raise the precision", "refine the input"],
    "NEEDS_SPLIT" => ["split the input and ask for each piece"],
    "NOT_UNIQUE" => ["tighter bounds or more precision"],
    "NO_SOLUTION" => ["none: the answer is proved"],
    "NOT_UNIT" => ["none: the answer is proved; for an adele, unitof(y) certifies a unit (SPEC 4.5)"],
    "DOMAIN" => ["none: the answer is proved"],
    "UNSUPPORTED" => ["the milestone that brings the feature: not given by the driver"],
    "PARSE" => ["check the text against the grammar of docs/conventions.md 9.2"],
    "LIMIT" => ["the bound that was hit and its value: not given by the driver"])

# Sharper reasons and actions for the calls of this slice, from the sections of tools/adf/README.md.
const OP_REASON = Dict(
    ("roots_at", "DOMAIN") => ("no branch: the existence criterion fails on the whole ball, or the degree " *
        "is 0 (tools/adf/README.md:509-510)", ["another prime or another degree"]),
    ("root_at", "DOMAIN") => ("no branch with this seed: the existence criterion fails on the whole ball, " *
        "or the seed or the degree is invalid (tools/adf/README.md:509-510)",
        ["a seed that roots(x, n; at) lists"]),
    ("roots_at", "NOT_DETERMINED") => ("the ball is outside the guard of SPEC 9.3.3, or contains 0 " *
        "(tools/adf/README.md:507-508)", ["refine the input at the prime", "split the ball"]),
    ("root_at", "NOT_DETERMINED") => ("the ball is outside the guard of SPEC 9.3.3, or contains 0 " *
        "(tools/adf/README.md:507-508)", ["refine the input at the prime", "split the ball"]),
    ("recover", "NOT_UNIQUE") => ("at least two reduced fractions n/d with n = c d mod m lie in the box " *
        "|n| <= num, 0 < d <= den (SPEC 9.2; tools/adf/README.md:190-191)",
        ["tighter bounds: 2 num den < m is sufficient for uniqueness (SPEC 9.2)"]),
    ("recover", "NOT_DETERMINED") => ("uniqueness not certified: the search limit 1000 of the driver ended " *
        "the search before the set of solutions was decided (tools/adf/README.md:191-192)",
        ["a larger search limit (fixed at 1000 in the driver: not reachable from this notebook)",
         "tighter bounds: 2 num den < m is sufficient for uniqueness (SPEC 9.2)"]),
    ("recover", "NO_SOLUTION") => ("no reduced fraction n/d with n = c d mod m lies in the box " *
        "(SPEC 9.2)", ["wider bounds"]))

"""
    card_html(status, op; place = nothing)

The card of a status: the name, its class and shape, the place (`not given` when the call does not
report it), one sentence of reason, the actions.
"""
function card_html(status::AbstractString, op::AbstractString; place = nothing, source = "driver")
    cls, shape = status_class(status)
    why, acts = get(OP_REASON, (op, status), (MEANING[status], ACTIONS[status]))
    io = IOBuffer()
    print(io, "<div class=\"status s-", shape, "\" role=\"status\">")
    print(io, "<span class=\"nm\">", esc(status), " · ", esc(cls), "</span>")
    print(io, "<span class=\"meta\">", esc(shape), " border · place: ",
          place === nothing ? "not given" : esc(string(place)), " · from: ", esc(source), " ", esc(op), "</span>")
    print(io, "<span class=\"why\">", esc(why), "</span>")
    print(io, "<div class=\"acts\">")
    for a in acts
        print(io, "<span class=\"act\">", esc(a), "</span>")
    end
    print(io, "</div></div>")
    return String(take!(io))
end
