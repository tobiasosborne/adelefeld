"""Three planted faults. Scratch sources only; no mutation tool."""
import os
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[2]
lane = root / "lanes/r-slice2"
source = (root / "src/roots_real.c").read_text()
tests = (root / "tests/test_roots_real_isolate.c").read_text()
tests = '#define ADF_TEST_NO_MAIN\n' + tests + r"""
int main(void) {
    for (adf_test *t = adf_test_head; t; t = t->next) {
        if (strcmp(t->name, "review_cost_inputs") && strcmp(t->name, "count_guided_regions")) continue;
        adf_test_current = t->name; t->fn();
    }
    printf("planted checks %lu failures %lu\n", adf_test_checks, adf_test_failures);
    return adf_test_failures ? 1 : 0;
}
"""
(lane / "bite_tests.c").write_text(tests)
faults = {
    "stop_early": ("it->n < count", "it->n < count - 1"),
    "unchecked_fast": ("ok = var01(outside, tmp) == 0;", "ok = 1;"),
    "zero_count_one": ("if (d < 1 || count == 0)", "if (d < 1 || count <= 1)"),
}
for name, (old, new) in faults.items():
    assert old in source
    scratch = lane / ("fault_" + name + ".c")
    scratch.write_text(source.replace(old, new))
    exe = lane / ("fault_" + name)
    cmd = ["timeout", "60", "cc", "-std=c11", "-O2", "-Iinclude", "-Itests",
           str(lane / "bite_tests.c"), str(scratch), str(lane / "build/libadelefeld.a"),
           "-lflint", "-lgmp", "-lm", "-o", str(exe)]
    subprocess.run(cmd, cwd=root, check=True)
    result = subprocess.run(["timeout", "60", str(exe)], cwd=root, capture_output=True, text=True)
    (lane / "runs" / ("bite_" + name + ".log")).write_text(result.stdout + result.stderr)
    print(name, "exit", result.returncode, result.stdout[-150:].strip(), flush=True)
    assert result.returncode == 1, (name, result.returncode)
print("bite: 3 of 3 planted faults caught")
