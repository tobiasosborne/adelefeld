import json,pathlib,re
lane=pathlib.Path('lanes/ux-drive-bench')
r=[json.loads(s) for s in (lane/'out/finite_ball.records.jsonl').read_text().splitlines()]
assert r[0]['class']=='na' and 'output' not in r[0]
assert [x['row'] for x in r if x['status']=='NOT_DETERMINED']==[2,3,7,11]
assert r[3]['output']=='5 [2]: 15622 + O(5^6); 5 [3]: 3 + O(5^6)'
assert all((x*x-9)%15625==0 for x in [3,15622])
e=[json.loads(s) for s in (lane/'out/exp_edge.records.jsonl').read_text().splitlines()]
assert len([x for x in e if 'output' in x])==40
assert len([x for x in e if x['class']=='below'])==15
html=(lane/'out/exp_edge.html').read_text()
assert all(f'data-p="{p}"' not in html for p in [19,23,29])
a=(lane/'out/attack.html').read_text()
assert 'contradict this finding at 3, 5, 11, 13, 19, 29, 37, 43.' in a
print('finite ball: 1 omitted real cell, 4 open cells, 2 roots verified modulo 15625')
print('budget: 40 answers, 15 omitted cells at 19, 23, 29; 0 drawn below stop line')
print('attack: 8 correct counterexamples; badge remains pattern: seen in the swept rows, not proved')
assert '<span class="badge st-pattern">pattern: seen in the swept rows, not proved</span>' in a
