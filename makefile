
CC = clang

CFLAGS = -Wall -Wextra -ggdb
CFLAGS += -O2

DEFINES += -DPROFILE_CODE

RAYLIB_FLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11


# TODO make this cleaner with %.o: %.c stuff.

all: build/bin/main_simple build/bin/main_simple_threaded build/bin/main_shader build/bin/main_shader_buffer build/bin/main_with_math


# ---------------------------------------------------
#             All the different versions
#    Only difference is the voronoi_*.o
# ---------------------------------------------------

build/bin/main_simple: build/main.o build/voronoi_simple.o                        | build/bin
	$(CC) $(CFLAGS) $(DEFINES) -o build/bin/main_simple build/main.o build/voronoi_simple.o $(RAYLIB_FLAGS)

build/bin/main_simple_threaded: build/main.o build/voronoi_simple_threaded.o      | build/bin
	$(CC) $(CFLAGS) $(DEFINES) -o build/bin/main_simple_threaded build/main.o build/voronoi_simple_threaded.o $(RAYLIB_FLAGS)

build/bin/main_shader: build/main.o build/voronoi_shader.o                        | build/bin
	$(CC) $(CFLAGS) $(DEFINES) -o build/bin/main_shader build/main.o build/voronoi_shader.o $(RAYLIB_FLAGS)

build/bin/main_shader_buffer: build/main.o build/voronoi_shader_buffer.o          | build/bin
	$(CC) $(CFLAGS) $(DEFINES) -o build/bin/main_shader_buffer build/main.o build/voronoi_shader_buffer.o $(RAYLIB_FLAGS)

build/bin/main_with_math: build/main.o build/voronoi_with_math.o                  | build/bin
	$(CC) $(CFLAGS) $(DEFINES) -o build/bin/main_with_math build/main.o build/voronoi_with_math.o $(RAYLIB_FLAGS)


# ---------------------------------------------------
#                  The Main File
# ---------------------------------------------------

build/main.o: src/main.c src/voronoi.h src/common.h src/profiler.h    | build
	$(CC) $(CFLAGS) $(DEFINES) -c -o build/main.o src/main.c


# ---------------------------------------------------
#             Different Voronoi Backends
# ---------------------------------------------------

build/voronoi_simple.o: src/voronoi.h src/voronoi_simple.c src/common.h                                     | build
	$(CC) $(CFLAGS) $(DEFINES) -c -o build/voronoi_simple.o src/voronoi_simple.c

build/voronoi_simple_threaded.o: src/voronoi.h src/voronoi_simple_threaded.c src/common.h                   | build
	$(CC) $(CFLAGS) $(DEFINES) -c -o build/voronoi_simple_threaded.o src/voronoi_simple_threaded.c

build/voronoi_shader.o: src/voronoi.h src/voronoi_shader.c src/common.h                                     | build
	$(CC) $(CFLAGS) $(DEFINES) -c -o build/voronoi_shader.o src/voronoi_shader.c

build/voronoi_shader_buffer.o: src/voronoi.h src/voronoi_shader_buffer.c src/common.h                       | build
	$(CC) $(CFLAGS) $(DEFINES) -c -o build/voronoi_shader_buffer.o src/voronoi_shader_buffer.c

build/voronoi_with_math.o: src/voronoi.h src/voronoi_with_math.c src/common.h                               | build
	$(CC) $(CFLAGS) $(DEFINES) -c -o build/voronoi_with_math.o src/voronoi_with_math.c


src/common.h: src/profiler.h


build:
	mkdir -p build/
build/bin:
	mkdir -p build/bin/

clean:
	rm -rf build/
