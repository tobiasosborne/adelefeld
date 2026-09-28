"""Rebuild tests/test_dump_limits.c and tests/test_dump.c against src/dump.c as it was before the
repairs of R1, R2 and R4, and run them: the red run of the final versions of the tests.

The reverted file is src/dump.c with the four repairs of lane m1-repair-dump taken out by the
exact reverse of the edits that introduced them.  Run from the repository root:

    python3 -B lanes/m1-repair-dump/checks/red_c.py
"""
import pathlib
import shutil
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[3]
HERE = pathlib.Path(__file__).resolve().parent
RED = HERE / "red"
RED.mkdir(exist_ok=True)

REVERSE = [
    # R1: the cap on the block count, in dp_w_ctx
    ("""   every occurrence (8.5 item 4, gate finding G8). Stage 5: at most ADF_MODCTX_MAX_BLOCKS blocks
   (decision M1-D5), decided from the count of the text alone, before the blocks are read into
   integers, before the predicate of conventions 5.14 and before any allocation in proportion
   to the count.  DP_OCC: the occurrence is counted and, if asked for, copied. Stage 6 is the
   caller's (need_blocks differs). */""",
     """   every occurrence (8.5 item 4, gate finding G8). DP_OCC: the occurrence is counted and, if
   asked for, copied. Stage 6 is the caller's (need_blocks differs). */"""),
    ("""    if (st->mode == DP_WORDS && x->k > (size_t) ADF_MODCTX_MAX_BLOCKS)
        return dp_fail(st, ADF_UNSUPPORTED);
""", ""),
    # R2: the bound on the binary exponents of a piece, as a limit of stage 4
    ("""    int st;                            /* ADF_OK, or the status of the first failure */
    int arb_exp_limit;                 /* DP_LIMITS: bound the arb exponents (M1-D9) */""",
     """    int st;                            /* ADF_OK, or the status of the first failure */"""),
    ("""        return dp_parse_fail(st);
    /* The bound of M1-D9 on the exponents of a qclass piece is a limit of stage 4
       (conventions 8.5 item 4), so it is decided here, on the digit strings, before any
       semantic check: dp_abs_over reads a token of any length and calls a value of more than 16
       hexadecimal digits, hence of more than 2^64, over the bound (M1-D7 is the pattern: no
       hidden bound of a machine word).  The other bodies have no such bound. */
    if (st->mode == DP_LIMITS && st->arb_exp_limit
        && (dp_abs_over(a->e, ADF_DUMP_QCLASS_EXP_MAX) || dp_abs_over(a->re, ADF_DUMP_QCLASS_EXP_MAX)))
        return dp_fail(st, ADF_LIMIT);
    return 1;""",
     """        return dp_parse_fail(st);
    return 1;"""),
    ("""    if (st->mode == DP_LIMITS && pieces)
        st->arb_exp_limit = 1;
""", ""),
    # R4: the alphabet of stage 2
    ("""    st.arb_exp_limit = 0;              /* not read in DP_OCC */
""", ""),
    ("""        if ((b < 0x20 && b != 0x09 && b != 0x0a && b != 0x0d) || b > 0x7e)""",
     """        if (b < 0x20 || b > 0x7e)"""),
    # R2 again: the bound of M1-D9 as it was before, inside stage 6
    ("""    fmpz_t A, H, d, m, e;
    arf_t mid, rad, lo, hi;""",
     """    fmpz_t A, H, d, m, e, lim20;
    arf_t mid, rad, lo, hi;"""),
    ("""    fmpz_init(m);
    fmpz_init(e);
    arf_init(mid);
    arf_init(rad);
    arf_init(lo);
    arf_init(hi);
    dp_fmpz(m, a->m);
    dp_fmpz(e, a->e);
    arf_set_fmpz_2exp(mid, m, e);
    dp_fmpz(m, a->rm);
    dp_fmpz(e, a->re);
    arf_set_fmpz_2exp(rad, m, e);
    if (arf_sgn(mid) < 0 || arf_cmp_si(mid, 1) > 0 || !fmpz_is_one(d))
        ok = dp_fail(st, ADF_DOMAIN);
""",
     """    fmpz_init(m);
    fmpz_init(e);
    fmpz_init_set_ui(lim20, UWORD(1) << 20);
    arf_init(mid);
    arf_init(rad);
    arf_init(lo);
    arf_init(hi);
    dp_fmpz(m, a->e);
    dp_fmpz(e, a->re);
    fmpz_abs(m, m);
    fmpz_abs(e, e);
    if (fmpz_cmp(m, lim20) > 0 || fmpz_cmp(e, lim20) > 0)
        ok = dp_fail(st, ADF_LIMIT);
    if (ok)
    {
        dp_fmpz(m, a->m);
        dp_fmpz(e, a->e);
        arf_set_fmpz_2exp(mid, m, e);
        dp_fmpz(m, a->rm);
        dp_fmpz(e, a->re);
        arf_set_fmpz_2exp(rad, m, e);
        if (arf_sgn(mid) < 0 || arf_cmp_si(mid, 1) > 0 || !fmpz_is_one(d))
            ok = dp_fail(st, ADF_DOMAIN);
    }
"""),
    ("""    fmpz_clear(m);
    fmpz_clear(e);
    arf_clear(mid);""",
     """    fmpz_clear(m);
    fmpz_clear(e);
    fmpz_clear(lim20);
    arf_clear(mid);"""),
]

src = (ROOT / "src" / "dump.c").read_text()
for new, old in REVERSE:
    if new not in src:
        raise SystemExit("the text to revert was not found: " + new[:70])
    src = src.replace(new, old)
(RED / "dump_before.c").write_text(src)

run = lambda cmd: subprocess.run(cmd, cwd=ROOT, check=False)
run(["cc", "-Iinclude", "-std=c11", "-O2", "-g", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-c",
     str(RED / "dump_before.c"), "-o", str(RED / "dump.o")])
shutil.copy(ROOT / "build" / "libadelefeld.a", RED / "libadelefeld.a")
run(["ar", "d", str(RED / "libadelefeld.a"), "dump.o"])
subprocess.run(["ar", "r", "libadelefeld.a", "dump.o"], cwd=RED, check=True)
for t in ("test_dump_limits", "test_dump", "test_dump_ctx", "test_dump_golden"):
    run(["cc", "-Iinclude", "-Itests", "-std=c11", "-O2", "-g", str(ROOT / "tests" / (t + ".c")),
         str(ROOT / "build" / "support" / "golden.o"), str(ROOT / "build" / "support" / "jsonl.o"),
         str(RED / "libadelefeld.a"), "-lflint", "-lgmp", "-lm", "-o", str(RED / t)])
print("built the four dump tests against the library before the repairs", flush=True)
sys.exit(0)
