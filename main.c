
#include <stdio.h>

#include "raylib.h"
#include "raymath.h"

#define PROFILER_IMPLEMENTATION
#include "profiler.h"

#include "voronoi.h"


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

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(width, height, "Voronoi");

    init_voronoi();

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

    RenderTexture2D target = LoadRenderTexture(width, height);

    while (!WindowShouldClose()) {
        PROFILER_ZONE("total frame time");

        #ifdef RESIZABLE
            int new_width  = GetScreenWidth();
            int new_height = GetScreenHeight();

            if (width != new_width || height != new_height) {
                width  = new_width;
                height = new_height;

                UnloadRenderTexture(target);
                target = LoadRenderTexture(width, height);
            }
        #endif // RESIZABLE

        assert(width > 0 && height > 0);


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
            draw_voronoi(target, points_pos, points_colors, NUM_POINTS);

            // remember to draw this flipped, because of how the GPU dose things...
            Rectangle source = {0, 0, target.texture.width, -target.texture.height};
            DrawTextureRec(target.texture, source, (Vector2){0, 0}, WHITE);
        PROFILER_ZONE_END();

        for (size_t i = 0; i < NUM_POINTS; i++) {
            DrawCircleV(points_pos[i], 5, BLUE);
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

    UnloadRenderTexture(target);

    CloseWindow();
    PROFILER_FREE();

    finish_voronoi();

    return 0;
}
