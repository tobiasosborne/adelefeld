# Statuses and results (docs/api-j.md:159-172; decision J3 at docs/api-j.md:19-20).

# Plain C int values 0..10, named exactly as include/adelefeld/status.h:17-28; no bit flags.
# The load gate compares every name with adf_status_str and the count with adf_status_count
# (docs/api-j.md:70-76; load.jl).
@enum AdfStatus::Int32 begin
    OK=0; NOT_DETERMINED=1; UNIT_NOT_CERTIFIED=2; NEEDS_SPLIT=3; NOT_UNIQUE=4; NO_SOLUTION=5
    NOT_UNIT=6; DOMAIN=7; UNSUPPORTED=8; PARSE=9; LIMIT=10
end

"""
    AdfResult{T}(status, value, where, details)

The result of a `try_` method (docs/api-j.md:165-168). `status` always exists; `value` is a fresh owner
only when the C header certifies the output on that status, else `nothing` (no dummy zero is returned).
`where` is a certified failing place when the C function supplies one; no function of this step does,
so it is `nothing` here (the `Place` type lands in J-b, docs/api-j.md:248). `details` holds extra data.
"""
struct AdfResult{T}
    status::AdfStatus
    value::Union{T,Nothing}
    where::Any
    details::NamedTuple
end

AdfResult{T}(status::AdfStatus, value) where {T} = AdfResult{T}(status, value, nothing, NamedTuple())

"""
    AdfError <: Exception

Thrown by every throwing form on a status other than OK (decision J5, docs/api-j.md:24-25). It carries the
operation, the status, a COPY of the static `adf_status_str` text, and the whole result (docs/api-j.md:169-171).
"""
struct AdfError <: Exception
    op::Symbol
    status::AdfStatus
    message::String
    result::AdfResult
end

Base.showerror(io::IO, e::AdfError) = print(io, "AdfError: ", e.op, " returned status ", e.message)

"""
    AdfAbiError <: Exception

A C function returned an int that is not a status of include/adelefeld/status.h. The raw value is kept;
it is never coerced to a status (docs/api-j.md:170).
"""
struct AdfAbiError <: Exception
    op::Symbol
    raw::Cint
end

function Base.showerror(io::IO, e::AdfAbiError)
    print(io, "AdfAbiError: ", e.op, " returned ", e.raw)
    if e.op === :text_classify
        print(io, ", which is not a text kind (0 to 12)")
    else
        print(io, ", which is not a status code (0 to 10)")
    end
end

# The status of a raw C return value; an unknown value is an ABI error.
function to_status(raw::Cint, op::Symbol)
    0 <= raw <= 10 || throw(AdfAbiError(op, raw))
    return AdfStatus(raw)
end

"""
    unwrap(r::AdfResult)

Returns `r.value` on OK; throws `AdfError` on every other status (docs/api-j.md:172).
"""
unwrap(r::AdfResult) = _unwrap(r, :unwrap)

function _unwrap(r::AdfResult, op::Symbol)
    r.status === OK && return r.value
    throw(AdfError(op, r.status, status_name(Int(r.status)), r))
end

# Point comparison (include/adelefeld/status.h:33-37; fball.h:258-265; docs/api-j.md:223, 315): the
# three-valued comparison of two unknown points, one in each ball. Not a status: a separate enum, so a value
# can never be passed where an AdfStatus is expected. The C library has no name function for these values
# (status.h exports only adf_status_str and adf_status_count), so there is no load-time name check; the
# numbers are transcribed literally from status.h:35-37 and every raw C value outside 0..2 is an AdfAbiError.
@enum PointComparison::Int32 begin
    EQUAL=0; DIFFERENT=1; UNDECIDED=2
end

function to_point_comparison(raw::Cint, op::Symbol)
    0 <= raw <= 2 || throw(AdfAbiError(op, raw))
    return PointComparison(raw)
end

# A C predicate's int 0 or 1 as a Bool (docs/api-j.md:224: no AdfStatus cast); any other value is an
# ABI error (fball.h:234-237: the predicates return int 0 or 1).
function to_bool(raw::Cint, op::Symbol)
    raw == 1 && return true
    raw == 0 && return false
    throw(AdfAbiError(op, raw))
end

# Text kind: include/adelefeld/text.h:118-135. There is no adf_text_kind_str in that header or
# the export inventory, so the names/numbers are transcribed literally, stripping only ADF_.
# The size/alignment gate is text.h:366-369. Julia 1.12.5 @enum help: lanes/j-slice4/help.log:4-38.
@enum TextKind::Int32 begin
    TEXT_RAT=0; TEXT_FBALL=1; TEXT_ADELE=2; TEXT_CADELE=3; TEXT_UCOSET=4
    TEXT_IDELE=5; TEXT_IDCLASS=6; TEXT_LBALL=7; TEXT_SBALL=8; TEXT_QCLASS=9
    TEXT_FFUN=10; TEXT_RFUN=11; TEXT_CHAR=12
end

function to_text_kind(raw::Cint)
    0 <= raw <= 12 || throw(AdfAbiError(:text_classify, raw))
    return TextKind(raw)
end
