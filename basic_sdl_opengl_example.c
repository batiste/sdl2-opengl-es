
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "SDL.h"
#include "SDL_opengl.h"

SDL_Window * mainwindow;   /* Our window handle */
SDL_GLContext maincontext; /* Our opengl context handle */
Uint32 then, now, frames;  /* Used for FPS */

/* cleanup before quiting */
static int
cleanup(int rc)
{
    /* Print out some timing information */
    now = SDL_GetTicks();
    if (now > then) {
        printf("%2.2f frames per second\n",
               ((double) frames * 1000) / (now - then));
    }
    if(maincontext)
        SDL_GL_DeleteContext(maincontext);
    if(mainwindow)
        SDL_DestroyWindow(mainwindow);
    SDL_Quit();
    return 0;
}

void
checkSDLError(int line)
{
    const char *error = SDL_GetError();
    if (*error != '\0')
    {
        printf("SDL Error: %s\n", error);
        if (line != -1)
            printf(" + line: %i\n", line);
        SDL_ClearError();
    }
}

GLuint
checkGlError(int line)
{
    GLuint err = glGetError();
    if (err > 0 )  {
        printf("glGetError(%d)\n", err);
        if (line != -1)
            printf(" + line: %i\n", line);
    }
    return err;
}


struct textureInfos {
   char* filename;
   GLuint texture;
   int width;
   int height;
};

int
loadTexture(struct textureInfos * infos) {

    // This surface will tell us the details of the image
    SDL_Surface * surface;
    GLenum texture_format;
    GLint  nOfColors;

    if ( !(surface = SDL_LoadBMP(infos->filename)) ) {
        printf("SDL could not load %s: %s\n", infos->filename, SDL_GetError());
        return 0;
    }

    // Check that the image's width is a power of 2
    if ( (surface->w & (surface->w - 1)) != 0 ) {
        printf("warning: %s width is not a power of 2\n", infos->filename);
    }

    // Also check if the height is a power of 2
    if ( (surface->h & (surface->h - 1)) != 0 ) {
        printf("warning: %s height is not a power of 2\n", infos->filename);
    }

        // get the number of channels in the SDL surface
        nOfColors = surface->format->BytesPerPixel;
        if (nOfColors == 4)     // contains an alpha channel
        {
            if (surface->format->Rmask == 0x000000ff)
                    texture_format = GL_RGBA;
            else
                    texture_format = GL_BGRA;
        } else if (nOfColors == 3)     // no alpha channel
        {
            if (surface->format->Rmask == 0x000000ff)
                    texture_format = GL_RGB;
            else
                    texture_format = GL_BGR;
        } else {
            printf("warning: the image is not truecolor..  this will probably break\n");
            // this error should not go unhandled
        }

    // Have OpenGL generate a texture object handle for us
    glGenTextures( 1, &infos->texture );

    // Bind the texture object
    glBindTexture( GL_TEXTURE_2D, infos->texture );

    // Set the texture's stretching properties
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    // Edit the texture object's image data using the information SDL_Surface gives us
    glTexImage2D( GL_TEXTURE_2D, 0, nOfColors, surface->w, surface->h, 0,
                    texture_format, GL_UNSIGNED_BYTE, surface->pixels );

    infos->width = surface->w;
    infos->height = surface->h;

    SDL_FreeSurface( surface );
    return 1;
}

void drawTexture(struct textureInfos * infos) {
    float imagew_2 = infos->width / 2;
    float imageh_2 = infos->height / 2;
    glBindTexture( GL_TEXTURE_2D, infos->texture );
    glBegin( GL_QUADS );
        //Bottom-left vertex (corner)
        glTexCoord2i( 0, 0 );
        glVertex3f( -imagew_2, -imageh_2, 0.0f );

        //Bottom-right vertex (corner)
        glTexCoord2i( 1, 0 );
        glVertex3f( imagew_2, -imageh_2, 0.f );

        //Top-right vertex (corner)
        glTexCoord2i( 1, 1 );
        glVertex3f( imagew_2, imageh_2, 0.f );

        //Top-left vertex (corner)
        glTexCoord2i( 0, 1 );
        glVertex3f( -imagew_2, imageh_2, 0.f );
    glEnd();
}

int
main(int argc, char *argv[])
{
    int windowWidth = 512;
    int windowHeight = 512;

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) { /* Initialize SDL's Video subsystem */
        printf("Unable to initialize SDL");
        return cleanup(0);
    }

    /* Request opengl 2.0 context.
     * SDL doesn't have the ability to choose which profile at this time of writing,
     * but it should default to the core profile */
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    /* Create our window centered at 512x512 resolution */
    mainwindow = SDL_CreateWindow("Simple rotating texture", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        windowWidth, windowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!mainwindow) {/* Die if creation failed */
        printf("Unable to create window");
        return cleanup(0);
    }

    checkSDLError(__LINE__);

    /* Create our opengl context and attach it to our window */
    maincontext = SDL_GL_CreateContext(mainwindow);
    checkSDLError(__LINE__);
    if (!maincontext) {
        fprintf(stderr, "SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return cleanup(0);
    }

    checkSDLError(__LINE__);
    checkGlError(__LINE__);

    /* Set rendering settings */
    glEnable( GL_TEXTURE_2D );
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glViewport( 0, 0, windowWidth, windowHeight );
    glClear( GL_COLOR_BUFFER_BIT );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho(0.0f, windowWidth, windowHeight, 0.0f, -1.0f, 1.0f);

    /* load texture */
    GLuint textureid;
    struct textureInfos texture;
    texture.filename = "SDL_logo.bmp";
    loadTexture(&texture);
    glBindTexture( GL_TEXTURE_2D, texture.texture );

    checkGlError(__LINE__);

    /* Swap our back buffer to the front */
    SDL_GL_SwapWindow(mainwindow);
    glClear(GL_COLOR_BUFFER_BIT);

    SDL_Event event;
    float theta = 0;
    int done = 0;
    then = SDL_GetTicks();
    float imagew_2 = texture.width / 2;
    float imageh_2 = texture.height / 2;

    while (!done) {
        ++frames;
        theta = theta + 0.1;
        while (SDL_PollEvent(&event)) {
            if(event.type == SDL_WINDOWEVENT_CLOSE || event.type == SDL_QUIT) {
                done = 1;
            }
        }

        glClear( GL_COLOR_BUFFER_BIT );

        glPushMatrix();
            glTranslatef( windowWidth/2, windowHeight/2, 0.0f );
            glRotatef( theta, 0.0f, 0.0f, 1.0f );
            drawTexture(&texture);
            glRotatef( theta, 0.0f, 0.0f, 1.0f );
            drawTexture(&texture);
            glRotatef( theta, 0.0f, 0.0f, 1.0f );
            drawTexture(&texture);
        glPopMatrix();

        //checkGlError(__LINE__);

        SDL_GL_SwapWindow(mainwindow);
        //SDL_Delay(0);
    }

    glDeleteTextures( 1, &textureid );
    return cleanup(0);
}
