CFLAGS ?= -Wall -Wextra -g
CC ?= gcc
GAME_ID ?= ""
PLAYER ?= 1
CONFIG ?= "client.conf"

.PHONY: play clean

sysprak-client: main.o shmfunctions.o performConnection.o config.o
	$(CC) $(CFLAGS) $^ -o $@

main.o:	main.c
	$(CC) $(CFLAGS) -c $^

shmfunctions.o: shmfunctions.c shmfunctions.h
	$(CC) $(CFLAGS) -c $<

performConnection.o: performConnection.c performConnection.h
	$(CC) $(CFLAGS) -c $<

config.o: config.c config.h
	$(CC) $(CFLAGS) -c $<

play: sysprak-client
	./$< -g $(GAME_ID) -p $(PLAYER) -c $(CONFIG)

clean:
	rm -f sysprak-client main.o shmfunctions.o performConnection.o config.o
