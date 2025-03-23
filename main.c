
#include <stdio.h>

#include "raylib.h"
#include "raymath.h"

#define PROFILE_CODE
#define PROFILER_IMPLEMENTATION
#include "profiler.h"


#define FONT_SIZE 20

// in pixels per second
#define SPEED 100
#define NUM_POINTS 50

#define RESIZABLE

#ifndef RESIZABLE
    #define width  1600
    #define height  900
#else
    int width  = 1600;
    int height =  900;
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


float randf(void) {
    return (float) GetRandomValue(0, __INT_MAX__) / (float) __INT_MAX__;
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

    size_t pixels_capacity = width * height;
    Color *pixel_buffer = malloc(pixels_capacity * sizeof(Color));
    float *depth_buffer = malloc(pixels_capacity * sizeof(float));

    assert(NUM_POINTS > 0);
    Vector2 points_pos[NUM_POINTS] = {0};
    Vector2 points_vel[NUM_POINTS] = {0};
    Color points_colors[NUM_POINTS] = {0};


    for (size_t i = 0; i < NUM_POINTS; i++) {
        points_pos[i].x  = randf() * width;
        points_pos[i].y  = randf() * height;

        points_vel[i].x = randf() * (SPEED-1) + 1;
        points_vel[i].y = randf() * (SPEED-1) + 1;

        if (GetRandomValue(0, 1)) points_vel[i].x *= -1;
        if (GetRandomValue(0, 1)) points_vel[i].y *= -1;

        points_colors[i] = ColorFromHSV(randf() * 360, 0.7, 0.7);
    }

    bool paused = false;

    while (!WindowShouldClose()) {
        PROFILER_ZONE("total frame time");

        #ifdef RESIZABLE
            width  = GetScreenWidth();
            height = GetScreenHeight();

            size_t new_capacity = width * height;
            if (pixels_capacity < new_capacity) {
                pixels_capacity = new_capacity;
                pixel_buffer = realloc(pixel_buffer, pixels_capacity * sizeof(Color));
                depth_buffer = realloc(depth_buffer, pixels_capacity * sizeof(float));
            }
        #endif // RESIZABLE


        { // keys
            paused ^= IsKeyPressed(KEY_SPACE);
        }


        float delta = GetFrameTime();
        if (delta > 0.25) delta = 0.25;
        if (paused) delta = 0;


        PROFILER_ZONE("walk points");
            // move points in a random walk
            // TODO make better
            for (size_t i = 0; i < NUM_POINTS; i++) {
                Vector2 *xy  = &points_pos[i];
                Vector2 *vxy = &points_vel[i];

                xy->x += vxy->x * delta;
                xy->y += vxy->y * delta;

                if (xy->x < 0)      vxy->x =  fabs(vxy->x);
                if (xy->x > width)  vxy->x = -fabs(vxy->x);

                if (xy->y < 0)      vxy->y =  fabs(vxy->y);
                if (xy->y > height) vxy->y = -fabs(vxy->y);
            }
        PROFILER_ZONE_END();


        BeginDrawing();
        ClearBackground(RED);


        PROFILER_ZONE("voronoi the background");

            // voronoi the background
            PROFILER_ZONE("Calculate pixel buffer");

#if 0
            { // initialize depth buffer
                Vector2 fp = points_pos[0];
                Color fc = points_colors[0];
                for (int j = 0; j < height; j++) {
                    for (int i = 0; i < width; i++) {
                        depth_buffer[j * width + i] = dist_sqr(fp.x, fp.y, i, j);
                    }
                }
                for (size_t i = 0; i < pixels_capacity; i++) pixel_buffer[i] = fc;
            }

            // use depth buffer
            for (size_t k = 1; k < NUM_POINTS; k++) {
                Vector2 pos = points_pos[k];
                Color color = points_colors[k];

                for (int j = 0; j < height; j++) {
                    for (int i = 0; i < width; i++) {
                        float d2 = dist_sqr(pos.x, pos.y, i, j);
                        float d1 = depth_buffer[j * width + i];
                        if (d2 < d1) {
                            depth_buffer[j * width + i] = d2;
                            pixel_buffer[j * width + i] = color;
                        }
                    }
                }
            }
#else
            for (int j = 0; j < height; j++) {
                for (int i = 0; i < width; i++) {

                    // find the closest point
                    size_t close_index = 0;
                    float d1 = dist_sqr(points_pos[0].x, points_pos[0].y, i, j);
                    for (size_t k = 1; k < NUM_POINTS; k++) {
                        float d2 = dist_sqr(points_pos[k].x, points_pos[k].y, i, j);
                        if (d2 < d1) {
                            d1 = d2;
                            close_index = k;
                        }
                    }

                    pixel_buffer[j * width + i] = points_colors[close_index];
                }
            }
#endif
            PROFILER_ZONE_END();


            PROFILER_ZONE("draw pixel buffer");
            for (int j = 0; j < height; j++) {
                int i = 0;
                while (i < width) {
                    int low_i = i;
                    Color this_color = pixel_buffer[j*width + i];
                    for (; i < width; i++) {
                        if (!ColorIsEqual(this_color, pixel_buffer[j*width + i])) break;
                    }
                    DrawRectangle(low_i, j, i - low_i, 1, this_color);
                }
            }
            PROFILER_ZONE_END();

        PROFILER_ZONE_END();

        for (size_t i = 0; i < NUM_POINTS; i++) {
            DrawCircleV(points_pos[i], 5, BLACK);
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
    free(pixel_buffer);

    return 0;
}
