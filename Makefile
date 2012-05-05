CFLAGS = -g -O2
LDFLAGS_GL = -lGL
LDFLAGS_MESA = -lMesaGL
SDLFLAGS = $(shell sdl2-config --libs --cflags)
CC=gcc

# -lXi -lXmu

all:	exemple1 exemple1mesa

exemple1:
	$(CC) $(CFLAGS) -o example1_gl basic_sdl_opengl_example.c $(LDFLAGS_GL) $(SDLFLAGS)

exemple1mesa:
	$(CC) $(CFLAGS) -o example1_mesa basic_sdl_opengl_example.c -I/soft/X11R6.3/include -L/soft/X11R6.3/lib -lGLESv2 -lXext -lXt -lX11 -lICE -lSM -lm $(SDLFLAGS)

clean:
	rm example1_gl example1_mesa
