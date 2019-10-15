SOURCES = Client.c Server.c lib/utility.c
OBJS = $(SOURCES:%.c=%.o)
CC   = gcc
CFLAGS =-g -ansi -Wall
LDFLAGS = -lm

all: user FS

FS: Server.o
	$(CC) -g Server.c -o FS $(LDFLAGS)

user: Client.o lib/utility.o
	$(CC) -g $^ -o user $(LDFLAGS)

Server.o: Server.c
Client.o: Client.c lib/utility.h
lib/utility.o: lib/utility.c lib/utility.h

Client.o lib/utility.o:
	$(CC) -g -c -o $@ $<

clean:
	@echo Cleaning...
	rm -f $(OBJS) user FS
