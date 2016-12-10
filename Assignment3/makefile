CC = gcc
CFLAGS = -Wall -g -pedantic -std=gnu99
All : clubber clubhub
clubber : clubber.o
	$(CC) clubber.o -o clubber
clubber.o : clubber.c
	$(CC) $(CFLAGS) -c clubber.c
clubhub : clubhub.o
	$(CC) clubhub.o -o clubhub
clubhub.o : clubhub.c
	$(CC) $(CFLAGS) -c clubhub.c
