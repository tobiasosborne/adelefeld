# ux/notebook/src/Adelica.jl: the notebook slice of docs/ux/brief-1-adelica-notebook.md (lane ux-notebook).
#
# A notebook is a list of cells, each one Julia expression of a small language: the constructors of the
# package Adelefeld (Rat, FBall, readvalue, adf"..."), + - *, reconstruct, and roots, root, recover and
# project, which run the driver build/adf as a subprocess (the package has no local roots yet, docs/api-j.md
# section 9, J-b to J-d). Adelica.run evaluates the cells in order and writes ONE static HTML page and the
# records sidecar (one JSON object per output, the agent's window); Adelica.replay runs the driver line of
# each record and compares the value text. No Pluto (not on this machine); the page cannot compute.
#
# Every number on a page is computed here: by the package (value text, dump), by the driver (roots, recover,
# project), or by Julia BigInt arithmetic on the parsed value text (valuations, digits, trial division).
module Adelica

using Adelefeld

export roots, root, recover, project, @adf_str, Fan, Branch, Projection, DriverError

function run end   # Adelica.run (the notebook), declared before any use so that Base.run is never bound

const REPO = normpath(joinpath(@__DIR__, "..", "..", ".."))

include("valuetext.jl")
include("digits.jl")
include("strip.jl")
include("driver.jl")
include("cards.jl")
include("json.jl")
include("notebook.jl")

end
