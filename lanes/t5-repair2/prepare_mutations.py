"""Stage only the changed C file and its three direct test binaries."""
from pathlib import Path
import shutil
root = Path(__file__).resolve().parents[2]
lane = root / 'lanes/t5-repair2'
stage = lane / 'build-mutation-root'
if stage.exists():
    shutil.rmtree(stage)
stage.mkdir()
shutil.copytree(root / 'include', stage / 'include')
(stage / 'src').mkdir()
for name in ('localfactor.c', 'invariants.h'):
    shutil.copy2(root / 'src' / name, stage / 'src' / name)
shutil.copytree(root / 'tests/support', stage / 'tests/support')
for name in ('test_tate_local.c', 'test_tate_gamma.c', 'test_localfactor.c', 'test_runner.h'):
    shutil.copy2(root / 'tests' / name, stage / 'tests' / name)
for name in ('t5-slice1', 't5-slice3', 'f-slice14', 'f-repair9', 't5-repair1'):
    shutil.copytree(root / 'tests/ref/vectors' / name, stage / 'tests/ref/vectors' / name)
prebuilt = stage / 'lanes/prebuilt'
prebuilt.mkdir(parents=True)
shutil.copy2(lane / 'build-san-inv/libadelefeld.a', prebuilt / 'libadelefeld.a')
for name in ('golden.o', 'jsonl.o'):
    shutil.copy2(lane / 'build-san-inv/support' / name, prebuilt / name)
(stage / 'Makefile').write_text("""CC = cc
CFLAGS = -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -fno-omit-frame-pointer
CPPFLAGS = -Iinclude -Isrc -Itests
ifeq ($(SAN),1)
CFLAGS += -fsanitize=address,undefined
else
$(error This staged archive requires SAN=1)
endif
ifeq ($(INV),1)
CPPFLAGS += -DADF_CHECK_INVARIANTS
else
$(error This staged archive requires INV=1)
endif
TESTS = test_tate_local test_tate_gamma test_localfactor
SUPPORT = lanes/prebuilt/golden.o lanes/prebuilt/jsonl.o
LIB = lanes/prebuilt/libadelefeld.a
LDLIBS = -lflint -lgmp -lm -pthread
localfactor.o: src/localfactor.c
	timeout 60 $(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(TESTS): %: tests/%.c localfactor.o
	timeout 60 $(CC) $(CPPFLAGS) $(CFLAGS) $< localfactor.o $(SUPPORT) $(LIB) $(LDLIBS) -o $@
check: $(TESTS)
	@fail=0; for t in $(TESTS); do timeout 60 ./$$t || fail=1; done; exit $$fail
.PHONY: check
""")
print('staged', stage)
