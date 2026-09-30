"""Summarize saved logs; never substitutes an unfinished call by an invented measurement."""
import math
import re
from pathlib import Path
lane = Path('lanes/f-slice5')

def stats(path):
    s = path.read_text()
    def triplet(key):
        m = re.search(key+r'=([\d.]+),([\d.]+),([\d.]+)', s)
        return tuple(map(float,m.groups())) if m else None
    done = re.findall(r'trial=\d+ seconds=([\d.]+) status=0',s)
    return dict(time=triplet('seconds_min_med_max'), mul=triplet('mul_mod_min_med_max'),
                flint=triplet('flint_min_med_max'), done=list(map(float,done)), text=s)

header = '| p | N | f | before s | after s | FLINT s | mul mod s | after / FLINT | after / mul |'
lines = [header, '|---|---:|---|---:|---:|---:|---:|---:|---:|']
for p in (2,3,5,'big'):
    for n in ((10000,) if p=='big' else (2000,10000,100000)):
        for f in ('exp','log','Log'):
            a=stats(lane/f'after-{f}-{p}-{n}.log')
            b=stats(lane/f'before-{f}-{p}-{n}.log')
            assert a['time'] and a['mul'] and a['flint']
            old=f"{b['time'][1]:.6f}" if b['time'] else (
                f"{b['done'][0]:.6f} (partial run)" if b['done'] else ('>120' if p=='big' else '>180'))
            t,m,r=a['time'][1],a['mul'][1],a['flint'][1]
            lines.append(f'| {p} | {n} | {f} | {old} | {t:.6f} | {r:.6f} | {m:.6f} | '
                         f'{t/r:.3f} | {t/m:.1f} |')
(lane/'table.md').write_text('\n'.join(lines)+'\n')
# A MODEL for the actual dense integer centre, in the existing fmpz layout.
# The same before/after clock probes calibrate only the after run, conditional on 1-cycle add.
probes=[]
for name in ('clock-before.log','clock-after.log'):
    s=(lane/name).read_text()
    m=re.search(r'min ([\d.]+) median ([\d.]+) max ([\d.]+)',s)
    probes.extend(map(float,m.groups()))
floors=['| f | centre bytes | store cycles | MODEL store floor us | after / store floor |',
        '|---|---:|---:|---:|---:|']
for f in ('exp','log','Log'):
    a=stats(lane/f'after-{f}-big-10000.log')
    bits=int(re.search(r'centre_bits=(\d+)',a['text'])[1]); size=(bits+7)//8
    cycles=size/64
    lo=cycles/max(probes); hi=cycles/min(probes) # ns
    t=a['time'][1]*1e9
    floors.append(f'| {f} | {size} | {cycles:.3f} | {lo/1000:.3f} to {hi/1000:.3f} | '
                  f'{t/hi:.0f} to {t/lo:.0f} |')
(lane/'floors.md').write_text('\n'.join(floors)+'\n')
print('rows=30 floor_rows=3 clock_adds_per_ns=',min(probes),max(probes))
