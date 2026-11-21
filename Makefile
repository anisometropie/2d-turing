CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic
PKG_CONFIG ?= pkg-config
SDL_CFLAGS = $(shell $(PKG_CONFIG) --cflags sdl2)
SDL_LIBS = $(shell $(PKG_CONFIG) --libs sdl2)

.PHONY: all clean

all: 2d-turing

2d-turing: main.c window.c window.h
	@$(PKG_CONFIG) --exists sdl2 || { echo "SDL2 development files are required (see README.md)" >&2; exit 1; }
	$(CC) $(CFLAGS) $(SDL_CFLAGS) main.c window.c -o $@ $(SDL_LIBS)

clean:
	rm -f 2d-turing
