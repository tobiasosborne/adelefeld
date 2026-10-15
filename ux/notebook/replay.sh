#!/bin/sh
# ux/notebook/replay.sh RECORDS: run the driver line of every record and compare (exit 1 on a difference).
set -eu
exec sh ux/notebook/julia.sh ux/notebook/src/replay_main.jl "$@"
