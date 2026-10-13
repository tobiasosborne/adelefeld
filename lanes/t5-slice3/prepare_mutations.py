#!/usr/bin/env python3
"""Stage a small root for the bounded mutation run of src/localfactor.c against tests/test_tate_gamma.c.

Run: timeout 60 python3 -B lanes/t5-slice3/prepare_mutations.py
The dependency archive and support objects are the SAN=1 INV=1 build lanes/t5-slice3/build-mut. Each mutant
recompiles localfactor.c with the same flags; that object precedes the archive on the link line, so the
archive's localfactor.o is not pulled. run.sh builds and runs the one test program.
"""
from pathlib import Path
import shutil
root = Path(__file__).resolve().parents[2]
stage = root / 'lanes/t5-slice3/mutation-root'
build = root / 'lanes/t5-slice3/build-mut'
if stage.exists():
    shutil.rmtree(stage)
stage.mkdir()
shutil.copytree(root / 'include', stage / 'include')
(stage / 'src').mkdir()
for name in ('localfactor.c', 'invariants.h'):
    shutil.copy2(root / 'src' / name, stage / 'src' / name)
(stage / 'tests/support').mkdir(parents=True)
(stage / 'tests/ref/vectors/t5-slice3').mkdir(parents=True)
for name in ('test_tate_gamma.c', 'test_runner.h'):
    shutil.copy2(root / 'tests' / name, stage / 'tests' / name)
for header in (root / 'tests/support').glob('*.h'):
    shutil.copy2(header, stage / 'tests/support' / header.name)
shutil.copy2(root / 'tests/ref/vectors/t5-slice3/gamma.jsonl', stage / 'tests/ref/vectors/t5-slice3/gamma.jsonl')
(stage / 'prebuilt').mkdir()
shutil.copy2(build / 'libadelefeld.a', stage / 'prebuilt/libadelefeld.a')
for name in ('golden.o', 'jsonl.o'):
    shutil.copy2(build / 'support' / name, stage / 'prebuilt' / name)
(stage / 'run.sh').write_text('''#!/bin/sh
set -eu
flags='-std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -DADF_CHECK_INVARIANTS'
san='-fsanitize=address,undefined -fno-omit-frame-pointer'
timeout 60 cc -Iinclude -Isrc $flags $san -c src/localfactor.c -o localfactor.o
timeout 60 cc -Iinclude -Itests $flags $san tests/test_tate_gamma.c localfactor.o \\
    prebuilt/golden.o prebuilt/jsonl.o prebuilt/libadelefeld.a -lflint -lgmp -lm -pthread -o test_tate_gamma
ASAN_OPTIONS=detect_leaks=1 timeout 120 ./test_tate_gamma
''')
print('staged', stage)
