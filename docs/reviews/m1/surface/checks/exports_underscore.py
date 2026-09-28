#!/usr/bin/env python3
"""exports_underscore.py: tests/test_exports.sh on a copy of the tree with one extra source file
that exports two functions no header declares: `adf_undeclared` and `_adf_undeclared`.
conventions 4.1 item 5 names underscore functions as part of the interface ("Underscore
functions state their aliasing rules"), so `_adf_*` is a name space of the library.
The script says "exported and not declared ... must be empty" (tests/test_exports.sh:17-22).

    python3 docs/reviews/m1/surface/checks/exports_underscore.py     (from the repository root)"""
import os, shutil, subprocess

tree = os.path.abspath("build/review_exports_tree")
shutil.rmtree(tree, ignore_errors=True)
os.makedirs(os.path.join(tree, "tests"))
shutil.copytree("include", os.path.join(tree, "include"))
shutil.copytree("src", os.path.join(tree, "src"))
shutil.copy("tests/test_exports.sh", os.path.join(tree, "tests"))
os.makedirs(os.path.join(tree, "build"))
extra = os.path.join(tree, "src", "zz_review.c")
for variant in (["_adf_undeclared"], ["adf_undeclared"]):
    with open(extra, "w") as fh:
        for name in variant:
            fh.write("int %s(void);\nint %s(void) { return 0; }\n" % (name, name))
    p = subprocess.run(["sh", "tests/test_exports.sh"], cwd=tree, capture_output=True, text=True)
    tail = [l for l in (p.stdout + p.stderr).splitlines()
            if "exported and not declared" in l or "test_exports:" in l or "underscore" in l
            or "_adf_undeclared" in l]
    print("%s exported: exit %d" % (variant[0], p.returncode))
    for l in tail:
        print("    " + l)
shutil.rmtree(tree, ignore_errors=True)
