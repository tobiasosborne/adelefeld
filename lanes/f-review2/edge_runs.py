import resource
import subprocess


def bounded():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    resource.setrlimit(resource.RLIMIT_AS, (1024**3, 1024**3))


for mode, binary, cases in (
        ("inv", "edge-inv", ("arch", "num", "ident", "setself", "swap", "swapself", "control")),
        ("prec", "edge", ("exp", "sin", "root", "log", "sqrt"))):
    for case in cases:
        cmd = ["timeout", "10", "lanes/f-review2/" + binary, mode, case]
        r = subprocess.run(cmd, capture_output=True, text=True, preexec_fn=bounded, timeout=12)
        print("COMMAND", " ".join(cmd), "exit", r.returncode, flush=True)
        print(r.stdout + r.stderr, end="", flush=True)
