#!/usr/bin/env python3
"""Replay fixtures and check a bounded collection of additional builder calls.

Run from the repository root:
timeout 170 python3 lanes/ux-atlas/check_scenes.py --driver lanes/ux-atlas/build/adf
"""
import argparse
from fractions import Fraction
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "ux/atlas"))
from scene import build_scene  # noqa: E402

parser = argparse.ArgumentParser()
parser.add_argument("--driver", type=Path, default=ROOT / "build/adf")
driver = parser.parse_args().driver.resolve()
lane = ROOT / "lanes/ux-atlas"

for name in ("sqrt9", "idempotents30"):
    scene = json.loads((ROOT / "ux/atlas/scenes" / (name + ".json")).read_text())
    p = scene["provenance"]
    run = subprocess.run(["timeout", "120", str(driver)], input=p["stdin"], text=True,
                         capture_output=True, timeout=125, check=False)
    assert (run.stdout, run.stderr, run.returncode) == (p["stdout"], p["stderr"], p["exit_code"])
    print(name, "replay", len(p["records"]), "lines", "exit", run.returncode)

cases = [
    ("minus1", "-1", "roots 2", [2, 3, 5, 7]),
    ("exp5", "(* ; 5 mod 25)", "exp", [2, 3, 5]),
    ("logminus1", "-1", "log", [2, 3, 5]),
    ("fraction", "1/5", "identity", [2, 5]),
    ("fractionroot", "1/25", "roots 2", [5]),
    ("ballroot", "1 mod 16", "roots 2", [2]),
    ("coarse", "0 mod 1", "identity", [2, 3, 5]),
    ("sqrt2", "2", "roots 2", [2, 3, 5, 7]),
    ("exp0", "0", "exp", [2, 3, 5]),
    ("log1", "1", "log", [2, 3, 5]),
]
scenes = {}
for name, value, lens, primes in cases:
    scene = build_scene(value, lens, primes, 1, 5, driver)
    scenes[name] = scene
    (lane / (name + ".json")).write_text(json.dumps(scene, indent=2) + "\n")
    for tree in scene["trees"]:
        p = tree["prime"]
        for result in tree["depths"].values():
            for path in result["paths"]:
                s, n = path["start_exponent"], path["end_exponent"]
                reconstructed = sum((digit * Fraction(p) ** (s + i)
                                     for i, digit in enumerate(path["digits"])), Fraction(0))
                difference = (Fraction(path["centre"]) - reconstructed) / Fraction(p) ** n
                assert difference.denominator % p != 0
    print(name, "records", len(scene["provenance"]["records"]), "real", scene["ruler"]["status"],
          "places", [(t["prime"], t["depths"]["5"]["status"], t["depths"]["5"]["lit_path_count"])
                     for t in scene["trees"]])

minus = scenes["minus1"]
assert minus["ruler"]["balls"] == []
assert [t["depths"]["5"]["branch_count"] for t in minus["trees"]] == [0, 0, 2, 0]
exp = scenes["exp5"]
assert exp["ruler"]["status"] == "DOMAIN"
assert [t["depths"]["5"]["status"] for t in exp["trees"]] == ["NOT_DETERMINED", "NOT_DETERMINED", "OK"]
assert exp["trees"][2]["depths"]["5"]["paths"][0]["end_exponent"] == 2
assert scenes["fraction"]["trees"][1]["depths"]["5"]["paths"][0]["start_exponent"] == -1
assert {t["value"] for t in scenes["fractionroot"]["threads"]} == {"1/5", "-1/5"}
assert scenes["sqrt2"]["ruler"]["balls"][0]["dyadic_endpoints"] == "not given"
assert scenes["sqrt2"]["threads"] == []
assert scenes["exp0"]["threads"][0]["value"] == "1"
assert scenes["log1"]["threads"][0]["value"] == "0"
print("10 additional scenes checked; 2 fixture transcripts replayed byte-for-byte")
