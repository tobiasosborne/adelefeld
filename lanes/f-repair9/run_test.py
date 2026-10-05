"""Run one configuration with a 30-second child timeout and retain exact counts and elapsed time."""
import json
import os
import re
import subprocess
import sys
import time
from pathlib import Path

lane = Path(__file__).resolve().parent
root = lane.parents[1]
config = sys.argv[1]
build = lane / ('build' if config == 'plain' else 'build-'+config)
env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
cmd = ['taskset', '-c', '0,1', 'timeout', '30', str(build / 'test_localfactor')]
start = time.monotonic()
r = subprocess.run(cmd, cwd=root, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
seconds = time.monotonic()-start
(lane / f'check-{config}.log').write_text(r.stdout)
summary = re.findall(r'^(\d+) tests, (\d+) checks, (\d+) failed checks, (\d+) failed tests$', r.stdout, re.M)
record = dict(command=cmd, exit=r.returncode, seconds=round(seconds, 6), summary=summary)
(lane / f'check-{config}.json').write_text(json.dumps(record, indent=2)+'\n')
print(json.dumps(record))
raise SystemExit(r.returncode)
