import json,pathlib,re
lane=pathlib.Path('lanes/ux-drive-bench')
page=(lane/'out/brief_a.html').read_text()
paths=re.findall(r'(?:ux/bench/out|ux/bench/experiments)/brief_a\.[\w.]+',page)
paths=[v.rstrip('.') for v in paths]
assert len(paths)==2 and all(not pathlib.Path(p).exists() for p in paths)
print('D1: custom-output footer lists 2 nonexistent paths:',paths)
a=(lane/'out/attack.html').read_text()
assert 'contradict this finding at 3, 5, 11, 13, 19, 29, 37, 43.' in a
assert 'st-pattern">pattern: seen in the swept rows, not proved' in a
print('D2: 8 counterexamples coexist with the badge pattern: seen in the swept rows, not proved')
meta=json.loads((lane/'out/timeout_real.run.json').read_text())
run=[json.loads(s) for s in (lane/'checks.jsonl').read_text().splitlines()]
time=next(r for r in run if r['id']=='builder_timeout')['seconds']
assert time>=4 and meta['process_time_s']<.1 and meta['timed_out']
print('D3: measured wrapper seconds',time,'recorded process_time_s',meta['process_time_s'])
print('0 returned mathematical numbers or statuses found wrong; 3 display/metadata defects reproduced')
