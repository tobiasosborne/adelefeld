"""Stage only the catalogue test and a matching SAN/INV archive for the bounded mutation run."""
from pathlib import Path
import shutil

root = Path(__file__).resolve().parents[2]
stage = root / 'lanes/f-slice13/build/mutroot'
stage.mkdir(parents=True, exist_ok=True)
for name in ('include', 'src'):
    shutil.copytree(root/name, stage/name, dirs_exist_ok=True)
(stage/'tests/support').mkdir(parents=True, exist_ok=True)
for name in ('tests/test_catalogue.c', 'tests/test_runner.h', 'tests/support/jsonl.c', 'tests/support/jsonl.h'):
    shutil.copy2(root/name, stage/name)
shutil.copytree(root/'tests/ref/vectors/f-slice13', stage/'tests/ref/vectors/f-slice13', dirs_exist_ok=True)
(stage/'lanes/f-slice13').mkdir(parents=True, exist_ok=True)
shutil.copy2(root/'lanes/f-slice13/build-mutbase/libadelefeld.a', stage/'lanes/f-slice13/libbase.a')
continuation = ' '+chr(92)
lines = [
    'check-catalogue:',
    '\ttimeout 60 cc -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer'+continuation,
    '\t    -DADF_CHECK_INVARIANTS -Iinclude -Isrc -c src/catalogue.c -o catalogue.o',
    '\ttimeout 60 cc -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer'+continuation,
    '\t    -DADF_CHECK_INVARIANTS -Iinclude -Itests tests/test_catalogue.c tests/support/jsonl.c'+continuation,
    '\t    catalogue.o lanes/f-slice13/libbase.a -lflint -lgmp -lm -pthread -o test_catalogue',
    '\tASAN_OPTIONS=detect_leaks=0 timeout 60 ./test_catalogue',
]
(stage/'Makefile').write_text('\n'.join(lines)+'\n')

print(stage)
