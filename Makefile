CFLAGS = -g -O2
LDFLAGS_GL = -lGL
LDFLAGS_MESA = -lMesaGL
SDLFLAGS = $(shell sdl2-config --libs --cflags)
CC=gcc

# -lXi -lXmu

all:	eglbuild

glbuild:
	$(CC) $(CFLAGS) -o app_gl my_app.c $(LDFLAGS_GL) $(SDLFLAGS)

eglbuild:
	$(CC) $(CFLAGS) -o app_egl my_app.c -I/soft/X11R6.3/include -L/soft/X11R6.3/lib -lGLESv2 -lXext -lXt -lX11 -lICE -lSM -lm $(SDLFLAGS)

clean:
	rm app_egl app_gl
