# Full-ball reconstruction only, J-a.4; recon.h:8-30,49-57; SPEC 9.2 first item.
# No partial residues or bounded search. Every non-OK status leaves every output untouched;
# there is no certified failure value, place or auxiliary data (recon.h:24-29).

"""
    try_reconstruct(x::FBall, lo::Rat, hi::Rat) -> AdfResult{Rat}
    reconstruct(x::FBall, lo::Rat, hi::Rat) -> Rat

`adf_fball_reconstruct` in the exact closed interval [lo, hi] (recon.h:49-52).
OK: the single rational. NO_SOLUTION: none, including lo > hi. NOT_UNIQUE: several.
Non-OK has value=nothing and empty details. Cost: a constant number of exact rational divisions,
floors and ceilings at input size (recon.h:29-30). Inputs remain unchanged; output is a fresh owner.
"""
function try_reconstruct(x::FBall, lo::Rat, hi::Rat)
    _check_live(x); _check_live(lo); _check_live(hi)
    q = _new_rat()
    raw = GC.@preserve q x lo hi ccall(p_adf_fball_reconstruct[], Cint, (P, P, P, P),
                                     _ptr(q), _ptr(x), _ptr(lo), _ptr(hi))
    return _result(q, raw, :reconstruct)
end

"""
    try_reconstruct(x::Adele) -> AdfResult{Rat}
    reconstruct(x::Adele) -> Rat

`adf_adele_reconstruct` uses the real ball's exact dyadic end points (recon.h:54-57).
OK/NO_SOLUTION/NOT_UNIQUE as above. LIMIT when a nonzero midpoint or radius has absolute binary
exponent above 1048576, before any integer of that size is built (recon.h:16-24,41-43).
DOMAIN on a non-finite real ball is only a release-build courtesy outside the canonical-input contract
(recon.h:27-29, SPEC M1-D11). Every failing status has value=nothing and empty details.
NOT_DETERMINED belongs to the separate partial-data API, not these calls (recon.h:25-27).
Cost: exact endpoint conversion within that bound, followed by finite-ball reconstruction.
"""
function try_reconstruct(x::Adele)
    _check_live(x)
    q = _new_rat()
    raw = GC.@preserve q x ccall(p_adf_adele_reconstruct[], Cint, (P, P), _ptr(q), _ptr(x))
    return _result(q, raw, :reconstruct)
end

reconstruct(x::FBall, lo::Rat, hi::Rat) = _unwrap(try_reconstruct(x, lo, hi), :reconstruct)
reconstruct(x::Adele) = _unwrap(try_reconstruct(x), :reconstruct)
