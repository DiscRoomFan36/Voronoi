
#include <stdlib.h>
#include <stdio.h>

#include "voronoi.h"

#include "common.h"

static Color *pixel_buf = 0;
static u64 buf_capacity = 0;

float dist_sqr(float x1, float y1, float x2, float y2) {
    return (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2);
}

// because VSCode is being stupid
// we need this for barriers
#ifndef __USE_XOPEN2K
#define __USE_XOPEN2K
#endif // __USE_XOPEN2K

#include <pthread.h>

#define NUM_THREADS 12

pthread_t thread_ids[NUM_THREADS];
pthread_barrier_t start_barrier;
pthread_barrier_t end_barrier;
bool finished;

#define THREAD_CHUNK_SIZE 512
pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;
u64 counter;

// these can be seen by the threads
u64 thread_width;
u64 thread_height;
Vector2 *thread_points;
Color *thread_colors;
u64 thread_num_points;

void *thread_function(void *args) {
    u64 id = (u64) args;
    (void) id;

    while (1) {
        pthread_barrier_wait(&start_barrier);
        if (finished) break;

        // grab a chunk of the work, and do it.
        while (1) {
            u64 work_to_do;
            pthread_mutex_lock(&counter_lock);

            if (counter < thread_width * thread_height) {
                work_to_do = counter;
                counter += THREAD_CHUNK_SIZE;

            } else {
                // were finished
                pthread_mutex_unlock(&counter_lock);
                break;
            }

            pthread_mutex_unlock(&counter_lock);

            for (u64 i = work_to_do; i < work_to_do + THREAD_CHUNK_SIZE; i++) {
                if (i >= thread_width * thread_height) break;

                float x = (float) (i % thread_width);
                float y = (float) (i / thread_width);

                // find the closest point
                u64 close_index = 0;
                float d1 = dist_sqr(thread_points[0].x, thread_points[0].y, x, y);
                for (u64 k = 1; k < thread_num_points; k++) {
                    float d2 = dist_sqr(thread_points[k].x, thread_points[k].y, x, y);
                    if (d2 < d1) {
                        d1 = d2;
                        close_index = k;
                    }
                }

                // thread_pixel_buf[i] = thread_colors[close_index];
                pixel_buf[i] = thread_colors[close_index];
            }

            // repeat chunk loop
        }

        pthread_barrier_wait(&end_barrier);
    }

    return NULL;
}


void init_voronoi(void) {

    if (pthread_barrier_init(&start_barrier, NULL, NUM_THREADS+1)) {
        fprintf(stderr, "ERROR: cannot init start barrier\n");
        exit(1);
    }
    if (pthread_barrier_init(&end_barrier, NULL, NUM_THREADS+1)) {
        fprintf(stderr, "ERROR: cannot init end barrier\n");
        exit(1);
    }

    finished = false;

    // start the threads
    for (u64 i = 0; i < NUM_THREADS; i++) {
        int res = pthread_create(&thread_ids[i], NULL, thread_function, (void *) i);
        if (res) {
            fprintf(stderr, "ERROR: thread could not be created\n");
            exit(1);
        }
    }
}

void finish_voronoi(void) {
    // free the buffer
    if (pixel_buf) free(pixel_buf);
    pixel_buf = 0;
    buf_capacity = 0;

    finished = true;
    pthread_barrier_wait(&start_barrier);

    // finish the treads
    for (u64 i = 0; i < NUM_THREADS; i++) {
        int ret = pthread_join(thread_ids[i], NULL);
        if (ret) {
            fprintf(stderr, "ERROR: on id %zu when closeing\n", i);
        }
    }

    pthread_barrier_destroy(&start_barrier);
    pthread_barrier_destroy(&end_barrier);
}


void draw_voronoi(RenderTexture2D target, Vector2 *points, Color *colors, size_t num_points) {
    u64 width  = target.texture.width;
    u64 height = target.texture.height;

    if (buf_capacity < width * height) {
        buf_capacity = width * height;
        free(pixel_buf);
        pixel_buf = malloc(buf_capacity * sizeof(Color));
    }


    PROFILER_ZONE("Calculate pixel buffer");

        // setup
        thread_width  = width;
        thread_height = height;
        thread_points = points;
        thread_colors = colors;
        thread_num_points = num_points;
        counter = 0;

        // start the waiting threads
        pthread_barrier_wait(&start_barrier);
        // wait for them to stop
        pthread_barrier_wait(&end_barrier);

    PROFILER_ZONE_END();


    PROFILER_ZONE("draw into texture");
    BeginTextureMode(target);

    for (u64 j = 0; j < height; j++) {
        u64 i = 0;
        while (i < width) {
            u64 low_i = i;
            Color this_color = pixel_buf[j*width + i];
            for (; i < width; i++) {
                if (!ColorIsEqual(this_color, pixel_buf[j*width + i])) break;
            }

            DrawRectangle(low_i, height - 1 - j, i - low_i, 1, this_color);
        }
    }

    EndTextureMode();
    PROFILER_ZONE_END();
}

