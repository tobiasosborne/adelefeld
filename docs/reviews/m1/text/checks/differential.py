"""Own grammar-aware generator, compared with proto/text_grammar.py."""
import ctypes as C
import json
import random
import sys
import time
from collections import Counter
from pathlib import Path

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[5]
sys.path.insert(0, str(ROOT / 'proto'))
import text_grammar as ref
from oracle import lib, Limits

rng = random.Random(280928)


def join(*parts):
    return rng.choice(['', ' ', '\t', '\r\n']).join(parts)


def uint():
    return rng.choice(['0', '1', '0001', str(rng.randrange(10000))])


def rat(unsigned=False):
    return ('' if unsigned or rng.randrange(2) else '-') + uint() + (
        '/' + uint() if rng.randrange(2) else '')


def dec(unsigned=False):
    return ('' if unsigned or rng.randrange(2) else '-') + uint() + (
        '.' + uint() if rng.randrange(2) else '') + (
        rng.choice(['e', 'E']) + rng.choice(['', '+', '-']) +
        rng.choice(['0', '0000000', '5', '25', '26', '99999999999999999999']) if rng.randrange(2) else '')


def real():
    return join(dec(), '+/-', dec(True)) if rng.randrange(2) else dec()


def complex_():
    return join('(', real(), ')', '+', '(', real(), ')', '*', 'i')


def fin():
    return join(rat(), 'mod', rat(True)) if rng.randrange(2) else rat()


def adele():
    return join('(', real(), ';', fin(), ')')


def unit():
    return join('[', rng.choice(['', '-']) + uint(), 'mod', uint(), ']')


def local():
    return join(rat(), '+', 'O', '(', uint(), '^', rng.choice(['', '-']) + uint(), ')')


def term():
    return join('term', '(', 'P', '=', '[', ','.join(complex_() for _ in range(rng.randrange(3))),
                ']', ',', 'A', '=', complex_(), ',', 'B', '=', complex_(), ',', 'C', '=', complex_(), ')')


def sentence(k):
    if k == 0: return rat()
    if k == 1: return join('(', '*', ';', fin(), ')')
    if k == 2: return adele()
    if k == 3: return join('(', complex_(), ';', fin(), ')')
    if k == 4: return unit()
    if k == 5: return join('(', real(), ';', rat(True), '*', unit(), ')')
    if k == 6: return join('<', real(), ';', unit(), '>')
    if k == 7: return join('[', 'p', '=', uint(), ':', local(), ']')
    if k == 8: return join('{', 'inf', ':', complex_(), ';', 'p', '=', uint(), ':', local(), '}')
    if k == 9: return join('union', '(', ','.join(adele() for _ in range(rng.randrange(1, 4))), ')', '+', 'Q')
    if k == 10: return join('ffun', '(', 'D', '=', uint(), ',', 'M', '=', uint(), ';', complex_(), ')')
    if k == 11: return join('rfun', '(', ','.join(term() for _ in range(rng.randrange(3))), ')')
    return join('char', '(', 'q', '=', uint(), ',', 'n', '=', uint(), ',', 's', '=', complex_(), ')')


def main():
    start = time.monotonic()
    totals = Counter()
    mutations = Counter()
    typed = Counter()
    for i in range(100000):
        k = i % 13
        data = sentence(k).encode()
        mode = i % 5
        mutations[mode] += 1
        at = rng.randrange(len(data) + 1)
        if mode == 1: data = data[:at] + rng.choice([b'Z', b'z', b' ', b'/', b'+/-', b'\0']) + data[at:]
        if mode == 2: data = data[:at] + data[at+1:]
        if mode == 3: data += rng.choice([b'Z', b'z', b'\xff', b' ; ', b'\r\n'])
        if mode == 4: data = b'\t' + data + b'\n'
        n = len(data) - 1 if i % 41 == 0 else 1048576
        lim = Limits(n, 25, 0, 0)
        rlim = ref.Limits(n, 25, 0, 0)
        want = ref.classify(data, rlim)
        out = C.c_int(987)
        st = lib.adf_text_classify(C.byref(out), data, len(data), C.byref(lim))
        got = ref.TYPES[out.value] if st == 0 else '!' + next(t for t, v in ref.STATUS.items() if v == st)
        assert got == want, (data, got, want)
        assert st == 0 or out.value == 987
        totals[got] += 1
        for tk in range(4):
            # canonical is the reference's full public parser, not just its recognizer.
            rwant = ref.canonical(ref.TYPES[tk], data, rlim)
            expected = ref.STATUS[rwant[1:]] if rwant.startswith('!') else 0
            actual = lib.review_status(tk, data, len(data), C.byref(lim))
            assert actual == expected, (tk, data, actual, rwant)
            typed[str(actual)] += 1
    print(json.dumps(dict(texts=100000, classifier=dict(totals), parser_calls=400000,
                          typed_statuses=dict(typed), mutation_modes=dict(mutations), failures=0,
                          seconds=round(time.monotonic()-start, 3)), sort_keys=True))


if __name__ == '__main__':
    main()
