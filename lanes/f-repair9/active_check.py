"""Verify that all 6730 family calls actually execute the derivative refinement."""
import json
import subprocess
from pathlib import Path

lane = Path(__file__).resolve().parent
root = lane.parents[1]
source = (root / 'src/localfactor.c').read_text()
needle = 'mag_mul(B, sum, f);'
assert source.count(needle) == 1
source = source.replace(needle, needle+' mag_set(review_B, B); review_has_B = 1;')
(lane / 'build/localfactor_trace.c').write_text(source)
cmd = ['taskset', '-c', '0,1', 'timeout', '30', 'cc', '-std=c11', '-O2', '-g', '-Iinclude', '-Isrc',
       '-DREVIEW_TRACE', str(lane / 'bridge.c'), str(lane / 'build/libadelefeld.a'),
       '-lflint', '-lgmp', '-lm', '-o', str(lane / 'build/bridge-active')]
subprocess.run(cmd, cwd=root, check=True)
records = [json.loads(s) for s in (root / 'tests/ref/vectors/f-repair9/near-poles.jsonl').read_text().splitlines()]
inputs = []
for row in records:
    if 's' not in row:
        continue
    x, xe, y, ye, rx, ry = row['s']
    for prec in (16, 32, 64, 128, 256):
        inputs.append(f'0 {prec} {x} {xe} {y} {ye} 1 {rx} {0 if ry is None else 1} {ry or 0}\n')
run = subprocess.run(['taskset', '-c', '0,1', 'timeout', '25', str(lane / 'build/bridge-active')],
                     cwd=root, input=''.join(inputs), capture_output=True, text=True, check=True)
outputs = [json.loads(s) for s in run.stdout.splitlines()]
assert len(outputs) == 6730
assert all(s['status'] == 0 and s['B'] is not None and s['repr'] == s['where'] == 1 for s in outputs)
print('refinement active: 6730 calls, 6730 finite B traces, 6730 OK, 0 failures')
