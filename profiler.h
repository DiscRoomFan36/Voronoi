

#ifndef PROFILER_H_
#define PROFILER_H_

#ifdef PROFILE_CODE
    #define PROFILER_ZONE(title) profiler_zone(title, __FILE__, __LINE__)
    #define PROFILER_ZONE_END()  profiler_zone_end()

    #define PROFILER_PRINT() profiler_print()
    #define PROFILER_RESET() profiler_reset()
    #define PROFILER_FREE()  profiler_free()
#else
    #define PROFILER_ZONE(...)
    #define PROFILER_ZONE_END(...)

    #define PROFILER_PRINT(...)
    #define PROFILER_RESET(...)
    #define PROFILER_FREE(...)
#endif // PROFILE_CODE


#include <stdlib.h>
#include <time.h>

#ifndef PROFILER_ASSERT
# include <assert.h>
# define PROFILER_ASSERT assert
#endif


typedef clock_t time_unit;

typedef struct Profiler_Data {
    const char *title;
    time_unit start_time;
    time_unit end_time;

    const char *file; // these could be useful
    int         line; // these could be useful
} Profiler_Data;

typedef struct Profiler_Data_Array {
    Profiler_Data *items;
    size_t count;
    size_t capacity;
} Profiler_Data_Array;

typedef struct Time_Unit_Array {
    time_unit *items;
    size_t count;
    size_t capacity;
} Time_Unit_Array;


// collects like Profile data, useing file line and func
typedef struct Profiler_Stats {
    const char *title;

    const char *file;
    int         line;
    const char *func;

    Time_Unit_Array times;
} Profiler_Stats;

typedef struct Profiler_Stats_Array {
    Profiler_Stats *items;
    size_t count;
    size_t capacity;
} Profiler_Stats_Array;


time_unit get_time(void);
double time_units_to_secs(time_unit x);


int profiler_equal(Profiler_Data a, Profiler_Data b);

void profiler_zone(const char *title, const char *__file__, int __line__);
void profiler_zone_end(void);

void profiler_print(void);
void profiler_reset(void);
void profiler_free(void);

Profiler_Stats_Array collect_stats(void);


#define profiler_da_append(da, item)                                                                        \
    do {                                                                                                   \
        if ((da)->count >= (da)->capacity) {                                                               \
            (da)->capacity = (da)->capacity == 0 ? 32 : (da)->capacity*2;                                  \
            (da)->items = (typeof((da)->items)) realloc((da)->items, (da)->capacity*sizeof(*(da)->items)); \
            assert((da)->items != NULL && "Buy More RAM lol");                                             \
        }                                                                                                  \
                                                                                                           \
        (da)->items[(da)->count++] = (item);                                                               \
    } while (0)

#define profiler_da_free(da)                 \
    do {                                    \
        if ((da)->items) free((da)->items); \
        (da)->items    = 0;                 \
        (da)->count    = 0;                 \
        (da)->capacity = 0;                 \
    } while (0)



#endif // PROFILER_H_


#ifdef PROFILER_IMPLEMENTATION

#ifndef PROFILER_IMPLEMENTATION_
#define PROFILER_IMPLEMENTATION_


Profiler_Data_Array __base_zones = {0};


time_unit get_time(void) {
    return clock();
}
double time_units_to_secs(time_unit x) {
    return (double) x / (double) CLOCKS_PER_SEC;
}

int profiler_equal(Profiler_Data a, Profiler_Data b) {
    if (a.title != b.title) return 0;
    if (a.file != b.file) return 0;
    if (a.line != b.line) return 0;
    return 1;
}


// profile the things after this call
// will stop with profiler_end_zone
void profiler_zone(const char *title, const char *__file__, int __line__) {
    time_unit start = get_time();
    Profiler_Data data = {
        .title = title,
        .start_time = start,
        .end_time = 0,

        .file = __file__,
        .line = __line__,
    };

    profiler_da_append(&__base_zones, data);
}

void profiler_zone_end(void) {
    time_unit end = get_time();

    for (int i = __base_zones.count-1; i >= 0; i--) {
        Profiler_Data *it = &__base_zones.items[i];

        // if its not zero, this zone has already been completed...
        if (it->end_time != 0) continue;

        it->end_time = end;
        return;
    }

    PROFILER_ASSERT(0 && "Unreachable: couldn't find a un-ended zone");
}


#define DIGITS_OF_PRECISION 3
// how many tens digits before the '.'
#define NUM_TENS_DIGITS 3
// +1 for the dot
#define PAD_DIGITS (DIGITS_OF_PRECISION + NUM_TENS_DIGITS + 1)

size_t profiler_strlen(const char *str) {
    size_t n = 0;
    while (*str++) n++;
    return n;
}

void profiler_print(void) {
    printf("Profiling Results:\n");

    int max_word_length = 0;
    for (size_t i = 0; i < __base_zones.count; i++) {
        Profiler_Data it = __base_zones.items[i];
        int title_len = profiler_strlen(it.title);
        if (max_word_length < title_len) max_word_length = title_len;
    }

    for (size_t i = 0; i < __base_zones.count; i++) {
        Profiler_Data it = __base_zones.items[i];

        if (it.end_time == 0) continue;
        float secs = time_units_to_secs(it.end_time - it.start_time);

        printf("|   ");
        printf("%-*s", max_word_length, it.title);
        printf(" : ");
        printf("%*.*f", PAD_DIGITS, DIGITS_OF_PRECISION, secs);
        printf("\n");
    }
}


void profiler_reset(void) {
    __base_zones.count = 0;
}
void profiler_free(void) {
    profiler_da_free(&__base_zones);
}


int maybe_index_in_array(Profiler_Data_Array array, Profiler_Data checking) {
    for (size_t i = 0; i < array.count; i++) {
        Profiler_Data item = array.items[i];
        if (profiler_equal(item, checking)) {
            return i;
        }
    }
    return -1;
}

Profiler_Stats_Array collect_stats(void) {
    Profiler_Stats_Array result = {0};     // linked arrays
    Profiler_Data_Array unique_data = {0}; // linked arrays

    for (size_t i = 0; i < __base_zones.count; i++) {
        Profiler_Data it = __base_zones.items[i];

        if (it.end_time == 0) continue;
        // check weather this data is in the unique data array.
        int maybe_index = maybe_index_in_array(unique_data, it);

        if (maybe_index == -1) {
            // its not in the array.
            Profiler_Stats new_stats = {
                .title = it.title,
                .file  = it.file,
                .line  = it.line,
            };

            profiler_da_append(&unique_data, it);
            profiler_da_append(&result, new_stats);
            maybe_index = unique_data.count-1;
        }

        Profiler_Stats *stats = &result.items[maybe_index];
        time_unit time = it.end_time - it.start_time;
        profiler_da_append(&stats->times, time);
    }

    profiler_da_free(&unique_data);
    return result;
}


// TODO move this
typedef struct Numerical_Average_Bounds {
    double sample_mean;
    double standard_deviation;
    double standard_error;
    double confidence_interval_upper;
    double confidence_interval_lower;
} Numerical_Average_Bounds;

typedef struct Double_Array {
    double *items;
    size_t count;
    size_t capacity;
} Double_Array;


#include <math.h>

Numerical_Average_Bounds get_numerical_average(Double_Array numbers) {
    double sample_mean = 0;
    for (size_t i = 0; i < numbers.count; i++) {
        double secs = time_units_to_secs(numbers.items[i]);

        // TODO? use https://en.wikipedia.org/wiki/Kahan_summation_algorithm
        sample_mean += secs;
    }
    sample_mean /= numbers.count;

    double standard_deviation = 0;
    for (size_t i = 0; i < numbers.count; i++) {
        double secs = time_units_to_secs(numbers.items[i]);

        // TODO? use https://en.wikipedia.org/wiki/Kahan_summation_algorithm
        standard_deviation += (secs - sample_mean)*(secs - sample_mean);
    }
    standard_deviation /= numbers.count;

    return (Numerical_Average_Bounds){
        .sample_mean = sample_mean,
        .standard_deviation = standard_deviation,
        .standard_error = standard_deviation / numbers.count,
        .confidence_interval_upper = sample_mean + 0.95 * (standard_deviation / sqrt(numbers.count)),
        .confidence_interval_lower = sample_mean - 0.95 * (standard_deviation / sqrt(numbers.count)),
    };
}


#endif // PROFILER_IMPLEMENTATION_

#endif // PROFILER_IMPLEMENTATION
