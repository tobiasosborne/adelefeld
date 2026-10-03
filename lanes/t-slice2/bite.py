#!/usr/bin/env python3
"""lanes/t-slice2/bite.py: faults of src/text_local.c, one at a time, and the tests that bite.

Lane t-slice2, brief item 4 (four faults of your choice; no mutation run).

    python3 lanes/t-slice2/bite.py <name>

The scratch copy is src/text_local.c itself: the file is regenerated from lanes/t-slice2/text_local_tail.c by
lanes/t-slice2/assemble_text_local.py before and after every fault, so the tree is left exactly as it was (the
assembler is the only writer of that file). The build goes to lanes/t-slice2/build-bite, a directory of this
lane; the test binary runs from the repository root.

Every fault is a literal replacement in src/text_local.c. The script refuses to run when the replacement
changes nothing (the fault is not in the file any more).
"""

import os
import subprocess
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
ASM = os.path.join("lanes", "t-slice2", "assemble_text_local.py")
BUILD = os.path.join("lanes", "t-slice2", "build-bite")
SRC = os.path.join("src", "text_local.c")

FAULTS = {
    # 1. the sign of the exponent in O(p^N) is dropped
    "sign": (
        '        flint_sprintf(nbuf, "%wd", x->N);',
        '        flint_sprintf(nbuf, "%wd", x->N < 0 ? -x->N : x->N);',
        "every_row_of_the_golden_file_lball (the rows with a negative N) and "
        "round_trips_of_generated_values",
    ),
    # 2. the canonical order of the places is not enforced at the complex tag (the only path that does not
    #    hand the components to adf_sball_set_arb_lballs, which sorts them itself)
    "order": (
        "                adf_lball_set(&t->loc[k], &loc[k]);",
        "                adf_lball_set(&t->loc[m - 1 - k], &loc[k]);",
        "sball_the_complex_tag (a complex entry with two primes)",
    ),
    # 3. the output is written before the last check: the components are put into x on every status
    "commit": (
        """    adf_lball_init(t);
    st = tl_lball_build(t, v, s, &e, tl_exp_value(s, &e));
    if (st == ADF_OK)
    {
        adf_lball_swap(x, t);""",
        """    adf_lball_init(t);
    st = tl_lball_build(t, v, s, &e, tl_exp_value(s, &e));
    {
        adf_lball_swap(x, t);""",
        "hostile_input ([p=5: 1/3 + O(5^ADF_LBALL_EXP_MAX)]): the output changed on ADF_LIMIT",
    ),
    # 4. p = 4 is accepted: the primality test of the place is dropped
    "prime": (
        "    st = adf_place_prime(&v, p);",
        "    st = ADF_OK;   /* no primality test */",
        "every_row_of_the_golden_file_lball ([p=4: 1], [p=1: 1], [p=0: 1]), hostile_input, "
        "the_order_of_the_stages",
    ),
    # 5. the base inside O(...) is not compared with the prime
    "base": (
        "        if (!tl_base_get_ulong(s, &e.lc.base, &base) || base != p)",
        "        if (!tl_base_get_ulong(s, &e.lc.base, &base))",
        "every_row_of_the_golden_file_lball ([p=5: 3 + O(7^4)]) and the_order_of_the_stages",
    ),
    # 6. the item limit is not applied to a partial ball
    "items": (
        "    if (lim->max_items < 0 || (size_t) lim->max_items < list.n)",
        "    if (0)",
        "hostile_input (max_items = 3 with four entries, and 10000 entries)",
    ),
    # 7. the centre of a ball is printed as the stored unit u instead of p^v u (the defect the tests found)
    "centre": (
        "    st = adf_lball_get_center(c, x);",
        "    st = ADF_OK;   /* the centre is the unit */",
        "every_row_of_the_golden_file_lball, every_row_of_the_golden_file_sball, "
        "round_trips_of_generated_values",
    ),
}


def run(cmd, **kw):
    return subprocess.run(cmd, cwd=ROOT, **kw)


def main():
    if len(sys.argv) != 2 or sys.argv[1] not in FAULTS:
        print("usage: python3 lanes/t-slice2/bite.py " + "|".join(sorted(FAULTS)), file=sys.stderr)
        return 2
    name = sys.argv[1]
    old, new, must = FAULTS[name]
    run([sys.executable, ASM], check=True, stdout=subprocess.DEVNULL)
    path = os.path.join(ROOT, SRC)
    with open(path) as f:
        text = f.read()
    if old not in text:
        print("bite %s: the text to replace is not in %s" % (name, SRC))
        return 1
    with open(path, "w") as f:
        f.write(text.replace(old, new, 1))
    try:
        p = run(["make", "-j2", "BUILD=" + BUILD, BUILD + "/test_text_local"],
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=900)
        log = p.stdout.decode()
        with open(os.path.join(ROOT, "lanes", "t-slice2", "bite-%s.build" % name), "w") as f:
            f.write(log)
        if p.returncode != 0:
            print("bite %s: the build failed" % name)
            print("\n".join(log.split("\n")[-6:]))
            return 1
        try:
            r = run(["timeout", "300", "./" + BUILD + "/test_text_local"],
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=400)
            out = r.stdout.decode()
            status = r.returncode
        except subprocess.TimeoutExpired:
            out = "(the test did not end in 300 s)"
            status = -1
        with open(os.path.join(ROOT, "lanes", "t-slice2", "bite-%s.out" % name), "w") as f:
            f.write(out)
        lines = [l for l in out.split("\n") if l.startswith("FAIL")]
        failed_tests = sorted(set(l.split(": ", 2)[2].split(":")[0] for l in lines if ": check" in l))
        print("bite %s: exit %d, %d failed check lines, %d failed checks" %
              (name, status, len(lines), sum(1 for l in lines if ": check" in l)))
        print("  last line: " + (out.strip().split("\n")[-1] if out.strip() else "(nothing)"))
        print("  failed tests: %s" % (", ".join(failed_tests) or "(none)"))
        print("  must bite: %s" % must)
        for l in lines[:3]:
            print("  " + l[:160])
    finally:
        run([sys.executable, ASM], check=True, stdout=subprocess.DEVNULL)   # the file is restored
    return 0


if __name__ == "__main__":
    sys.exit(main())