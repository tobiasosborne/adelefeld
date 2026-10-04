import json
import subprocess
from oracle import finite_pole_cases, LANE

lines = []
for c in finite_pole_cases():
    k = c['pole_index']
    fields = [str(c['p']), k, str(int(c['geometry'] == 'pole'))]
    for d in c['s']:
        fields += d
    lines.append(' '.join(fields))
(LANE / 'pole-geometry.inputs').write_text('\n'.join(lines) + '\n')
r = subprocess.run(['timeout', '30', str(LANE / 'build/pole-geometry')],
                   input='\n'.join(lines) + '\n', stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
(LANE / 'pole-geometry.log').write_text(r.stdout + r.stderr)
print(json.dumps({'exit': r.returncode, 'output': r.stdout.strip()}))
raise SystemExit(r.returncode)
