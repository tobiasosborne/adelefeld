#!/usr/bin/env python3
"""Remove the R3 entry check from a scratch copy of fball.c."""

from pathlib import Path
import sys

source = Path("src/fball.c").read_text()
needle = "adf_fball_neg(adf_fball_t y, const adf_fball_t x)\n{\n    ADF_INV_FBALL(x);\n"
assert source.count(needle) == 1
Path(sys.argv[1]).write_text(source.replace(needle, needle.replace("    ADF_INV_FBALL(x);\n", "")))
