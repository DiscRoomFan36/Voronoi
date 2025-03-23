
CC = clang

CFLAGS = -Wall -Wextra -ggdb
CFLAGS += -O2

DEFINES += -DPROFILE_CODE

RAYLIB_FLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11


main: main.o voronoi.o
	$(CC) $(CFLAGS) $(DEFINES) -o main main.o voronoi.o $(RAYLIB_FLAGS)

main.o: main.c profiler.h
	$(CC) $(CFLAGS) $(DEFINES) -c -o main.o main.c

voronoi.o: voronoi.h voronoi.c profiler.h
	$(CC) $(CFLAGS) $(DEFINES) -c -o voronoi.o voronoi.c

clean:
	rm main
