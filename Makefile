CC ?= cc
CPPFLAGS ?=
CFLAGS ?=
LDFLAGS ?=
LDLIBS ?=

WARNINGS = -Wall -Wextra -Wpedantic -Werror
LOCAL_CPPFLAGS = -Iinclude
LOCAL_CFLAGS = -std=c99 $(WARNINGS)

.PHONY: all test clean

all: test

test:
	$(CC) $(CPPFLAGS) $(LOCAL_CPPFLAGS) $(CFLAGS) $(LOCAL_CFLAGS) src/mcbor.c tests/test_all.c $(LDFLAGS) $(LDLIBS) -lm -o tests/test_all
	./tests/test_all

clean:
	$(RM) tests/test_all tests/test_all.exe
