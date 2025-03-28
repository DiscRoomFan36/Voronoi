#include <stdio.h>
#include <assert.h>

#include "voronoi.h"

#include <rlgl.h>

#include "profiler.h"


typedef unsigned int ShaderBufferId;

Shader shader;
RenderTexture2D small_texture;

size_t buffer_cap;

ShaderBufferId points_buffer_id;
ShaderBufferId color_buffer_id;

// uniform variables
int width_loc;
int height_loc;
int num_points_loc;



#define SHADER_PATH "./shaders/shader_buffer.glsl"

void init_voronoi(void) {
    shader = LoadShader(0, SHADER_PATH);
    assert(IsShaderValid(shader));

    small_texture = LoadRenderTexture(1, 1);

    { // the uniform variables
        width_loc      = GetShaderLocation(shader, "width");
        height_loc     = GetShaderLocation(shader, "height");
        num_points_loc = GetShaderLocation(shader, "num_points");

        if (width_loc      == -1) fprintf(stderr, "WARNING: 'width_loc'  was not set\n");
        if (height_loc     == -1) fprintf(stderr, "WARNING: 'height_loc' was not set\n");
        if (num_points_loc == -1) fprintf(stderr, "WARNING: 'num_points_loc' was not set\n");
    }

    // the buffers to store the points and colors
    buffer_cap = 1028;
    points_buffer_id = rlLoadShaderBuffer(buffer_cap*sizeof(Vector2), NULL, 0);
    color_buffer_id  = rlLoadShaderBuffer(buffer_cap*sizeof(Color),   NULL, 0);
}

void finish_voronoi(void) {
    UnloadShader(shader);
    UnloadRenderTexture(small_texture);

    rlUnloadShaderBuffer(points_buffer_id);
    rlUnloadShaderBuffer(color_buffer_id);
}


void draw_voronoi(RenderTexture2D target, Vector2 *points, Color *colors, size_t num_points) {

    if (width_loc  != -1) SetShaderValue(shader, width_loc,  &target.texture.width,  SHADER_UNIFORM_INT);
    if (height_loc != -1) SetShaderValue(shader, height_loc, &target.texture.height, SHADER_UNIFORM_INT);
    if (num_points_loc != -1) SetShaderValue(shader, num_points_loc, &num_points, SHADER_UNIFORM_INT);

    // resize buffer cap if too many points
    if (num_points > buffer_cap) {
        rlUnloadShaderBuffer(points_buffer_id);
        rlUnloadShaderBuffer(color_buffer_id);

        while (buffer_cap < num_points) buffer_cap *= 2;
        points_buffer_id = rlLoadShaderBuffer(buffer_cap*sizeof(Vector2), NULL, 0);
        color_buffer_id  = rlLoadShaderBuffer(buffer_cap*sizeof(Color),   NULL, 0);
    }

    PROFILER_ZONE("Load shader buffers");
        rlUpdateShaderBuffer(points_buffer_id, points, num_points*sizeof(Vector2), 0);
        rlUpdateShaderBuffer(color_buffer_id,  colors, num_points*sizeof(Color),   0);

        rlBindShaderBuffer(points_buffer_id, 1);
        rlBindShaderBuffer(color_buffer_id, 2);
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

