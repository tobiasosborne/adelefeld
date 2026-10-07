#!/usr/bin/env python3
"""Scope the unchanged mutation tool to the added dump implementation and two tests."""
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'lanes/f4-slice7/mutroot'
if OUT.exists():
    shutil.rmtree(OUT)
for name in ('include', 'tests/support', 'tests/golden', 'tests/ref/vectors/f4-slice7'):
    shutil.copytree(ROOT / name, OUT / name)
(OUT / 'src').mkdir()
(OUT / 'lanes/f4-slice7').mkdir(parents=True)
for name in ('test_ffun_dump.c', 'test_rfun_dump.c'):
    shutil.copy2(ROOT / 'tests' / name, OUT / 'tests' / name)
shutil.copy2(ROOT / 'lanes/f4-slice7/dump_test.h', OUT / 'lanes/f4-slice7/dump_test.h')
shutil.copy2(ROOT / 'src/invariants.h', OUT / 'src/invariants.h')
shutil.copy2(ROOT / 'lanes/f4-slice7/combined/libadelefeld.a', OUT / 'src/base.a')
source = (ROOT / 'src/dump.c').read_text()
a = source.index('/* Slice 4c dump forms:')
(OUT / 'src/dump_unchanged.h').write_text(source[:a])
(OUT / 'src/dump.c').write_text('#include "dump_unchanged.h"\n' + source[a:])
(ROOT / 'lanes/f4-slice7/mutation-map.txt').write_text(
    'src/dump.c scratch line 2 = repository line ' + str(source[:a].count('\n') + 1) + '\n')
(OUT / 'Makefile').write_text('''CC = clang
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
OBJ = dump.o jsonl.o golden.o
check: test_ffun_dump test_rfun_dump
\ttimeout 120 ./test_ffun_dump
\ttimeout 120 ./test_rfun_dump
dump.o: src/dump.c src/dump_unchanged.h
\ttimeout 60 $(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
jsonl.o: tests/support/jsonl.c
\ttimeout 60 $(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
golden.o: tests/support/golden.c
\ttimeout 60 $(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
test_ffun_dump: tests/test_ffun_dump.c lanes/f4-slice7/dump_test.h $(OBJ) src/base.a
\ttimeout 60 $(CC) $(CPPFLAGS) $(CFLAGS) $< $(OBJ) src/base.a -lflint -lgmp -lm -pthread -o $@
test_rfun_dump: tests/test_rfun_dump.c lanes/f4-slice7/dump_test.h $(OBJ) src/base.a
\ttimeout 60 $(CC) $(CPPFLAGS) $(CFLAGS) $< $(OBJ) src/base.a -lflint -lgmp -lm -pthread -o $@
''')
print('Only the added functions/helpers are mutation candidates; two full tests, SAN+INV.')
