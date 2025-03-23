
#include <stdio.h>

#include "raylib.h"
#include "raymath.h"

#define PROFILE_CODE
#define PROFILER_IMPLEMENTATION
#include "profiler.h"


#define FONT_SIZE 20

// in pixels per second
#define SPEED 100
#define NUM_POINTS 10

#define RESIZABLE

#ifdef RESIZABLE
    int width  = 1600;
    int height =  900;
#else
    #define width  1600
    #define height  900
#endif // RESIZABLE


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
                width - numbers_width - 10 - max_title_text_width - 10,
                10 + i*FONT_SIZE,
                FONT_SIZE, WHITE);
        DrawText(numbers_text,
                width - numbers_width - 10,
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
    float x, y;
    float vx, vy;

    Color color;
} Point;


float dist_sqr(float x1, float y1, float x2, float y2) {
    float mag_sqr = (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2);
    return mag_sqr;
}


int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(width, height, "Voronoi");

    size_t pixels_capacity = width * height * sizeof(Color);
    Color *pixels = malloc(pixels_capacity);

    Point points[NUM_POINTS] = {0};
    for (size_t i = 0; i < NUM_POINTS; i++) {

        points[i].x  = (float) GetRandomValue(0, width );
        points[i].y  = (float) GetRandomValue(0, height);

        points[i].vx = (float) GetRandomValue(1, SPEED);
        points[i].vy = (float) GetRandomValue(1, SPEED);

        if (GetRandomValue(0, 1)) points[i].vx *= -1;
        if (GetRandomValue(0, 1)) points[i].vy *= -1;

        points[i].color = ColorFromHSV((float) GetRandomValue(0, 360), 0.7, 0.7);
    }

    while (!WindowShouldClose()) {
        PROFILER_ZONE("total frame time");

        #ifdef RESIZABLE
            width  = GetScreenWidth();
            height = GetScreenHeight();
            
            size_t new_capacity = width * height * sizeof(Color);
            if (pixels_capacity < new_capacity) {
                pixels_capacity = new_capacity;
                pixels = realloc(pixels, new_capacity);
            }
        #endif // RESIZABLE

        float delta = GetFrameTime();


        PROFILER_ZONE("walk points");
            // move points in a random walk
            // TODO make better
            for (size_t i = 0; i < NUM_POINTS; i++) {
                Point *point = &points[i];

                point->x += point->vx * delta;
                point->y += point->vy * delta;

                if (point->x < 0)      point->vx =  fabs(point->vx);
                if (point->x > width)  point->vx = -fabs(point->vx);

                if (point->y < 0)      point->vy =  fabs(point->vy);
                if (point->y > height) point->vy = -fabs(point->vy);
            }
        PROFILER_ZONE_END();


        BeginDrawing();
        ClearBackground(RED);


        PROFILER_ZONE("voronoi the background");

            // voronoi the background
            PROFILER_ZONE("Calculate background");
            for (int j = 0; j < height; j++) {
                for (int i = 0; i < width; i++) {

                    // find the closest point
                    assert(NUM_POINTS > 0);

                    Point closest = points[0];
                    float d1 = dist_sqr(i, j, closest.x, closest.y);
                    for (size_t k = 0; k < NUM_POINTS; k++) {
                        float d2 = dist_sqr(i, j, points[k].x, points[k].y);
                        if (d2 < d1) {
                            d1 = d2;
                            closest = points[k];
                        }
                    }

                    pixels[j * width + i] = closest.color;
                }
            }
            PROFILER_ZONE_END();


            PROFILER_ZONE("draw background");
            for (int j = 0; j < height; j++) {
                int i = 0;
                while (i < width) {
                    int low_i = i;
                    Color this_color = pixels[j*width + i];
                    for (; i < width; i++) {
                        if (!ColorIsEqual(this_color, pixels[j*width + i])) break;
                    }
                    DrawRectangle(low_i, j, i - low_i, 1, this_color);
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

        EndDrawing();

        PROFILER_ZONE_END();
        // if (profiler_zone_count() > 1028) PROFILER_RESET();
    }

    CloseWindow();
    PROFILER_FREE();
    free(pixels);

    return 0;
}
