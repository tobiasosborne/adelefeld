"""Final artifact bounds and syntax, independent of the C computations."""
import json
from pathlib import Path
paths=['src/gfunc_log.c','tests/test_gfunc_log.c','tests/julia/gfunc_log.jl']
for name in paths:
    assert all(len(s)<=116 for s in Path(name).read_text().splitlines()),name
for name,marker in [('docs/api-1f8.md','## Log on ideles'),
                    ('tools/adf/README.md','### Log on ideles'),
                    ('include/adelefeld/gfunc.h','/* ---- Log on ideles:')]:
    text=Path(name).read_text(); text=text[text.index(marker):]
    assert all(len(s)<=116 for s in text.splitlines()),name
sizes=[]
for p in sorted(Path('tests/ref/vectors/f-slice11').glob('*.jsonl')):
    rows=[json.loads(s) for s in p.read_text().splitlines()]
    sizes.append((p.name,len(rows),p.stat().st_size))
assert sum(s[2] for s in sizes)<1000000
assert {r['witness'] for r in map(json.loads,Path('tests/ref/vectors/f-slice11/local.jsonl').read_text().splitlines())
        if 'witness' in r}==set(range(1,9))
print('artifact lines: 0 over 116; fixture files:',sizes,'total bytes',sum(s[2] for s in sizes))
