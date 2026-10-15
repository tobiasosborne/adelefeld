"""Lane ux-atlas-r1: bounded hand mutants of the lines this repair wrote. Works on a copy; the slice is not touched.
Usage from the repository root: timeout 170 python3 lanes/ux-atlas-r1/mutants.py SCRATCH_DIR"""
import shutil, subprocess, sys
from pathlib import Path

REPO = Path.cwd()
MUTANTS = [
    ("atlas.js", "Math.floor(GEOMETRY.treeWidth / (2 * GEOMETRY.litRadius)) - 1",
     "Math.floor(GEOMETRY.treeWidth / (2 * GEOMETRY.litRadius))"),
    ("atlas.js", "if (tree.prime > MAX_PRIME) {", "if (tree.prime > MAX_PRIME + 4) {"),
    ("atlas.js", "if (widest > rowCapacity()) {", "if (widest >= rowCapacity()) {"),
    ("atlas.js", "node.exponent = startExponent + node.depth;", "node.exponent = node.depth;"),
    ("atlas.js", "{ y: (0 - top) * step, label", "{ y: (1 - top) * step, label"),
    ("atlas.js", "return [...(result.reason ? [result.reason] : []),\n      'Reported failing place: ' + "
     "(result.failing_place ?? 'not given')];",
     "return ['Reported failing place: ' + (result.failing_place ?? 'not given'),\n"
     "      ...(result.reason ? [result.reason] : [])];"),
    ("atlas.js", "return `${scene.lens} · ${scene.solved_polynomial} = 0`;", "return scene.lens;"),
    ("scene.py", "        if p > MAX_PRIME:", "        if p > 2 * MAX_PRIME:"),
    ("scene.py", 'parser.exit(2, f"scene: {refusal}\\n")', 'parser.exit(1, f"scene: {refusal}\\n")'),
    ("scene.py", "if widest > ROW_CAPACITY:", "if widest >= ROW_CAPACITY:"),
    ("scene.py", ' and session.output(canonical).startswith("(* ;")', ""),
    ("scene.py", '\n            and session.output(real_projection).startswith("error: "))', ")"),
    ("scene.py", "if k == 0 or abs(c) != 1 else", "if True else"),
    ("scene.py", 'fog["source_record"] = canonical', 'fog["source_record"] = real_projection'),
    ("scene.py", '"solved_polynomial": polynomial_text(polynomial) if polynomial else None',
     '"solved_polynomial": polynomial'),
    ("atlas.html", "gateLines(real).forEach((line, i) => text(65, 115 + 16 * i, line));",
     "text(65, 115, 'Reported failing place: not given');"),
    ("atlas.html", "G.treeWidth, G.treeHeight, { topExponent, bottomExponent })", "G.treeWidth, G.treeHeight)"),
]

def run(cmd, cwd, env=None):
    return subprocess.run(["timeout", "120"] + cmd, cwd=cwd, capture_output=True, text=True, env=env).returncode

def main():
    import os
    scratch = Path(sys.argv[1]).resolve()
    adf = str((REPO / "build/adf").resolve())
    env = dict(os.environ, ADF=adf, PYTHONDONTWRITEBYTECODE="1")
    scenes = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else scratch
    killed = 0
    for n, (name, old, new) in enumerate(MUTANTS, 1):
        root = scratch / f"m{n}"
        shutil.rmtree(root, ignore_errors=True)
        shutil.copytree(REPO / "ux/atlas", root / "ux/atlas", ignore=shutil.ignore_patterns("__pycache__"))
        (root / "tools").symlink_to(REPO / "tools")
        target = root / "ux/atlas" / name
        text = target.read_text()
        assert text.count(old) == 1, (n, old)
        target.write_text(text.replace(old, new))
        results = {
            "python": run([sys.executable, "-m", "unittest", "discover", "-s", "ux/atlas"], root, env),
            "node": run(["node", "--test", "--test-isolation=none", "ux/atlas/test_atlas.mjs"], root),
        }
        if name == "atlas.html":
            checks = [("ux/atlas/scenes/idempotents30.json", "idem"), (str(scenes / "onefifth.json"), "onefifth"),
                      (str(scenes / "exp5.json"), "exp")]
            results["page"] = max(run(["node", str(REPO / "lanes/ux-atlas-r1/page_check.cjs"), f, k], root)
                                  for f, k in checks)
        dead = any(results.values())
        killed += dead
        print(f"M{n:02d} {name:10s} {'killed' if dead else 'SURVIVED'} {results}  {new.strip()[:60]!r}")
        shutil.rmtree(root)
    print(f"{killed} of {len(MUTANTS)} killed")

main()
