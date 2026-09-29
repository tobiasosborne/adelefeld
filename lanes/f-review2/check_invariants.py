import resource
import subprocess

resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
missed = 0
for method in ("place", "exact", "zero", "prec", "set", "swap", "identical", "rat", "ratball",
               "add_control"):
    r = subprocess.run(["timeout", "5", "lanes/f-review2/invariant_probe", method],
                       capture_output=True, text=True, timeout=7)
    print(f"method={method} exit={r.returncode} stdout={r.stdout.strip()} stderr={r.stderr.strip()}")
    if method != "add_control":
        missed += r.returncode == 0
    else:
        assert r.returncode in (-6, 134), r
print(f"invalid_entry_calls=9 missed_checks={missed} aborting_controls=1")
raise SystemExit(1 if missed else 0)
