"""Recreate exactly the ten proposed equivalents without changing the reviewed source."""
import ctypes as C
import json
import random
import subprocess as S
import sys
from pathlib import Path

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[4]
sys.path.insert(0, str(ROOT / 'tools/mutate'))
import mutate
import oracle as o

selected = {(100, 27, 'cmp'), (100, 53, 'cmp'), (114, 18, 'cmp'), (713, 14, 'zero_one'),
            (753, 12, 'cmp'), (753, 15, 'zero_one'), (804, 22, 'zero_one'),
            (889, 11, 'cmp'), (889, 14, 'zero_one'), (1096, 16, 'zero_one')}
source = (ROOT / 'src/text.c').read_text()
mutants = [m for m in mutate.mutants_of('src/text.c', source, str(ROOT))
           if (m.line, m.col, m.kind) in selected]
assert len(mutants) == 10
(HERE / 'mutants').mkdir(exist_ok=True)
rng = random.Random(280929)
texts = ['(1 +/- 1 ; 0)', '(0 +/- 536870912 ; 0)', '(3e0 ; 0)', '(12.0 ; 0)',
         '1+/-', '(1;0)+QZ', '(1;0)+Qz', '(1;0)+Q', '7modZ6', '7modz6', '(1 ; 0)+/-']
texts += [f'({o.decimal(rng)} +/- {o.decimal(rng, False)} ; 0)' for _ in range(1000)]
prints = [(str(m).encode(), e, r, re, n) for m in [-125, -1, 0, 1, 125]
          for e in [-1, 0, 1] for r in [0, 1, 536870912] for re in [-1, 0, 1] for n in [1, 20]]
baseline = o.lib
parse_want = [o.parse(t, 13) for t in texts]
print_want = [o.take(o.lib.review_print(*args)) for args in prints]
for m in mutants:
    stem = HERE / 'mutants' / f'{m.line}_{m.col}_{m.kind}'
    stem.with_suffix('.c').write_text(m.apply(source))
    S.run(['cc', '-std=c11', '-O2', '-fPIC', '-shared', '-Iinclude', str(HERE / 'bridge.c'),
           str(stem.with_suffix('.c')), '-Wl,-u,adf_str_free', str(HERE / 'build/libadelefeld.a'),
           '-lflint', '-lgmp', '-lm', '-o', str(stem.with_suffix('.so'))], check=True)
    o.lib = C.CDLL(str(stem.with_suffix('.so')))
    for name in ['review_parse', 'review_status', 'review_print', 'adf_str_free', 'adf_text_classify']:
        getattr(o.lib, name).argtypes = getattr(baseline, name).argtypes
        getattr(o.lib, name).restype = getattr(baseline, name).restype
    assert [o.parse(t, 13) for t in texts] == parse_want, str(m)
    assert [o.take(o.lib.review_print(*args)) for args in prints] == print_want, str(m)
    # Classification includes the end-of-input and keyword boundary cases.
    for t in texts:
        data = t.encode()
        a, b = C.c_int(99), C.c_int(99)
        sa = baseline.adf_text_classify(C.byref(a), data, len(data), None)
        sb = o.lib.adf_text_classify(C.byref(b), data, len(data), None)
        assert (sa, a.value) == (sb, b.value), (str(m), t)
    print(json.dumps(dict(mutant=str(m), parser=len(texts), classifier=len(texts),
                          printer=len(prints), differences=0)), flush=True)
o.lib = baseline
