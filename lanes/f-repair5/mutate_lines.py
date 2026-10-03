#!/usr/bin/env python3
"""f-repair5: tools/mutate/mutate.py restricted to the lines this lane changed (lanes/COMMON-C.md rule 5, brief
"mutation testing of the lines you changed"). The tool has no line filter; this wrapper loads it as a module and
filters the mutants of each file to the new-side line numbers of the hunks of `git diff -U0 <file>`, saved as
lanes/f-repair5/<name>.diff before the run. Everything else (choice by --seed and --limit, build, judgement) is the
tool's own. Usage: mutate_lines.py <mutate.py options>"""
import importlib.util, os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
TOOL = os.path.join(HERE, "..", "..", "tools", "mutate", "mutate.py")


def changed_lines(diff_path):
    lines = set()
    for l in open(diff_path):
        m = re.match(r"^@@ -\S+ \+(\d+)(?:,(\d+))? @@", l)
        if m:
            start, count = int(m.group(1)), int(m.group(2) or "1")
            lines.update(range(start, start + count))
    return lines


spec = importlib.util.spec_from_file_location("mutate", TOOL)
mutate = importlib.util.module_from_spec(spec)
sys.modules["mutate"] = mutate
spec.loader.exec_module(mutate)
CHANGED = {"src/lpow.c": changed_lines(os.path.join(HERE, "lpow.diff")),
           "src/lroot.c": changed_lines(os.path.join(HERE, "lroot.diff"))}
_orig = mutate.mutants_of


def mutants_of(path, text, root="."):
    found = _orig(path, text, root)
    keep = [m for m in found if m.line in CHANGED.get(path, set())]
    print("mutate_lines: %s: %d of %d mutants on changed lines" % (path, len(keep), len(found)))
    return keep


mutate.mutants_of = mutants_of
sys.exit(mutate.main(sys.argv[1:]))
