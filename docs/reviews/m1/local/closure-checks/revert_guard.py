#!/usr/bin/env python3
"""Put the former local rejection back into a scratch copy of cap.c."""

from pathlib import Path
import sys

source = Path("src/cap.c").read_text()
needle = (
    "adf_fball_cap(adf_fball_t y, const adf_fball_t x, const adf_rat_t C)\n"
    "{\n"
)
assert source.count(needle) == 1
replacement = needle + "    if (x->backend == ADF_LOCAL)\n        return ADF_UNSUPPORTED;\n"
Path(sys.argv[1]).write_text(source.replace(needle, replacement))
