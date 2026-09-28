# tools/mutate/example/Makefile: a minimal tree for the self-test of tools/mutate/mutate.py.
#
#   make check   build and run tests/test_weak.c (the deliberately weak test)
#   make strong  build and run tests/test_strong.c instead
#   make clean   remove build/
#
# The same interface as the Makefile of the repository, so that mutate.py needs no option to
# work on this example: it copies Makefile, include, src and tests, writes the mutant into
# src/example.c and runs `make -s -j2 check` in the copy.

CC     ?= cc
CFLAGS ?= -std=c11 -O1 -g -Wall -Wextra -Werror
# src/ holds example.h, so that the test finds it with "example.h"
CPPFLAGS ?= -Isrc
TEST   ?= tests/test_weak.c
BIN     = build/example

.PHONY: check strong clean

build:
	mkdir -p build

$(BIN): src/example.c src/example.h $(TEST) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) src/example.c $(TEST) -o $@

check: $(BIN)
	./$(BIN)

strong:
	$(MAKE) check TEST=tests/test_strong.c

clean:
	rm -rf build
