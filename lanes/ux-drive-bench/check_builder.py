"""Repeat the builder's mutation and metadata checks without modifying its files."""
import hashlib
import importlib
import io
import json
import pathlib
import subprocess
import sys
import types
import unittest

lane=pathlib.Path('lanes/ux-drive-bench')
sys.path.insert(0,str(pathlib.Path('ux/bench').resolve()))
tests=importlib.import_module('test_bench')
name='test_count_min_is_inclusive'
baseline=unittest.TextTestRunner(stream=sys.stdout).run(unittest.TestSuite([tests.RenderTest(name)]))
assert baseline.wasSuccessful()
path=pathlib.Path('ux/bench/render.py')
original=path.read_text()
needle="r['count'] >= cond['count_min']"
assert original.count(needle)==1
changed=original.replace(needle,"r['count'] > cond['count_min']")
(lane/'render_count_min_mutant.py').write_text(changed)
mod=types.ModuleType('render_count_min_mutant')
mod.__file__=str(path.resolve())
exec(compile(changed,str(lane/'render_count_min_mutant.py'),'exec'),mod.__dict__)
tests.render=mod
result=unittest.TextTestRunner(stream=sys.stdout).run(unittest.TestSuite([tests.RenderTest(name)]))
assert len(result.failures)==1 and len(result.errors)==0
assert path.read_text()==original
print('mutation: 1 baseline pass; 1 count_min mutant killed by assertion; original bytes unchanged')
meta=json.loads((lane/'out/brief_a.run.json').read_text())
assert meta['driver_sha256']==hashlib.sha256(pathlib.Path('build/adf').read_bytes()).hexdigest()
assert meta['answered']==112 and len(meta['cell_times'])==112
print('metadata: driver SHA256 matches; 112 cell times and 112 answers')
timeout_meta=json.loads((lane/'out/timeout_real.run.json').read_text())
assert timeout_meta['timed_out'] and timeout_meta['answered']==2
html=(lane/'out/timeout_real.html').read_text()
assert 'p &gt;= 7: not swept' in html and 'Not swept either: 2' in html
assert not any(f'data-p="{p}"' in html for p in [7,17])
processes=subprocess.check_output(['timeout','5','ps','-eo','pid,args'],text=True)
remaining=[s for s in processes.splitlines() if len(s.split(None,1))==2
           and s.split(None,1)[1].split()[0].endswith('/build/adf')]
assert remaining==[],remaining
print('timeout: 2 answers, 2 omitted cells, skipped prime 2 disclosed, 0 remaining build/adf processes')
