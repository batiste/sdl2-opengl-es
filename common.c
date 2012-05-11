// Common functions

#include "SDL.h"
#include "SDL_opengl.h"

#ifdef __ANDROID_API__

#include <jni.h>
#include <android/log.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define  LOG_TAG    "libgl2jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#else

#define  LOGI(...)  printf(__VA_ARGS__)
#define  LOGE(...)  printf(__VA_ARGS__)

#endif


void
checkSDLError(int line)
{
    const char *error = SDL_GetError();
    if (*error != '\0')
    {
        printf("SDL Error: %s\n", error);
        if (line != -1)
            LOGE(" + line: %i\n", line);
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
            LOGE(" + line: %i\n", line);
    }
    return err;
}


char * loadfile(char *file)
{
    FILE *fp;
    long lSize;
    char * buffer;

    fp = fopen(file , "r");
    if( !fp ) {
        perror(file);
        exit(1);
    }

    fseek( fp , 0L , SEEK_END);
    lSize = ftell( fp );
    rewind( fp );

    /* allocate memory for entire content */
    buffer = calloc( 1, lSize+1 );
    if( !buffer ) {
        fclose(fp);
        LOGE("memory alloc fails\n");
        exit(1);
    }
    /* copy the file into the buffer */
    if( 1!=fread( buffer , lSize, 1 , fp) ) {
        fclose(fp);
        free(buffer);
        LOGE("entire read fails\n");
        exit(1);
    }
    fclose(fp);
    return buffer;
}


// Create a shader object, load the shader source, and
// compile the shader.
GLuint
loadShader(GLenum type, char* filename)
{
    GLuint shader;
    GLint compiled;
    // Create the shader object
    shader = glCreateShader(type);
    if(shader == 0) {
        printf("Shader failed\n %d\n", type);
        return 0;
    }
    // Load the shader source
    char * buffer;
    buffer = loadfile(filename);
    glShaderSource(shader, 1, &buffer, NULL);
    free(buffer);

    // Compile the shader
    glCompileShader(shader);
        // Check the compile status
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if(!compiled)
    {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if(infoLen > 1)
        {
            char* infoLog = malloc(sizeof(char) * infoLen);
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            LOGE("Error compiling shader:\n%s\n", infoLog);
            free(infoLog);
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
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
        LOGE("SDL could not load %s: %s\n", infos->filename, SDL_GetError());
        return 0;
    }

    // Check that the image's width is a power of 2
    if ( (surface->w & (surface->w - 1)) != 0 ) {
        LOGE("warning: %s width is not a power of 2\n", infos->filename);
    }

    // Also check if the height is a power of 2
    if ( (surface->h & (surface->h - 1)) != 0 ) {
        LOGE("warning: %s height is not a power of 2\n", infos->filename);
    }

    // get the number of channels in the SDL surface
    nOfColors = surface->format->BytesPerPixel;
    if (nOfColors == 4) // contains an alpha channel
    {
        LOGE("Image %s has an alpha channel\n", infos->filename);
        if (surface->format->Rmask == 0x000000ff)
            texture_format = GL_RGBA;//GL_RGBA;
        else
            texture_format = GL_RGBA;//GL_BGRA;
    } else if (nOfColors == 3)     // no alpha channel
    {
        LOGE("Image %s does not have an alpha channel\n", infos->filename);
        if (surface->format->Rmask == 0x000000ff)
            texture_format = GL_RGB;
        else
            texture_format = GL_RGB;
    } else {
        LOGE("warning: the image is not truecolor..  this will probably break\n");
        // this error should not go unhandled
    }

    // Have OpenGL generate a texture object handle for us
    glGenTextures( 1, &infos->texture );

    // Bind the texture
    glActiveTexture(GL_TEXTURE0);

    // Bind the texture object
    glBindTexture( GL_TEXTURE_2D, infos->texture );

    // Set the texture's stretching properties
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

    // Edit the texture object's image data using the information SDL_Surface gives us
    glTexImage2D( GL_TEXTURE_2D, 0, nOfColors, surface->w, surface->h, 0,
                    texture_format, GL_UNSIGNED_BYTE, surface->pixels );

    infos->width = surface->w;
    infos->height = surface->h;

    SDL_FreeSurface( surface );
    return 1;
}