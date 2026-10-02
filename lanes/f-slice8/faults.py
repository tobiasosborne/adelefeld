#!/usr/bin/env python3
"""Five deliberate faults, only in scratch copies. No mutation tool or fuzz target."""
from pathlib import Path
import subprocess

base = Path('lanes/f-slice8/build')
work = base/'faults'
work.mkdir(exist_ok=True)
source = Path('src/lroot.c').read_text()
faults = [
    ('guard-one-low', 'x->N-x->v<c+*s', 'x->N-x->v<c+*s-1'),
    ('omit-valuation-term', 'j+(x->N-x->v)-s', 'x->N-s'),
    ('missing-branch', 'tmp=flint_malloc((size_t)d*sizeof(adf_lball_struct));',
     'if (d>1) d--;\n    tmp=flint_malloc((size_t)d*sizeof(adf_lball_struct));'),
    ('square-mod-four', 'st=adf_lball_Log(l,u,c+*s);',
     'st=adf_lball_Log(l,u,x->p==2 && n==2 ? 2 : c+*s);'),
    ('omit-log-division', 'st=adf_lball_div(l,l,d);', 'st=ADF_OK;'),
]
for name, before, after in faults:
    assert source.count(before) == 1, name
    cfile = work/(name+'.c')
    binary = work/name
    cfile.write_text(source.replace(before, after))
    command = ['timeout', '60', 'cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
               '-Iinclude', '-Isrc', '-Itests', str(cfile), 'tests/test_lroot.c',
               str(base/'support/jsonl.o'), str(base/'libadelefeld.a'),
               '-lflint', '-lgmp', '-lm', '-o', str(binary)]
    with (work/(name+'-build.log')).open('w') as log:
        built = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=False)
    assert built.returncode == 0, name
    with (work/(name+'.log')).open('w') as log:
        result = subprocess.run(['timeout', '60', str(binary)], stdout=log,
                                stderr=subprocess.STDOUT, check=False)
    lines = (work/(name+'.log')).read_text().splitlines()
    assert result.returncode == 1 and any(line.startswith('FAIL ') for line in lines), (name, result.returncode)
    print(f'{name}: exit {result.returncode}; {lines[-1]}', flush=True)
print('5 faults, 5 caught by assertions, 0 timeouts')
