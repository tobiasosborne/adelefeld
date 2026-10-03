from pathlib import Path
import subprocess
P=Path('lanes/f-review8')
line='S 65537 1764 1 0 2 0 2 42 2\n'
for binary in (P/'build/h',P/'build/faults/root_wrong_unsampled_lift/h'):
    r=subprocess.run(['timeout','30',str(binary)],input=line,text=True,capture_output=True,check=True)
    print(str(binary),r.stdout.strip())
print('exact witness 42 is excluded by 65579+65537^2 Z_65537; precision=65537^2')
