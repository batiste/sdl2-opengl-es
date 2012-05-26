
double distance2points(float x1, float y1, float x2, float y2) {
    float distance;
    float distance_x = x1 - x2;
    float distance_y = y1 - y2;
    distance = sqrt((distance_x * distance_x) + (distance_y * distance_y));
    return distance;
}


// Returns 1 if the lines intersect, otherwise 0. In addition, if the lines
// intersect the intersection point may be stored in the floats i_x and i_y.
int get_line_intersection(float p0_x, float p0_y, float p1_x, float p1_y,
    float p2_x, float p2_y, float p3_x, float p3_y, float *i_x, float *i_y)
{
    float s1_x, s1_y, s2_x, s2_y;
    s1_x = p1_x - p0_x;     s1_y = p1_y - p0_y;
    s2_x = p3_x - p2_x;     s2_y = p3_y - p2_y;

    float s, t;
    s = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y)) / (-s2_x * s1_y + s1_x * s2_y);
    t = ( s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x)) / (-s2_x * s1_y + s1_x * s2_y);

    if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
    {
        // Collision detected
        if (i_x != NULL)
            *i_x = p0_x + (t * s1_x);
        if (i_y != NULL)
            *i_y = p0_y + (t * s1_y);
        return 1;
    }

    return 0; // No collision
}


struct Line {
    float ax;
    float ay;
    float bx;
    float by;
};


int find_intersect_points(struct TextureInfos * texture, struct Line * line, GLfloat * points) {

    int i;
    float intersect_x;
    float intersect_y;
    int nbPoints = 0;
    int size = texture->verticesSize * 5;

    for(i=0; i<size; i=i+5) {
        if(get_line_intersection(
            texture->vertices[i % size], texture->vertices[(i+1) % size],   // p1
            texture->vertices[(i+5) % size], texture->vertices[(i+6) % size], // p2
            line->ax, line->ay, // p3
            line->bx, line->by, // p4
            &intersect_x, &intersect_y
        )) {
            points[2 * nbPoints] = intersect_x;
            points[2 * nbPoints + 1] = intersect_y;
            nbPoints = nbPoints + 1;
            if(nbPoints > 2) {
                LOG("Intersection error, more than 2 points found: %d", nbPoints)
                exit(1);
            }

        }
    }

    return nbPoints;
}


struct Intersection {
    float x;
    float y;
    int index_p1;
    int index_p2;
};


int split_vertex(struct TextureInfos * texture, struct Line * line,
                 struct TextureInfos * texture1, struct TextureInfos * texture2) {

    int i;
    float intersect_x;
    float intersect_y;
    int nbIntersections = 0;
    struct Intersection intersections[2];
    int size = texture->verticesSize * 5;

    for(i=0; i<size; i=i+5) {
        if(get_line_intersection(
            texture->vertices[i % size], texture->vertices[(i+1) % size],   // p1
            texture->vertices[(i+5) % size], texture->vertices[(i+6) % size], // p2
            line->ax, line->ay, // p3
            line->bx, line->by, // p4
            &intersect_x, &intersect_y
        )) {
            intersections[nbIntersections].x = intersect_x;
            intersections[nbIntersections].y = intersect_y;

            intersections[nbIntersections].index_p1 = i % size;
            intersections[nbIntersections].index_p2 = (i+5) % size;

            nbIntersections = nbIntersections + 1;
            if(nbIntersections > 2) {
                LOG("Intersection error, more than 2 points found: %d", nbIntersections)
                exit(1);
            }
        }
    }

    if(nbIntersections != 2)
        return 0;

    LOG("INTERSECTIONS %f %f",intersections[1].x, intersections[1].y)



    createNewVertexFromIntersection(&intersections, texture, texture1);
    struct Intersection inverted_intersections[2];
    inverted_intersections[0] = intersections[1];
    inverted_intersections[1] = intersections[0];

    createNewVertexFromIntersection(&inverted_intersections, texture, texture2);

    return nbIntersections;
}


int createNewVertexFromIntersection(

    struct Intersection intersections[2],
    struct TextureInfos * texture,
    struct TextureInfos * newTexture) {

    int size = texture->verticesSize * 5;

    // start point to built the new vertex
    int current_index = intersections[0].index_p2;

    // there has to be at least 1 vertices to take from the old
    int nbVertices = 0;
    // count the new vertices to take from the old vertices
    int safeGuard = 5 * 12;
    while(current_index != intersections[1].index_p2) {
        nbVertices = nbVertices + 1;
        current_index = (current_index+5) % size;
        safeGuard = safeGuard - 1;
        if(safeGuard < 0) {
            LOG("Unexpected inifinte loop, current index %d, %d", current_index, intersections[1].index_p2)
            exit(1);
        }
    }

    newTexture->verticesSize = 2 + nbVertices;

    // allocate memory for the new vertices
    GLfloat * new_vertices = malloc((5 * (newTexture->verticesSize)) * sizeof(GLfloat));

    // allocate memory for the texture indices
    newTexture->indices = malloc(9 * sizeof(GLshort));
    newTexture->indices[0] = 0;
    newTexture->indices[1] = 1;
    newTexture->indices[2] = 2;
    newTexture->indices[3] = 0;
    newTexture->indices[4] = 2;
    newTexture->indices[5] = 3;
    // TODO: Find the proper indices
    newTexture->indices[6] = 0;
    newTexture->indices[7] = 3;
    newTexture->indices[8] = 4;


    float segment_length = distance2points(
            texture->vertices[intersections[0].index_p1+0],
            texture->vertices[intersections[0].index_p1+1],
            texture->vertices[intersections[0].index_p2+0],
            texture->vertices[intersections[0].index_p2+1]
    );
    float distance_intersection = distance2points(
            texture->vertices[intersections[0].index_p2+0],
            texture->vertices[intersections[0].index_p2+1],
            intersections[0].x,
            intersections[0].y);
    float ratio_1 = distance_intersection / segment_length;

    segment_length = distance2points(
            texture->vertices[intersections[1].index_p1+0],
            texture->vertices[intersections[1].index_p1+1],
            texture->vertices[intersections[1].index_p2+0],
            texture->vertices[intersections[1].index_p2+1]
    );
    distance_intersection = distance2points(
            texture->vertices[intersections[1].index_p1+0],
            texture->vertices[intersections[1].index_p1+1],
            intersections[1].x,
            intersections[1].y);

    float ratio_2 = distance_intersection / segment_length;

    if(ratio_1 > 1.0 || ratio_2 > 1.0) {
        LOG("ERROR RATIO %f, %f", ratio_1, ratio_2);
        exit(1);
    }


    LOG("RATIO %f, %f", ratio_1, ratio_2)

    // start the new vertices with the 2 collision points
    new_vertices[0] = intersections[1].x;
    new_vertices[1] = intersections[1].y;
    new_vertices[2] = 0.0f;
    // texture coordinates
    float x_texture;
    float y_texture;

    float a = texture->vertices[intersections[1].index_p1+3];
    float b = texture->vertices[intersections[1].index_p2+3];
    float c = abs(a - b) * ratio_2;
    if(a < b) {
        x_texture = a + c;
    } else {
        x_texture = a - c;
    }

    a = texture->vertices[intersections[1].index_p1+4];
    b = texture->vertices[intersections[1].index_p2+4];
    c = abs(a - b) * ratio_2;
    if(a < b) {
        y_texture = a + c;
    } else {
        y_texture = a - c;
    }


    new_vertices[3] = x_texture ; // x
    new_vertices[4] = y_texture ; // y

    new_vertices[5] = intersections[0].x;
    new_vertices[6] = intersections[0].y;
    new_vertices[7] = 0.0f;
    // texture coordinates


    a = texture->vertices[intersections[0].index_p2+3];
    b = texture->vertices[intersections[0].index_p1+3];
    c = abs(a - b) * ratio_1;
    if(a < b) {
        x_texture = a + c;
    } else {
        x_texture = a - c;
    }

    a = texture->vertices[intersections[0].index_p2+4];
    b = texture->vertices[intersections[0].index_p1+4];
    c = abs(a - b) * ratio_1;
    if(a < b) {
        y_texture = a + c;
    } else {
        y_texture = a - c;
    }

    new_vertices[8] = x_texture ; // x
    new_vertices[9] = y_texture ; // y

    LOG("TEXTURE COORD %f %f", new_vertices[3], new_vertices[4])
    LOG("TEXTURE COORD %f %f", new_vertices[8], new_vertices[9])


    int old_index = intersections[0].index_p2;

    // fill with the rest old vertices
    int new_index = 10;
    while(nbVertices > 0) {
        new_vertices[new_index] = texture->vertices[old_index];
        new_vertices[new_index+1] = texture->vertices[old_index+1];
        new_vertices[new_index+2] = texture->vertices[old_index+2];
        // same texture coordinates as before
        new_vertices[new_index+3] = texture->vertices[old_index+3];
        new_vertices[new_index+4] = texture->vertices[old_index+4];

        old_index = (old_index+5) % size;
        nbVertices = nbVertices-1;
        new_index = (new_index+5) % (newTexture->verticesSize * 5);
    }

    // copy vertices
    newTexture->vertices = new_vertices;
    // texture object
    newTexture->texture = texture->texture;

    LOG("nb vertices %d", newTexture->verticesSize)
    int i;
    for(i=0; i<(newTexture->verticesSize * 5); i=i+5) {
        LOG("x: %f y: %f", newTexture->vertices[i], newTexture->vertices[i+1]);
    }

    return 0;
}

