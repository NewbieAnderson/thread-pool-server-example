CC=gcc
CFLAGS=-pthread
SRCDIR=./src
BINDIR=./bin

all: server client

server: $(SRCDIR)/server/main.c $(SRCDIR)/server/server.c $(SRCDIR)/server/work_queue.c
	$(CC) $(CFLAGS) -o $(BINDIR)/server/server $^

client: $(SRCDIR)/client/client.c
	$(CC) $(CFLAGS) -o $(BINDIR)/client/client $^

clean:
	rm -f $(BINDIR)/server/server
	rm -f $(BINDIR)/client/client
