CC=gcc
CFLAGS=-g -Wall -I.

srcds-stdio: srcds-stdio.o
	$(CC) $(CFLAGS) -o srcds-stdio srcds-stdio.o
