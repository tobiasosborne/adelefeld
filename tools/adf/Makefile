# tools/adf/Makefile: the command-line driver adf (docs/PLAN.md 6, row 1.5).
#
#   make            build ../../build/adf
#   make SAN=1      build ../../build/adf-san with the address and undefined-behaviour
#                   sanitizers (a separate name, so that a sanitized build never replaces
#                   an ordinary one and the two never mix)
#   make clean      remove both
#
# The driver is one translation unit over the public interface and a static library of
# src/, which is built here when it is missing.  The top-level Makefile does not know this
# program; tests/test_driver.sh calls this Makefile.
#
# The archive the driver links must carry the same sanitizer flags as the driver itself: a
# driver built without the sanitizers cannot link an archive that was built with them, and
# an instrumented driver over a plain archive leaves the code of the library unwatched.  So
# each setting has its own archive, below build/, and the two never mix.  With SAN=1 the
# driver and the library are both instrumented, so an overflow inside a library call is
# reported by the address sanitizer and by the undefined-behaviour sanitizer.
# (Repair of lane m1-repair-driver: before this, `sh tests/test_driver.sh` after
# `make check SAN=1` failed at the link with a page of undefined references, and
# `SAN=1 sh tests/test_driver.sh` over the plain archive watched only the driver.)

ROOT  := ../..
BUILD := $(ROOT)/build

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
# relative to the repository root, which is where the sub-make below runs
DRVBUILD   := build/drv-san
else
DRVBUILD   := build/drv-plain
endif
LIB := $(ROOT)/$(DRVBUILD)/libadelefeld.a

BIN := $(BUILD)/adf$(SAN_SUFFIX)

.PHONY: all clean
.DELETE_ON_ERROR:

all: $(BIN)

$(BIN): adf.c $(LIB)
	@mkdir -p $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) adf.c $(LIB) $(LDLIBS) -o $@

# The library of the driver; built on demand, with the same SAN setting, in its own
# directory below build/ (see above).
$(LIB):
	$(MAKE) -C $(ROOT) BUILD=$(DRVBUILD) SAN=$(SAN) all

clean:
	rm -f $(BUILD)/adf $(BUILD)/adf-san
	rm -rf $(ROOT)/$(DRVBUILD)
