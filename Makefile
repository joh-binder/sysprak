CFLAGS ?= -Wall -Wextra -g
CC ?= gcc
GAME_ID ?= ""
PLAYER ?= 1

.PHONY: play clean

sysprak-client: main.o shmfunctions.o performConnection.o
	$(CC) $(CFLAGS) $^ -o $@

main.o:	main.c
	$(CC) $(CFLAGS) -c $^

shmfunctions.o: shmfunctions.c shmfunctions.h
	$(CC) $(CFLAGS) -c $<

performConnection.o: performConnection.c performConnection.h
	$(CC) $(CFLAGS) -c $<

play: sysprak-client
	./$< -g $(GAME_ID) -p $(PLAYER)

clean:
	rm -f sysprak-client main.o shmfunctions.o performConnection.o
