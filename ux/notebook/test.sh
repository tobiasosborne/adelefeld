#!/bin/sh
# ux/notebook/test.sh: run the tests of the notebook slice, from the repository root.
set -eu
exec sh ux/notebook/julia.sh ux/notebook/test/runtests.jl
