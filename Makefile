CFLAGS = -Wall -Wextra -g
CC = gcc
GAME_ID = ""
PLAYER = 1

all: main.o shmfunctions.o thinkerfunctions.o
	$(CC) $(CFLAGS) main.o shmfunctions.o thinkerfunctions.o -o sysprak-client

main.o:	main.c
	$(CC) $(CFLAGS) -c main.c

shmfunctions.o: shmfunctions.c shmfunctions.h
	$(CC) $(CFLAGS) -c shmfunctions.c

thinkerfunctions.o: thinkerfunctions.c thinkerfunctions.h
	$(CC) $(CFLAGS) -c thinkerfunctions.c

play: sysprak-client
	./sysprak-client -g $(GAME_ID) -p $(PLAYER)

clean:
	rm -f sysprak-client main.o shmfunctions.o