
CC = clang
CFLAGS = -Wall -Wextra -ggdb -pedantic -O2
RAYLIB_FLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

main: main.c
	$(CC) $(CFLAGS) -o main main.c $(RAYLIB_FLAGS)

clean:
	rm main
