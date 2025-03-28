
#include <stdlib.h>
#include <assert.h>

#include "voronoi.h"

#include "profiler.h"

float dist_sqr(float x1, float y1, float x2, float y2) {
    return (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2);
}

typedef struct Polygon {
    // the points of the polygon
    Vector2 *items;
    size_t count;
    size_t capacity;
} Polygon;

void init_voronoi(void) {}

void finish_voronoi(void) {}


void draw_voronoi(RenderTexture2D target, Vector2 *points, Color *colors, size_t num_points) {
    (void) target;
    (void) points;
    (void) colors;
    (void) num_points;
}

