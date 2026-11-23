CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic
PKG_CONFIG ?= pkg-config
SDL_CFLAGS = $(shell $(PKG_CONFIG) --cflags sdl2)
SDL_LIBS = $(shell $(PKG_CONFIG) --libs sdl2)
TEST_SUPPORT_SOURCES ?= turing.c

.PHONY: all test watch-test clean

all: 2d-turing

2d-turing: main.c window.c window.h tape_view.c tape_view.h ui_text.c ui_text.h turing.c turing.h digit.h machine_catalog.c machine_catalog.h catalog_view.c catalog_view.h
	@$(PKG_CONFIG) --exists sdl2 || { echo "SDL2 development files are required (see README.md)" >&2; exit 1; }
	$(CC) $(CFLAGS) $(SDL_CFLAGS) main.c window.c tape_view.c ui_text.c turing.c machine_catalog.c catalog_view.c -o $@ $(SDL_LIBS) -lm

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
