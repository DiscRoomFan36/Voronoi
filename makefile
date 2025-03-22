
CC = clang
CFLAGS = -Wall -Wextra -ggdb -pedantic
CFLAGS += -O2
RAYLIB_FLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

main: FORCE
	$(CC) $(CFLAGS) -o main main.c $(RAYLIB_FLAGS)

clean:
	rm main

FORCE:
