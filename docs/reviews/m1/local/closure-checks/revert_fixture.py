#!/usr/bin/env python3
"""Restore the invalid R4 fixture in a scratch copy of test_fball.c."""

from pathlib import Path
import sys

source = Path("tests/test_fball.c").read_text()
declaration = "    ulong q4 = 4;\n"
assert source.count(declaration) == 1
source = source.replace(declaration, declaration + "    char dummy = 0;\n")
old = """    fmpz_set_si(x->A, 0);
    fmpz_set_si(x->H, 4);
    fmpz_set_si(x->d, 2);
    x->backend = ADF_LOCAL;
    x->mctx = ctx4;
    x->res = (ulong *) flint_malloc(sizeof(ulong));
    x->res[0] = 2;
"""
new = """    fmpz_set_si(x->A, 2);
    fmpz_set_si(x->H, 4);
    fmpz_set_si(x->d, 2);
    x->backend = ADF_LOCAL;
    x->mctx = (const adf_modctx_struct *) (const void *) &dummy;
    x->res = (ulong *) flint_malloc(2 * sizeof(ulong));
    x->res[0] = 0;
    x->res[1] = 1;
"""
assert source.count(old) == 1
Path(sys.argv[1]).write_text(source.replace(old, new))
