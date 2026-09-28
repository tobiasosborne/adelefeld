# the mini tree of the reproducers of tools/mutate/mutate.py (review surface)
CC ?= cc
CFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror
check: build/test_mini
	./build/test_mini
build/test_mini: src/mini.c src/mini.h tests/test_mini.c
	mkdir -p build
	$(CC) -Isrc -Itests $(CFLAGS) src/mini.c tests/test_mini.c -o $@
