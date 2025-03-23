
#ifndef VORONOI_H_
#define VORONOI_H_

#include "raylib.h"

typedef unsigned long size_t;

void init_voronoi(void);

Color *draw_voronoi(size_t width, size_t height, Vector2 *points, Color *colors, size_t num_points);

void finish_voronoi(void);

#endif // VORONOI_H_
