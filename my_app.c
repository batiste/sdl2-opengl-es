
#include "common.c"
#include "intersection.c"

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
    ImageData png;
    png.filename = "bamboo-2.png";
    loadPNG(&png);

    TextureInfos texture;
    loadTexture(&texture, &png, 1.0, 1.0);
    transformTexture(&texture, 0.0, 0.0, 0.0);


    // load texture
    ImageData background;
    background.filename = "bamboo-b2.png";
    loadPNG(&background);

    TextureInfos back;
    float scalex = screen.w / (float)background.width;
    loadTexture(&back, &background, scalex, scalex);
    transformTexture(&back, 0, -back.height / 2.0 + screen.h / 2.0, 0);


    CHECK_GL();
    CHECK_SDL();

    glClearColor(110. / 255., 127. / 255., 91. / 255., 0.0f);

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

    GenericList pieces = { 0, NULL, NULL };

    transformTexture(&texture, 0, screen.h / 3.0, 0.00);

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


        float background_bottom_position = (back.height - screen.h) / 2.0;
        float floor_position = -((screen.h / 2.0) -30 + 2 * (background_bottom_position - back.y));

        if(back.y < background_bottom_position) {
            transformTexture(&back, 0, 4, 0);
            transformTexture(&texture, 0, 0, 0.01);
        }
        else if(texture.y > floor_position) {
            transformTexture(&texture, 0, -4, 0);
            transformTexture(&texture, 0, 0, 0.01);
        }

        drawTexture(&back, 0, 0, 0);
        drawTexture(&texture, 0, 0, 0);

        CHECK_GL();
        useProgram(lineProgram);
        CHECK_GL();

        // Load the vertex position
        GLfloat vertices[6];

        getMouse(&mouse_x, &mouse_y);

        ListElement *el;

        MousePosition *pos1 = 0;
        MousePosition *pos2 = 0;
        for(el = mouse_buffer.first; el != NULL; el=el->next) {
            pos2 = pos1;
            pos1 = (MousePosition *) el->data;
            if(pos1 &&  pos2) {
                vertices[0] = pos1->x;
                vertices[1] = pos1->y;
                vertices[2] = pos2->x;
                vertices[3] = pos2->y;
                drawLines(vertices, 2);
            }
        }


        vx = (vx / 1.5) + ((mouse_x - mouse_x_prev) / 30.0);
        vy = (vy / 1.5) + ((mouse_y - mouse_y_prev) / 30.0);
        mouse_y_prev = mouse_y_prev + vy;
        mouse_x_prev = mouse_x_prev + vx;

        vertices[0] = mouse_x_prev;
        vertices[1] = mouse_y_prev;
        vertices[2] = mouse_x;
        vertices[3] = mouse_y;
        //drawLines(vertices, 2);

        MousePosition *first = 0;
        MousePosition *last = 0;
        first = (MousePosition *)mouse_buffer.first->data;
        last = (MousePosition *)mouse_buffer.last->data;

        struct Line line;
        GLfloat points[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        if(first != last) {
            line.ax = first->x;
            line.ay = first->y;
            line.bx = last->x;
            line.by = last->y;
        }

        int nbPoints = 0;
        if(nbPoints = find_intersect_points(&texture, &line, points)) {
            if(nbPoints > 1 && lastCut < frames) {

                useProgram(pointProgram);
                drawLines(points, nbPoints);
                lastCut = frames + 5;

                TextureInfos * texture1 = malloc(sizeof(TextureInfos));
                TextureInfos * texture2 = malloc(sizeof(TextureInfos));

                split_vertex(&texture, &line, texture1, texture2);

                addToList(&pieces, texture1);
                addToList(&pieces, texture2);

                texture1->vx = 2.0;
                texture2->vx = -2.0;

                texture1->vy = -1.0;
                texture2->vy = -1.0;

                texture1->vr = 0.01;
                texture2->vr = -0.01;
            }


            useProgram(pointProgram);
            drawLines(points, nbPoints);
        }

        TextureInfos * myTexture;
        for(el = pieces.first; el != NULL; el=el->next) {
            myTexture = (TextureInfos *) el->data;
            // TODO: free and cleanup
            if(myTexture->y < -screen.h) {
                removeFromList(&pieces, el);
                free(myTexture);
                continue;
            }

            if(nbPoints = find_intersect_points(myTexture, &line, points)) {
                useProgram(pointProgram);
                drawLines(points, nbPoints);
                if(nbPoints > 1 && lastCut < frames) {

                    lastCut = frames + 5;

                    TextureInfos * texture1 = malloc(sizeof(TextureInfos));
                    TextureInfos * texture2 = malloc(sizeof(TextureInfos));

                    split_vertex(myTexture, &line, texture1, texture2);

                    addToList(&pieces, texture1);
                    addToList(&pieces, texture2);

                    texture1->vx = 2.0;
                    texture2->vx = -2.0;

                    texture1->vy = -2.0;
                    texture2->vy = -2.0;

                    texture1->vr = 0.02;
                    texture2->vr = -0.02;
                    removeFromList(&pieces, el);

                }
            }

            if(myTexture->y < floor_position) {
                myTexture->vy = -myTexture->vy / 2.0;
                myTexture->vx = 5 * myTexture->vx;
                myTexture->vr = 2 * myTexture->vr;
            }

            // gravity
            myTexture->vy = myTexture->vy - 0.15;

            transformTexture(myTexture, myTexture->vx, myTexture->vy, myTexture->vr);

            useProgram(textureProgram);
            drawTexture(myTexture, 0, 0, 0);
        }

        //transformTexture(&texture, 0, -200.0, 0.00);

        SDL_GL_SwapWindow(mainwindow);

        SDL_Delay(time_left());
        next_time += 16;
    }

    /* Clean up */
    return cleanup(0);
}
