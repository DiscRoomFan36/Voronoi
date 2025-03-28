#include <stdio.h>
#include <assert.h>

#include "voronoi.h"

#include "profiler.h"


Shader shader;
RenderTexture2D small_texture;

// uniform variables
int num_points_loc;
int points_loc;
int colors_loc;

int width_loc;
int height_loc;


#define SHADER_PATH "./shaders/shader.glsl"

void init_voronoi(void) {
    shader = LoadShader(0, SHADER_PATH);
    assert(IsShaderValid(shader));

    num_points_loc = GetShaderLocation(shader, "num_points");
    points_loc     = GetShaderLocation(shader, "points");
    colors_loc     = GetShaderLocation(shader, "colors");

    width_loc      = GetShaderLocation(shader, "width");
    height_loc     = GetShaderLocation(shader, "height");

    if (num_points_loc == -1) fprintf(stderr, "WARNING: 'num_points_loc' was not set\n");
    if (points_loc     == -1) fprintf(stderr, "WARNING: 'points_loc' was not set\n");
    if (colors_loc     == -1) fprintf(stderr, "WARNING: 'colors_loc' was not set\n");

    if (width_loc      == -1) fprintf(stderr, "WARNING: 'width_loc'  was not set\n");
    if (height_loc     == -1) fprintf(stderr, "WARNING: 'height_loc' was not set\n");

    small_texture = LoadRenderTexture(1, 1);
}

void finish_voronoi(void) {
    UnloadShader(shader);
    UnloadRenderTexture(small_texture);
}


void draw_voronoi(RenderTexture2D target, Vector2 *points, Color *colors, size_t num_points) {
    PROFILER_ZONE("Setup shader");
    if (num_points_loc != -1) SetShaderValue(shader, num_points_loc, &num_points, SHADER_UNIFORM_INT);
    if (points_loc != -1) SetShaderValueV(shader, points_loc, points, SHADER_UNIFORM_VEC2, num_points);
    if (colors_loc != -1) SetShaderValueV(shader, colors_loc, colors, SHADER_UNIFORM_INT, num_points);

    if (width_loc  != -1) SetShaderValue(shader, width_loc,  &target.texture.width,  SHADER_UNIFORM_INT);
    if (height_loc != -1) SetShaderValue(shader, height_loc, &target.texture.height, SHADER_UNIFORM_INT);
    PROFILER_ZONE_END();

    PROFILER_ZONE("Use shader");
    BeginTextureMode(target);
        BeginShaderMode(shader);
            // just draw over the entire screen
            DrawTexturePro(
                small_texture.texture,
                (Rectangle){0, 0, 1, 1},
                (Rectangle){0, 0, target.texture.width, target.texture.height},
                (Vector2){0, 0},
                0,
                WHITE
            );
        EndShaderMode();
    EndTextureMode();
    PROFILER_ZONE_END();
}

