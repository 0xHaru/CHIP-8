CC=gcc
CFLAGS=-Wall -Wextra -Wpedantic -std=c99 -g -DDEBUG
# CFLAGS=-Wall -Wextra -Wpedantic -std=c99 -s -O2

SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LDFLAGS := $(shell sdl2-config --libs)

all: chip8

chip8: sdl.c chip8.c chip8.h
	$(CC) sdl.c chip8.c -o $@ $(CFLAGS) $(SDL_CFLAGS) $(SDL_LDFLAGS)

clean:
	rm chip8
