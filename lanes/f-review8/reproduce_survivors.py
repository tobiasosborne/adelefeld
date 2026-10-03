#!/usr/bin/env python3
from pathlib import Path
import subprocess
import json
P=Path('lanes/f-review8')
LIB=P/'build/libadelefeld.a'
FAULTS=P/'build/faults'
cases=[('R_drop_A_B_65537','lpow','W 65537 1 1 0 1 0 0 1 0 0 0 2'),
       ('powrat_wrong_centre_65537','lpow','P 65537 1 1 0 2 0 1 2 1 2'),
       ('root_wrong_unsampled_lift','lroot','S 65537 1 1 0 0 1 65536 42 2')]
for name,unit,line in cases:
    obj=FAULTS/name/f'{unit}.o'
    if not obj.exists(): continue
    binary=FAULTS/name/'h'
    subprocess.run(['timeout','60','cc','-std=gnu11','-O2','-Iinclude',str(P/'h.c'),str(obj),
                    str(LIB),'-lflint','-lgmp','-lm','-o',str(binary)],check=True)
    results=[]
    for executable in (P/'build/h',binary):
        r=subprocess.run(['timeout','30',str(executable)],input=line+'\n',text=True,
                         capture_output=True,check=True)
        results.append(r.stdout.strip())
    row={'fault':name,'input':line,'original':results[0],'mutant':results[1]}
    if name=='R_drop_A_B_65537':
        row['witness']='base 65538, exponent 1, output 65538, outside 1+65537^2 Z_65537'
    elif name=='powrat_wrong_centre_65537':
        row['witness']='input 1, root 1, output 1, outside 65538+65537^2 Z_65537'
    else:
        m=65537**2
        q=int(results[1].split()[-1])
        row['modular_certificate']={'modulus':m,'root_power':pow(q,65536,m),'expected':1}
        assert pow(q,65536,m)!=1
    print(json.dumps(row),flush=True)
