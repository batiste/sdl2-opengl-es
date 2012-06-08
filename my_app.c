
#include "common.c"
#include "math.c"

int main(int argc, char** argv)
{
    init();

    GLuint lineProgram;
    lineProgram = initProgram("vertex-shader-1.vert", "line-shader-1.frag");
    CHECK_GL();

    GLuint pointProgram;
    pointProgram = initProgram("vertex-shader-1.vert", "point-shader-1.frag");
    CHECK_GL();


    GLuint textureProgram;
    textureProgram = initProgram("vertex-shader-1.vert", "texture-shader-1.frag");
    CHECK_GL();

    // load texture
    struct ImageData png;
    png.filename = "bamboo.png";
    loadPNG(&png);

    struct TextureInfos texture;
    loadTexture(&texture, &png, 1.0, 1.0);
    transformTexture(&texture, 0.0, 0.0, 0.0);


    // load texture
    struct ImageData background;
    background.filename = "background.png";
    loadPNG(&background);

    struct TextureInfos back1;
    float scalex = screen.w / (float)background.width;
    float scaley = screen.h / (float)background.height + .01;

    loadTexture(&back1, &background, scalex, scaley);
    struct TextureInfos back2;
    loadTexture(&back2, &background, scalex, scaley);

    CHECK_GL();
    CHECK_SDL();

    glClearColor(0.5f, 0.7f, 1.0f, 0.0f);

    float x, y;

    // Main loop variables
    SDL_Event event;
    float theta = 0;
    then = SDL_GetTicks();
    next_time = SDL_GetTicks() + 16;
    int done = 0;

    float vx = 0;
    float vy = 0;
    int i = 0;
    int mouse_x = 0;
    int mouse_y = 0;
    int mouse_x_prev = 0;
    int mouse_y_prev = 0;
    int lastCut = 0;

    struct TextureInfos *pieces[100];
    int nbPieces = 0;

    transformTexture(&back2, 0, -back2.height, 0);

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

        if(back2.y > (back2.height / 2.0 + screen.h / 2.0))
            transformTexture(&back2, 0, -2 * back2.height, 0);
        transformTexture(&back2, 0, 5, 0);
        drawTexture(&back2, 0, 0, 0);

        if(back1.y > (back2.height / 2.0 + screen.h / 2.0))
            transformTexture(&back1, 0, -2 * back2.height, 0);
        transformTexture(&back1, 0, 5, 0);
        drawTexture(&back1, 0, 0, 0);


        transformTexture(&texture, 0, 0, 0.01);
        drawTexture(&texture, 0, 0, 0);

        CHECK_GL();
        useProgram(lineProgram);
        CHECK_GL();

        // Load the vertex position
        GLfloat vertices[6];

        getMouse(&mouse_x, &mouse_y);

        vx = (vx / 1.5) + ((mouse_x - mouse_x_prev) / 30.0);
        vy = (vy / 1.5) + ((mouse_y - mouse_y_prev) / 30.0);
        mouse_y_prev = mouse_y_prev + vy;
        mouse_x_prev = mouse_x_prev + vx;

        vertices[0] = mouse_x_prev;
        vertices[1] = mouse_y_prev;
        vertices[2] = mouse_x;
        vertices[3] = mouse_y;
        drawLines(vertices, 2);

        struct Line line;
        line.ax = vertices[0];
        line.ay = vertices[1];
        line.bx = vertices[2];
        line.by = vertices[3];
        GLfloat points[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};


        int nbPoints = 0;
        if(nbPoints = find_intersect_points(&texture, &line, points)) {
            useProgram(pointProgram);
            drawLines(points, nbPoints);
            if(nbPoints > 1 && nbPieces < 95 && lastCut < frames) {

                lastCut = frames + 10;

                struct TextureInfos * texture1 = malloc(sizeof(struct TextureInfos));
                struct TextureInfos * texture2 = malloc(sizeof(struct TextureInfos));

                split_vertex(&texture, &line, texture1, texture2);

                pieces[nbPieces] = texture1;
                nbPieces = nbPieces + 1;
                pieces[nbPieces] = texture2;
                nbPieces = nbPieces + 1;

                texture1->vx = 2.0;
                texture2->vx = -2.0;

                texture1->vy = -2.0;
                texture2->vy = -2.0;

                texture1->vr = 0.01;
                texture2->vr = -0.01;

                /*useProgram(lineProgram);
                drawLinesFromVertices(texture1->vertices, texture1->verticesSize);
                drawLinesFromVertices(texture2->vertices, texture2->verticesSize);*/
            }


            useProgram(pointProgram);
            drawLines(points, nbPoints);
        }

        struct TextureInfos * myTexture;
        for(i=0; i<nbPieces; i++) {

            myTexture = pieces[i];
            if(myTexture == NULL)
                continue;

            // TODO: freee and cleanup
            if(myTexture->y < -screen.h) {
                pieces[i] = NULL;
            }

            // gravity
            myTexture->vy = myTexture->vy - 0.25;

            transformTexture(myTexture, myTexture->vx, myTexture->vy, myTexture->vr);

            useProgram(textureProgram);
            drawTexture(myTexture, 0, 0, 0);

        }


        SDL_GL_SwapWindow(mainwindow);

        SDL_Delay(time_left());
        next_time += 16;
    }

    /* Clean up */
    return cleanup(0);
}
