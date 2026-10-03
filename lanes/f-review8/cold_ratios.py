from pathlib import Path
import re
P=Path('lanes/f-review8')
for log in sorted(P.glob('cost-pow*.log')):
    for m in re.finditer(r'trial=(\d+) total=([\d.]+) components=([\d.]+)',log.read_text()):
        ratio=float(m[2])/float(m[3])
        if ratio>1.5: print(log.name,'trial',m[1],'ratio',f'{ratio:.6f}')
