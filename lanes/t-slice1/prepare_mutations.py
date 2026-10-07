#!/usr/bin/env python3
"""Stage only selected tests with a prebuilt clang/SAN/INV dependency archive.
Each mutant recompiles all of localfactor.c; the dependency archive is unchanged.
The replacement object precedes the archive, so the archive localfactor.o is not pulled.
"""
from pathlib import Path
import shutil
root = Path(__file__).resolve().parents[2]
stage = root/'lanes/t-slice1/mutation-root'
if stage.exists():
    shutil.rmtree(stage)
stage.mkdir()
for entry in ('include','src','tests'):
    shutil.copytree(root/entry, stage/entry, ignore=shutil.ignore_patterns('__pycache__'))
shutil.copy2(root/'Makefile',stage/'Makefile')
(stage/'prebuilt').mkdir()
build = root/'lanes/t-slice1/build-mutation'
shutil.copy2(build/'libadelefeld.a',stage/'prebuilt/libadelefeld.a')
for name in ('golden.o','jsonl.o'):
    shutil.copy2(build/'support'/name,stage/'prebuilt'/name)
(stage/'run.sh').write_text('''#!/bin/sh
set -eu
flags='-std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -DADF_CHECK_INVARIANTS'
san='-fsanitize=address,undefined -fno-omit-frame-pointer'
timeout 60 clang -Iinclude -Isrc $flags $san -c src/localfactor.c -o localfactor.o
for n in test_tate_local test_localfactor; do
    timeout 60 clang -Iinclude -Itests $flags $san tests/$n.c localfactor.o \\
        prebuilt/golden.o prebuilt/jsonl.o prebuilt/libadelefeld.a -lflint -lgmp -lm -pthread -o "$n"
    timeout 120 "./$n"
done
''')
print('staged clang SAN=1 INV=1 archive and 2 test programs')
