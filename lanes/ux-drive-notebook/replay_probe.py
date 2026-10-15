from copy import deepcopy
from pathlib import Path
import json
import subprocess

lane = Path(__file__).resolve().parent
root = lane/'work'
original = [json.loads(x) for x in (root/'ux/notebook/out/b_which_rational.records.jsonl').read_text().splitlines()]
cases = {'unchanged': lambda r: None,
         'wrong_value': lambda r: r[1].update(value='23/7'),
         'wrong_status': lambda r: r[1].update(status='NO_SOLUTION'),
         'wrong_dump': lambda r: r[1].update(dump='adf1 Q rat 17 7'),
         'wrong_digit': lambda r: r[1]['places'][0].update(digits='(0)0'),
         'wrong_input': lambda r: r[1].update(input='recover(c; num = 0, den = 1)')}
for name, change in cases.items():
    records = deepcopy(original); change(records)
    p = lane/('replay_'+name+'.jsonl')
    p.write_text(''.join(json.dumps(r, separators=(',',':'))+'\n' for r in records))
    result = subprocess.run(['timeout','30','sh','ux/notebook/replay.sh',str(p)], cwd=root,
                            text=True, capture_output=True)
    print(name, 'exit', result.returncode)
    print(result.stdout.strip())
    if result.stderr: print(result.stderr.strip())
