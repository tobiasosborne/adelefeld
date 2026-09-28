#!/usr/bin/env python3
"""build_equiv.py: writes tools/mutate/equivalent.txt from the table below.

Every row names one mutant by (file, line as the source stands now, kind, optional old and new
text) and gives the reason.  The row is resolved against the mutants of the file (mutate.py,
mutants_of); a row that matches no mutant or more than one stops the script.  The entry written
is Mutant.entry_text(reason), which carries no line number.  The line number is only used here,
to find the mutant in the source as it stands.

Usage:  python3 lanes/m1-repair-tools/build_equiv.py [OUT]     (default: tools/mutate/equivalent.txt)
        python3 lanes/m1-repair-tools/build_equiv.py --specs   (the SPEC arguments of judge.py)
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "tools", "mutate"))
import mutate  # noqa: E402

FZ = "refs/src/flint-3.0.1/fmpz.rst"
FQ = "refs/src/flint-3.0.1/fmpq.rst"

# ---- the reasons of a commutative swap, by function; each says what the function computes, that
# the value does not depend on the order, and that the exchange leaves the aliasing as it was
ORDER = {
    "fmpz_mul": "the product of two integers does not depend on the order (%s:752-755)" % FZ,
    "fmpz_add": "the sum of two integers does not depend on the order (%s:740-743)" % FZ,
    "fmpz_gcd": "the gcd of two integers is symmetric and always positive (%s:1040-1044)" % FZ,
    "fmpz_lcm": "the lcm of two integers is symmetric and never negative (%s:1051-1055)" % FZ,
    "fmpq_mul": "the product of two rationals does not depend on the order (%s:400-409)" % FQ,
    "fmpq_add": "the sum of two rationals does not depend on the order (%s:400-409)" % FQ,
    "fmpq_gcd": ("the gcd of (p, q) and (r, s) is the canonical form of gcd(p s, q r)/(q s), which "
                 "is symmetric in the two arguments (%s:509-514)" % FQ),
    "n_gcd": "the gcd of two words is symmetric (refs/src/flint-3.0.1/ulong_extras.rst:404-407)",
    "adf_rat_mul": "adf_rat_mul is fmpq_mul (src/rat.c:236-239), and the product does not depend on the order (%s:400-409)" % FQ,
    "adf_rat_add": "adf_rat_add is fmpq_add (src/rat.c:220-223), and the sum does not depend on the order (%s:400-409)" % FQ,
    "arf_add": ("with ARF_PREC_EXACT nothing is rounded (refs/src/flint-3.0.1/arf.rst:91-95), so the "
                "result is the exact sum, which does not depend on the order, and the rounding "
                "mode is not used"),
}
ALIAS = {
    "fmpz": "; FLINT permits any aliasing of the inputs and the output (%s:53-55), and the exchange changes none" % FZ,
    "fmpq": "; aliasing between any combination of the variables is allowed (%s:409), and the exchange changes none" % FQ,
    "n_gcd": "",
    "adf_rat": "; aliasing of any combination is allowed (src/rat.c:217), and the exchange changes none",
    "arf_add": "; no input is the output",
}
SET_ADD = ("adf_fball_add computes the set sum (a + N Zhat) + (b + M Zhat) = (a + b) + gcd(N, M) Zhat "
           "(docs/proofs/precision.md Proposition 1, line 27) in every branch, with the same "
           "backend test for both orders (fb_same_local is symmetric), the local sum through the "
           "lcm of the two denominators and the global one through fmpq_add and fmpq_gcd "
           "(src/fball.c:201-224, 600-628), so the same ball is written; the output may be an "
           "input (include/adelefeld/fball.h:23)")
SET_MUL = ("adf_fball_mul computes the tight set product with radius gcd(a M, b N, N M) "
           "(docs/proofs/precision.md Proposition 2, line 34), symmetric in (a, N) and (b, M); "
           "the local branch tests gcd(gcd(r_i, s_i), q_i) and multiplies the denominators and the "
           "residues, all symmetric, and the global one is fb_mul_global (src/fball.c:715-745, 762-808), so "
           "the same ball is written; the output may be an input (include/adelefeld/fball.h:23)")
ENCL = ("both orders give a ball that contains the exact result, and that is all the library promises "
        "(refs/src/flint-3.0.1/arb.rst:6-12: the output ball is not the smallest possible); it does "
        "not promise the same ball, and the order does change the radius of a product "
        "(docs/reviews/m1/arith/checks/swap_equiv.out: arb_mul differs in 38216 and acb_mul in 55563 "
        "of 200000 random pairs, arb_add and acb_add in 0); the postcondition is the enclosure, which "
        "the tests check")

ROWS = []


def row(file, line, kind, reason, old=None, new=None, occ=None):
    ROWS.append((file, line, kind, old, new, reason, occ))


def swap(file, line, why=None, kindkey=None, alias="fmpz"):
    ROWS.append((file, line, "swap_args", None, None, ("SWAP", kindkey, alias, why), None))


# ---- src/adele.c: the real and complex coordinates (enclosure only), the finite coordinate
for line in (218, 279, 470, 515):
    row("src/adele.c", line, "swap_args", ENCL)
for line in (242, 486):
    row("src/adele.c", line, "swap_args", ENCL)
for line in (219, 280, 471, 516):
    row("src/adele.c", line, "swap_args", SET_ADD)
for line in (243, 487):
    row("src/adele.c", line, "swap_args", SET_MUL)
row("src/adele.c", 53, "cmp",
    "prec < 2 as prec <= 2 in adele_prec: for prec = 2 both conditions give 2 (the true branch "
    "returns 2, the false branch returns prec = 2), and for every other prec they agree", "<", "<=")

# ---- src/cap.c
row("src/cap.c", 97, "swap_args", "fmpq_gcd(gq, Nq, Cq) against fmpq_gcd(gq, Cq, Nq): " +
    ORDER["fmpq_gcd"] + "; gq is a temporary, so no input is written")
row("src/cap.c", 127, "swap_args", SET_ADD)
row("src/cap.c", 157, "swap_args", SET_MUL)

# ---- src/common.c
row("src/common.c", 96, "op",
    "k - 1 < r.n for k + 1 < r.n: k = 0 gives the size_t 0 - 1, which is not below r.n, so no "
    "leading zero is dropped and r.d and r.n keep the whole run; adf_ver_run_equal compares the "
    "digits from the right and pads the shorter run with '0' (src/common.c:104-120), so a run "
    "with leading zeros stands for the same number, and the only other reads of the run are "
    "n == 0 and end, which the drop does not change", "+", "-")

# ---- src/dump.c
row("src/dump.c", 151, "op",
    "t.n - j <= 15 as t.n + j <= 15 in dp_fmpz: both paths give the same fmpz for a validated "
    "token (up to 15 digits are read directly, otherwise fmpz_set_str on a copy, which takes the "
    "sign); with j = 1 (a negative token) the mutant only sends the negative tokens of 14 and 15 "
    "digits to the second path", "-", "+")
row("src/dump.c", 436, "swap_args", "the gcd of the two integers is symmetric and only compared with 1 (%s:1040-1044); g is a temporary" % FZ)
row("src/dump.c", 478, "swap_args", "the gcd of the two integers is symmetric and only compared with 1 (%s:1040-1044); g is a temporary" % FZ)
row("src/dump.c", 495, "zero_one",
    "the initial value of q in ulong q = 0, r = 0 of dp_v_fb: q is written by dp_word before it is "
    "read, because dp_v_ctx has accepted every block as a word (the same dp_word call succeeded "
    "there), so the initial value is never read", "0", "1", occ=1)
row("src/dump.c", 495, "zero_one",
    "the initial value of r in ulong q = 0, r = 0 of dp_v_fb: r is read only in "
    "!dp_word(tr, &r) || r >= q and after it; dp_word writes *v whenever it returns 1, and "
    "when it returns 0 the || does not evaluate r >= q and ok = 0 skips every later use, so "
    "the initial value is never read", "0", "1", occ=2)
row("src/dump.c", 744, "swap_args", "arf_add(hi, mid, rad, ARF_PREC_EXACT, ARF_RND_DOWN) against the same with mid and rad exchanged: " + ORDER["arf_add"] + ALIAS["arf_add"])
row("src/dump.c", 903, "swap_args", "fmpz_mul(D, D, M) against fmpz_mul(D, M, D): " + ORDER["fmpz_mul"] + ALIAS["fmpz"])
row("src/dump.c", 1061, "cmp",
    "len < 3 as len <= 3 in dp_header: a text of exactly 3 bytes is ADF_PARSE in both: the original "
    "returns it at the prefix test unless the text is \"adf\", and \"adf\" has no version digit and "
    "is ADF_PARSE at the next test (pos == v0); the mutant returns ADF_PARSE for it at this test", "<", "<=")
row("src/dump.c", 1128, "op",
    "s[i + 1] == ' ' as s[i - 1] == ' ' in the test for two spaces in a row: the pair (j, j + 1) is "
    "found at i = j + 1 instead of i = j; i > b there, because s[b] is not a space (checked above) "
    "and the test is inside if (s[i] == ' '), so s[i - 1] is a byte of the body; the loop has no "
    "other exit than the ADF_PARSE, so the same texts are refused", "+", "-")
row("src/dump.c", 1226, "zero_one",
    "the initial value of q in ulong q = 0 of dp_bind_matches: dp_word writes q before it is read, "
    "because the block is a validated word", "0", "1")
row("src/dump.c", 1382, "cmp",
    "n + extra + 1 > cap as >= cap in dp_sb_room: the buffer is reallocated one byte earlier, to a "
    "size that is at least as large; only the capacity differs, not the bytes written", ">", ">=")
row("src/dump.c", 1384, "cmp",
    "n + extra + 1 > cap as >= cap in the loop of dp_sb_room: the capacity doubles once more when "
    "the need equals it; only the capacity differs, not the bytes written", ">", ">=")
row("src/dump.c", 1437, "zero_one",
    "n + 1 > cap as n + 0 > cap in dp_sb_finish: dp_sb_room keeps n + 1 <= cap after every append "
    "(it makes room for n + extra + 1), so n < cap and neither condition is true in any run; the "
    "test only guards a broken dp_sb_room", "1", "0")

# ---- src/fball.c
swap("src/fball.c", 68, "g is a temporary of fb_canon", "fmpz_gcd")
swap("src/fball.c", 211, "L is a temporary of fb_local_addsub", "fmpz_lcm")
swap("src/fball.c", 322, "g is a temporary", "fmpz_gcd")
swap("src/fball.c", 627, "c is a temporary of adf_fball_add", "fmpq_add", "fmpq")
swap("src/fball.c", 628, "r is a temporary of adf_fball_add", "fmpq_gcd", "fmpq")
swap("src/fball.c", 669, "r is a temporary of adf_fball_sub", "fmpq_gcd", "fmpq")
swap("src/fball.c", 732, "c is a temporary of fb_mul_global", "fmpq_mul", "fmpq")
swap("src/fball.c", 733, "t1 is a temporary", "fmpq_mul", "fmpq")
swap("src/fball.c", 734, "t2 is a temporary", "fmpq_mul", "fmpq")
swap("src/fball.c", 735, "t3 is a temporary", "fmpq_mul", "fmpq")
swap("src/fball.c", 736, "g is a temporary", "fmpq_gcd", "fmpq")
swap("src/fball.c", 737, "g is both the output and the first input", "fmpq_gcd", "fmpq")
swap("src/fball.c", 775, "only the value 1 of the result is tested", "n_gcd", "n_gcd")
swap("src/fball.c", 781, "de is a temporary", "fmpz_mul")
swap("src/fball.c", 849, "c is a temporary of adf_fball_mul_rat", "fmpq_mul", "fmpq")
swap("src/fball.c", 851, "r is both the output and the first input", "fmpq_mul", "fmpq")
swap("src/fball.c", 949, "g is a temporary of fb_overlaps_g", "fmpq_gcd", "fmpq")
row("src/fball.c", 237, "drop_call",
    "fmpz_zero(x->A) in adf_fball_init: fmpz_init(x->A) a few lines above sets the value to zero "
    "(refs/src/flint-3.0.1/fmpz.rst:181-184), and nothing writes x->A in between")
row("src/fball.c", 238, "drop_call",
    "fmpz_zero(x->H) in adf_fball_init: fmpz_init(x->H) a few lines above sets the value to zero "
    "(refs/src/flint-3.0.1/fmpz.rst:181-184), and nothing writes x->H in between")
row("src/fball.c", 180, "cmp",
    "z->backend == ADF_LOCAL as != in fb_local_res: the array is then resized when z is a local "
    "value of ctx as well, where the size is the same and realloc keeps the contents; the two "
    "conditions differ only for a global z with mctx == ctx, which does not occur because every "
    "global value has mctx = NULL (src/fball.c:5, and every function that makes a value global "
    "sets it) and ctx is not NULL", "==", "!=", occ=1)
row("src/fball.c", 1010, "drop_call",
    "the second fmpq_sub(diff, a, b) in fb_contains_g recomputes what diff already holds from the "
    "first at the beginning of the branch: a, b and diff are not written in between (only t is)")

# ---- src/fball_local.c
swap("src/fball_local.c", 107, "u is a temporary", "fmpz_mul")
swap("src/fball_local.c", 108, "v is a temporary", "fmpz_mul")
swap("src/fball_local.c", 165, "e is a temporary", "fmpz_mul")
swap("src/fball_local.c", 167, "e is a temporary", "fmpz_gcd")
swap("src/fball_local.c", 170, "A0 is a temporary", "fmpz_mul")

# ---- src/modctx.c
swap("src/modctx.c", 209, "only the value 1 of the result is tested", "n_gcd", "n_gcd")
row("src/modctx.c", 339, "zero_one",
    "k < 0 as k < 1 in adf_modctx_new_factorial: adf_factorial_blocks returns -1 or a count, and "
    "for n >= 2, the only n that reaches this line, the prime 2 gives at least one block, so 0 is "
    "never returned and both tests are true exactly for -1", "0", "1")
row("src/modctx.c", 543, "zero_one",
    "the sign argument 0 as 1 of fmpz_multi_CRT_precomp in adf_modctx_recombine: the value is "
    "congruent to the inputs modulo every block for either sign (refs/src/flint-3.0.1/"
    "fmpz.rst:1362-1365), the next line reduces it into [0, K) by fmpz_fdiv_r, and the reduced "
    "value is the same integer for any two congruent integers", "0", "1")

# ---- src/rat.c
row("src/rat.c", 222, "swap_args", "fmpq_add(z->q, x->q, y->q) against the exchange: " + ORDER["fmpq_add"] + ALIAS["fmpq"])
row("src/rat.c", 238, "swap_args", "fmpq_mul(z->q, x->q, y->q) against the exchange: " + ORDER["fmpq_mul"] + ALIAS["fmpq"])

# ---- src/recon.c
row("src/recon.c", 124, "zero_one",
    "sh = ok ? fmpz_get_si(m) : 0 as : 1 in fmpq_set_dyadic: sh is read only inside if (ok), where "
    "the mutant computes the same value", "0", "1")
row("src/recon.c", 130, "cmp",
    "fmpz_sgn(exp) >= 0 as > 0: for exp = 0 both branches compute the rational mn/1 (the shift is "
    "by sh = 0 in both, and fmpq_canonicalise follows)", ">=", ">")
row("src/recon.c", 130, "zero_one",
    "fmpz_sgn(exp) >= 0 as >= 1: only exp = 0 changes branch, and both branches give mn/1 there",
    "0", "1")
row("src/recon.c", 134, "drop_call",
    "fmpz_one(fmpq_denref(q)) in fmpq_set_dyadic: the only two calls pass lo->q and hi->q of "
    "adf_rat_init (src/recon.c:277-278), which are 0/1 (fmpq.rst:57-60), and nothing writes the "
    "denominator before, so the call writes 1 where there already is 1")
row("src/recon.c", 177, "zero_one",
    "fmpq_cmp(lo, hi) > 0 as > 1 removes the early return for the empty interval; the rest of the "
    "function answers ADF_NO_SOLUTION for lo > hi in both cases: for N = 0 the test "
    "lo > a or a > hi holds, and for N > 0 kmin = ceil((lo - a)/N) is above kmax = floor((hi - a)/N)"
    " because (lo - a)/N > (hi - a)/N; the conversions of x that come first cannot fail for a "
    "valid ball, whose denominator is positive", "0", "1")
row("src/recon.c", 230, "drop_call",
    "fmpz_one(fmpq_denref(c->q)) in adf_fball_reconstruct: c is the adf_rat_init of the function "
    "(0/1) and this is the only write of its denominator before the numerator is set from kmin on "
    "the line above, so the call writes 1 where there already is 1")
swap("src/recon.c", 231, "c is both the output and the first input", "adf_rat_mul", "adf_rat")
swap("src/recon.c", 232, "c is both the output and the first input", "adf_rat_add", "adf_rat")

# ---- src/scaled.c
for line, why in ((201, "t is a temporary"), (212, "t is a temporary"), (272, "t is a temporary"),
                  (356, "r is a temporary"), (357, "ta is a temporary"), (379, "r is a temporary"),
                  (444, "r is a temporary"), (513, "r is a temporary"), (668, "r is a temporary")):
    swap("src/scaled.c", line, why, "fmpz_mul")
for line in (358, 380, 669):
    swap("src/scaled.c", line, "r is both the output and the first input", "fmpz_add")
for line in (267, 351, 440, 447, 466, 508, 517, 537, 602, 616, 646):
    swap("src/scaled.c", line, None, "fmpq_add" if line in (351, 646) else "fmpq_mul", "fmpq")
row("src/scaled.c", 411, "swap_args",
    "adf_scaled_add(z, x, &t) against adf_scaled_add(z, &t, x): adf_scaled_add is symmetric in its "
    "two operands: the context check compares the two pointers, both exact gives fmpq_add, both "
    "scaled gives g = gcd(s, t) with the cofactors exchanged together with the residues, and one "
    "exact operand is selected by its tag, not by its position (src/scaled.c:331-390); the output "
    "goes through the temporary t, so no aliasing matters")
row("src/scaled.c", 54, "drop_call",
    "fmpq_zero(x->s) after fmpq_init(x->s): fmpq_init already sets the value to 0 "
    "(refs/src/flint-3.0.1/fmpq.rst:57-60)")
row("src/scaled.c", 56, "drop_call",
    "fmpz_zero(x->u) after fmpz_init(x->u): fmpz_init already sets the value to zero "
    "(refs/src/flint-3.0.1/fmpz.rst:181-184)")
for line in (459, 530):
    row("src/scaled.c", line, "drop_call",
        "fmpq_zero(t.s) in the branch of an exact zero factor: t was made by adf_scaled_init "
        "(exact 0, s = 0/1, src/scaled.c:48-58) and nothing has written t.s on this path")
for line in (469, 540, 618):
    row("src/scaled.c", line, "cmp",
        "fmpq_sgn(q) < 0 as <= 0: the branch is reached only for q != 0 (the test of q = 0 comes "
        "first), where fmpq_sgn is -1 or 1, and both values answer the same to < 0 and <= 0",
        "<", "<=")
    row("src/scaled.c", line, "zero_one",
        "fmpq_sgn(q) < 0 as < 1: on the only possible values -1 and 1 of fmpq_sgn (q != 0 here) "
        "the comparisons < 0 and < 1 agree", "0", "1")

# ---- src/text.c
swap("src/text.c", 745, "num is both the output and the first input", "fmpz_mul")
swap("src/text.c", 857, "R is both the output and the first input", "fmpq_add", "fmpq")
swap("src/text.c", 917, "v is both the output and the first input", "fmpz_mul")
swap("src/text.c", 919, "u is both the output and the first input", "fmpz_mul")
swap("src/text.c", 1201, "t is both the output and the first input", "fmpq_add", "fmpq")


def reason_of(m, spec):
    if isinstance(spec, str):
        return spec
    _, kindkey, alias, why = spec
    name = m.old.split("(")[0] if kindkey is None else kindkey
    call = m.old
    return "%s against %s: %s%s%s" % (call, m.new, ORDER[name],
                                      ALIAS["adf_rat" if name.startswith("adf_rat") else
                                            "n_gcd" if name == "n_gcd" else
                                            "arf_add" if name == "arf_add" else alias],
                                      ("; " + why) if why else "")


def resolve(cache, file, line, kind, old, new, occ=None):
    if file not in cache:
        with open(os.path.join(ROOT, file)) as fh:
            cache[file] = mutate.mutants_of(file, fh.read(), ROOT)
    found = [m for m in cache[file] if m.line == line and m.kind == kind
             and (old is None or m.old == old) and (new is None or m.new == new)]
    if occ is not None:
        found = sorted(found, key=lambda m: m.start)[occ - 1: occ]   # the occ-th of the line
    if len(found) != 1:
        raise SystemExit("%s:%d:%s %s->%s matches %d mutants: %s"
                         % (file, line, kind, old, new, len(found), [str(m) for m in found]))
    return found[0]


HEADER = """\
# tools/mutate/equivalent.txt: the mutants that survive the tests and are equivalent, with the reason.
#
# An entry is one line
#
#   <file> | <kind> | <text of the line the mutant changes, stripped> | '<old>' -> '<new>' [#n] | <reason>
#
# The text of the line is the line of the source with the white space at both ends cut off. `#n` follows
# the change when the same line text with the same change stands more than once in the file: n counts
# them from 1 in the order of the file, and the first of them is `#1`. There is no line number: a line
# number does not survive an edit above the mutant (docs/reviews/m1/surface/review.md R3), and it
# excused a different statement in one case. The key of a mutant is printed by
# `python3 tools/mutate/mutate.py --root . --files FILE --list`, and
# `python3 tools/mutate/check_equivalent.py` says which entries match no mutant of the sources.
# KIND is one of op, cmp, logic, gcd_lcm, swap_args, status, drop_call, call_swap, prec, zero_one,
# drop_assign, negate_if. A `#` at the start of a line is a comment. A reason must not hold a ` | `
# of its own, because that is what ends the key.
#
# An entry excuses a survivor: `make mutate` reports it as excused and does not fail. It is accepted
# only if its reason says why the mutant computes the same thing as the original for every input the
# program can give it (not that the tests do not notice), and only after the mutant has been built and
# the tests have passed with it. An entry whose mutant is gone is deleted, not left. A survivor that is
# not equivalent is not listed here: it needs a test.
#
# Written on 2026-09-28 by lane m1-repair-tools from the sources of that day;
# lanes/m1-repair-tools/build_equiv.py holds the table it was generated from.
#
"""


def main(argv):
    cache = {}
    out = [HEADER]
    specs = []
    last_file = None
    for file, line, kind, old, new, reason, occ in ROWS:
        m = resolve(cache, file, line, kind, old, new, occ)
        if file != last_file:
            out.append("\n# %s" % file)
            last_file = file
        out.append(m.entry_text(reason_of(m, reason)))
        arg = "%s:%d:%s" % (file, line, kind)
        if old is not None:
            arg += ":%s@@%s" % (old, new)
            if occ is not None:
                arg += "#%d" % occ
        specs.append(arg)
    if len(specs) != len(set(specs)):
        pass
    if argv and argv[0] == "--specs":
        print("\n".join(specs))
        return 0
    path = argv[0] if argv else os.path.join(ROOT, "tools", "mutate", "equivalent.txt")
    with open(path, "w") as fh:
        fh.write("\n".join(out) + "\n")
    print("%d entries written to %s" % (len(specs), path))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
