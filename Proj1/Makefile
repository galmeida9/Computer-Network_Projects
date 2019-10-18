SOURCES = Server.c Client.c lib/utility.c lib/commandlinereader.c
OBJS = $(SOURCES:%.c=%.o)
CC   = gcc
CFLAGS = -g -Wall -Wextra -Werror
CFLAGS += -fPIE -fstack-protector-all -D_FORTIFY_SOURCE=2
LDFLAGS = -lm

all: user FS

FS: Server.o lib/utility.o
	$(CC) $(CFLAGS) $^ -o FS $(LDFLAGS)

user: Client.o lib/utility.o lib/commandlinereader.o
	$(CC) $(CFLAGS) $^ -o user $(LDFLAGS)

clean:
	@echo Cleaning...
	rm -f $(OBJS) user FS
