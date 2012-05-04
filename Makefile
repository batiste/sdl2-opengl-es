CFLAGS = -g -O2 -D_REENTRANT -DHAVE_OPENGL
LDFLAGS = -Wl,-rpath,/usr/local/lib -I/usr/local/include/SDL2 -I/usr/local/lib -L/usr/local/lib -lm -lSDL2 -lpthread -lGL

all: basic_sdl_opengl_example

clean:
	rm basic_sdl_opengl_example
