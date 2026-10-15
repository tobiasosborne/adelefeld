"""Run from the repository root: timeout 120 python3 -m unittest discover -s ux/atlas."""
import unittest
from fractions import Fraction
import json
from pathlib import Path

from scene import digit_path, fog_count, parse_output, parse_real, parse_value, STATUS_CLASSES


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


if __name__ == "__main__":
    unittest.main()
