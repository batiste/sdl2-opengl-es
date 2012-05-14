// Common functions

#include "SDL.h"

#define  LOG_TAG    "SDL"

#ifdef ANDROID
#include <jni.h>
#include <android/log.h>

#define  LOG(...)   __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__);
#define  LOGI(...)  __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__);
#define  LOGE(...)  __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__);
#define  ASSETS_DIR ""

#else

#define  LOG(...)   printf(__VA_ARGS__);printf("\n");
#define  LOGI(...)  printf(__VA_ARGS__);printf("\n");
#define  LOGE(...)  printf(__VA_ARGS__);printf("\n");
#define  ASSETS_DIR "assets/"

#endif

#define  CHECK_SDL(...)   _checkSDLError(__FILE__, __LINE__);
#define  CHECK_GL(...)   _checkGLError(__FILE__, __LINE__);

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

SDL_Window * mainwindow;   // Our window handle
SDL_GLContext maincontext; // Our opengl context handle
Uint32 then, now, frames;  // Used for FPS

float mouse_x, mouse_y;


#define TICK_INTERVAL    30

static Uint32 next_time;

Uint32 time_left(void)
{
    Uint32 now;
    now = SDL_GetTicks();
    if(next_time <= now)
        return 0;
    else
        return next_time - now;
}

// cleanup before quiting
static int
cleanup(int rc)
{
    // Print out some timing information
    now = SDL_GetTicks();
    if (now > then) {
        LOGE("%2.2f frames per second",
               ((double) frames * 1000) / (now - then));
    }
    if(maincontext)
        SDL_GL_DeleteContext(maincontext);
    if(mainwindow)
        SDL_DestroyWindow(mainwindow);
    SDL_Quit();
    exit(rc);
}

void
checkSDLError(int line)
{
    const char *error = SDL_GetError();
    if (*error != '\0')
    {
        LOG("SDL ERROR: %s", error);
        if (line != -1)
            LOG("at line: %i", line);
        SDL_ClearError();
    }
}

void
_checkSDLError(char * file, int line)
{
    const char *error = SDL_GetError();
    if (*error != '\0')
    {
        LOG("SDL ERROR: file:%s line:%d error:%s", file, line, error);
        SDL_ClearError();
    }
}

GLuint
checkGlError(int line)
{
    GLuint err = glGetError();
    if (err > 0 )  {
        LOG("GL ERROR(%d)", err);
        if (line != -1)
            LOG("at line: %i", line);
    }
    return err;
}

GLuint
_checkGLError(char * file, int line)
{
    GLuint err = glGetError();
    if (err > 0 )  {
        LOG("SDL ERROR: file:%s line:%d error:%d", file, line, err);
    }
    return err;
}


char * loadFile(const char * filename) {

    SDL_RWops * file;
    char * buffer;

    char filename_final[256] = "";
    strcpy(filename_final, ASSETS_DIR );
    strcat(filename_final, filename);

    LOG("open filename %s", filename_final);
    file = SDL_RWFromFile(filename_final, "rb");
    if (file==NULL) {
        LOG("Unable to open file");
        return 0;
    }
    LOG("Read success");

    SDL_RWseek(file, 0, SEEK_END);
    LOG("Seek");
    int finalPos = SDL_RWtell(file);
    LOG("Final Position (%d)", finalPos);
    SDL_RWclose(file);

    LOG("File closed");
    /* allocate memory for entire content */
    buffer = calloc( 1, sizeof(char) * (finalPos + 1));
    LOG("Memory allocated");

    file = SDL_RWFromFile(filename_final, "rb");
    LOG("SDL_RWFromFile");
    int n_blocks = SDL_RWread(file, buffer, 1, finalPos);
    LOG("Read the file");
    if(n_blocks < 0) {
        LOG("Unable to read any block");
    }
    if(n_blocks == 0) {
        LOG("No block read");
    }

    LOG("Block read %d", n_blocks);

    SDL_RWclose(file);
    LOG("Quit loadFile");
    return buffer;
}



GLuint
loadShader(GLenum type, const char * filename) {
    // Create a shader object, load the shader source, and
    // compile the shader.
    GLuint shader;
    GLint compiled;
    // Create the shader object
    LOG("glCreateShader");
    shader = glCreateShader(type);
    if(shader == 0) {
        LOG("Shader failed %d", type);
        return 0;
    }
    // Load the shader source
    const GLchar* buffer = (const GLchar*) loadFile(filename);
    LOG("glShaderSource Buffer \n%s", buffer);
    glShaderSource(shader, 1, &buffer, NULL);

    // Compile the shader
    LOG("Compile shader");
    glCompileShader(shader);
    //free(buffer);

    CHECK_GL();
    // Check the compile status
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    CHECK_GL();
    if(!compiled)
    {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if(infoLen > 1)
        {
            char* infoLog = malloc(sizeof(char) * infoLen);
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            LOGE("Error compiling shader:\n %s", infoLog);
            free(infoLog);
            exit(1);
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


void convertBGRAtoRGBA(char * bgra, int num)
{
    LOG("convert BGRA to RGBA");
    int i;
    int buffer;
    // inverse B and R
    for (i=0; i<(num * 4); i=i+4) {
        buffer = bgra[i];
        bgra[i] = bgra[i+2];
        bgra[i+2] = buffer;
    }
}

void convertBGRtoRGB(char * bgr, int num)
{
    LOG("convert BGR to RGB");
    int i;
    int buffer;
    // inverse B and R
    for (i=0; i<(num * 3); i=i+3) {
        buffer = bgr[i];
        bgr[i] = bgr[i+2];
        bgr[i+2] = buffer;
    }
}

int
loadTexture(struct textureInfos * infos) {

    // This surface will tell us the details of the image
    SDL_Surface * surface;
    GLenum texture_format;
    GLint  nOfColors;

    char filename_final[256] = "";
    strcpy( filename_final, ASSETS_DIR );
    strcat( filename_final, infos->filename );
    LOG("loadTexture %s", filename_final)

    if ( !(surface = SDL_LoadBMP(filename_final)) ) {
        LOG("SDL could not load %s: %s", filename_final, SDL_GetError());
        exit(0);
    }

    Uint32 rmask, gmask, bmask, amask;
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        rmask = 0xff000000;
        gmask = 0x00ff0000;
        bmask = 0x0000ff00;
        amask = 0x000000ff;
    #else
        rmask = 0x000000ff;
        gmask = 0x0000ff00;
        bmask = 0x00ff0000;
        amask = 0xff000000;
    #endif

    int max_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
    LOG("Max image size %d", max_size);
    if(surface->w > max_size || surface->h > max_size) {
        LOG("Image is too big, max size is %d", max_size);
        exit(0);
    }

    // Check that the image's width is a power of 2
    if ( (surface->w & (surface->w - 1)) != 0 ) {
        LOG("warning: %s width is not a power of 2", infos->filename);
    }

    // Also check if the height is a power of 2
    if ( (surface->h & (surface->h - 1)) != 0 ) {
        LOG("warning: %s height is not a power of 2", infos->filename);
    }

    // get the number of channels in the SDL surface
    // Openg ES only support GL_RGB and GL_RGBA
    nOfColors = surface->format->BytesPerPixel;
    int nbPixels = (surface->w * surface->h);
    LOG("surface->format->BytesPerPixel %d", nOfColors);
    LOG("surface->format->Rmask %d", surface->format->Rmask);

    if (nOfColors == 4) // contains an alpha channel
    {
        LOG("Image %s has an alpha channel", infos->filename);
        if (surface->format->Rmask == rmask) {
            LOG("Image format: GL_RGBA");
            texture_format = GL_RGBA;
        } else {
            convertBGRAtoRGBA(surface->pixels, nbPixels);
            texture_format = GL_RGBA;
        }
    } else if (nOfColors == 3) // no alpha channel
    {
        LOGE("Image %s does not have an alpha channel", infos->filename);
        if (surface->format->Rmask == rmask) {
            LOG("Image format: GL_RGB");
            texture_format = GL_RGB;
        } else {
            convertBGRtoRGB(surface->pixels, nbPixels);
            texture_format = GL_RGB;
        }
    } else {
        LOG("The image is not truecolor.");
        exit(0);
    }

    CHECK_GL();
    // glPixelStorei(GL_PACK_ALIGNMENT, 4);
    // Have OpenGL generate a texture object handle for us
    glGenTextures( 1, &infos->texture );
    CHECK_GL();

    // Bind the texture
    glActiveTexture(GL_TEXTURE0);
    CHECK_GL();

    // Bind the texture object
    glBindTexture( GL_TEXTURE_2D, infos->texture );
    CHECK_GL();

    // Set the texture's stretching properties
    // Those parameters are needed.
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    CHECK_GL();

    // internalformat specifies the color components in the texture. Must be same as format.
    // Edit the texture object's image data using the information SDL_Surface gives us
    glTexImage2D(GL_TEXTURE_2D, 0, texture_format, surface->w, surface->h, 0,
                    texture_format, GL_UNSIGNED_BYTE, surface->pixels );
    CHECK_GL();

    infos->width = surface->w;
    infos->height = surface->h;

    SDL_FreeSurface(surface);
    return 0;
}


#ifdef ANDROID

// Called before SDL_main() to initialize JNI bindings in SDL library
void SDL_Android_Init(JNIEnv* env, jclass cls);

// Library init
jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    LOG("JNI_OnLoad");
    return JNI_VERSION_1_4;
}

// Start up the SDL app
void Java_org_libsdl_app_SDLActivity_nativeInit(JNIEnv* env, jclass cls, jobject obj)
{
    LOG("Java_org_libsdl_app_SDLActivity_nativeInit");
    // This interface could expand with ABI negotiation, calbacks, etc.
    SDL_Android_Init(env, cls);

    // Run the application code!
    int status;
    char *argv[2];
    argv[0] = strdup("SDL_app");
    argv[1] = NULL;

    char * buffer;
    buffer = loadFile("vertex-shader-1.vert");
    free(buffer);

    status = SDL_main(1, argv);
    // Do not issue an exit or the whole application will terminate instead of just the SDL thread
    //exit(status);
}

// End the SDL app
void Java_org_libsdl_app_SDLActivity_nativeQuit(JNIEnv* env, jclass cls, jobject obj)
{
    LOG("Java_org_libsdl_app_SDLActivity_nativeQuit");
    cleanup(0);
    //exit(0);
}

// End the SDL app
void Java_org_libsdl_app_SDLActivity_onNativeKeyDown(JNIEnv* env, jclass cls, jobject obj)
{
    LOG("Java_org_libsdl_app_SDLActivity_onNativeKeyDown");
    cleanup(0);
}

void Java_org_libsdl_app_SDLActivity_onNativeTouch(
                                     JNIEnv* env, jclass jcls,
                                     jint touch_device_id_in, jint pointer_finger_id_in,
                                     jint action, jfloat x, jfloat y, jfloat p)
 {
    LOG("onNativeTouch (%f, %f)", x, y);
    mouse_x = x;
    mouse_y = y;
 }



#endif