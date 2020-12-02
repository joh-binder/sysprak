CFLAGS = -Wall -Wextra -g
CC = gcc

all: main.o shmfunctions.o
	$(CC) $(CFLAGS) main.o shmfunctions.o -o sysprak-client

main.o:	main.c
	$(CC) $(CFLAGS) -c main.c

shmfunctions.o: shmfunctions.c shmfunctions.h
	$(CC) $(CFLAGS) -c shmfunctions.c

clean:
	rm -f sysprak-client main.o shmfunctions.o