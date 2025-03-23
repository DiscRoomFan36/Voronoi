
CC = clang
CFLAGS = -Wall -Wextra -ggdb
CFLAGS += -O2
RAYLIB_FLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

main: main.o voronoi.o
	$(CC) $(CFLAGS) -o main main.o voronoi.o $(RAYLIB_FLAGS)

main.o: main.c profiler.h
	$(CC) $(CFLAGS) -c -o main.o main.c

voronoi.o: voronoi.h voronoi.c
	$(CC) $(CFLAGS) -c -o voronoi.o voronoi.c

clean:
	rm main
