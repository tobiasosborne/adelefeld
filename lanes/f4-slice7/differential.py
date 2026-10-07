#!/usr/bin/env python3
"""Bounded dump differential smoke run against the exact reference; no C oracle import."""
from pathlib import Path
import ctypes as C
import json
import random
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'proto'))
import text_grammar as ref

lib = C.CDLL(str(ROOT / 'lanes/f4-slice7/libadelefeld.so'))
free = lib.adf_str_free
free.argtypes = [C.c_void_p]
rng = random.Random(410807)
counts = {'texts': 0, 'edited': 0, 'valid': 0, 'mismatches': 0}
results = []
for kind in ('ffun', 'rfun'):
    corpus = [json.loads(line)['text'].encode('latin1') for line in
              (ROOT / ('tests/ref/vectors/f4-slice7/' + kind + '.jsonl')).read_text().splitlines()]
    def fn(suffix, restype, args):
        f = getattr(lib, 'adf_' + kind + '_' + suffix)
        f.restype = restype
        f.argtypes = args
        return f
    size = getattr(lib, 'adf_sizeof_' + kind)
    size.restype = C.c_size_t
    size.argtypes = []
    align = getattr(lib, 'adf_alignof_' + kind)
    align.restype = C.c_size_t
    align.argtypes = []
    value = C.create_string_buffer(size())
    assert C.addressof(value) % align() == 0
    init = fn('init', None, [C.c_void_p])
    clear = fn('clear', None, [C.c_void_p])
    load = fn('load_str', C.c_int, [C.c_void_p, C.c_void_p, C.c_size_t, C.c_void_p, C.c_void_p])
    inspect = fn('dump_inspect', C.c_int, [C.c_void_p, C.c_void_p, C.c_void_p, C.c_size_t, C.c_void_p])
    dump = fn('dump_str', C.c_void_p, [C.c_void_p, C.c_void_p])
    init(value)
    def printed():
        n = C.c_size_t()
        p = dump(C.byref(n), value)
        assert p
        try:
            return C.string_at(p, n.value)
        finally:
            free(p)
    try:
        for i in range(7500):
            text = rng.choice(corpus)
            if i % 4:
                counts['edited'] += 1
                pos = rng.randrange(len(text) + 1)
                action = rng.randrange(5)
                token = rng.choice([b'0', b'1', b'-', b' ', b'\t', b'\x00', b'\x80', b'z', b' f'])
                if action == 0:
                    text = text[:pos]
                elif action == 1:
                    text = text[:pos] + token + text[pos:]
                elif action == 2:
                    text = text[:pos] + token + text[pos + 1:]
                elif action == 3:
                    text = text[:pos] + text[pos + 1:]
                else:
                    text = text + token
            try:
                node = ref._dump_syntax(text, ref.DEFAULT_LIMITS)
                if node[0] != kind:
                    expected = '!PARSE'
                else:
                    ref._dump_validate(node, ref.DEFAULT_LIMITS)
                    expected = ref._dump_print(node)
            except ref.TextError as e:
                expected = '!' + e.status
            status = {'PARSE': 9, 'LIMIT': 10, 'DOMAIN': 7, 'UNSUPPORTED': 8}
            want = status[expected[1:]] if expected.startswith('!') else 0
            raw = C.create_string_buffer(text)
            before = value.raw
            old = printed()
            nctx = C.c_size_t(773)
            got_inspect = inspect(C.byref(nctx), None, raw, len(text), None)
            got_load = load(value, raw, len(text), None, None)
            now = printed()
            counts['texts'] += 1
            if want == 0:
                counts['valid'] += 1
            good = got_inspect == got_load == want and nctx.value == (0 if want == 0 else 773)
            good &= now == expected.encode('ascii') if want == 0 else value.raw == before and now == old
            if not good:
                counts['mismatches'] += 1
                results.append({'kind': kind, 'iteration': i, 'input_hex': text.hex(),
                                'want': want, 'inspect': got_inspect, 'load': got_load})
                if len(results) >= 10:
                    raise AssertionError(results)
    finally:
        clear(value)
assert counts['mismatches'] == 0, results
print(json.dumps(counts, sort_keys=True))
