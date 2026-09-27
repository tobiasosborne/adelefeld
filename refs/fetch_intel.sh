#!/usr/bin/env bash
# Fetch the uops.info instruction pages used by the Intel hardware profile of docs/PERF.md (section 1b)
# into refs/src/uops-intel/ (gitignored) and record their sha256 in refs/manifest-intel.sha256.
# Modelled on refs/fetch_sources.sh. Each uops.info page holds the measurements of every
# microarchitecture the site covers; the Intel profile reads the Alder Lake-P blocks (uops.info has no
# Raptor Lake column), and the register-register forms also give the Zen 2 figures that
# refs/src/uops-zen2 lacks (finding P1 of lanes/m0-sources/report.md).
# Usage:
#   ./fetch_intel.sh          fetch whatever is missing, then (re)write the manifest
#   ./fetch_intel.sh --check  fetch nothing; verify the manifest with sha256sum -c
#   ./fetch_intel.sh --force  re-download everything
set -eu
cd "$(dirname "$0")"

MODE=fetch
FORCE=0
for arg in "$@"; do
  case "$arg" in
    --check) MODE=check ;;
    --force) FORCE=1 ;;
    *) echo "unknown argument: $arg" >&2; exit 2 ;;
  esac
done

if [ "$MODE" = check ]; then
  echo "== manifest-intel.sha256 (fetched bytes)"
  sha256sum -c manifest-intel.sha256
  exit 0
fi

get() { # get URL DEST  -- download DEST from URL unless it exists (or --force); fail on HTTP errors
  url=$1; dest=$2
  if [ -s "$dest" ] && [ "$FORCE" = 0 ]; then
    echo "have $dest"
    return 0
  fi
  mkdir -p "$(dirname "$dest")"
  echo "get  $dest"
  curl -fsSL --retry 2 --max-time 180 -o "$dest" "$url"
}

# ------------------------------------------ the same seven pages as refs/src/uops-zen2
SAME="ADD_R64_M64 ADC_R64_M64 MUL_R64 IMUL_R64_R64 IMUL_R64_R64_I8 IMUL_R64_R64_I32 VPMULUDQ_YMM_YMM_YMM"
# ------------------------------------------ register forms and the other instructions on the dependency
# paths of bench/asm/chain_add_run.s and chain_mul_run.s (opcodes read off the saved bytes:
# 01 add, 83 /0 ib add r64 imm8, 11 adc, 29 sub, 39 cmp, 0f 42 cmovb, 0f 43 cmovae = cmovnb,
# d3 /5 shr r64 cl; d3 /4 shl r64 cl in the batch kernel)
MORE="ADD_01_R64_R64 ADD_R64_I8 ADC_11_R64_R64 SUB_29_R64_R64 CMP_39_R64_R64 CMOVB_R64_R64 CMOVNB_R64_R64"
MORE="$MORE SHR_R64_CL SHL_R64_CL"
# ------------------------------------------ scalar and 256-bit loads and stores, for the I/O models
IO="MOV_R64_M64 MOV_M64_R64 VMOVDQU_YMM_M256 VMOVDQU_M256_YMM"

for n in $SAME $MORE $IO; do
  get "https://uops.info/html-instr/$n.html" "src/uops-intel/$n.html"
done

find src/uops-intel -type f | LC_ALL=C sort | xargs sha256sum > manifest-intel.sha256
echo "== manifest-intel.sha256: $(wc -l < manifest-intel.sha256) files"
