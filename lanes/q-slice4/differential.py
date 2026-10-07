#!/usr/bin/env python3
"""Bounded grammar-directed dump differential against the exact reference, seed 310408."""
from pathlib import Path
import ctypes as C
import json
import random
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "proto"))
import text_grammar as ref

lib = C.CDLL(str(ROOT / "lanes/q-slice4/build/libadelefeld.so"))


def bind(name, result, arguments):
    f = getattr(lib, name)
    f.restype, f.argtypes = result, arguments
    return f


sizeof = bind("adf_sizeof_qclass", C.c_size_t, [])
alignof = bind("adf_alignof_qclass", C.c_size_t, [])
init = bind("adf_qclass_init", None, [C.c_void_p])
clear = bind("adf_qclass_clear", None, [C.c_void_p])
load = bind("adf_qclass_load_str_binds", C.c_int,
            [C.c_void_p, C.c_void_p, C.c_size_t, C.c_void_p, C.c_size_t, C.c_void_p])
dump = bind("adf_qclass_dump_str", C.c_void_p, [C.POINTER(C.c_size_t), C.c_void_p])
inspect = bind("adf_qclass_dump_inspect", C.c_int,
               [C.POINTER(C.c_size_t), C.c_void_p, C.c_void_p, C.c_size_t, C.c_void_p])
free = bind("adf_str_free", None, [C.c_void_p])
adele_size = bind("adf_sizeof_adele", C.c_size_t, [])


class Qclass(C.Structure):
    _fields_ = [("form", C.c_int), ("length", C.c_long), ("piece", C.c_void_p)]


assert C.sizeof(Qclass) == sizeof() and C.alignment(Qclass) == alignof()
q = Qclass()
init(C.byref(q))
bases = [json.loads(line)["input"] for line in
         (ROOT / "tests/ref/vectors/q-slice4/dump.jsonl").read_text().splitlines()]
bases += [line.split("\t")[0] for line in (ROOT / "tests/golden/dump.tsv").read_text().splitlines()
          if line.startswith("adf1 Q qclass ")]
tokens = ["0", "-1", "1", "2", "-0", "00", "F", "g", "l", "x", "pieces", "lift", "Q",
          "100000", "-100000", "100001", "-100001", "40000001", "3fffffff",
          "ffffffffffffffff", "10000000000000000", "ffffffffffffffffffffffffffffffff"]
rng = random.Random(310408)
ok = loaded = changed = 0
try:
    for i in range(30000):
        s = rng.choice(bases)
        operation = rng.randrange(8)
        words = s.split(" ")
        index = rng.randrange(len(words))
        if operation == 1:
            words[index] = rng.choice(tokens)
            s = " ".join(words)
        elif operation == 2:
            del words[index]
            s = " ".join(words)
        elif operation == 3:
            words.insert(index, words[index])
            s = " ".join(words)
        elif operation == 4:
            j = rng.randrange(len(words))
            words[index], words[j] = words[j], words[index]
            s = " ".join(words)
        elif operation == 5:
            s = s[:rng.randrange(len(s)+1)]
        elif operation == 6:
            s = s[:index] + rng.choice(["\0", "\x80", "\n", "\t", "\r", " "]) + s[index:]
        elif operation == 7:
            s = rng.choice(["adf2 Q ", "adf1 K ", "adf01 Q "]) + s[7:]
        changed += operation != 0
        raw = s.encode("latin1")
        bytes_ = C.create_string_buffer(raw, len(raw)) if raw else None
        expected = ref.dump_roundtrip(raw)
        want = ref.STATUS[expected[1:]] if expected.startswith("!") else 0
        count = C.c_size_t(999)
        status = inspect(C.byref(count), None, bytes_, len(raw), None)
        assert status == want, (i, repr(raw), "inspect", status, expected)
        occurrences = ref.dump_contexts(raw) if want == 0 else None
        if want == 0:
            ok += 1
            assert count.value == len(occurrences)
        else:
            assert count.value == 999
        before = C.string_at(C.byref(q), sizeof())
        members = C.string_at(q.piece, q.length * adele_size())
        status = load(C.byref(q), bytes_, len(raw), None, 0, None)
        load_want = 7 if want == 0 and occurrences else want
        assert status == load_want, (i, repr(raw), "load", status, load_want)
        if status:
            assert C.string_at(C.byref(q), sizeof()) == before
            assert C.string_at(q.piece, q.length * adele_size()) == members
        else:
            loaded += 1
            n = C.c_size_t()
            p = dump(C.byref(n), C.byref(q))
            try:
                assert C.string_at(p, n.value) == raw
            finally:
                free(p)
finally:
    clear(C.byref(q))
    bind("flint_cleanup", None, [])()
print(f"dump differential: 30000 texts, {changed} edits, {ok} valid inspections, {loaded} global loads")
print("0 status, context-count, output-preservation, or dump-byte mismatches")
