#!/usr/bin/env python3
from pathlib import Path
import sys
import hashlib
import json
import re
P=Path('lanes/f-review8')
name='report-preview.md' if len(sys.argv)>1 else 'report.md'
report=(P/name).read_text()
assert max(map(len,report.splitlines()))<=116
assert len(re.findall(r'^\| pow(?:rat|unit) \|',report,re.M))==36
assert len(re.findall(r'^\| (?:E_|R_|hull_|powunit_K|root_K|zeta_|CRT_|early_|seed_|powrat_wrong|root_wrong)',
                      report,re.M))==16
assert all(f'P{i}. Claim:' in report for i in range(1,9)) and 'R8. Claim:' in report
for required in ('Findings against the specification','Files written','Not done','Sources pending'):
    assert required in report
for line in (P/'source-sha256.txt').read_text().splitlines():
    digest,name0=line.split()
    assert hashlib.sha256(Path(name0).read_bytes()).hexdigest()==digest
assert (P/'build/faults/baseline/lpow.c').read_text()==Path('src/lpow.c').read_text()
rows=[json.loads(s) for s in (P/'build/faults/results.jsonl').read_text().splitlines()]
assert len(rows)==54
latest={(r['fault'],r['test']):r for r in rows}
assert len(latest)==51 and not any(r['exit']==124 for r in rows)
assert sum(all(latest[name,t]['exit']==0 for t in ('test_lpow','test_lroot','test_rfunc_prime'))
           for name in {r['fault'] for r in rows if r['fault']!='baseline'})==4
cost=json.loads((P/'cost-data.json').read_text())
assert len(cost)==36 and sum(r['trials'] for r in cost)==94
assert sum(r['method']=='inside call' for r in cost)==6
assert all(r['total'][1]/r['components'][1]<=1.5 for r in cost)
# Exact residue certificates for the working-precision example.
lines=(P/'working-precision.log').read_text().splitlines()
y=int(lines[0].split()[-1]); root=int(lines[1].split()[-1]); mod=5**20
assert pow(root,3,mod)==2 and pow(root,2,mod)==y
# Exact residue witnesses for each value-changing survivor.
survivors=[json.loads(s) for s in (P/'survivors.log').read_text().splitlines()]
assert len(survivors)==3
p=65537; mod=p*p
for row in survivors:
    assert row['original']!=row['mutant']
assert (65538-1)%mod!=0 and (1-65538)%mod!=0
assert pow(3467431638,65536,mod)==1 and pow(3467497175,65536,mod)==511319675
assert (65579-42)%mod!=0
if len(sys.argv)==1:
    for path in (P/'files-manifest.txt').read_text().splitlines():
        assert Path(path).is_file(),path
print('final_validation failures=0 report_cost_rows=36 fault_rows=16 timings=94 '
      'source_hashes=4 residue_precision=5^20,65537^2')
