//
// dynamic_array.h - simple dynamic array
// Credit to tsoding for showing me the way: @Tsoding - https://www.youtube.com/c/Tsoding
//
// Fletcher M - 10/03/2025
//

#ifndef DYNAMIC_ARRAY_H_
#define DYNAMIC_ARRAY_H_

#include <assert.h>


#define DA_INIT_CAP 32

#define da_append(da, item)                                                                                \
    do {                                                                                                   \
        if ((da)->count >= (da)->capacity) {                                                               \
            (da)->capacity = (da)->capacity == 0 ? DA_INIT_CAP : (da)->capacity*2;                         \
            (da)->items = (typeof((da)->items)) realloc((da)->items, (da)->capacity*sizeof(*(da)->items)); \
            assert((da)->items != NULL && "Buy More RAM lol");                                             \
        }                                                                                                  \
                                                                                                           \
        (da)->items[(da)->count++] = (item);                                                               \
    } while (0)


#define da_stamp_and_remove(da, index)                      \
    do {                                                    \
        (da)->items[(index)] = (da)->items[(da)->count-1];  \
        (da)->count -= 1;                                   \
    } while (0)


#define da_free(da)                         \
    do {                                    \
        if ((da)->items) free((da)->items); \
        (da)->items    = 0;                 \
        (da)->count    = 0;                 \
        (da)->capacity = 0;                 \
    } while (0)


#define da_free_items(da)                       \
    do {                                        \
        for (u64 i = 0; i < (da)->count; i++) { \
            free((da)->items[i]);               \
        }                                       \
        (da)->count = 0;                        \
    } while (0)


#endif // DYNAMIC_ARRAY_H_
