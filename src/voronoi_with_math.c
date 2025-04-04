
#include <stdlib.h>
#include <assert.h>

#include "voronoi.h"

#include "raymath.h"

#include "common.h"

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


typedef struct DoubleVector2 {
    double x, y;
} DoubleVector2;


typedef struct Polygon {
    // the points of the polygon
    DoubleVector2 *items;
    size_t count;
    size_t capacity;
} Polygon;

#include <stdio.h>

// draw a convex polygon, with points in clockwise order
// flip height, if not zero, flip vertical
void draw_polygon(Polygon polygon, Color color, int flip_height) {
    for (size_t i = 1; i < polygon.count - 1; i++) {
        Vector2 v1 = {polygon.items[0  ].x, polygon.items[0  ].y};
        Vector2 v2 = {polygon.items[i  ].x, polygon.items[i  ].y};
        Vector2 v3 = {polygon.items[i+1].x, polygon.items[i+1].y};

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
DoubleVector2 line_line_intersection(DoubleVector2 p1, DoubleVector2 p2, DoubleVector2 p3, DoubleVector2 p4) {
    float w = (p1.x - p2.x)*(p3.y - p4.y) - (p1.y - p2.y)*(p3.x - p4.x);

    if (w == 0) {
        // parallel or coincident, return junk number
        return (DoubleVector2){FINF, FINF};
    }

    float d_x = (p1.x*p2.y - p1.y*p2.x)*(p3.x - p4.x) - (p1.x - p2.x)*(p3.x*p4.y - p3.y*p4.x);
    float d_y = (p1.x*p2.y - p1.y*p2.x)*(p3.y - p4.y) - (p1.y - p2.y)*(p3.x*p4.y - p3.y*p4.x);

    // intersection points.
    float i_x = d_x / w;
    float i_y = d_y / w;

    return (DoubleVector2){i_x, i_y};
}

// check if two line points, are intersected with a point
// that is known to intersect with it.
bool intersection_point_intersects(DoubleVector2 p1, DoubleVector2 p2, DoubleVector2 intersect) {
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
bool isLeft(DoubleVector2 a, DoubleVector2 b, DoubleVector2 c) {
    return (b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x) > 0;
}


// the polygons are global variables, so they keep their malloc's between draw calls.
Polygon polygon;
Polygon tmp_poly1;
Polygon tmp_poly2;

Polygon points_double = {0};


void init_voronoi(void) {
    // clear the polygon's
    polygon   = (Polygon){0};
    tmp_poly1 = (Polygon){0};
    tmp_poly2 = (Polygon){0};

    points_double = (Polygon){0};
}

void finish_voronoi(void) {
    da_free(&polygon);
    da_free(&tmp_poly1);
    da_free(&tmp_poly2);

    da_free(&points_double);
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


    // Notes:
    //
    // I believe this solution has the potential to be one
    // of the fastest implementations, something even better
    // than the shaders. Why? because even without more
    // optimization, this solution gets around 600 points
    // before an fps drop, and its basically an O(n^2) algorithm
    // at this point, but i believe it can go kinda close to
    // an O(n*c) algorithm.
    //
    // if you first check the points that are closest to the checking point,
    // you will remove massive chucks of the polygon,
    // and after those close points, no other point will have
    // an effect on the resulting polygon.
    //
    // also, a fast way to check if a point will effect the polygon,
    // (without getting the perpendicular line), is to check the distance
    // of the point vs the point on the polygon that is furthest away
    // from the point. if the distance to the checking point is > 2x the distance
    // the the furthest point on the polygon, it can be safely rejected.
    //
    // so a fast algorithm would sort the points by distance, and then
    // when this distance is to large, reject the rest of the points.
    //
    // when observing the voronoi diagram, most sections have 3-5 edges,
    // so at most this fast algorithm would only check ~10 points before
    // all its final edges have been formed, and all points would only check ~10
    // of its neighbors, in the avarage case.
    //
    // NOTE: real sorting would be extreally slow, (it would make the algorithm
    // O(n^2 * ln(n)), because every point needs to sort its neighbors.)
    // so the thought is to first sort the points into a spacial array, and then
    // for every point, check its neighbors with a progressively larger search,
    // until you can be sure no closer points exist.
    //
    // (the above algorithm is still O(n^2) in the worst case, but the average
    // case is O(n*c))
    //
    // ALSO NOTE: this solution get 600 points before dropping below 60fps,
    // and that is without, useing multithreading, (witch the optimized solution above
    // will also be acceptable to.), a 5x-6x speedup is easily possible.


    // useing this polygon as a double vector2 array
    points_double.count = 0;
    for (size_t i = 0; i < num_points; i++) {
        da_append(&points_double, ((DoubleVector2){(double)points[i].x, (double)points[i].y}));
    }


    for (size_t point_index = 0; point_index < num_points; point_index++) {
        // 1. Get a point.
        DoubleVector2 point = points_double.items[point_index];

        // 2. Construct a polygon that fills the screen
        polygon.count = 0;
        da_append(&polygon, ((DoubleVector2){    0,      0}));
        da_append(&polygon, ((DoubleVector2){width,      0}));
        da_append(&polygon, ((DoubleVector2){width, height}));
        da_append(&polygon, ((DoubleVector2){    0, height}));


        // 3. For every other point:
        for (size_t other_point_index = 0; other_point_index < num_points; other_point_index++) {
            if (other_point_index == point_index) continue;

            DoubleVector2 other_point = points_double.items[other_point_index];

            // 4. Find the mid line parallel to those point

            // the points in here form a perpendicular line.
            DoubleVector2 perpendicular_points[2];
            {
                DoubleVector2 p1 = point;
                DoubleVector2 p2 = other_point;

                // mid point
                // m = (p1 + p2) / 2
                DoubleVector2 m = {(p1.x + p2.x) / 2, (p1.y + p2.y) / 2};

                // direction vector
                // v = p2 - p1
                DoubleVector2 v = {p2.x - p1.x, p2.y - p1.y}; // Vector2Subtract(p2, p1);

                // rotated 90 direction vector
                // v1 = (-v.y, v.x)
                DoubleVector2 v_1 = {-v.y, v.x};

                // add to the mid point
                // m1 = m + v1
                DoubleVector2 m_1 = {m.x + v_1.x, m.y + v_1.y}; // Vector2Add(m, v_1);

                perpendicular_points[0] = m;
                perpendicular_points[1] = m_1;
            }


            // 5. Cut the polygon and keep the side that is close to the original point
            DoubleVector2 intersection_points[2];
            size_t intersection_points_i[2];
            size_t intersection_points_count = 0;

            // loop over all edges
            for (size_t i = 0; i < polygon.count; i++) {
                DoubleVector2 p1 = polygon.items[i];
                DoubleVector2 p2 = polygon.items[(i+1)%polygon.count];

                // perpendicular points.
                DoubleVector2 p3 = perpendicular_points[0];
                DoubleVector2 p4 = perpendicular_points[1];

                // get intersection
                DoubleVector2 intersect = line_line_intersection(p1, p2, p3, p4);

                // if the line segment intersects with the intersect point, add it to the intersection array.
                if (intersection_point_intersects(p1, p2, intersect)) {

                    if (intersection_points_count == 2) {
                        // TODO debug this
                        // printf("WTF %zu\n", intersection_points_count);
                        continue;
                    }
                    intersection_points[intersection_points_count] = intersect;
                    intersection_points_i[intersection_points_count] = i;
                    intersection_points_count += 1;
                }
            }

            if (!(intersection_points_count == 0 || intersection_points_count == 2)) {
                // TODO debug this
                // printf("intersection_points_count: %zu\n", intersection_points_count);
            }

            // check if the line intersected the polygon.
            // this should either be 0 (for when the line missed)
            // or 2 (where it entered and exited)
            //
            // (however this is somewhat broken, possibly because of float precision?)
            if (intersection_points_count == 2) {
                // cut the polygon into 2

                // reset the tmp polygons
                tmp_poly1.count = 0;
                tmp_poly2.count = 0;

                // index that loops over the points in the polygon
                size_t index = 0;

                // add points to the first polygon until we get to the first intersection point
                while (index != intersection_points_i[0]) {
                    da_append(&tmp_poly1, polygon.items[index]);
                    index++;
                }
                // add the start of the intersected line and the first intersection point.
                da_append(&tmp_poly1, polygon.items[index]);
                da_append(&tmp_poly1, intersection_points[0]);

                // second polygon starts from here with the first intersection point
                da_append(&tmp_poly2, intersection_points[0]);
                // loop unil the next intersect
                index++;
                while (index != intersection_points_i[1]) {
                    da_append(&tmp_poly2, polygon.items[index]);
                    index++;
                }
                // same as before and the point and the second intersect
                da_append(&tmp_poly2, polygon.items[index]);
                da_append(&tmp_poly2, intersection_points[1]);

                // and add the second intersection the the first polygon
                da_append(&tmp_poly1, intersection_points[1]);
                // finally add the rest to the first polygon
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
        draw_polygon(polygon, colors[point_index], height);
    }

    EndTextureMode();
}

