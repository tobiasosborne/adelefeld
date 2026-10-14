# Loading and ABI refusal (docs/api-j.md section 2, lines 36-88; decision J2 at docs/api-j.md:16-18).
#
# Julia sources: the installed help of Julia 1.12.5 in lanes/j-slice1/help.log; dlopen flags and the Linux
# default RTLD_LAZY|RTLD_DEEPBIND|RTLD_LOCAL (line 28), dlsym (36-44).
#
# Order of the gates in __init__, each of which throws AdfLoadError naming the gate, the expected and the
# observed value; nothing is allocated and no adf init runs unless all of them pass (docs/api-j.md:18, 369):
#   1. words: 64-bit pointers, Int64/UInt64 words, Csize_t = 8, Cint = 4 (docs/api-j.md:46-47);
#   2. the library: ADELEFELD_LIB, else build/libadelefeld.so relative to the repository root
#      (docs/api-j.md:17), opened once with RTLD_NOW | RTLD_LOCAL and kept open until exit (:38-40);
#      the missing GMP symbol of this machine gives the exact preload command (:80-87);
#   3. symbols: every function of REQUIRED, from that one handle only; a missing one names itself and the
#      path (:40); pointers are cached at run time, never serialized into precompiled code (:38-39);
#   4. FLINT version: adf_version_check() must be OK (include/adelefeld/common.h:78-83; docs/api-j.md:44-45);
#   5. layouts: size AND alignment of every row of LAYOUT_ROWS, against literal rows (docs/api-j.md:48-50);
#   6. statuses: the eleven names, UNKNOWN at -1 and 11, adf_status_count() == 11 ==
#      length(instances(AdfStatus)) (docs/api-j.md:70-76; include/adelefeld/status.h:43-81).

"""
    AdfLoadError <: Exception

A load gate failed. Fields: the gate, the expected and the observed value, the library path and a hint.
"""
struct AdfLoadError <: Exception
    gate::String
    expected::String
    observed::String
    library::String
    hint::String
end

function Base.showerror(io::IO, e::AdfLoadError)
    print(io, "Adelefeld load gate \"", e.gate, "\" failed for library ", e.library,
          ": expected ", e.expected, "; observed ", e.observed, ".")
    isempty(e.hint) || print(io, " ", e.hint)
end

# Every C function this step calls. The gate queries come first, in the order the gates use them, so that
# a library from before J-status reports adf_status_count as its missing symbol (docs/api-j.md:75-76).
const REQUIRED = (
    :adf_version_check, :adf_flint_version_compiled,
    :adf_sizeof_rat, :adf_alignof_rat, :adf_sizeof_text_limits, :adf_alignof_text_limits,
    :adf_status_str, :adf_status_count,
    :adf_text_limits_default, :adf_str_free,
    :adf_rat_init, :adf_rat_clear, :adf_rat_set, :adf_rat_set_si, :adf_rat_set_str, :adf_rat_get_str,
    :adf_rat_add, :adf_rat_sub, :adf_rat_mul, :adf_rat_neg, :adf_rat_div, :adf_rat_inv,
    :adf_rat_equal, :adf_rat_identical,
)

# One Ref per symbol, named p_<symbol>, filled by __init__ (never at precompile time).
for s in REQUIRED
    @eval const $(Symbol(:p_, s)) = Ref{Ptr{Cvoid}}(C_NULL)
end

# The layout table: independent literal rows, not live reads (docs/api-j.md:48-61). The binding allocates
# the checked size of the row, never a newly observed one (docs/api-j.md:49). Later steps add rows.
#   rat:         docs/api-m1.md:35 (16 bytes), :45 (alignment 8); include/adelefeld/rat.h:28.
#   text_limits: docs/api-m1.md:41, :45; include/adelefeld/text.h:89-90. Passed by pointer to every parser
#                of this step, so it is gated too (not named by the brief; see docs/api-ja.md).
const LAYOUT_ROWS = (
    (suffix = :rat, size = 16, align = 8, source = "docs/api-m1.md:35,45"),
    (suffix = :text_limits, size = 32, align = 8, source = "docs/api-m1.md:41,45"),
)

const RAT_SIZE = 16
const RAT_ALIGN = 8

const LOAD_STATE = Ref{Symbol}(:not_loaded)   # :not_loaded, :ok or :failed
const LOAD_ERROR = Ref{String}("")
const LIBPATH = Ref{String}("")
const LIBHANDLE = Ref{Ptr{Cvoid}}(C_NULL)

# The repository root is three levels above this file (julia/Adelefeld/src).
const DEFAULT_LIB = normpath(joinpath(@__DIR__, "..", "..", "..", "build", "libadelefeld.so"))

library_path() = (p = get(ENV, "ADELEFELD_LIB", ""); isempty(p) ? DEFAULT_LIB : abspath(p))

# The system libgmp that the preload needs (tests/julia/README.md:39-69; tests/test_julia.sh).
function system_gmp()
    for p in ("/lib/x86_64-linux-gnu/libgmp.so.10", "/usr/lib/x86_64-linux-gnu/libgmp.so.10",
              "/usr/lib64/libgmp.so.10", "/usr/lib/libgmp.so.10")
        isfile(p) && return p
    end
    return "<path of the system libgmp.so.10>"
end

function preload_hint()
    launcher = normpath(joinpath(@__DIR__, "..", "bin", "adelefeld-julia"))
    return "This Julia loaded its bundled libgmp before the library; start Julia with the system GMP " *
           "preloaded: LD_PRELOAD=$(system_gmp()) julia ..., or use the launcher $launcher " *
           "(docs/api-j.md:80-87; tests/julia/README.md)."
end

loadfail(gate, expected, observed, hint="") =
    throw(AdfLoadError(gate, string(expected), string(observed), LIBPATH[], hint))

function gate_words()
    obs = (Sys.WORD_SIZE, sizeof(Ptr{Cvoid}), sizeof(Int64), sizeof(UInt64), sizeof(Csize_t), sizeof(Cint))
    obs == (64, 8, 8, 8, 8, 4) || loadfail("words",
        "WORD_SIZE 64, Ptr 8, Int64 8, UInt64 8, Csize_t 8, Cint 4 (docs/api-j.md:46-47)", obs)
end

function gate_open()
    path = LIBPATH[]
    isfile(path) || loadfail("library path", "an existing file (ADELEFELD_LIB or build/libadelefeld.so)",
                             "no such file", "Build it with `sh tests/test_exports.sh` from the repository root.")
    h = try
        Libdl.dlopen(path, Libdl.RTLD_NOW | Libdl.RTLD_LOCAL)
    catch e
        msg = sprint(showerror, e)
        if occursin("__gmpn_modexact_1_odd", msg)
            loadfail("dlopen (GMP symbol)", "dlopen to succeed", msg, preload_hint())
        end
        loadfail("dlopen", "dlopen to succeed", msg)
    end
    LIBHANDLE[] = h
end

function gate_symbols()
    for s in REQUIRED
        p = Libdl.dlsym(LIBHANDLE[], s; throw_error=false)
        p === nothing && loadfail("symbol", "the exported function $s", "missing")
        getfield(@__MODULE__, Symbol(:p_, s))[] = p
    end
end

function gate_version()
    st = ccall(p_adf_version_check[], Cint, ())
    if st != 0
        compiled = unsafe_string(ccall(p_adf_flint_version_compiled[], Ptr{UInt8}, ()))  # static, not freed
        loadfail("FLINT version", "adf_version_check() == 0 (OK)",
                 "$st; the library was compiled against FLINT $compiled and the FLINT loaded at run time " *
                 "differs in major or minor version (include/adelefeld/common.h:72-83)")
    end
end

function gate_layouts()
    for r in LAYOUT_ROWS
        size_sym = Symbol(:adf_sizeof_, r.suffix)
        align_sym = Symbol(:adf_alignof_, r.suffix)
        size = ccall(getfield(@__MODULE__, Symbol(:p_, size_sym))[], Csize_t, ())
        align = ccall(getfield(@__MODULE__, Symbol(:p_, align_sym))[], Csize_t, ())
        size == r.size || loadfail("layout", "$size_sym() == $(r.size) ($(r.source))", Int(size))
        align == r.align || loadfail("layout", "$align_sym() == $(r.align) ($(r.source))", Int(align))
    end
    # The Julia side of the text limits passed by pointer: offsets 0, 8, 16, 24 (include/adelefeld/text.h:89-90).
    offs = Int.(fieldoffset.(TextLimits, 1:4))
    (sizeof(TextLimits), offs) == (32, [0, 8, 16, 24]) ||
        loadfail("layout", "Julia TextLimits of 32 bytes at offsets 0, 8, 16, 24", (sizeof(TextLimits), offs))
end

# A copy of the static name of a status code; never freed (include/adelefeld/status.h:43-49).
status_name(i::Integer) = unsafe_string(ccall(p_adf_status_str[], Ptr{UInt8}, (Cint,), Cint(i)))

status_count() = Int(ccall(p_adf_status_count[], Cint, ()))

function gate_statuses()
    for s in instances(AdfStatus)
        got = status_name(Int(s))
        got == string(s) || loadfail("status name", "adf_status_str($(Int(s))) == \"$(s)\"", "\"$got\"")
    end
    for i in (-1, 11)
        got = status_name(i)
        got == "UNKNOWN" || loadfail("status name", "adf_status_str($i) == \"UNKNOWN\"", "\"$got\"")
    end
    n = status_count()
    (n == 11 && length(instances(AdfStatus)) == 11) ||
        loadfail("status count", "adf_status_count() == 11 == length(instances(AdfStatus)) == " *
                 string(length(instances(AdfStatus))), n)
end

function __init__()
    LOAD_STATE[] = :not_loaded
    LIBPATH[] = library_path()
    try
        gate_words()
        gate_open()
        gate_symbols()
        gate_version()
        gate_layouts()
        gate_statuses()
    catch e
        LOAD_STATE[] = :failed
        LOAD_ERROR[] = sprint(showerror, e)
        rethrow()
    end
    LOAD_STATE[] = :ok
    return nothing
end

# Called before every owning allocation: no adf init runs after a rejected load (docs/api-j.md:369).
function require_loaded()
    LOAD_STATE[] === :ok && return nothing
    error("Adelefeld is not loaded (state $(LOAD_STATE[])): $(LOAD_ERROR[])")
end
