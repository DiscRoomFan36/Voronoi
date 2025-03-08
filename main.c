
#include <stdio.h>

#include "raylib.h"
#include "raymath.h"

#define SPEED 5
#define NUM_POINTS 10

int width = 1600;
int height = 900;

#define USE_INT

typedef struct Point {
#ifndef USE_INT
    float x, y;
    float vx, vy;
#else
    int x, y;
    int vx, vy;
#endif // USE_INT

    Color color;
} Point;


int int_dist_sqr(int x1, int y1, int x2, int y2) {
    return (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2);
}

float dist_sqr(float x1, float y1, float x2, float y2) {
    float mag_sqr = (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2);
    return mag_sqr;
}


int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(width, height, "Voronoi");

    SetTargetFPS(60);

    Point points[NUM_POINTS] = {0};
    for (size_t i = 0; i < NUM_POINTS; i++) {

#ifndef USE_INT
        points[i].x  = (float) GetRandomValue(0, width );
        points[i].y  = (float) GetRandomValue(0, height);

        points[i].vx = (float) GetRandomValue(1, SPEED);
        points[i].vy = (float) GetRandomValue(1, SPEED);
#else
        points[i].x  = GetRandomValue(0, width );
        points[i].y  = GetRandomValue(0, height);

        points[i].vx = GetRandomValue(1, SPEED);
        points[i].vy = GetRandomValue(1, SPEED);
#endif // USE_INT

        if (GetRandomValue(0, 1)) points[i].vx *= -1;
        if (GetRandomValue(0, 1)) points[i].vy *= -1;

        points[i].color = ColorFromHSV((float) GetRandomValue(0, 360), 0.7, 0.7);
    }

    while (!WindowShouldClose()) {
        width = GetScreenWidth();
        height = GetScreenHeight();


        // move points in a random walk
        // TODO make better
        for (size_t i = 0; i < NUM_POINTS; i++) {
            Point *point = &points[i];

            point->x += point->vx;
            point->y += point->vy;

            if (point->x < 0)      point->vx *= -1;
            if (point->x > width)  point->vx *= -1;

            if (point->y < 0)      point->vy *= -1;
            if (point->y > height) point->vy *= -1;
        }


        BeginDrawing();
        ClearBackground(RED);

        // voronoi the background
        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {

                // find the closest point
                Point closest = points[0];
#ifndef USE_INT
                float d1 = dist_sqr((float) i, (float) j, closest.x, closest.y);
                for (size_t k = 1; k < NUM_POINTS; k++) {
                    float d2 = dist_sqr((float) i, (float) j, points[k].x, points[k].y);
                    if (d2 < d1) {
                        d1 = d2;
                        closest = points[k];
                    }
                }
#else
                int d1 = int_dist_sqr(i, j, closest.x, closest.y);
                for (size_t k = 1; k < NUM_POINTS; k++) {
                    int d2 = int_dist_sqr(i, j, points[k].x, points[k].y);
                    if (d2 < d1) {
                        d1 = d2;
                        closest = points[k];
                    }
                }
#endif // USE_INT

                DrawPixel(i, j, closest.color);
            }
        }

        for (size_t i = 0; i < NUM_POINTS; i++) {
            DrawCircle(points[i].x, points[i].y, 5, BLACK);
        }

        DrawFPS(10, 10);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
