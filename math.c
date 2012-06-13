

double distance_2_points(float x1, float y1, float x2, float y2) {
    float distance;
    float distance_x = x1 - x2;
    float distance_y = y1 - y2;
    distance = sqrt((distance_x * distance_x) + (distance_y * distance_y));
    return distance;
}




