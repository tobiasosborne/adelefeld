"""Run the mutation harness as part of the suite: every mutant must be killed."""
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import mutants


class MutationHarness(unittest.TestCase):
    def test_all_mutants_killed(self):
        import io
        import contextlib

        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            code = mutants.main()
        report = buf.getvalue()
        self.assertEqual(code, 0, report)
        self.assertIn("mutants survived: 0", report)


if __name__ == "__main__":
    unittest.main()
