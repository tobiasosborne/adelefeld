#!/usr/bin/env python3
"""Prepare a slice-only mutation root over the matching SAN/INV archive.

The original Makefile and unowned sources are read-only. The scratch check target
runs the complete qclass test, including all vectors, golden rows and debug entries.
Compile the mutated qclass before the archive, so the archive's qclass.o is not used.
"""
from pathlib import Path
import shutil

root = Path('lanes/q-slice1/mutroot')
if root.exists():
    shutil.rmtree(root)
for path in ('src', 'tests/support', 'tests/golden', 'tests/ref/vectors/q-slice1', 'lanes'):
    (root / path).mkdir(parents=True, exist_ok=True)
shutil.copytree('include', root / 'include')
for path in ('src/qclass.c', 'src/invariants.h', 'tests/test_qclass.c',
             'tests/support/jsonl.c', 'tests/support/jsonl.h',
             'tests/support/golden.c', 'tests/support/golden.h', 'tests/golden/qclass.tsv',
             'tests/ref/vectors/q-slice1/translation.jsonl',
             'tests/ref/vectors/q-slice1/golden.jsonl'):
    shutil.copy2(path, root / path)
shutil.copy2('lanes/q-slice1/mutbase/libadelefeld.a', root / 'src/base.a')
(root / 'Makefile').write_text('''CC = clang
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
check: test_qclass
\tASAN_OPTIONS=detect_leaks=0 timeout 60 ./test_qclass
qclass.o: src/qclass.c
\ttimeout 60 $(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
test_qclass.o: tests/test_qclass.c
\ttimeout 60 $(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
jsonl.o: tests/support/jsonl.c
\ttimeout 60 $(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
golden.o: tests/support/golden.c
\ttimeout 60 $(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
test_qclass: qclass.o test_qclass.o jsonl.o golden.o src/base.a
\ttimeout 60 $(CC) $(CFLAGS) $^ -lflint -lgmp -lm -pthread -o $@
''')
print('mutation root: complete test_qclass, SAN/INV archive; independent mutated qclass.o')
