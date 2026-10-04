"""Replay only the 13 original survivors with strengthened debug diagnostics, not another sweep."""
from pathlib import Path
import random
import runpy
import shutil
import subprocess

root = Path(__file__).resolve().parents[2]
api = runpy.run_path(str(root/'tools/mutate/mutate.py'))
stage = root/'lanes/f-slice13/build/mutroot'
mutants = api['mutants_of']('src/catalogue.c', (stage/'src/catalogue.c').read_text(), str(stage))
random.Random(1).shuffle(mutants)
selected = mutants[:60]
log = (root/'lanes/f-slice13/mutate.log').read_text()
survivors = {line[len('SURVIVED '):] for line in log.splitlines() if line.startswith('SURVIVED ')}
keep = next((root/'lanes/f-slice13/build/mutate').glob('run-*/keep'))
results = []
for i, mutant in enumerate(selected):
    if str(mutant) not in survivors:
        continue
    work = keep/f'{i:05d}'
    shutil.copy2(root/'tests/test_catalogue.c', work/'tests/test_catalogue.c')
    proc = subprocess.run(['timeout', '90', 'make', '-s', '-j2', 'check-catalogue', 'INV=1'],
                          cwd=work, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    target = root/'lanes/f-slice13'/f'survivor-{i:02d}.log'
    target.write_text(proc.stdout)
    first = next((line for line in proc.stdout.splitlines() if line.startswith('FAIL')), 'no failure')
    summary = next((line for line in reversed(proc.stdout.splitlines()) if 'failed checks' in line), '')
    line = f'{i:02d}: exit={proc.returncode}; {mutant}; {summary}; {first}'
    results.append(line)
    print(line, flush=True)
assert len(results) == len(survivors)
(root/'lanes/f-slice13/survivor-replay.log').write_text('\n'.join(results)+'\n')
