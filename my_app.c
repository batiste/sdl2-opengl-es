
#include "common.c"

int main(int argc, char** argv)
{
    init();
    GLuint programObject;
    programObject = initProgram();

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

        x = 0;
        y = 0;
        int i;
        for(i=0; i<50; i++) {
            drawTexture(&texture, -0.5 + i/50.0, y, frames / (float)(50 + i));
        }



        x = 2 * (mouse_x - (screen.w / 2.0)) / (float)screen.w;
        y = -2 * (mouse_y - (screen.h / 2.0)) / (float)screen.h;
        drawTexture(&texture, x, y, frames / 50.0);

        SDL_GL_SwapWindow(mainwindow);

        SDL_Delay(time_left());
        next_time += 16;
    }

    /* Clean up */
    //glDeleteTextures(1, texture.texture);
    return cleanup(0);
}
