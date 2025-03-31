
#include <stdlib.h>
#include <assert.h>

#include "voronoi.h"

#include "raymath.h"

#include "profiler.h"


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


#define SWAP(a, b) do {typeof(a) tmp = a; a = b; b = tmp;} while(0)


typedef struct Polygon {
    // the points of the polygon
    Vector2 *items;
    size_t count;
    size_t capacity;
} Polygon;

#include <stdio.h>

// draw a convex polygon, with points in clockwise order
// flip height, if not zero, flip vertical
void draw_polygon(Polygon polygon, Color color, int flip_height) {
    for (size_t i = 1; i < polygon.count - 1; i++) {
        Vector2 v1 = polygon.items[0];
        Vector2 v2 = polygon.items[i];
        Vector2 v3 = polygon.items[i+1];

        if (flip_height) {
            v1.y = flip_height - v1.y;
            v2.y = flip_height - v2.y;
            v3.y = flip_height - v3.y;

            SWAP(v2, v3);
        }

        // DrawLineEx(v1, v2, 10, GOLD);
        // DrawLineEx(v2, v3, 10, GOLD);
        // DrawLineEx(v1, v3, 10, GOLD);

        // draw the points in reverse order, because the polygon is always clockwise.
        DrawTriangle(v3, v2, v1, color);
    }
}

// TODO actually use floating inf
#define FINF 999999999.0f

// https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection
Vector2 line_line_intersection(Vector2 p1, Vector2 p2, Vector2 p3, Vector2 p4) {
    float w = (p1.x - p2.x)*(p3.y - p4.y) - (p1.y - p2.y)*(p3.x - p4.x);

    if (w == 0) {
        // parallel or coincident, return junk number
        return (Vector2){FINF, FINF};
    }

    float d_x = (p1.x*p2.y - p1.y*p2.x)*(p3.x - p4.x) - (p1.x - p2.x)*(p3.x*p4.y - p3.y*p4.x);
    float d_y = (p1.x*p2.y - p1.y*p2.x)*(p3.y - p4.y) - (p1.y - p2.y)*(p3.x*p4.y - p3.y*p4.x);

    // intersection points.
    float i_x = d_x / w;
    float i_y = d_y / w;

    return (Vector2){i_x, i_y};
}

bool intersection_point_intersects(Vector2 p1, Vector2 p2, Vector2 intersect) {
    if (p1.x == p2.x) {
        // vertical line

        // assert p1 has smaller y
        if (p2.y < p1.y) SWAP(p1, p2);

        // if its between the correct y range. is good
        if (p1.y < intersect.y && intersect.y < p2.y) {
            return true;
        } else {
            return false;
        }

    } else if (p1.y == p2.y) {
        // horizontal line

        // assert p1 has smaller x
        if (p2.x < p1.x) SWAP(p1, p2);

        if (p1.x < intersect.x && intersect.x < p2.x) {
            return true;
        } else {
            return false;
        }

    } else {
        // general case
        // just check that the intersect is within the x range

        // assert p1 has smaller x
        if (p2.x < p1.x) SWAP(p1, p2);

        if (p1.x < intersect.x && intersect.x < p2.x) {
            return true;
        } else {
            return false;
        }
    }

    assert(false && "Unreachable");
}

// https://stackoverflow.com/questions/1560492/how-to-tell-whether-a-point-is-to-the-right-or-left-side-of-a-line
bool isLeft(Vector2 a, Vector2 b, Vector2 c) {
    return (b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x) > 0;
}


// the polygons are global variables, so they keep their malloc's between draw calls.
Polygon polygon;
Polygon tmp_poly1;
Polygon tmp_poly2;


void init_voronoi(void) {
    // clear the polygon's
    polygon   = (Polygon){0};
    tmp_poly1 = (Polygon){0};
    tmp_poly2 = (Polygon){0};
}

void finish_voronoi(void) {
    da_free(&polygon);

    da_free(&tmp_poly1);
    da_free(&tmp_poly2);
}


void draw_voronoi(RenderTexture2D target, Vector2 *points, Color *colors, size_t num_points) {
    if (num_points == 0) return;

    int width  = target.texture.width;
    int height = target.texture.height;


    BeginTextureMode(target);
    ClearBackground(GRAY);


    // how to draw voronoi with just math, and not checking every pixel:
    //
    // 1. Get a point.
    // 2. Construct a polygon that fills the screen
    // 3. For every other point:
    // 4.     Find the mid line parallel to those point
    // 5.     Cut the polygon and keep the side that is close to the original point
    // 6.     Repeat 4-5 until all other points have been considered.
    // 7. Convert the resulting convex polygon into triangles and draw them (maybe the bounding lines as well.)
    // 8. repeat 1-7 for every point.


    for (size_t point_index = 0; point_index < num_points; point_index++) {
        // 1. Get a point.
        Vector2 point = points[point_index];

        // 2. Construct a polygon that fills the screen
        polygon.count = 0;
        da_append(&polygon, ((Vector2){    0,      0}));
        da_append(&polygon, ((Vector2){width,      0}));
        da_append(&polygon, ((Vector2){width, height}));
        da_append(&polygon, ((Vector2){    0, height}));


        // 3. For every other point:
        for (size_t other_point_index = 0; other_point_index < num_points; other_point_index++) {
            if (other_point_index == point_index) continue;

            Vector2 other_point = points[other_point_index];

            // 4. Find the mid line parallel to those point
            Vector2 perpendicular_points[2] = {0}; // TODO dont init
            // Line perpendicular;
            {
                Vector2 p1 = point;
                Vector2 p2 = other_point;
                // m = (p1 + p2) / 2
                Vector2 m = {(p1.x + p2.x) / 2, (p1.y + p2.y) / 2};

                // v = p2 - p1
                Vector2 v = Vector2Subtract(p2, p1);

                // v_1 = (-v.y, v.x); // rot90
                Vector2 v_1 = {-v.y, v.x};

                // m_1 = m + v1; // two points form a line.
                Vector2 m_1 = Vector2Add(m, v_1);

                // perpendicular = two_points_to_line(m, m_1);
                perpendicular_points[0] = m;
                perpendicular_points[1] = m_1;
            }

            // { // draw perpendicular line.
            //     if (perpendicular.a == 0) {
            //         // straight left and right
            //         float y = line_y_given_x(perpendicular, 0);
            //         DrawLineEx((Vector2){0, y}, (Vector2){width, y}, 10, BLACK);

            //     } else if (perpendicular.b == 0) {
            //         // straight up and down.
            //         float x = line_x_given_y(perpendicular, 0);
            //         DrawLine(x, 0, x, height, BLACK);
            //     } else {
            //         // general case

            //         float x1 = 0;
            //         float y1 = line_y_given_x(perpendicular, x1);

            //         float x2 = width;
            //         float y2 = line_y_given_x(perpendicular, x2);

            //         DrawLine(x1, y1, x2, y2, BLACK);
            //     }
            // }


            // 5. Cut the polygon and keep the side that is close to the original point

            Vector2 intersection_points[2] = {0};
            size_t intersection_points_i[2] = {0};
            size_t intersection_points_count = 0;

            // loop over all edges
            for (size_t i = 0; i < polygon.count; i++) {
                Vector2 p1 = polygon.items[i];
                Vector2 p2 = polygon.items[(i+1)%polygon.count];

                // line points
                // TODO we actually dont need the cannon line arguments?
                // TODO also this might / 0
                // Vector2 p3 = {0, line_y_given_x(perpendicular, 0)};
                // Vector2 p4 = {1, line_y_given_x(perpendicular, 1)};
                Vector2 p3 = perpendicular_points[0];
                Vector2 p4 = perpendicular_points[1];

                // get intersection
                Vector2 intersect = line_line_intersection(p1, p2, p3, p4);

                // check if it intersects the actual points.
                bool intersects_with_line = intersection_point_intersects(p1, p2, intersect);

                if (intersects_with_line) {

                    // assert(intersection_points_count < 2);
                    if (intersection_points_count == 2) {
                        // TODO debug this
                        // printf("WTF %zu\n", intersection_points_count);
                        // int k = 1234;
                        continue;
                    }
                    intersection_points[intersection_points_count] = intersect;
                    intersection_points_i[intersection_points_count] = i;
                    intersection_points_count += 1;
                }
            }

            // assert(intersection_points_count == 0 || intersection_points_count == 2);
            if (!(intersection_points_count == 0 || intersection_points_count == 2)) {
                // TODO debug this
                // printf("intersection_points_count: %zu\n", intersection_points_count);
                // int k = 123;
            }

            if (intersection_points_count == 2) {
                // cut the polygon into 2

                // reset the tmp polygons
                tmp_poly1.count = 0;
                tmp_poly2.count = 0;

                // index that loops over the points in the polygon
                size_t index = 0;

                assert(intersection_points_i[0] < polygon.count);
                assert(intersection_points_i[1] < polygon.count);

                while (index != intersection_points_i[0]) {
                    da_append(&tmp_poly1, polygon.items[index]);
                    index++;
                }
                da_append(&tmp_poly1, polygon.items[index]);
                da_append(&tmp_poly1, intersection_points[0]);

                da_append(&tmp_poly2, intersection_points[0]);
                index++;
                while (index != intersection_points_i[1]) {
                    da_append(&tmp_poly2, polygon.items[index]);
                    index++;
                }

                da_append(&tmp_poly2, polygon.items[index]);
                da_append(&tmp_poly2, intersection_points[1]);

                da_append(&tmp_poly1, intersection_points[1]);
                index++;
                while (index < polygon.count) {
                    da_append(&tmp_poly1, polygon.items[index]);
                    index++;
                }


                // now we have 2 polygons,
                // find the one that contains the original point and keep it.
                //
                // dont ask why this function works, dont know.
                if (isLeft(intersection_points[0], intersection_points[1], point)) {
                    // the left one contains the point.
                    SWAP(polygon, tmp_poly1);
                } else {
                    // the right one contains the point.
                    SWAP(polygon, tmp_poly2);
                }
            }

            // 6. Repeat 4-5 until all other points have been considered.
        }

        // 7. Convert the resulting convex polygon into triangles and draw them (maybe the bounding lines as well.)
        draw_polygon(polygon, colors[point_index], target.texture.height);
    }

    EndTextureMode();
}

