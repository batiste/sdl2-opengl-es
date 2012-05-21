
#include "common.c"

int main(int argc, char** argv)
{
    init();

    GLuint lineProgram;
    lineProgram = initProgram("vertex-shader-1.vert", "line-shader-1.frag");
    CHECK_GL();


    GLuint textureProgram;
    textureProgram = initProgram("vertex-shader-1.vert", "texture-shader-1.frag");
    CHECK_GL();

    // load texture
    struct textureInfos texture;
    texture.filename = "SDL_logo.bmp";
    loadTexture(&texture);

    CHECK_GL();
    CHECK_SDL();

    glClearColor(0.1f, 0.5f, 1.0f, 0.0f);

    float x, y;

    // Main loop variables
    SDL_Event event;
    float theta = 0;
    then = SDL_GetTicks();
    next_time = SDL_GetTicks() + 16;
    int done = 0;


    // Main loop
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
        useProgram(textureProgram);
        CHECK_GL();

        x = 0;
        y = 0;
        int i;
        float nb_texture = 3;
        for(i=0; i<nb_texture; i++) {
            drawBufferTexture(&texture, -0.5 + i/nb_texture, y, frames / (float)(40 + i));
        }

        /*x = 2 * (mouse_x - (screen.w / 2.0)) / (float)screen.w;
        y = -2 * (mouse_y - (screen.h / 2.0)) / (float)screen.h;
        drawTexture(&texture, x, y, frames / 50.0);*/


        CHECK_GL();
        useProgram(lineProgram);
        CHECK_GL();

        // Load the vertex position
        GLfloat vertices[6] = {0.0, 0.0, 100.0, 100.0, 100.0, 0.0};

        //drawLine(vertices, 3);

        mouse_x_prev = mouse_x_prev + ((mouse_x - mouse_x_prev) / 10.0);
        mouse_y_prev = mouse_y_prev + ((mouse_y - mouse_y_prev) / 10.0);

        vertices[0] = 2 * (mouse_x_prev - (screen.w / 2.0));
        vertices[1] = -2 * (mouse_y_prev - (screen.h / 2.0));

        LOG("(%f %f)", vertices[0], vertices[1]);

        vertices[2] = 2 * (mouse_x - (screen.w / 2.0));
        vertices[3] = -2 * (mouse_y - (screen.h / 2.0));

        drawLine(vertices, 2);

        SDL_GL_SwapWindow(mainwindow);

        SDL_Delay(time_left());
        next_time += 16;
    }

    /* Clean up */
    return cleanup(0);
}
