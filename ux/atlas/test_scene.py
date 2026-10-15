"""Run from the repository root: timeout 120 python3 -m unittest discover -s ux/atlas."""
import unittest
from fractions import Fraction
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

import scene as S
from scene import digit_path, fog_count, parse_output, parse_real, parse_value, STATUS_CLASSES

HERE = Path(__file__).resolve().parent
DRIVER = Path(os.environ.get("ADF", S.ROOT / "build/adf")).resolve()


class SceneTests(unittest.TestCase):
    def test_nine_mod_125(self):
        self.assertEqual(digit_path("9", 5, 3)["digits"], [4, 1, 0])

    def test_negative_mod_125(self):
        self.assertEqual(digit_path("-1", 5, 3)["digits"], [4, 4, 4])

    def test_rational_digits(self):
        self.assertEqual(digit_path("1/2", 5, 3)["digits"], [3, 2, 2])

    def test_negative_valuation(self):
        path = digit_path("1/5", 5, 3)
        self.assertEqual(path["start_exponent"], -1)
        self.assertEqual(path["digits"], [1, 0, 0, 0])
        self.assertEqual(path["nodes"][0]["value_form"], "[p=5: 1/5 + O(5^0)]")

    def test_branch_list(self):
        result = parse_output("2 mod 5^3; 123 mod 5^3", 5)
        self.assertEqual(result["status"], "OK")
        self.assertEqual([b["centre"] for b in result["balls"]], ["2", "123"])
        self.assertEqual([b["precision"] for b in result["balls"]], [3, 3])

    def test_seeded_branch_list(self):
        result = parse_output("5 [2]: 2 + O(5^3); 5 [3]: 123 + O(5^3)", 5)
        self.assertEqual([b["seed"] for b in result["balls"]], ["2", "3"])

    def test_partial_ball(self):
        result = parse_output("5: 1/5 + O(5^2)", 5)
        self.assertEqual(result["balls"][0]["centre"], "1/5")

    def test_error(self):
        result = parse_output("error: NOT_DETERMINED", 5)
        self.assertEqual(result["status"], "NOT_DETERMINED")
        self.assertEqual(result["status_class"], "open")
        self.assertEqual(result["failing_place"], "not given")
        self.assertEqual(result["balls"], [])

    def test_empty_list_is_complete(self):
        result = parse_output("none", 3)
        self.assertEqual(result["status"], "OK")
        self.assertEqual(result["balls"], [])

    def test_bad_output_is_rejected(self):
        with self.assertRaises(ValueError):
            parse_output("unrecognised text", 5)
        with self.assertRaises(ValueError):
            parse_output("1 mod 3^2", 5)

    def test_fog(self):
        self.assertEqual(fog_count(parse_value("9"), [2, 3])["count"], "infinitely many")
        fog = fog_count(parse_value("0 mod 2310"), [2, 3, 5])
        self.assertEqual(fog["count"], "every other p")
        self.assertEqual(fog["undrawn_modulus_primes_below_1000"], [7, 11])
        self.assertEqual(fog["undrawn_modulus_prime_count_below_1000"], 2)

    def test_fractional_modulus_fog(self):
        fog = fog_count(parse_value("(* ; 1/3 mod 10/21)"), [2, 3])
        self.assertEqual(fog["undrawn_modulus_primes_below_1000"], [5, 7])

    def test_value_forms(self):
        self.assertEqual(parse_value("(* ; -2/3)")["centre"], Fraction(-2, 3))
        self.assertEqual(parse_value("(* ; 2 mod 6)")["modulus"], Fraction(6))
        with self.assertRaises(ValueError):
            parse_value("9\nshow 1")

    def test_real_endpoints_are_honest(self):
        ball = parse_real("1.4142135623730950488 +/- 1.7e-21")
        self.assertEqual(ball["dyadic_endpoints"], "not given")
        self.assertEqual(ball["printed"], "1.4142135623730950488 +/- 1.7e-21")
        self.assertEqual(parse_real("3")["dyadic_endpoints"], ["3", "3"])

    def test_all_statuses(self):
        for status, cls in STATUS_CLASSES.items():
            if status != "OK":
                result = parse_output("error: " + status, 5)
                self.assertEqual(result["status_class"], cls)
        with self.assertRaises(ValueError):
            parse_output("error: INVENTED", 5)

    def test_negative_precision(self):
        path = digit_path("0", 5, -2)
        self.assertEqual(path["start_exponent"], -2)
        self.assertEqual(path["digits"], [])


class FixtureTests(unittest.TestCase):
    def scenes(self):
        for name in ("sqrt9", "idempotents30"):
            yield json.loads((Path(__file__).parent / "scenes" / (name + ".json")).read_text())

    def test_transcripts_are_verbatim_and_complete(self):
        for scene in self.scenes():
            p = scene["provenance"]
            commands = [line for line in p["stdin"].splitlines()
                        if not line.startswith(("prec ", "digits "))]
            self.assertEqual(commands, [r["command"] for r in p["records"]])
            self.assertEqual(p["stdout"], "".join(r["output"] + "\n" for r in p["records"]))
            self.assertEqual(p["stderr"], "")
            errors = any(r["output"].startswith("error: ") for r in p["records"])
            self.assertEqual(p["exit_code"], int(errors))

    def test_every_digit_reconstructs_its_driver_centre(self):
        for scene in self.scenes():
            for tree in scene["trees"]:
                p = tree["prime"]
                for depth, result in tree["depths"].items():
                    for path in result["paths"]:
                        source = scene["provenance"]["records"][path["source_record"]]["output"]
                        self.assertIn(path["printed"], source)
                        reconstruction = sum((digit * Fraction(p) ** (path["start_exponent"] + i)
                                              for i, digit in enumerate(path["digits"])), Fraction(0))
                        delta = (Fraction(path["centre"]) - reconstruction) / Fraction(p) ** path["end_exponent"]
                        self.assertNotEqual(delta.denominator % p, 0)
                        self.assertLessEqual(path["end_exponent"], int(depth))
                        for k, node in enumerate(path["nodes"], 1):
                            prefix = sum((digit * Fraction(p) ** (path["start_exponent"] + i)
                                          for i, digit in enumerate(path["digits"][:k])), Fraction(0))
                            self.assertEqual(Fraction(node["centre"]), prefix)
                            key = ".".join(map(str, path["digits"][:k]))
                            self.assertEqual(result["node_balls"][key], node["value_form"])

    def test_counts_keep_coarse_collisions(self):
        sqrt9, idem = list(self.scenes())
        self.assertEqual(len(sqrt9["provenance"]["records"]), 46)
        self.assertEqual(len(idem["provenance"]["records"]), 51)
        for scene in (sqrt9, idem):
            for tree in scene["trees"]:
                for depth, result in tree["depths"].items():
                    collision = scene is sqrt9 and tree["prime"] in (2, 3) and depth == "1"
                    self.assertEqual(result["branch_count"], 2)
                    self.assertEqual(result["lit_path_count"], 1 if collision else 2)

    def test_branch_seeds_use_full_certificate_precision(self):
        sqrt9, _ = list(self.scenes())
        for tree in sqrt9["trees"]:
            for result in tree["depths"].values():
                seeds = [path["seed"] for path in result["paths"]]
                self.assertNotIn("not given", seeds)
                self.assertEqual(len(set(seeds)), 2)

    def test_rational_threads_match_all_places(self):
        sqrt9, idem = list(self.scenes())
        self.assertEqual({t["value"] for t in sqrt9["threads"]}, {"-3", "3"})
        self.assertEqual(idem["threads"], [])
        for thread in sqrt9["threads"]:
            self.assertEqual(sqrt9["provenance"]["records"][thread["record"]]["output"], thread["value"])
            for tree in sqrt9["trees"]:
                for depth, result in tree["depths"].items():
                    digits = thread["places"][str(tree["prime"])][depth]["digits"]
                    self.assertIn(digits, [p["digits"] for p in result["paths"]])

    def test_eight_modular_solutions_are_exhaustive_and_driver_checked(self):
        _, idem = list(self.scenes())
        solutions = idem["modular_solutions"]
        self.assertEqual(len(solutions), 8)
        self.assertEqual([int(s["residue"]) for s in solutions],
                         [e for e in range(30) if (e * e - e) % 30 == 0])
        for solution in solutions:
            e = int(solution["residue"])
            self.assertEqual(solution["digits"], [e % p for p in (2, 3, 5)])
            self.assertEqual(int(solution["square"]), e * e)
            self.assertEqual(solution["contains"], "true")


def fixtures():
    return [json.loads((HERE / "scenes" / (name + ".json")).read_text()) for name in ("sqrt9", "idempotents30")]


class RepairTests(unittest.TestCase):
    """Lane ux-atlas-r1: items 1 to 5 of lanes/ux-atlas-r1/brief.md."""

    def test_item1_prime_above_limit_is_refused_before_any_driver_call(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / "p101.json"
            run = subprocess.run([sys.executable, str(HERE / "scene.py"), "9", "--lens", "roots 2",
                                  "--primes", "101", "--driver", str(Path(tmp) / "no-driver"),
                                  "--output", str(out)], capture_output=True, text=True, timeout=60)
            self.assertEqual(run.returncode, 2)
            self.assertEqual(run.stdout, "")
            lines = run.stderr.splitlines()
            self.assertEqual(len(lines), 1, run.stderr)
            self.assertRegex(lines[0], r"\b101\b")
            self.assertRegex(lines[0], r"\b13\b")
            self.assertFalse(out.exists())
        with self.assertRaises(S.Refusal):
            S.build_scene("9", "roots 2", [2, 17], 1, 5, Path("/nonexistent/adf"))
        self.assertEqual(S.MAX_PRIME, 13)

    def test_item1_limits_and_rows_agree_with_atlas_js(self):
        cases = [([[0, 0], [1, 0]], 13), ([[0, 0], [1, 0], [2, 0]], 13), ([[0] * 8], 7), ([[1, 0, 1]], 2)]
        script = ("const a = require(process.argv[1]); const cases = JSON.parse(process.argv[2]);"
                  "console.log(JSON.stringify({limits: [a.MAX_PRIME, a.rowCapacity()], rows: cases.map("
                  "([paths, p]) => { const n = a.chooseNodes(paths, p); const r = [];"
                  "n.forEach(x => { r[x.depth] = (r[x.depth] || 0) + 1; }); return r; })}))")
        run = subprocess.run(["timeout", "30", "node", "-e", script, str(HERE / "atlas.js"), json.dumps(cases)],
                             capture_output=True, text=True, timeout=40, check=True)
        result = json.loads(run.stdout)
        self.assertEqual(result["limits"], [S.MAX_PRIME, S.ROW_CAPACITY])
        self.assertEqual(result["rows"], [S.row_sizes(paths, p) for paths, p in cases])
        self.assertEqual(S.row_sizes([[0, 0], [1, 0], [2, 0]], 13), [1, 13, 39])
        S.check_rows([[0, 0], [1, 0]], 13)
        S.check_rows([[a, b, 0] for a in range(3) for b in range(3)], 3)  # a row of exactly 27
        with self.assertRaisesRegex(S.Refusal, r"39.*27"):
            S.check_rows([[0, 0], [1, 0], [2, 0]], 13)

    def test_item2_polynomial_text(self):
        self.assertEqual(S.polynomial_text("0 -1 1"), "X^2 - X")
        self.assertEqual(S.polynomial_text("-9 0 1"), "X^2 - 9")
        self.assertEqual(S.polynomial_text("-1 0 2"), "2X^2 - 1")
        self.assertEqual(S.polynomial_text("1 0 0 -1"), "-X^3 + 1")
        self.assertEqual(S.polynomial_text("0 3"), "3X")
        self.assertEqual(S.polynomial_text("5"), "5")

    def test_item2_fixtures_carry_the_solved_polynomial(self):
        sqrt9, idem = fixtures()
        self.assertEqual(sqrt9["solved_polynomial"], "X^2 - 9")
        self.assertEqual(idem["solved_polynomial"], "X^2 - X")
        for scene in (sqrt9, idem):
            commands = [r["command"] for r in scene["provenance"]["records"]]
            self.assertIn("realroots " + scene["polynomial"], commands)
            self.assertTrue(any(c.startswith(f"roots {scene['polynomial']} with ") for c in commands))

    def test_item3_missing_real_coordinate_has_its_reason(self):
        self.assertTrue(DRIVER.is_file(), f"driver missing: {DRIVER}; build it or set ADF")
        scene = S.build_scene("(* ; 5 mod 25)", "exp", [2, 3, 5], 1, 5, DRIVER)
        ruler, records = scene["ruler"], scene["provenance"]["records"]
        self.assertEqual((ruler["status"], ruler["status_class"]), ("DOMAIN", "proved no"))
        self.assertEqual(ruler["failing_place"], "not given")
        self.assertEqual(ruler["reason"], "no real coordinate in the input")
        show, project = (records[i] for i in ruler["reason_records"])
        self.assertEqual(show, {"command": "show (* ; 5 mod 25)", "output": "(* ; 5 mod 25)"})
        self.assertEqual(project, {"command": "project (* ; 5 mod 25) with real", "output": "error: DOMAIN"})
        self.assertEqual(records[ruler["record"]]["output"], "error: DOMAIN")

    def test_item3_a_real_domain_error_keeps_no_such_reason(self):
        self.assertTrue(DRIVER.is_file(), f"driver missing: {DRIVER}; build it or set ADF")
        scene = S.build_scene("-1", "log", [2, 3], 1, 2, DRIVER)
        self.assertEqual(scene["ruler"]["status"], "DOMAIN")
        self.assertNotIn("reason", scene["ruler"])
        for scene in fixtures():
            self.assertNotIn("reason", scene["ruler"])

    def test_item5_derived_numbers_name_their_driver_lines(self):
        for scene in fixtures():
            derived, records = scene["derived"], scene["provenance"]["records"]
            self.assertEqual(set(derived), {"rule", "digits", "node_balls", "counts", "fog", "modular_digits"})
            self.assertIn("D-UX5", derived["rule"])
            for key in ("digits", "node_balls", "counts", "fog", "modular_digits"):
                self.assertIn("provenance.records[", derived[key])
            source = records[scene["fog"]["source_record"]]
            self.assertEqual(source["output"], scene["value"])
            self.assertTrue(source["command"].startswith("show "))
            for tree in scene["trees"]:
                for result in tree["depths"].values():
                    for path in result["paths"]:
                        self.assertIn(path["printed"], records[path["source_record"]]["output"])
            for solution in scene["modular_solutions"]:
                self.assertTrue(records[solution["records"][3]]["command"].startswith("project "))

    def test_fixtures_regenerate_identically(self):
        self.assertTrue(DRIVER.is_file(), f"driver missing: {DRIVER}; build it or set ADF")
        with tempfile.TemporaryDirectory() as tmp:
            subprocess.run(["timeout", "170", "sh", str(HERE / "build_scenes.sh"), tmp], check=True,
                           env=dict(os.environ, ADF=str(DRIVER)), capture_output=True, timeout=175)
            for name in ("sqrt9", "idempotents30"):
                self.assertEqual((Path(tmp) / (name + ".json")).read_text(),
                                 (HERE / "scenes" / (name + ".json")).read_text(), name)

    def test_items_1_and_5_readme_states_limit_and_derivation(self):
        text = (HERE / "README.md").read_text()
        lines = text.splitlines()
        self.assertLess(len(lines), 80)
        self.assertLessEqual(max(map(len, lines)), 116)
        self.assertRegex(text, r"primes up to 13")
        self.assertIn("D-UX5", text)
        self.assertIn("source_record", text)


if __name__ == "__main__":
    unittest.main()
