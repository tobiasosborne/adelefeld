"""Bound each child to 20 CPU seconds and 256 MiB; never demand laptop exhaustion."""
import json
import resource
import subprocess
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent


def limits():
    resource.setrlimit(resource.RLIMIT_AS, (256 * 1024**2, 256 * 1024**2))
    resource.setrlimit(resource.RLIMIT_CPU, (20, 20))
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))


def run(args):
    start = time.monotonic()
    p = subprocess.Popen([str(HERE / 'probe'), *args], stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE, preexec_fn=limits)
    # wait4 gives per-child peak memory. Output of these probes is at most a few hundred bytes.
    _, status, usage = __import__('os').wait4(p.pid, 0)
    p.returncode = __import__('os').waitstatus_to_exitcode(status)
    out, err = p.communicate()
    print(json.dumps(dict(args=args, returncode=p.returncode, seconds=round(time.monotonic()-start, 3),
                          peak_kib=usage.ru_maxrss, stdout=out.decode(), stderr=err.decode())))


for e in [100000, 1000000, 10000000, 100000000, 1073741824, -100000, -1000000,
          -10000000, -100000000, -1073741824, 18446744073709551616]:
    run(['print', str(e)])
for k in range(4):
    run(['predicate', str(k)])
run(['roundtrip'])
run(['nested'])
