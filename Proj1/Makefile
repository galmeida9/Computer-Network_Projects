SOURCES = Server.c Client.c lib/utility.c
OBJS = $(SOURCES:%.c=%.o)
CC   = gcc
CFLAGS = -g -Wall -Wextra -Werror
#CFLAGS = -g -Wall -Wextra -Werror -Wconversion -Wsign-conversion -pedantic
#CFLAGS += -fPIE -fstack-protector-all -D_FORTIFY_SOURCE=2
LDFLAGS = -lm

all: user FS

FS: Server.o lib/utility.o
	$(CC) $(CFLAGS) $^ -o FS $(LDFLAGS)

user: Client.o lib/utility.o
	$(CC) $(CFLAGS) $^ -o user $(LDFLAGS)

Server.o: Server.c
Client.o: Client.c lib/utility.h
lib/utility.o: lib/utility.c lib/utility.h

clean:
	@echo Cleaning...
	rm -f $(OBJS) user FS
