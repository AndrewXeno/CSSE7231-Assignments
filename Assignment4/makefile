CC = gcc
CFLAGS = -Wall -g -pedantic -std=gnu99 -pthread
All : station
station : station.o
	$(CC) -pthread station.o -o station
station.o : station.c
	$(CC) $(CFLAGS) -c station.c