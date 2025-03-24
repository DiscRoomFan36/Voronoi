
CC = clang

CFLAGS = -Wall -Wextra -ggdb
CFLAGS += -O2

DEFINES += -DPROFILE_CODE

RAYLIB_FLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11


all: build/bin/main_simple build/bin/main_simple_threaded


# ---------------------------------------------------
#             All the different versions
#    Only difference is the voronoi_*.o
# ---------------------------------------------------

build/bin/main_simple: build/main.o build/voronoi_simple.o                        | build/bin
	$(CC) $(CFLAGS) $(DEFINES) -o build/bin/main_simple build/main.o build/voronoi_simple.o $(RAYLIB_FLAGS)

build/bin/main_simple_threaded: build/main.o build/voronoi_simple_threaded.o      | build/bin
	$(CC) $(CFLAGS) $(DEFINES) -o build/bin/main_simple_threaded build/main.o build/voronoi_simple_threaded.o $(RAYLIB_FLAGS)


# ---------------------------------------------------
#                  The Main File
# ---------------------------------------------------

build/main.o: main.c profiler.h    | build
	$(CC) $(CFLAGS) $(DEFINES) -c -o build/main.o main.c


# ---------------------------------------------------
#             Different Voronoi Backends
# ---------------------------------------------------

build/voronoi_simple.o: voronoi.h voronoi_simple.c profiler.h                      | build
	$(CC) $(CFLAGS) $(DEFINES) -c -o build/voronoi_simple.o voronoi_simple.c

build/voronoi_simple_threaded.o: voronoi.h voronoi_simple_threaded.c profiler.h    | build
	$(CC) $(CFLAGS) $(DEFINES) -c -o build/voronoi_simple_threaded.o voronoi_simple_threaded.c



build:
	mkdir -p build/
build/bin:
	mkdir -p build/bin/

clean:
	rm -rf build/
