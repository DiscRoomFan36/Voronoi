
#include <stdio.h>

#include "raylib.h"
#include "raymath.h"

#define PROFILE_CODE
#define PROFILER_IMPLEMENTATION
#include "profiler.h"


#define FONT_SIZE 20

#define SPEED 5
#define NUM_POINTS 10

#define WIDTH  1600
#define HEIGHT  900

void draw_profiler(void) {
    // TODO this is inefficient...
    Profiler_Stats_Array stats = collect_stats();
    Double_Array numbers = {0};

    int numbers_width = MeasureText(": 0.000000 +- 0.000000", FONT_SIZE);

    int max_title_text_width = 0;
    for (size_t i = 0; i < stats.count; i++) {
        Profiler_Stats stat = stats.items[i];
        int title_text_width = MeasureText(stat.title, FONT_SIZE);
        if (max_title_text_width < title_text_width) {
            max_title_text_width = title_text_width;
        }
    }

    for (size_t i = 0; i < stats.count; i++) {
        Profiler_Stats stat = stats.items[i];

        numbers.count = 0;
        for (size_t i = 0; i < stat.times.count; i++) {
            profiler_da_append(&numbers, stat.times.items[i]);
        }

        Numerical_Average_Bounds nab = get_numerical_average(numbers);


        const char *title_text = TextFormat("%-30s", stat.title, nab.sample_mean, nab.standard_deviation);
        const char *numbers_text = TextFormat(": %.6f +- %.6f", nab.sample_mean, nab.standard_deviation);


        DrawText(title_text,
                WIDTH - numbers_width - 10 - max_title_text_width - 10,
                10 + i*FONT_SIZE,
                FONT_SIZE, WHITE);
        DrawText(numbers_text,
                WIDTH - numbers_width - 10,
                10 + i*FONT_SIZE,
                FONT_SIZE, WHITE);
    }

    for (size_t i = 0; i < stats.count; i++) {
        profiler_da_free(&stats.items[i].times);
    }
    profiler_da_free(&stats);
    profiler_da_free(&numbers);
}


typedef struct Point {
    int x, y;
    int vx, vy;

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
    // SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WIDTH, HEIGHT, "Voronoi");

    SetTargetFPS(60);

    Color *pixels = malloc(WIDTH * HEIGHT * sizeof(Color));

    Point points[NUM_POINTS] = {0};
    for (size_t i = 0; i < NUM_POINTS; i++) {

        points[i].x  = GetRandomValue(0, WIDTH );
        points[i].y  = GetRandomValue(0, HEIGHT);

        points[i].vx = GetRandomValue(1, SPEED);
        points[i].vy = GetRandomValue(1, SPEED);

        if (GetRandomValue(0, 1)) points[i].vx *= -1;
        if (GetRandomValue(0, 1)) points[i].vy *= -1;

        points[i].color = ColorFromHSV((float) GetRandomValue(0, 360), 0.7, 0.7);
    }

    while (!WindowShouldClose()) {
        // WIDTH = GetScreenWidth();
        // HEIGHT = GetScreenHeight();


        PROFILER_ZONE("walk points");
            // move points in a random walk
            // TODO make better
            for (size_t i = 0; i < NUM_POINTS; i++) {
                Point *point = &points[i];

                point->x += point->vx;
                point->y += point->vy;

                if (point->x < 0)      point->vx *= -1;
                if (point->x > WIDTH)  point->vx *= -1;

                if (point->y < 0)      point->vy *= -1;
                if (point->y > HEIGHT) point->vy *= -1;
            }
        PROFILER_ZONE_END();


        BeginDrawing();
        ClearBackground(RED);


        PROFILER_ZONE("voronoi the background");

            // voronoi the background
            PROFILER_ZONE("Calculate background");
            for (int j = 0; j < HEIGHT; j++) {
                for (int i = 0; i < WIDTH; i++) {

                    // find the closest point
                    Point closest = points[0];
                    int d1 = int_dist_sqr(i, j, closest.x, closest.y);
                    for (size_t k = 1; k < NUM_POINTS; k++) {
                        int d2 = int_dist_sqr(i, j, points[k].x, points[k].y);
                        if (d2 < d1) {
                            d1 = d2;
                            closest = points[k];
                        }
                    }

                    pixels[j * WIDTH + i] = closest.color;
                }
            }
            PROFILER_ZONE_END();


            PROFILER_ZONE("draw background");
            for (int j = 0; j < HEIGHT; j++) {
                for (int i = 0; i < WIDTH; i++) {
                    DrawPixel(i, j, pixels[j * WIDTH + i]);
                }
            }
            PROFILER_ZONE_END();

        PROFILER_ZONE_END();

        for (size_t i = 0; i < NUM_POINTS; i++) {
            DrawCircle(points[i].x, points[i].y, 5, BLACK);
        }

        DrawFPS(10, 10);

#ifdef PROFILE_CODE

        PROFILER_ZONE("drawing profiler");
            draw_profiler();
        PROFILER_ZONE_END();

#endif // PROFILE_CODE

        // PROFILER_PRINT();
        // PROFILER_RESET();

        EndDrawing();
    }

    CloseWindow();
    PROFILER_FREE();
    free(pixels);

    return 0;
}
