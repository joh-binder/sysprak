CFLAGS ?= -Wall -Wextra -g
CC ?= gcc
CONFIG ?= "client.conf"

.PHONY: play clean

sysprak-client: main.o mainloop.o util.o shmfunctions.o config.o thinkerfunctions.o util.o
	$(CC) $(CFLAGS) $^ -o $@

main.o:	main.c
	$(CC) $(CFLAGS) -c $^

mainloop.o: mainloop.c mainloop.h
	$(CC) $(CFLAGS) -c $<

util.o: util.c util.h
	$(CC) $(CFLAGS) -c $<

shmfunctions.o: shmfunctions.c shmfunctions.h
	$(CC) $(CFLAGS) -c $<

config.o: config.c config.h
	$(CC) $(CFLAGS) -c $<

thinkerfunctions.o: thinkerfunctions.c thinkerfunctions.h
	$(CC) $(CFLAGS) -c $<
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
	rm -f sysprak-client main.o shmfunctions.o mainloop.o util.o config.o thinkerfunctions.o util.o
