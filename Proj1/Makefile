CC   = gcc
CFLAGS =-g -ansi -Wall
LDFLAGS = -lm

all:
	$(CC) Client.c -o user $(LDFLAGS)
	$(CC) Server.c -g -o FS $(LDFLAGS)

clean:
	rm -f user FS