from pathlib import Path
import shutil
root=Path(__file__).resolve().parents[2]
stage=root/'lanes/f-slice12/build/mutroot'
stage.mkdir(parents=True,exist_ok=True)
for name in ('include','src'):
    if (stage/name).exists(): shutil.rmtree(stage/name)
    shutil.copytree(root/name,stage/name)
(stage/'tests/support').mkdir(parents=True,exist_ok=True)
for f in ('tests/test_symbol.c','tests/test_runner.h','tests/support/jsonl.c','tests/support/jsonl.h'):
    shutil.copy2(root/f,stage/f)
shutil.copytree(root/'tests/ref/vectors/f-slice12',stage/'tests/ref/vectors/f-slice12',dirs_exist_ok=True)
(stage/'lanes/f-slice12').mkdir(parents=True,exist_ok=True)
shutil.copy2(root/'lanes/f-slice12/build-mutbase/libadelefeld.a',stage/'lanes/f-slice12/libbase.a')
(stage/'Makefile').write_text('''check-symbol:
\ttimeout 60 cc -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -DADF_CHECK_INVARIANTS -Iinclude -Isrc -c src/symbol.c -o symbol.o
\ttimeout 60 cc -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -DADF_CHECK_INVARIANTS -Iinclude -Itests tests/test_symbol.c tests/support/jsonl.c symbol.o lanes/f-slice12/libbase.a -lflint -lgmp -lm -pthread -o test_symbol
\tASAN_OPTIONS=detect_leaks=0 timeout 60 ./test_symbol
''')
print(stage)
