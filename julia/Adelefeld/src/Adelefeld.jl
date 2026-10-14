# julia/Adelefeld/src/Adelefeld.jl: the Julia binding of adelefeld, step 1 of J-a (docs/api-j.md section 9):
# package skeleton, load gates (section 2), statuses (section 4), the Rat handle (sections 3 and 4).
# A wrapper of the C contracts, not a second arithmetic implementation (docs/api-j.md:3): every value is
# computed by the C library through ccall; Julia code only checks arguments, owns storage and copies text.
module Adelefeld

using Libdl

export AdfStatus, AdfResult, AdfError, AdfAbiError, AdfLoadError, unwrap
export TextLimits, Rat, try_rat, try_parse, try_value_text, value_text
export try_add, try_sub, try_mul, try_neg, try_div, try_inv
export add, sub, mul, neg, divexact, identical

include("status.jl")
include("load.jl")
include("handle.jl")
include("rat.jl")

end
