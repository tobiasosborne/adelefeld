"""Enumerate the actual declarations and require symbols; retain the expected failing link."""
import json
import re
import subprocess as S
from pathlib import Path

HERE = Path(__file__).resolve().parent
aux = HERE / 'declarations.aux'
c = HERE / 'declarations.c'
c.write_text('#include "adelefeld.h"\n')
S.run(['cc', '-Iinclude', '-std=c11', '-aux-info', str(aux), '-c', str(c),
       '-o', str(HERE / 'declarations.o')], check=True)
names = set()
for line in aux.read_text().splitlines():
    if line.startswith('/* include/adelefeld/'):
        match = re.search(r'\b(adf_\w+)\s*\(', line)
        if match:
            names.add(match[1])
nm = S.check_output(['nm', '-D', '--defined-only', str(HERE / 'bridge.so')], text=True)
exported = {line.split()[-1] for line in nm.splitlines()}
missing = sorted(names - exported)
print(json.dumps(dict(declared=len(names), exported=len(names & exported), missing=missing)))
link = HERE / 'missing_link.c'
link.write_text('#include "adelefeld.h"\nint main(void) {\n'
                'adf_rat_t x; size_t n; adf_rat_init(x);\n'
                'char *s = adf_rat_dump_str(&n, x); adf_str_free(s); adf_rat_clear(x); return 0; }\n')
result = S.run(['cc', '-Iinclude', str(link), str(HERE / 'build/libadelefeld.a'),
                '-lflint', '-lgmp', '-lm', '-o', str(HERE / 'missing_link')], capture_output=True, text=True)
print(json.dumps(dict(link_returncode=result.returncode, stderr=result.stderr)))
assert result.returncode != 0 and 'adf_rat_dump_str' in result.stderr
flint_nm = S.check_output(['nm', '-D', '--defined-only', '/usr/lib/x86_64-linux-gnu/libflint.so'], text=True)
flint_names = {line.split()[-1] for line in flint_nm.splitlines()}
promised = ['arb_add_fmpq', 'arb_mul_fmpq']
print(json.dumps(dict(header_named_flint_functions={n: n in flint_names for n in promised})))
assert not any(n in flint_names for n in promised)
