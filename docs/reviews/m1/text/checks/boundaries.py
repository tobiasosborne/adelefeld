import ctypes as C
import json
import sys
from pathlib import Path
from oracle import Limits, lib

sys.path.insert(0, str(Path(__file__).resolve().parents[5] / 'proto'))
import text_grammar as ref

checked = 0


def check(kind, text, want, max_len=1048576, exp=100000):
    global checked
    s = text.encode() if isinstance(text, str) else text
    lim = Limits(max_len, exp, 0, 0)
    st = lib.review_status(kind, s, len(s), C.byref(lim))
    assert st == want, (kind, s[:60], st, want)
    checked += 1


for kind, text in enumerate(['1/3', '(* ; 1/3 mod 2)', '(1 ; 0)', '((1) + (2)*i ; 0)']):
    check(kind, text, 0, len(text))
    check(kind, text, 10, len(text)-1)
    check(kind, text + '\0', 10, len(text))
    check(kind, text + '\0', 9, len(text)+1)
    check(kind, text + 'Z', 9)
    # Default whole-input boundary, using whitespace so the numerical size stays constant.
    check(kind, text + ' ' * (1048576-len(text)), 0)
    check(kind, text + ' ' * (1048577-len(text)), 10)

for limit in [0, 7, 100000]:
    for sign in ['', '-']:
        for e, st in [(limit, 0), (limit+1, 10)]:
            literal = f'1e{sign}{e}'
            for text in [f'({literal} ; 0)', f'(0 +/- {literal} ; 0)']:
                check(2, text, st, exp=limit)
            for part in [f'({literal}) + (0)*i', f'(0) + ({literal})*i']:
                check(3, f'({part} ; 0)', st, exp=limit)

for s, want in [('(1e8 ; 1/0)', 10), ('(1e8 ; 1/0 x)', 9), ('(1e8 ; 1/0)\0', 9),
                ('(1e7 ; 1/0)', 7), ('((1e8) + (0)*i ; 1/0)', 10),
                ('((0) + (0 +/- 1e8)*i ; 1/0)', 10)]:
    check(3 if s.startswith('((') else 2, s, want, exp=7)

for s in ['[p=5:0+O(5^0)]', '[p=5:0+O(5^1)]', 'rfun()',
          'union((0;0),(0;0))+Q', '{p=5:0;p=7:0}', 'ffun(D=1,M=1;(0)+(0)*i)']:
    data = s.encode()
    lim = Limits(1048576, 0, 0, 0)
    out = C.c_int(987)
    assert lib.adf_text_classify(C.byref(out), data, len(data), C.byref(lim)) == 0
    checked += 1

# A shared defect in C and reference. A zero with a permitted 19-digit exponent is still zero.
text = b'(0e1000000000000000000 ; 0)'
lim = Limits(1048576, 1000000000000000000, 100000, 1048576)
st = lib.review_status(2, text, len(text), C.byref(lim))
reference = ref.canonical('adele', text, ref.Limits(max_exp10=lim.max_exp10))
assert st == 10 and reference == '!LIMIT'
print(json.dumps(dict(boundary_checks=checked, failures=0, exponent_input=text.decode(),
                      permitted_exp=lim.max_exp10, c_status=st, reference=reference,
                      required_status=0)))

# Parameter values possible on accepted real-bearing fuzzer inputs (including whitespace).
precs, digits = set(), set()
for prefix in [b'', b' ', b'\t', b'\n', b'\r']:
    for suffix in [b'', b' ', b'\t', b'\n', b'\r']:
        s = prefix + b'(1 ; 0)' + suffix
        assert lib.review_status(2, s, len(s), None) == 0
        precs.add(2 + s[0] % 190)
        digits.add(1 + s[-1] % 30)
print(json.dumps(dict(fuzzer_success_prec=sorted(precs), fuzzer_success_digits=sorted(digits))))
