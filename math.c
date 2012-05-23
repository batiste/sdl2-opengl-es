
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

    for(i=0; i<=20; i=i+5) {
        if(get_line_intersection(
            texture->vertices[i % 20], texture->vertices[(i + 1) % 20],   // p1
            texture->vertices[(i+5) % 20], texture->vertices[ (i + 6) % 20], // p2
            line->ax, line->ay, // p3
            line->bx, line->by, // p4
            &intersect_x, &intersect_y
        )) {
            points[2 * nbPoints] = intersect_x;
            points[2 * nbPoints + 1] = intersect_y;
            nbPoints = nbPoints + 1;
        }
    }

    return nbPoints;
}

