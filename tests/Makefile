CC      ?= gcc
CFLAGS  := -std=c99 -Wall -Wextra -Wpedantic -Werror -I../include
SRC     := ../src/mcbor.c
TESTS   := test_all.c

.PHONY: all clean

all: test_all
	@./test_all

test_all: $(TESTS) $(SRC) ../include/mcbor.h
	$(CC) $(CFLAGS) $(SRC) $(TESTS) -lm -o $@

clean:
	rm -f test_all
