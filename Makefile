CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic
PKG_CONFIG ?= pkg-config
SDL_CFLAGS = $(shell $(PKG_CONFIG) --cflags sdl2)
SDL_LIBS = $(shell $(PKG_CONFIG) --libs sdl2)
TEST_SUPPORT_SOURCES ?=

.PHONY: all test watch-test clean

all: 2d-turing

2d-turing: main.c window.c window.h square.c square.h turing.c turing.h
	@$(PKG_CONFIG) --exists sdl2 || { echo "SDL2 development files are required (see README.md)" >&2; exit 1; }
	$(CC) $(CFLAGS) $(SDL_CFLAGS) main.c window.c square.c turing.c -o $@ $(SDL_LIBS)

build/tests/%_test: tests/%_test.c %.c $(TEST_SUPPORT_SOURCES) $(wildcard *.h tests/*.h) Makefile
	@mkdir -p build/tests
	@$(CC) $(CFLAGS) -g3 -O0 -UNDEBUG $(SDL_CFLAGS) -I. $(sort $< $*.c $(TEST_SUPPORT_SOURCES)) -o $@ $(SDL_LIBS)

test:
	@python3 scripts/watch_tests.py --once

watch-test:
	@python3 scripts/watch_tests.py

clean:
	rm -f 2d-turing
	rm -rf build/tests
