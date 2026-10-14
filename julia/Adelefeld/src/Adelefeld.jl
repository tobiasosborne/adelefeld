# julia/Adelefeld/src/Adelefeld.jl: the Julia binding of adelefeld, step 1 of J-a (docs/api-j.md section 9):
# package skeleton, load gates (section 2), statuses (section 4), the Rat handle (sections 3 and 4); step 2
# of J-a: the FBall handle and the scalar mixed arithmetic (sections 3 and 4).
# A wrapper of the C contracts, not a second arithmetic implementation (docs/api-j.md:3): every value is
# computed by the C library through ccall; Julia code only checks arguments, owns storage and copies text.
module Adelefeld

using Libdl

export AdfStatus, AdfResult, AdfError, AdfAbiError, AdfLoadError, unwrap
export TextLimits, Rat, try_rat, try_parse, try_value_text, value_text
export try_add, try_sub, try_mul, try_neg, try_div, try_inv
export add, sub, mul, neg, divexact, identical
export FBall, try_fball, PointComparison, equal_set, overlaps, inside, compare_points, center, radius
export is_canonical
export Adele, CAdele, RealBall, ComplexBall, finitepart, realpart, complexpart

include("status.jl")
include("load.jl")
include("handle.jl")
include("rat.jl")
include("fball.jl")
include("adele.jl")
include("carrier.jl")

end
