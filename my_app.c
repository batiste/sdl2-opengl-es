
#include "common.c"

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
        LOGE("%2.2f frames per second",
               ((double) frames * 1000) / (now - then));
    }
    if(maincontext)
        SDL_GL_DeleteContext(maincontext);
    if(mainwindow)
        SDL_DestroyWindow(mainwindow);
    SDL_Quit();
    exit(0);
}


int main(int argc, char** argv)
{
    int windowWidth = 512;
    int windowHeight = 600;

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) { /* Initialize SDL's Video subsystem */
        LOG("Unable to initialize SDL");
        return cleanup(0);
    }

    /* Create our window centered at 512x512 resolution */
    mainwindow = SDL_CreateWindow("Simple rotating texture", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        windowWidth, windowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!mainwindow) {/* Die if creation failed */
        LOG("Unable to create window");
        return cleanup(0);
    }

    checkSDLError(__LINE__);

    /* Create our opengl context and attach it to our window */
    maincontext = SDL_GL_CreateContext(mainwindow);
    checkSDLError(__LINE__);

    if (!maincontext) {
        return cleanup(0);
    }

    // create the shaders
    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint programObject;
    GLint linked;

    vertexShader = loadShader(GL_VERTEX_SHADER, "vertex-shader-1.vert");
    CHECK_GL();
    fragmentShader = loadShader(GL_FRAGMENT_SHADER, "texture-shader-1.frag");
    CHECK_GL();
    programObject = glCreateProgram();
    if(programObject == 0) {
        LOGE("Unable to initialize the shader programm");
        return cleanup(0);
    }

    if(programObject == 0)
        return 0;

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
            LOGE("Error linking program:\n%s\n", infoLog);
            free(infoLog);
        }
        glDeleteProgram(programObject);
        return 0;
    }

    CHECK_GL();
    // You need to 'use' the program before you can get it's uniforms.
    glUseProgram(programObject);
    CHECK_GL();

    GLuint gvPositionHandle = glGetAttribLocation(programObject, "a_position");
    // gvNormalHandle=glGetAttribLocation(gProgram,"a_normal");
    GLuint gvTexCoordHandle = glGetAttribLocation(programObject, "a_texCoord");
    GLuint gvSamplerHandle = glGetUniformLocation(programObject, "s_texture");

    //LOGE("a_position %d\n", gvPositionHandle);
    //LOGE("a_texCoord %d\n", gvTexCoordHandle);
    //LOGE("s_texture %d\n", gvSamplerHandle);

    // load texture
    GLuint textureid;
    struct textureInfos texture;
    texture.filename = "SDL_logo_64.bmp";
    loadTexture(&texture);
    glBindTexture( GL_TEXTURE_2D, texture.texture );

    GLuint wtex;
    wtex = createWhiteTexture(wtex);

    CHECK_GL();
    CHECK_SDL();

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

    glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
    CHECK_GL();
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
        CHECK_GL();

        // Load the vertex position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                        vVertices);
                        CHECK_GL();
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                    vVertices+3);
        // Load the texture coordinate
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                        vVertices+6);

        CHECK_GL();

        glEnableVertexAttribArray(gvPositionHandle);
        //glEnableVertexAttribArray(gvNormalHandle);
        glEnableVertexAttribArray(gvTexCoordHandle);
        // Set the sampler texture unit to 0
        glUniform1i(gvSamplerHandle, 0);

        // Bind the texture
        glActiveTexture(GL_TEXTURE0);
        CHECK_GL();
        glBindTexture(GL_TEXTURE_2D, texture.texture);
        CHECK_GL();

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
        SDL_GL_SwapWindow(mainwindow);
        SDL_Delay(2000);

        CHECK_GL();
        glBindTexture(GL_TEXTURE_2D, wtex);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
        CHECK_GL();

        SDL_GL_SwapWindow(mainwindow);
        SDL_Delay(3000);


        done = 1;
    }

    //glDeleteTextures( 1, &textureid );
    glDeleteTextures( 1, &wtex );
    return cleanup(0);
}
