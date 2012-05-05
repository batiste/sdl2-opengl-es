CFLAGS = -g -O2
LDFLAGS = -lGL
SDLFLAGS = $(shell sdl2-config --libs --cflags)
CC=gcc

all:	exemple1

exemple1:
	$(CC) $(CFLAGS) -o basic_sdl_opengl_example basic_sdl_opengl_example.c $(LDFLAGS) $(SDLFLAGS)

clean:
	rm basic_sdl_opengl_example
