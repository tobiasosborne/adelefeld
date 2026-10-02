"""Build sanitizer test binaries once, separately from the bounded acceptance run."""
from pathlib import Path
import subprocess
names = ['build/san/'+p.stem for p in sorted(Path('tests').glob('test_*.c'))]
p = subprocess.run(['timeout', '175', 'make', '-j2', 'BUILD=build/san', 'SAN=1'] + names)
raise SystemExit(p.returncode)
