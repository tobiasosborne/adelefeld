import os
from pathlib import Path
import subprocess
import sys
import time

lane = Path(__file__).resolve().parent
name, expected, command = sys.argv[1:4]
cwd = Path(sys.argv[4]) if len(sys.argv) > 4 else lane / 'work'
stamp = subprocess.check_output(['timeout', '5', 'date', '-Iseconds'], text=True).strip()
env = os.environ.copy()
env.update(OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', JULIA_NUM_THREADS='1',
           JULIA_NUM_PRECOMPILE_TASKS='1', MAKEFLAGS='-j2')
cores = sorted(os.sched_getaffinity(0))[:2]
start = time.monotonic()
result = subprocess.run(['timeout', '180', 'taskset', '-c', ','.join(map(str, cores)),
                         'bash', '--noprofile', '--norc', '-c', command],
                        cwd=cwd, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
elapsed = time.monotonic() - start
path = lane / (name + '.txt')
path.write_text(result.stdout)
with (lane / 'drive.md').open('a') as log:
    log.write(f'### {name}: {stamp}\n\nExpected: {expected}\n\n')
    log.write(f'Cwd: `{cwd.relative_to(lane) if cwd.is_relative_to(lane) else cwd}`.\n\n')
    log.write('Command (outer timeout 180; affinity 2 cores):\n\n```sh\n' + command + '\n```\n\n')
    log.write(f'Result: exit {result.returncode}; {elapsed:.3f} seconds; output `{path.name}`.\n\n')
print(f'{name}: exit {result.returncode}, {elapsed:.3f} seconds, {len(result.stdout.splitlines())} output lines')
print(result.stdout[-18000:])
