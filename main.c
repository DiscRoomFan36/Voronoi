
#include <stdio.h>

#include "raylib.h"
#include "raymath.h"

#define PROFILER_IMPLEMENTATION
#include "profiler.h"

#include "voronoi.h"


#define FONT_SIZE 20

// in pixels per second
#define SPEED 100

int screen_width  = 1600;
int screen_height =  900;


void draw_profiler(void) {
    // TODO this is inefficient...
    Profiler_Stats_Array stats = collect_stats();

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

        Numerical_Average_Bounds nab = get_numerical_average(stat.times);

        const char *title_text = TextFormat("%-30s", stat.title, nab.sample_mean, nab.standard_deviation);
        const char *numbers_text = TextFormat(": %.6f +- %.6f", nab.sample_mean, nab.standard_deviation);


        DrawText(title_text,
                screen_width - numbers_width - 10 - max_title_text_width - 10,
                10 + i*FONT_SIZE,
                FONT_SIZE, WHITE);
        DrawText(numbers_text,
                screen_width - numbers_width - 10,
                10 + i*FONT_SIZE,
                FONT_SIZE, WHITE);
    }

    for (size_t i = 0; i < stats.count; i++) {
        profiler_da_free(&stats.items[i].times);
    }
    profiler_da_free(&stats);
}


float randf(void) {
    return (float) rand() / (float) RAND_MAX;
}


#define da_append(da, item)                                                                                \
    do {                                                                                                   \
        if ((da)->count >= (da)->capacity) {                                                               \
            (da)->capacity = (da)->capacity == 0 ? 32 : (da)->capacity*2;                                  \
            (da)->items = (typeof((da)->items)) realloc((da)->items, (da)->capacity*sizeof(*(da)->items)); \
            assert((da)->items != NULL && "Buy More RAM lol");                                             \
        }                                                                                                  \
                                                                                                           \
        (da)->items[(da)->count++] = (item);                                                               \
    } while (0)

#define da_free(da)                         \
    do {                                    \
        if ((da)->items) free((da)->items); \
        (da)->items    = 0;                 \
        (da)->count    = 0;                 \
        (da)->capacity = 0;                 \
    } while (0)


typedef struct  {
    Vector2 *items;
    size_t count;
    size_t capacity;
} Vector2_Array;

typedef struct Color_Array {
    Color *items;
    size_t count;
    size_t capacity;
} Color_Array;


Vector2_Array points_pos = {0};
Vector2_Array points_vel = {0};
Color_Array points_colors = {0};


void add_new_point() {
    Vector2 new_pos = {
        .x = randf() * screen_width,
        .y = randf() * screen_height,
    };

    Vector2 new_vel = {
        .x = (randf() * (SPEED-1) + 1),
        .y = (randf() * (SPEED-1) + 1),
    };
    if (rand() % 2) new_vel.x *= -1;
    if (rand() % 2) new_vel.y *= -1;

    Color new_color = ColorFromHSV(randf() * 360, 0.7, 0.7);

    da_append(&points_pos,    new_pos);
    da_append(&points_vel,    new_vel);
    da_append(&points_colors, new_color);
}


int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screen_width, screen_height, "Voronoi");

    init_voronoi();

    size_t num_points = 10;

    for (size_t i = 0; i < num_points; i++) add_new_point();

    bool paused = false;
    bool reset_profiler = false;
    bool draw_points = true;

    RenderTexture2D target = LoadRenderTexture(screen_width, screen_height);

    while (!WindowShouldClose()) {
        PROFILER_ZONE("total frame time");

        int new_width  = GetScreenWidth();
        int new_height = GetScreenHeight();

        if (screen_width != new_width || screen_height != new_height) {
            screen_width  = new_width;
            screen_height = new_height;

            UnloadRenderTexture(target);
            target = LoadRenderTexture(screen_width, screen_height);
        }

        assert(screen_width > 0 && screen_height > 0);


        { // key toggles
            paused         ^= IsKeyPressed(KEY_SPACE);
            reset_profiler ^= IsKeyPressed(KEY_R);
            draw_points    ^= IsKeyPressed(KEY_P);
        }

        { // Change number of points
            size_t old_num_points = num_points;

            if (IsKeyPressed(KEY_UP)) {
                num_points += 100;
            }
            if (IsKeyPressed(KEY_DOWN)) {
                if (num_points < 100) num_points = 100;
                num_points -= 100;
            }
            if (IsKeyPressed(KEY_RIGHT)) {
                num_points += 10;
            }
            if (IsKeyPressed(KEY_LEFT)) {
                if (num_points < 10) num_points = 10;
                num_points -= 10;
            }

            if (num_points > old_num_points) {
                // add some more points
                for (size_t i = 0; i < num_points - old_num_points; i++) add_new_point();
            } else if (num_points < old_num_points) {
                // remove some points
                points_pos   .count = num_points;
                points_vel   .count = num_points;
                points_colors.count = num_points;
            }
        }


        float delta = GetFrameTime();
        if (delta > 0.25) delta = 0.25;
        if (paused) delta = 0;


        PROFILER_ZONE("walk points");
            // move points in a random walk
            // TODO make better
            for (size_t i = 0; i < num_points; i++) {
                Vector2 *xy  = &points_pos.items[i];
                Vector2 *vxy = &points_vel.items[i];

                xy->x += vxy->x * delta;
                xy->y += vxy->y * delta;

                if (xy->x < 0)             vxy->x =  fabs(vxy->x);
                if (xy->x > screen_width)  vxy->x = -fabs(vxy->x);

                if (xy->y < 0)             vxy->y =  fabs(vxy->y);
                if (xy->y > screen_height) vxy->y = -fabs(vxy->y);
            }
        PROFILER_ZONE_END();


        BeginDrawing();
        ClearBackground(MAGENTA);

        PROFILER_ZONE("voronoi the background");
            draw_voronoi(target, points_pos.items, points_colors.items, num_points);

            DrawTexture(target.texture, 0, 0, WHITE);
        PROFILER_ZONE_END();

        PROFILER_ZONE("draw the points");
        if (draw_points) {
            for (size_t i = 0; i < num_points; i++) {
                DrawCircleV(points_pos.items[i], 5, BLUE);
            }
        }
        PROFILER_ZONE_END();

        { // draw num points
            const char *text = TextFormat("Points: %6zu", num_points);
            int text_width = MeasureText(text, FONT_SIZE);
            DrawText(text, screen_width/2 - text_width/2, 10, FONT_SIZE, WHITE);
        }


        DrawFPS(10, 10);

#ifdef PROFILE_CODE
        PROFILER_ZONE("drawing profiler");
            draw_profiler();
        PROFILER_ZONE_END();
#endif // PROFILE_CODE

        EndDrawing();

        PROFILER_ZONE_END();

        reset_profiler |= profiler_zone_count() > 65536;
        if (reset_profiler) {
            reset_profiler = false;
            PROFILER_RESET();
        }
    }

    UnloadRenderTexture(target);

    finish_voronoi();

    CloseWindow();
    PROFILER_FREE();

    return 0;
}
