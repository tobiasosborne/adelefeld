"""Sensitivity check of this lane's assertions. Mutations stay inside the lane."""
from pathlib import Path
import subprocess

lane=Path(__file__).resolve().parent
root=lane.parents[1]
source=(root/'src/lfunc.c').read_text()
faults=[
    ('exp_degree', 'slong L = count_exp(p, K, w) - 1, D, W, k;',
     'slong L = count_exp(p, K, w) - 2, D, W, k;', 'precision'),
    ('log_degree', 'T = count_log(p, K, vz);', 'T = count_log(p, K, vz) - 1;', 'precision'),
    ('radius_at_two', 'E = r >= c ? r : 2;', 'E = r;', 'small'),
    ('powered_division', 'fmpz_mul(S, S, t);', 'fmpz_add_ui(S, S, 0);', 'big'),
]
for i,(name,old,new,mode) in enumerate(faults,1):
    expected_occurrences=2 if name=='log_degree' else 1
    assert source.count(old)==expected_occurrences
    mutant=lane/f'fault-{i}.c'
    mutant.write_text(source.replace(old,new))
    binary=f'build/probe-fault-{i}'
    compile_=subprocess.run(['timeout','60','cc','-std=c11','-O2','-g','-Wall','-Wextra','-Werror',
        '-Iinclude','-Isrc',str(mutant),str(lane/'probe.c'),str(lane/'build/libadelefeld.a'),
        '-lflint','-lgmp','-lm','-o',str(lane/binary)],cwd=root,capture_output=True,text=True)
    assert compile_.returncode==0,compile_.stderr
    proc=subprocess.run(['timeout','30','python3','-B',str(lane/'oracle.py'),mode,binary],
                        cwd=root,capture_output=True,text=True)
    (lane/f'fault-{i}.log').write_text(proc.stdout+proc.stderr)
    assert proc.returncode not in (0,124), (name,proc.returncode)
    assert 'AssertionError' in proc.stderr
    print(f'{i} {name} suite={mode} exit={proc.returncode} caught=1',flush=True)
