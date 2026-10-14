# julia/Adelefeld/test/load_fault.jl: run in a SEPARATE Julia process by runtests.jl, one process per
# load-rejection case (docs/api-j.md:368-369). The library is chosen by ADELEFELD_LIB, as in
# production; the fixtures under test/fixtures falsify one query each. Usage:
#     julia --project=<package> load_fault.jl [<fixture library to ask for its init count>]
# It prints one line per fact, which runtests.jl checks:
#     LOADED=<true|false>
#     ERROR=<the error message, newlines replaced by " | ">
#     AFTER=<what calling Rat(1) after the failed load gave>
#     INIT_CALLS=<calls of adf_rat_init counted by the fixture>      (only with an argument)
using Libdl

loaded = try
    @eval using Adelefeld
    true
catch e
    # The methods of the package are newer than this top-level statement: call showerror in the latest world.
    msg = Base.invokelatest(sprint, showerror, e)
    println("ERROR=", replace(msg, '\n' => " | "))
    false
end
println("LOADED=", loaded)

# The module may or may not be reachable after its __init__ threw; if it is, no allocation may run.
after = if isdefined(Main, :Adelefeld)
    try
        Base.invokelatest(getfield(Main, :Adelefeld).Rat, 1)
        "allocated"
    catch e
        "refused: " * replace(sprint(showerror, e), '\n' => " | ")
    end
else
    "module not bound"
end
println("AFTER=", after)

if length(ARGS) == 1
    h = Libdl.dlopen(ARGS[1], Libdl.RTLD_NOW | Libdl.RTLD_LOCAL)
    println("INIT_CALLS=", ccall(Libdl.dlsym(h, :fixture_rat_init_calls), Cint, ()))
end
