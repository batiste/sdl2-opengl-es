gcc -o basic_sdl_opengl_example ./basic_sdl_opengl_example.c -g -O2 -D_REENTRANT -I/usr/local/include/SDL2 -DHAVE_OPENGL -Wl,-rpath,/usr/local/lib -L/usr/local/lib -lSDL2 -lpthread -lGL -lm

