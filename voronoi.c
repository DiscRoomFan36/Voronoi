
#include <stdlib.h>
#include <assert.h>

#include "voronoi.h"

#include "profiler.h"

static Color *pixel_buf = 0;
static float *depth_buf = 0;
static size_t buf_capacity = 0;


void init_voronoi(void) {}


float dist_sqr(float x1, float y1, float x2, float y2) {
    return (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2);
}

void draw_voronoi(RenderTexture2D target, Vector2 *points, Color *colors, size_t num_points) {
    size_t width  = target.texture.width;
    size_t height = target.texture.height;

    if (buf_capacity < width * height) {
        buf_capacity = width * height;
        free(pixel_buf);
        free(depth_buf);
        pixel_buf = malloc(buf_capacity * sizeof(Color));
        depth_buf = malloc(buf_capacity * sizeof(float));
    }

    PROFILER_ZONE("Calculate pixel buffer");
    #if 0
    { // initialize depth buffer
        Vector2 fp = points[0];
        Color fc = colors[0];
        for (size_t j = 0; j < height; j++) {
            for (size_t i = 0; i < width; i++) {
                depth_buf[j * width + i] = dist_sqr(fp.x, fp.y, i, j);
            }
        }
        for (size_t i = 0; i < buf_capacity; i++) pixel_buf[i] = fc;
    }

    // use depth buffer
    for (size_t k = 1; k < num_points; k++) {
        Vector2 pos = points[k];
        Color color = colors[k];

        for (size_t j = 0; j < height; j++) {
            for (size_t i = 0; i < width; i++) {
                float d2 = dist_sqr(pos.x, pos.y, i, j);
                float d1 = depth_buf[j * width + i];
                if (d2 < d1) {
                    depth_buf[j * width + i] = d2;
                    pixel_buf[j * width + i] = color;
                }
            }
        }
    }
#else
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
#endif
    PROFILER_ZONE_END();


    PROFILER_ZONE("draw into texture");
    BeginTextureMode(target);

    for (size_t j = 0; j < height; j++) {
        size_t i = 0;
        while (i < width) {
            size_t low_i = i;
            Color this_color = pixel_buf[j*width + i];
            for (; i < width; i++) {
                if (!ColorIsEqual(this_color, pixel_buf[j*width + i])) break;
            }

            DrawRectangle(low_i, j, i - low_i, 1, this_color);
        }
    }

    EndTextureMode();
    PROFILER_ZONE_END();
}


void finish_voronoi(void) {
    if (pixel_buf) free(pixel_buf);
    if (depth_buf) free(depth_buf);

    pixel_buf = 0;
    depth_buf = 0;
    buf_capacity = 0;
}


