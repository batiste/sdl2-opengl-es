// Common functions

#include "SDL.h"
#include "png.h"

#include <jni.h>
#include <android/log.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define  LOG_TAG    "SDL"
#define  LOG(...)   __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__);
#define  LOGI(...)  __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)

// for native asset manager
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>


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


char * loadfile(char *filename) {

    SDL_RWops * file;
    char * buffer;

    file = SDL_RWFromFile(filename, "rb");
    if (file==NULL) {
        LOG("Unable to open file");
        return 0;
    }
    LOG("Read success\n");

    SDL_RWseek(file, 0, SEEK_END);
    LOG("Seek\n");
    int finalPos = SDL_RWtell(file);
    LOG("Final Position (%d)\n", finalPos);
    SDL_RWclose(file);

    /* allocate memory for entire content */
    buffer = calloc( 1, sizeof(char) * (finalPos + 1));

    file = SDL_RWFromFile(filename, "rb");
    int n_blocks = SDL_RWread(file, buffer, 1, finalPos);
    if(n_blocks < 0) {
        LOG("Unable to read any block\n");
    }
    if(n_blocks == 0) {
        LOG("No block read\n");
    }

    LOG("Block read %d\n", n_blocks);

    SDL_RWclose(file);
    LOG("Quit loadfile\n");
    return buffer;
}


// Create a shader object, load the shader source, and
// compile the shader.
GLuint
loadShader(GLenum type, const GLchar * filename)
{
    GLuint shader;
    GLint compiled;
    // Create the shader object
    LOG("Shader 1");
    shader = glCreateShader(type);
    LOG("Shader 2");
    if(shader == 0) {
        printf("Shader failed\n %d\n", type);
        return 0;
    }
    // Load the shader source
    char * buffer;
    buffer = loadfile(filename);
    LOG("glShaderSource Buffer %s", buffer);
    glShaderSource(shader, 1, &buffer, NULL);

    // Compile the shader
    LOG("Compile shader");
    glCompileShader(shader);
    free(buffer);

    checkGlError(__LINE__);
    // Check the compile status
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    checkGlError(__LINE__);
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
    SDL_Surface * osurface;
    GLenum texture_format;
    GLint  nOfColors;

    /*GLuint tex_2d = SOIL_load_OGL_texture(
        "img.png",
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
    );*/


    if ( !(osurface = SDL_LoadBMP(infos->filename)) ) {
        LOGE("SDL could not load %s: %s\n", infos->filename, SDL_GetError());
        return 0;
    }

    SDL_Surface * surface =
    SDL_CreateRGBSurface(0, osurface->w, osurface->h, 24, 0xff000000, 0x00ff0000, 0x0000ff00, 0);
    SDL_BlitSurface(osurface, 0, surface, 0); // Blit onto a purely RGB Surface
    texture_format = GL_RGB;

    // Check that the image's width is a power of 2
    if ( (surface->w & (surface->w - 1)) != 0 ) {
        LOGE("warning: %s width is not a power of 2\n", infos->filename);
    }

    // Also check if the height is a power of 2
    if ( (surface->h & (surface->h - 1)) != 0 ) {
        LOGE("warning: %s height is not a power of 2\n", infos->filename);
    }

    // get the number of channels in the SDL surface
    // Openg ES only support GL_RGB and GL_RGBA
    nOfColors = surface->format->BytesPerPixel;
    LOG("surface->format->BytesPerPixel %d", nOfColors);
    LOG("surface->format->Rmask %d", surface->format->Rmask);

    // R: 111111110000000000000000 = 16711680
    // G:         1111111100000000 = 65280
    // B:                 11111111 = 255

    /*if (nOfColors == 4) // contains an alpha channel
    {
        LOGE("Image %s has an alpha channel\n", infos->filename);
        if (surface->format->Rmask == 0x000000ff) {
            LOG("GL_RGBA");
        } else {
            LOG("Image format unsupported");
        }
        texture_format = GL_RGBA;//GL_RGBA;
    } else if (nOfColors == 3)     // no alpha channel
    {
        LOGE("Image %s does not have an alpha channel\n", infos->filename);
        if (surface->format->Rmask == 0x000000ff) {
            LOGE("Image format GL_RGB");
            texture_format = GL_RGB;
        } else {
            LOGE("Image format unsupported");
        }
        texture_format = GL_RGB;
    } else {
        LOGE("warning: the image is not truecolor..  this will probably break\n");
        // this error should not go unhandled
    }*/

    // Have OpenGL generate a texture object handle for us
    glGenTextures( 1, &infos->texture );

    // Bind the texture
    glActiveTexture(GL_TEXTURE0);

    // Bind the texture object
    glBindTexture( GL_TEXTURE_2D, infos->texture );

    // Set the texture's stretching properties
    //glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    //glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

    // Edit the texture object's image data using the information SDL_Surface gives us
    glTexImage2D( GL_TEXTURE_2D, 0, 3, surface->w, surface->h, 0,
                    texture_format, GL_UNSIGNED_BYTE, surface->pixels );

    infos->width = surface->w;
    infos->height = surface->h;

    SDL_FreeSurface(surface);
    SDL_FreeSurface(osurface);
    return 1;
}

