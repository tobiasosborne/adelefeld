#!/usr/bin/env python3
"""Scope the unmodified mutation tool to the added/changed functions.

Unchanged code is moved verbatim into included scratch headers. The candidate C files contain
only this slice's functions. All three full translation units are still compiled into each test.
"""
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "lanes/q-slice4/mutroot"
if OUT.exists():
    shutil.rmtree(OUT)
for directory in ("src", "tests", "lanes"):
    (OUT / directory).mkdir(parents=True)
shutil.copytree(ROOT / "include", OUT / "include")
shutil.copytree(ROOT / "src", OUT / "src", dirs_exist_ok=True)
shutil.copytree(ROOT / "tests/support", OUT / "tests/support")
shutil.copytree(ROOT / "tests/golden", OUT / "tests/golden")
shutil.copytree(ROOT / "tests/ref/vectors/q-slice4", OUT / "tests/ref/vectors/q-slice4")
for name in ("test_qclass_text.c", "test_qclass_dump.c"):
    shutil.copy2(ROOT / "tests" / name, OUT / "tests" / name)
shutil.copy2(ROOT / "lanes/q-slice4/san-inv/libadelefeld.a", OUT / "src/base.a")
bounds = {
    "qclass": ("/* CV-45 uses canonical global keys", "/* Q1, with review R1"),
    "text": ("/* Slice 3.1-d. Reuse the lexical machinery", "\n#include <stdlib.h>\n\n/* conventions 9.4"),
    "dump": ("/* ---- Slice 3.1-d: conventions", None),
}
mapping = []
for name, (start, end) in bounds.items():
    source = (ROOT / "src" / (name + ".c")).read_text()
    a = source.index(start)
    b = source.index(end, a) if end else len(source)
    (OUT / "src" / (name + "_unchanged.h")).write_text(source[:a] + source[b:])
    (OUT / "src" / (name + ".c")).write_text(f'#include "{name}_unchanged.h"\n' + source[a:b])
    mapping.append(f"src/{name}.c: scratch line 2 = repository line {source[:a].count(chr(10))+1}")
(ROOT / "lanes/q-slice4/mutation-map.txt").write_text("\n".join(mapping) + "\n")
(OUT / "Makefile").write_text('''CC = clang
CFLAGS = -std=c11 -O1 -g -Wall -Wextra -Wpedantic -Werror -fno-omit-frame-pointer
CPPFLAGS = -Iinclude -Itests -Isrc
SAN ?= 0
INV ?= 0
ifeq ($(SAN),1)
CFLAGS += -fsanitize=address,undefined
endif
ifeq ($(INV),1)
CPPFLAGS += -DADF_CHECK_INVARIANTS
endif
OBJ = qclass.o dump.o text.o jsonl.o golden.o
check: test_qclass_text test_qclass_dump
\tASAN_OPTIONS=detect_leaks=0 timeout 60 ./test_qclass_text
\tASAN_OPTIONS=detect_leaks=0 timeout 60 ./test_qclass_dump
qclass.o: src/qclass.c src/qclass_unchanged.h
\ttimeout 60 $(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
dump.o: src/dump.c src/dump_unchanged.h
\ttimeout 60 $(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
text.o: src/text.c src/text_unchanged.h
\ttimeout 60 $(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
jsonl.o: tests/support/jsonl.c
\ttimeout 60 $(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
golden.o: tests/support/golden.c
\ttimeout 60 $(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
test_qclass_text: tests/test_qclass_text.c $(OBJ) src/base.a
\ttimeout 60 $(CC) $(CPPFLAGS) $(CFLAGS) $^ -lflint -lgmp -lm -pthread -o $@
test_qclass_dump: tests/test_qclass_dump.c $(OBJ) src/base.a
\ttimeout 60 $(CC) $(CPPFLAGS) $(CFLAGS) $^ -lflint -lgmp -lm -pthread -o $@
''')
print("Scoped source candidates: qclass constructor/helpers, qclass dump functions, qclass union reader")
print("\n".join(mapping))
