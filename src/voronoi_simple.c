
#include <stdlib.h>
#include <assert.h>

#include "voronoi.h"

#include "common.h"

static Color *pixel_buf = 0;
static size_t buf_capacity = 0;

float dist_sqr(float x1, float y1, float x2, float y2) {
    return (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2);
}

void init_voronoi(void) {}

void finish_voronoi(void) {
    // free the buffer
    if (pixel_buf) free(pixel_buf);
    pixel_buf = 0;
    buf_capacity = 0;
}


void draw_voronoi(RenderTexture2D target, Vector2 *points, Color *colors, size_t num_points) {
    size_t width  = target.texture.width;
    size_t height = target.texture.height;

    if (buf_capacity < width * height) {
        buf_capacity = width * height;
        free(pixel_buf);
        pixel_buf = malloc(buf_capacity * sizeof(Color));
    }


    PROFILER_ZONE("Calculate pixel buffer");

        for (size_t j = 0; j < height; j++) {
            for (size_t i = 0; i < width; i++) {

                // find the closest point
                size_t close_index = 0;
                float d1 = dist_sqr(points[0].x, points[0].y, i, j);
                for (size_t k = 1; k < num_points; k++) {
                    float d2 = dist_sqr(points[k].x, points[k].y, i, j);
                    if (d2 < d1) {
                        d1 = d2;
                        close_index = k;
                    }
                }

                pixel_buf[j * width + i] = colors[close_index];
            }
        }

    PROFILER_ZONE_END();


    PROFILER_ZONE("draw into texture");
    BeginTextureMode(target);

    for (size_t j = 0; j < height; j++) {
        // find a band of the same color.
        // this is MUCH faster than just useing DrawPixel()
        size_t i = 0;
        while (i < width) {
            size_t low_i = i;
            Color this_color = pixel_buf[j*width + i];
            for (; i < width; i++) {
                if (!ColorIsEqual(this_color, pixel_buf[j*width + i])) break;
            }

            // remember to draw this upsidedown.
            // bc how textures work, and the API demands it.
            DrawRectangle(low_i, height - 1 - j, i - low_i, 1, this_color);
        }
    }

    EndTextureMode();
    PROFILER_ZONE_END();
}

