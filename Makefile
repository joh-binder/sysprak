CFLAGS = -Wall -g
CC = gcc

all: HelloWorld.c
	$(CC) $(CFLAGS) HelloWorld.c -o HelloWorld

clean:
	rm -f HelloWorld
