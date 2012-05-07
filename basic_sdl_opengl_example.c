
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
        printf("memory alloc fails\n");
        exit(1);
    }
    /* copy the file into the buffer */
    if( 1!=fread( buffer , lSize, 1 , fp) ) {
        fclose(fp);
        free(buffer);
        printf("entire read fails\n");
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
            printf("Error compiling shader:\n%s\n", infoLog);
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
    if (nOfColors == 4) // contains an alpha channel
    {
        printf("Image %s has an alpha channel\n", infos->filename);
        if (surface->format->Rmask == 0x000000ff)
            texture_format = GL_RGBA;
        else
            texture_format = GL_BGRA;
    } else if (nOfColors == 3)     // no alpha channel
    {
        printf("Image %s does not have an alpha channel\n", infos->filename);
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

void drawTexture(struct textureInfos * infos) {
    float imagew_2 = infos->width / 2;
    float imageh_2 = infos->height / 2;
    glBindTexture( GL_TEXTURE_2D, infos->texture );
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

    // create the shaders
    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint programObject;
    GLint linked;
    // Load the vertex/fragment shaders
    vertexShader = loadShader(GL_VERTEX_SHADER, "shaders/vertex-shader-1.vert");
    checkGlError(__LINE__);
    fragmentShader = loadShader(GL_FRAGMENT_SHADER, "shaders/texture-shader-1.frag");
    checkGlError(__LINE__);
    programObject = glCreateProgram();
    if(programObject == 0) {
        printf("Unable to initialize the shader programm");
        return cleanup(0);
    }

    if(programObject == 0)
        return 0;

    checkGlError(__LINE__);
    glAttachShader(programObject, vertexShader);
    checkGlError(__LINE__);
    glAttachShader(programObject, fragmentShader);
    checkGlError(__LINE__);

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
            printf("Error linking program:\n%s\n", infoLog);
            free(infoLog);
        }
        glDeleteProgram(programObject);
        return 0;
    }

    checkGlError(__LINE__);
    // You need to 'use' the program before you can get it's uniforms.
    glUseProgram(programObject);
    checkGlError(__LINE__);

    GLuint gvPositionHandle = glGetAttribLocation(programObject, "a_position");
    // gvNormalHandle=glGetAttribLocation(gProgram,"a_normal");
    GLuint gvTexCoordHandle = glGetAttribLocation(programObject, "a_texCoord");
    GLuint gvSamplerHandle = glGetUniformLocation(programObject, "s_texture");

    //printf("a_position %d\n", gvPositionHandle);
    //printf("a_texCoord %d\n", gvTexCoordHandle);
    //printf("s_texture %d\n", gvSamplerHandle);

    // load texture
    GLuint textureid;
    struct textureInfos texture;
    texture.filename = "SDL_logo.bmp";
    loadTexture(&texture);
    glBindTexture( GL_TEXTURE_2D, texture.texture );

    checkGlError(__LINE__);
    checkSDLError(__LINE__);

    // setup the viewport
    glViewport( 0, 0, windowWidth, windowHeight );
    glClear( GL_COLOR_BUFFER_BIT );
    // Swap our back buffer to the front
    SDL_GL_SwapWindow(mainwindow);
    glClear(GL_COLOR_BUFFER_BIT);
    // enable blending
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    SDL_Event event;
    float theta = 0;
    int done = 0;
    then = SDL_GetTicks();
    float imagew_2 = texture.width / 2;
    float imageh_2 = texture.height / 2;

    glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
    checkGlError(__LINE__);
    GLfloat vVertices[] = {
        -0.75f, 0.75f, 0.0f, // Position 0
        //0.0f,1.0f,0.0f,
        0.0f, 0.0f, // TexCoord 0
        -.75f, -0.75f, 0.0f, // Position 1
        //0.0f,1.0f,0.0f,
        0.0f, 1.0f, // TexCoord 1
        .75f, -0.75f, 0.0f, // Position 2
        //0.0f,1.0f,0.0f,
        1.0f, 1.0f, // TexCoord 2
        .75f, 0.75f, 0.0f, // Position 3
        // 0.0f,1.0f,0.0f,
        1.0f, 0.0f // TexCoord 3
    };
    GLushort indices[] = { 0, 1, 2, 0, 2, 3 };
    GLsizei stride = 5 * sizeof(GLfloat); // 3 for position, 2 for texture

    while (!done) {
        ++frames;
        theta = theta + 0.1;
        while (SDL_PollEvent(&event)) {
            if(event.type == SDL_WINDOWEVENT_CLOSE || event.type == SDL_QUIT) {
                done = 1;
            }
        }

        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        checkGlError(__LINE__);

        // Load the vertex position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                        vVertices);
                        checkGlError(__LINE__);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                    vVertices+3);
        // Load the texture coordinate
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                        vVertices+6);

        checkGlError(__LINE__);

        glEnableVertexAttribArray(gvPositionHandle);
        //glEnableVertexAttribArray(gvNormalHandle);
        glEnableVertexAttribArray(gvTexCoordHandle);

        // Bind the texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture.texture);

        // Set the sampler texture unit to 0
        glUniform1i(gvSamplerHandle, 0);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);

        checkGlError(__LINE__);

        SDL_GL_SwapWindow(mainwindow);
        //SDL_Delay(1);
    }

    glDeleteTextures( 1, &textureid );
    return cleanup(0);
}
