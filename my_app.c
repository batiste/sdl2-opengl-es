
#include "common.c"

int main(int argc, char** argv)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) { /* Initialize SDL's Video subsystem */
        LOG("Unable to initialize SDL");
        return cleanup(0);
    }

    SDL_DisplayMode mode;
    SDL_GetDesktopDisplayMode(0, &mode);
    checkSDLError(__LINE__);
    LOG("Screen size %d %d", mode.w, mode.h);

    /* Create our window centered */
    mainwindow = SDL_CreateWindow("Simple texture moving", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        mode.w, mode.h, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!mainwindow) {/* Die if creation failed */
        LOG("Unable to create window");
        return cleanup(0);
    }
    SDL_SetWindowFullscreen(mainwindow, SDL_TRUE);

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

    GLuint gvMatrixHandle = glGetUniformLocation(programObject, "mvp_matrix");

    // load texture
    GLuint textureid;
    struct textureInfos texture;
    texture.filename = "SDL_logo.bmp";
    loadTexture(&texture);
    glBindTexture( GL_TEXTURE_2D, texture.texture );

    CHECK_GL();
    CHECK_SDL();

    // setup the viewport
    glViewport( 0, 0, mode.w, mode.h );
    glClear(GL_COLOR_BUFFER_BIT);
    // Swap our back buffer to the front
    SDL_GL_SwapWindow(mainwindow);
    glClear(GL_COLOR_BUFFER_BIT);
    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    SDL_Event event;
    float theta = 0;
    int done = 0;
    then = SDL_GetTicks();

    glClearColor(0.1f, 0.5f, 1.0f, 0.0f);
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

    float x, y;
    float pfIdentity[] =
    {
        0.5f,0.0f,0.0f,0.0f,
        0.0f,0.5f,0.0f,0.0f,
        0.0f,0.0f,1.0f,0.0f,
        0.0f,0.0f, 0.0f,1.0f
    };
    glEnableVertexAttribArray(gvPositionHandle);
    //glEnableVertexAttribArray(gvNormalHandle);
    glEnableVertexAttribArray(gvTexCoordHandle);
    // Set the sampler texture unit to 0
    glUniform1i(gvSamplerHandle, 0);

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

        // Bind the texture
        glActiveTexture(GL_TEXTURE0);
        CHECK_GL();
        glBindTexture(GL_TEXTURE_2D, texture.texture);
        CHECK_GL();

        x = cos(frames/100.0) / 2;
        y = sin(frames/100.0) / 2;
        pfIdentity[12] = x;
        pfIdentity[13] = y;
        glUniformMatrix4fv(gvMatrixHandle, 1, GL_FALSE, pfIdentity);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);

        pfIdentity[12] = y;
        pfIdentity[13] = x;
        glUniformMatrix4fv(gvMatrixHandle, 1, GL_FALSE, pfIdentity);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);

        SDL_GL_SwapWindow(mainwindow);
    }

    //glDeleteTextures(1, texture.texture);
    return cleanup(0);
}
