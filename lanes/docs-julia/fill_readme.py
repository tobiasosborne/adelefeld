#!/usr/bin/env python3
"""lanes/docs-julia/fill_readme.py: writes julia/Adelefeld/README.md from lanes/docs-julia/README.src.md.

@@BLOCK name@@ becomes a fenced julia block holding the block `name` of the tutorial output (tutorial.out);
@@EXPORTS@@ becomes the rows of exports_table.md; @@PKGTEST@@ becomes a fenced text block of the
`Test Summary` lines of pkgtest.log (each summary header with its row). Run from the worktree root.
"""
import re, sys

LANE = "lanes/docs-julia/"

def tutorial_blocks(path):
    blocks, name = {}, None
    for line in open(path).read().split("\n"):
        m = re.match(r"^#### block (\S+)$", line)
        if m:
            name = m.group(1); blocks[name] = []
        elif name is not None:
            blocks[name].append(line)
    return {k: "\n".join(v).rstrip("\n") for k, v in blocks.items()}

def pkgtest_lines(path):
    lines = open(path).read().split("\n")
    out = []
    for i, l in enumerate(lines):
        if l.startswith("Test Summary:"):
            out += [l, lines[i + 1]]
    out += [l for l in lines if "Testing Adelefeld tests passed" in l]
    return "\n".join(out)

def main():
    src = open(LANE + "README.src.md").read()
    blocks = tutorial_blocks(LANE + "tutorial.out")
    def block(m):
        return "```julia\n" + blocks[m.group(1)] + "\n```"
    out = re.sub(r"^@@BLOCK (\S+)@@$", block, src, flags=re.M)
    out = out.replace("@@EXPORTS@@\n", open(LANE + "exports_table.md").read())
    out = out.replace("@@PKGTEST@@", "```text\n" + pkgtest_lines(LANE + "pkgtest.log") + "\n```")
    assert "@@" not in out, "unfilled marker"
    open("julia/Adelefeld/README.md", "w").write(out)
    print("fill_readme: wrote julia/Adelefeld/README.md,", len(blocks), "tutorial blocks")

main()
