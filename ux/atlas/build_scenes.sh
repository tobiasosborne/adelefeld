#!/bin/sh
# Run from any directory. Override ADF for an isolated lane build.
set -eu
atlas_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(CDPATH= cd -- "$atlas_dir/../.." && pwd)
driver=${ADF:-"$repo_dir/build/adf"}
destination=${1:-"$atlas_dir/scenes"}
if [ ! -x "$driver" ]; then
    echo "Driver missing: $driver. Build with: timeout 180 make -j2 -C tools/adf" >&2
    exit 1
fi
mkdir -p "$destination"
timeout 130 python3 "$atlas_dir/scene.py" 9 --lens 'roots 2' --primes 2 3 5 7 \
    --driver "$driver" --output "$destination/sqrt9.json"
timeout 130 python3 "$atlas_dir/scene.py" '0 mod 30' --lens 'roots 2' --polynomial '0 -1 1' \
    --modular 30 --primes 2 3 5 --driver "$driver" --output "$destination/idempotents30.json"
