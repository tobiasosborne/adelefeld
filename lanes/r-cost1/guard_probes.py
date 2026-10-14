"""Run two sampled survivors against focused arithmetic checks, within the mutation window."""
import subprocess
from pathlib import Path

lane = Path("lanes/r-cost1")
source = Path("src/roots_real.c").read_text()
changes = [("bound", "r = fmpz_cmp(lhs, rhs) > 0;", "r = fmpz_cmp(lhs, rhs) > 1;"),
           ("overflow", "if (x != 0 && y > WORD_MAX / x)\n        return 0;",
            "if (x != 0 && y > WORD_MAX / x)\n        return 1;"),
           ("bound_status", "if (!mul_checked_nonneg(&s, a, n))\n        return -1;",
            "if (!mul_checked_nonneg(&s, a, n))\n        return 0;")]
for name, old, new in changes:
    assert source.count(old) == 1
    (lane / "roots_guard_probe.c").write_text(source.replace(old, new))
    command = ["timeout", "170", "taskset", "-c", "2,3", "cc", "-std=c11", "-O2", "-Iinclude",
               str(lane / "guard_probe.c"), "-lflint", "-lgmp", "-lm", "-o",
               str(lane / "build-mutcache/guard-probe")]
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stderr)
    result = subprocess.run(["timeout", "170", "taskset", "-c", "2,3",
                             str(lane / "build-mutcache/guard-probe")], capture_output=True, text=True)
    (lane / f"guard-{name}-red.log").write_text(result.stdout + result.stderr)
    print(name, "exit", result.returncode, result.stdout.strip(), flush=True)
    assert result.returncode == 1
