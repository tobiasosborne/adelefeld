# tools/adf/Makefile: the command-line driver adf (docs/PLAN.md 6, row 1.5).
#
#   make            build ../../build/adf
#   make SAN=1      build ../../build/adf-san with the address and undefined-behaviour
#                   sanitizers (a separate name, so that a sanitized build never replaces
#                   an ordinary one and the two never mix)
#   make clean      remove both
#
# The driver is one translation unit over the public interface and the static library
# build/libadelefeld.a, which is built here when it is missing.  The top-level Makefile
# does not know this program; tests/test_driver.sh calls this Makefile.
#
# The static library is the one the top-level Makefile built.  With SAN=1 the driver
# itself is instrumented; the archive keeps the flags of its own build.  The address
# sanitizer still watches every heap block of the library, because it interposes the
# allocator, so an overflow inside a library call is reported.

ROOT  := ../..
BUILD := $(ROOT)/build
LIB   := $(BUILD)/libadelefeld.a

CC       ?= cc
CFLAGS   ?= -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror
CPPFLAGS ?= -I$(ROOT)/include
LDFLAGS  ?=
LDLIBS   ?= -lflint -lgmp -lm

SAN ?= 0
ifeq ($(SAN),1)
CFLAGS  += -fsanitize=address,undefined -fno-omit-frame-pointer
LDFLAGS += -fsanitize=address,undefined
SAN_SUFFIX := -san
endif

BIN := $(BUILD)/adf$(SAN_SUFFIX)

.PHONY: all clean
.DELETE_ON_ERROR:

all: $(BIN)

$(BIN): adf.c $(LIB)
	@mkdir -p $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) adf.c $(LIB) $(LDLIBS) -o $@

# The library of the top-level Makefile; built on demand, with the same SAN setting.
$(LIB):
	$(MAKE) -C $(ROOT) SAN=$(SAN) all

clean:
	rm -f $(BUILD)/adf $(BUILD)/adf-san
