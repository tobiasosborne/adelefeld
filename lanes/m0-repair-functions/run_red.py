"""Run the mutation harness of proto/functions_checks.py against the pre-strengthening snapshot.

Produces the red half of the red-green record in lanes/m0-repair-functions/report.md. The green half is
check_mutation_testing in the tail of `python3 -B proto/functions_checks.py` (current file, all killed).

Run from the repository root:  python3 -B lanes/m0-repair-functions/run_red.py
"""

import importlib.util

spec = importlib.util.spec_from_file_location("fc_current", "proto/functions_checks.py")
fc = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fc)
target = "lanes/m0-repair-functions/functions_checks_pre.py"
lines, killed, survivors, skipped = fc.run_mutation_tests(target)
for line in lines:
    print(line)
print(f"red target: {target}")
print(f"planted_wrong_rules={len(fc.MUTANTS)} rejected={killed} survived={len(survivors)} {survivors}")
print(f"skipped={len(skipped)} {skipped}")
