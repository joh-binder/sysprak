CFLAGS ?= -Wall -Wextra -g
CC ?= gcc
CONFIG ?= "client.conf"

.PHONY: play clean

sysprak-client: main.o helperfunctions.a
	$(CC) $(CFLAGS) $^ -o $@

main.o:	main.c
	$(CC) $(CFLAGS) -c $^

helperfunctions.a: shmfunctions.o performConnection.o config.o thinkerfunctions.o
	ar -q helperfunctions.a $^

shmfunctions.o: shmfunctions.c shmfunctions.h
	$(CC) $(CFLAGS) -c $<

performConnection.o: performConnection.c performConnection.h
	$(CC) $(CFLAGS) -c $<

config.o: config.c config.h
	$(CC) $(CFLAGS) -c $<

thinkerfunctions.o: thinkerfunctions.c thinkerfunctions.h
	$(CC) $(CFLAGS) -c thinkerfunctions.c

play: sysprak-client
ifndef GAME_ID
	$(error Game-ID fehlt. Geben Sie diese in der Form GAME_ID="..." an)
else
ifndef PLAYER
	./$< -g $(GAME_ID) -c $(CONFIG)
else
	./$< -g $(GAME_ID) -p $(PLAYER) -c $(CONFIG)
endif
endif

clean:
	rm -f sysprak-client main.o shmfunctions.o performConnection.o config.o thinkerfunctions.o helperfunctions.a
