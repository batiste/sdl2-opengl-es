// Common functions

#include "SDL.h"
#include "lodepng.h"
#include "lodepng.c"

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
float _mouse_x = 0.0;
float _mouse_y = 0.0;
static Uint32 next_time;

GLuint gvPositionHandle;   // shader handler
GLuint gvTexCoordHandle;
GLuint gvSamplerHandle;
GLuint gvMatrixHandle;
GLuint gvRotateHandle;

struct { // screen size structure
    int w;
    int h;
} screen;

float mvp_matrix[] =
{
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

float rotate_matrix[] =
{
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

float ortho_matrix[] =
{
    1.0f, 0.0f, 0.0f, -1.0f,
    0.0f, 1.0, 0.0f, 1.0f,
    0.0f, 0.0f, -1.0f, -1.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};


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
    // TODO: delete texture objects

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
        LOG("GL ERROR: file:%s line:%d error:%d", file, line, err);
    }
    return err;
}


char * loadFile(const char * filename, int * size) {

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
    *size = finalPos;

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
        LOG("Shader creation failed %d", type);
        return 0;
    }
    // Load the shader source
    int size;
    const GLchar* buffer = (const GLchar*) loadFile(filename, &size);
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
            LOG("Error compiling shader:\n %s", infoLog);
            free(infoLog);
            return cleanup(1);
        }
        glDeleteShader(shader);
        return 0;
    }
    CHECK_GL();
    return shader;
}

GLuint initProgram(const char * vertexFile, const char * fragmentFile) {
    // create the shaders and the program
    GLuint vertexShader;
    GLuint fragmentShader;
    GLint linked;

    vertexShader = loadShader(GL_VERTEX_SHADER, vertexFile);
    CHECK_GL();
    fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentFile);
    CHECK_GL();
    GLuint programObject = glCreateProgram();
    if(programObject == 0) {
        LOGE("Unable to initialize the shader programm");
        return cleanup(1);
    }

    CHECK_GL();
    glAttachShader(programObject, vertexShader);
    CHECK_GL();
    glAttachShader(programObject, fragmentShader);
    CHECK_GL();

    // Link the program
    glLinkProgram(programObject);

    // Check the link status
    glGetProgramiv(programObject, GL_LINK_STATUS, &linked);
    if(!linked)
    {
        GLint infoLen = 0;
        glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);
        if(infoLen > 1)
        {
            char* infoLog = malloc(sizeof(char) * infoLen);
            glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
            LOG("Error linking program:\n%s\n", infoLog);
            free(infoLog);
            return cleanup(1);
        }
        glDeleteProgram(programObject);
        return 0;
    }

    CHECK_GL();

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    CHECK_GL();

    LOG("Shader completed")
    return programObject;
}

int useProgram(GLuint programObject) {

    CHECK_GL();
    // You need to 'use' the program before you can get it's uniforms.
    glUseProgram(programObject);
    CHECK_GL();

    gvPositionHandle = glGetAttribLocation(programObject, "a_position");
    // gvNormalHandle=glGetAttribLocation(gProgram,"a_normal");
    gvTexCoordHandle = glGetAttribLocation(programObject, "a_texCoord");
    gvSamplerHandle = glGetUniformLocation(programObject, "s_texture");

    gvMatrixHandle = glGetUniformLocation(programObject, "mvp_matrix");
    gvRotateHandle = glGetUniformLocation(programObject, "rotate_matrix");

    glEnableVertexAttribArray(gvPositionHandle);
    //glEnableVertexAttribArray(gvNormalHandle);
    glEnableVertexAttribArray(gvTexCoordHandle);
    CHECK_GL();
    // Set the sampler texture unit to 0
    // glUniform1i(gvSamplerHandle, 0);

}

struct waveInfos {
    SDL_AudioSpec spec;
    Uint8   *sound;         /* Pointer to wave data */
    Uint32   soundlen;      /* Length of wave data */
    int      soundpos;      /* Current play position */
};

loadSound(char * filename, struct waveInfos * sound) {
    // Load the WAV
    char filename_final[256] = "";
    strcpy(filename_final, ASSETS_DIR);
    strcat(filename_final, filename);
    LOG("loadSound %s", filename_final);

    if ( SDL_LoadWAV(filename_final, &sound->spec, &sound->sound,
        &sound->soundlen) == NULL ) {
        LOG("Couldn't load %s: %s\n",
                        "assets/sword2.wav", SDL_GetError());
        exit(1);
    }
}



struct ImageData {
    unsigned char * pixels;
    int width;
    int height;
    int nbColors;
    char * filename;
    GLuint type;
};

struct TextureInfos {
    float x;
    float y;
    float angle;
    // pivot point
    float px;
    float py;
    // velocity
    float vx;
    float vy;
    float vr;
    //char* filename;
    GLuint texture;
    int width;
    int height;
    GLfloat * vertices;
    int verticesSize;
    GLuint vertexBuffer;
    GLuint indexBuffer;
    GLushort * indices;
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

void loadPNG(struct ImageData * data)
{
    unsigned error;
    int size;
    char * buffer;
    buffer = loadFile(data->filename, &size);

    error = lodepng_decode32(&data->pixels, &data->width, &data->height, buffer, size);
    if(error) {
        LOG("error %u: %s\n", error, lodepng_error_text(error));
        exit(1);
    }
    checkImageDimension(data);

    data->nbColors = 4;
    data->type = GL_RGBA;
}

void loadBMP(struct ImageData * data)
{
    // This surface will tell us the details of the image
    SDL_Surface * surface;
    GLenum texture_format;
    GLint  nOfColors;

    char filename_final[256] = "";
    strcpy(filename_final, ASSETS_DIR);
    strcat(filename_final, data->filename);
    LOG("loadBMP %s", filename_final)

    if ( !(surface = SDL_LoadBMP(filename_final)) ) {
        LOG("SDL could not load %s: %s", filename_final, SDL_GetError());
        exit(0);
    }

    data->width = surface->w;
    data->height = surface->h;
    checkImageDimension(data);

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


    // get the number of channels in the SDL surface
    // Openg ES only support GL_RGB and GL_RGBA
    data->nbColors =  surface->format->BytesPerPixel;
    int nbPixels = (data->width * data->height);
    LOG("surface->format->BytesPerPixel %d", data->nbColors);
    LOG("surface->format->Rmask %d", surface->format->Rmask);

    if (data->nbColors == 4) // contains an alpha channel
    {
        LOG("Image %s has an alpha channel", data->filename);
        if (surface->format->Rmask == rmask) {
            LOG("Image format: GL_RGBA");
            data->type = GL_RGBA;
        } else {
            convertBGRAtoRGBA(surface->pixels, nbPixels);
            data->type = GL_RGBA;
        }
    } else if (data->nbColors == 3) // no alpha channel
    {
        LOGE("Image %s does not have an alpha channel", data->filename);
        if (surface->format->Rmask == rmask) {
            LOG("Image format: GL_RGB");
            data->type = GL_RGB;
        } else {
            convertBGRtoRGB(surface->pixels, nbPixels);
            data->type = GL_RGB;
        }
    } else {
        LOG("The image is not truecolor.");
        exit(1);
    }

    int buffer_size = sizeof(unsigned char) * nbPixels * data->nbColors;
    data->pixels = malloc(buffer_size);
    data->pixels = memcpy(data->pixels, surface->pixels, buffer_size);

    SDL_FreeSurface(surface);
}


int checkImageDimension(struct ImageData * data) {
    int max_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
    LOG("Max image size %d", max_size);
    if(data->width > max_size || data->height > max_size) {
        LOG("Image is too big (%d, %d)", data->width, data->height);
        exit(0);
    }

    // Check that the image's width is a power of 2
    if ( (data->width & (data->width - 1)) != 0 ) {
        LOG("warning: %s width is not a power of 2", data->filename);
    }

    // Also check if the height is a power of 2
    if ( (data->height & (data->height - 1)) != 0 ) {
        LOG("warning: %s height is not a power of 2", data->filename);
    }
}

int
loadTexture(struct TextureInfos * infos, struct ImageData * data, float xscale, float yscale) {

    CHECK_GL();
    // glPixelStorei(GL_PACK_ALIGNMENT, 4);
    // Have OpenGL generate a texture object handle for us
    glGenTextures( 1, &infos->texture );
    CHECK_GL();

    // Bind the texture
    glActiveTexture(GL_TEXTURE0);
    CHECK_GL();

    // Bind the texture object
    glBindTexture(GL_TEXTURE_2D, infos->texture);
    CHECK_GL();

    // Set the texture's stretching properties
    // Those parameters are needed.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CHECK_GL();

    // internalformat specifies the color components in the texture. Must be same as format.
    // Edit the texture object's image data using the information SDL_Surface gives us
    glTexImage2D(GL_TEXTURE_2D, 0, data->type, data->width, data->height, 0,
                    data->type, GL_UNSIGNED_BYTE, data->pixels );
    CHECK_GL();

    infos->width = xscale * data->width;
    infos->height = yscale * data->height;

    // init values
    infos->x = 0;
    infos->y = 0;
    infos->px = 0;
    infos->py = 0;
    infos->vx = 0;
    infos->vy = 0;

    float hw = infos->width / 2.0;
    float hh = infos->height / 2.0;

    CHECK_GL();

    infos->verticesSize = 4;
    infos->vertices = malloc(20 * sizeof(GLfloat));

    // Position 0
    infos->vertices[0] = -hw;
    infos->vertices[1] = hh;
    infos->vertices[2] = 0.0f;

    // TexCoord 0
    infos->vertices[3] = 0.0f;
    infos->vertices[4] = 0.0f;

    // Position 1
    infos->vertices[5] = -hw;
    infos->vertices[6] = -hh;
    infos->vertices[7] = 0.0f;

    // TexCoord 1
    infos->vertices[8] = 0.0f;
    infos->vertices[9] = 1.0f;

    // Position 2
    infos->vertices[10] = hw;
    infos->vertices[11] = -hh;
    infos->vertices[12] = 0.0f;

    // TexCoord 2
    infos->vertices[13] = 1.0f;
    infos->vertices[14] = 1.0f;

    // Position 4
    infos->vertices[15] = hw;
    infos->vertices[16] = hh;
    infos->vertices[17] = 0.0f;

    // TexCoord 3
    infos->vertices[18] = 1.0f;
    infos->vertices[19] = 0.0f;

    // buffers
    glGenBuffers(1, &infos->vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, infos->vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, 20 * sizeof(GLfloat), infos->vertices, GL_STATIC_DRAW);


    infos->indices = malloc(6 * sizeof(GLshort));
    infos->indices[0] = 0;
    infos->indices[1] = 1;
    infos->indices[2] = 2;
    infos->indices[3] = 0;
    infos->indices[4] = 2;
    infos->indices[5] = 3;

    glGenBuffers(1, &infos->indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, infos->indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLushort), infos->indices, GL_STATIC_DRAW);
    CHECK_GL();

    return 0;
}


int drawTexture(struct TextureInfos * texture, float x, float y, float angle) {

    // Specifies the byte offset between consecutive generic vertex attributes.
    // If stride is 0, the generic vertex attributes are understood to be tightly packed in the array
    GLsizei stride = 5 * sizeof(GLfloat); // 3 for position, 2 for texture

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Load the vertex position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
        texture->vertices);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
        texture->vertices+3);

    // Load the texture coordinate
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
        texture->vertices+6);

    // Bind the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->texture);

    // matrix transformations
    mvp_matrix[12] = 2.0 * x / (float)screen.w;
    mvp_matrix[13] = 2.0 * y / (float)screen.h;

    rotate_matrix[0] = cos(angle);
    rotate_matrix[1] = sin(angle);
    rotate_matrix[4] = -rotate_matrix[1];
    rotate_matrix[5] = rotate_matrix[0];

    glUniformMatrix4fv(gvRotateHandle, 1, GL_FALSE, rotate_matrix);
    glUniformMatrix4fv(gvMatrixHandle, 1, GL_FALSE, mvp_matrix);

    glDrawElements(GL_TRIANGLES, ((texture->verticesSize - 2) * 3), GL_UNSIGNED_SHORT, texture->indices);

    CHECK_GL();
}

int drawBufferTexture(struct TextureInfos * texture, float x, float y, float angle) {

    // Specifies the byte offset between consecutive generic vertex attributes.
    // If stride is 0, the generic vertex attributes are understood to be tightly packed in the array
    GLsizei stride = 5 * sizeof(GLfloat); // 3 for position, 2 for texture

    glBindBuffer(GL_ARRAY_BUFFER, texture->vertexBuffer);

    //glEnableVertexAttribArray(gvPositionHandle);
    //glEnableVertexAttribArray(gvTexCoordHandle);

    // Load the vertex position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (const GLvoid *)(3 * sizeof(GLfloat)));

    // Load the texture coordinate
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (const GLvoid *)(6 * sizeof(GLfloat)));

    // Bind the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->texture);

    // matrix transformations
    mvp_matrix[12] = 2.0 * x / (float)screen.w;
    mvp_matrix[13] = 2.0 * y / (float)screen.h;

    rotate_matrix[0] = cos(angle);
    rotate_matrix[1] = sin(angle);
    rotate_matrix[4] = -rotate_matrix[1];
    rotate_matrix[5] = rotate_matrix[0];

    glUniformMatrix4fv(gvRotateHandle, 1, GL_FALSE, rotate_matrix);
    glUniformMatrix4fv(gvMatrixHandle, 1, GL_FALSE, mvp_matrix);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, texture->indexBuffer);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    //CHECK_GL();
}

GLfloat * transformTexture(struct TextureInfos * texture, float tx, float ty, float angle) {
    // transform all the vertices of the current textureInfos object

    int i, j;
    float x, y, new_x, new_y;
    int size = texture->verticesSize * 5;

    // useful for basic caculation
    texture->x += tx;
    texture->y += ty;

    float cos_a = cos(angle);
    float sin_a = sin(angle);

    // rotate and translate the vertices
    for(i=0; i<size; i=i+5) {
        // back to the origin
        x = texture->vertices[i+0] - texture->px;
        y = texture->vertices[i+1] - texture->py;

        // rotate point
        float new_x = x * cos_a - y * sin_a;
        float new_y = x * sin_a + y * cos_a;

        // translate back from pivot and add translation
        new_x = new_x + texture->px + tx;
        new_y = new_y + texture->py + ty;

        texture->vertices[i+0] = new_x;
        texture->vertices[i+1] = new_y;
    }
}


int drawLines(GLfloat * vertices, int nbPoints) {

    CHECK_GL();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    CHECK_GL();
    glEnable(GL_BLEND);
    CHECK_GL();
    //glEnable(GL_LINE_SMOOTH);
    glLineWidth(5.0f);
    CHECK_GL();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    CHECK_GL();

    // Load the vertex position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0,
        vertices);
    CHECK_GL();

    // matrix transformations
    mvp_matrix[12] = 0;
    mvp_matrix[13] = 0;

    rotate_matrix[0] = cos(0);
    rotate_matrix[1] = sin(0);
    rotate_matrix[4] = -rotate_matrix[1];
    rotate_matrix[5] = rotate_matrix[0];

    glUniformMatrix4fv(gvRotateHandle, 1, GL_FALSE, rotate_matrix);
    glUniformMatrix4fv(gvMatrixHandle, 1, GL_FALSE, mvp_matrix);

    glEnableVertexAttribArray(0);

    glDrawArrays(GL_LINE_STRIP, 0, nbPoints);

    CHECK_GL();
}


int drawLinesFromVertices(GLfloat * vertices, int nbPoints) {

    CHECK_GL();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    CHECK_GL();
    glEnable(GL_BLEND);
    CHECK_GL();
    //glEnable(GL_LINE_SMOOTH);
    glLineWidth(10.0f);
    CHECK_GL();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    CHECK_GL();

    GLsizei stride = 5 * sizeof(GLfloat); // 3 for position, 2 for texture
    // Load the vertex position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride,
        vertices);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride,
        vertices);
    CHECK_GL();

    // matrix transformations
    mvp_matrix[12] = 0;
    mvp_matrix[13] = 0;

    rotate_matrix[0] = cos(0);
    rotate_matrix[1] = sin(0);
    rotate_matrix[4] = -rotate_matrix[1];
    rotate_matrix[5] = rotate_matrix[0];

    glUniformMatrix4fv(gvRotateHandle, 1, GL_FALSE, rotate_matrix);
    glUniformMatrix4fv(gvMatrixHandle, 1, GL_FALSE, mvp_matrix);

    glEnableVertexAttribArray(0);

    glDrawArrays(GL_LINE_STRIP, 0, nbPoints);

    CHECK_GL();
}

int drawPoints(GLfloat * vertices, int nbPoints) {

    CHECK_GL();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    CHECK_GL();
    glEnable(GL_BLEND);
    CHECK_GL();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    CHECK_GL();

    // Load the vertex position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0,
        vertices);
    CHECK_GL();

    // matrix transformations
    mvp_matrix[12] = 0;
    mvp_matrix[13] = 0;

    rotate_matrix[0] = cos(0);
    rotate_matrix[1] = sin(0);
    rotate_matrix[4] = -rotate_matrix[1];
    rotate_matrix[5] = rotate_matrix[0];

    glUniformMatrix4fv(gvRotateHandle, 1, GL_FALSE, rotate_matrix);
    glUniformMatrix4fv(gvMatrixHandle, 1, GL_FALSE, mvp_matrix);

    glEnableVertexAttribArray(0);
    glDrawArrays(GL_POINTS, 0, nbPoints);

    CHECK_GL();
}


int init() {
    // Init the window, the GL context
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) { /* Initialize SDL's Video subsystem */
        LOG("Unable to initialize SDL");
        return cleanup(0);
    }
    SDL_DisplayMode mode;
    SDL_GetDesktopDisplayMode(0, &mode);
    CHECK_SDL();

    #ifdef ANDROID
    screen.w = mode.w;
    screen.h = mode.h;
    #else
    screen.w = (int)(mode.h - 200) / 1.5;
    screen.h = mode.h - 200;
    #endif

    float zoom = 1.0f;
    // we want the top left corner to be the reference
    mvp_matrix[0] = (zoom * 2.0) / (float)screen.w;
    mvp_matrix[5] = (zoom * 2.0) / (float)screen.h;

    // Create our window centered
    mainwindow = SDL_CreateWindow("Simple texture moving", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        screen.w, screen.h, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!mainwindow) {
        // Die if creation failed
        LOG("Unable to create window");
        return cleanup(0);
    }
    #ifdef ANDROID
    SDL_SetWindowFullscreen(mainwindow, SDL_TRUE);
    #endif

    CHECK_SDL();

    // Create our opengl context and attach it to our window
    maincontext = SDL_GL_CreateContext(mainwindow);
    CHECK_SDL();
    if (!maincontext) {
        LOG("Unable to create GL context");
        return cleanup(0);
    }

    // setup the viewport
    glViewport(0, 0, screen.w, screen.h);
    glClear(GL_COLOR_BUFFER_BIT);
    // Swap our back buffer to the front
    SDL_GL_SwapWindow(mainwindow);
    glClear(GL_COLOR_BUFFER_BIT);
    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


struct waveInfos wave;

// The function to call when the audio device needs more data
void SDLCALL fill_audio(void *userdata, Uint8 *stream, int len)
{
    Uint8 *waveptr;
    int    waveleft;

    /* Set up the pointers */
    waveptr = wave.sound + wave.soundpos;
    waveleft = wave.soundlen - wave.soundpos;

    // Finish the sound
    while ( waveleft <= len ) {
        SDL_memcpy(stream, waveptr, waveleft);
        stream += waveleft;
        len -= waveleft;
        waveptr = wave.sound;
        waveleft = wave.soundlen;
        wave.soundpos = 0;
    }
    SDL_memcpy(stream, waveptr, len);
    wave.soundpos += len;
}

void playSound() {
    loadSound("sword2.wav", &wave);

    /* Initialize fill_audio() variables */
    SDL_AudioSpec wav_spec_obtained;
    wave.spec.callback = fill_audio;

    if ( SDL_OpenAudio(&wave.spec, &wav_spec_obtained) < 0 ) {
        LOG("Could not open audio: %s\n", SDL_GetError());
        SDL_FreeWAV(wave.sound);
        exit(1);
    }
    SDL_PauseAudio(0);

    /* Let the audio run */
    LOG("Using audio driver: %s\n", SDL_GetAudioDriver(0));
    /*while ( (SDL_GetAudioStatus() == SDL_AUDIO_PLAYING) )
        SDL_Delay(1000);*/
}


int getMouse(int* x, int* y) {
    // instead of top left reference we use the center of
    // the screen
    int hw = screen.w / 2.0;
    int hh = screen.h / 2.0;
    #ifdef ANDROID
    *(x) = (int)_mouse_x - hw;
    *(y) = -((int)_mouse_y - hh);
    #else
    int _x, _y;
    SDL_GetMouseState(&_x, &_y);
    *(x) =  _x - hw;
    *(y) = -(_y - hh);
    #endif
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
    int size;
    buffer = loadFile("vertex-shader-1.vert", &size);
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

// // End the SDL app
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

    //float hypo = sqrt((mouse_x-mouse_x_prev)*(mouse_x-mouse_x_prev) + (mouse_y-mouse_y_prev)*(mouse_y-mouse_y_prev));
    _mouse_x = x;
    _mouse_y = y;
 }



#endif