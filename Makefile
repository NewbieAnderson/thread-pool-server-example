CC = gcc
CFLAGS = -pthread
SRCDIR = ./src
BINDIR = ./bin

SERVER_SRCS = $(SRCDIR)/server/main.c \
              $(SRCDIR)/server/server.c \
			  $(SRCDIR)/server/session_buffer.c \
			  $(SRCDIR)/server/worker_thread.c

CLIENT_SRCS = $(SRCDIR)/client/client.c

SERVER_BINARY = $(BINDIR)/server/server

CLIENT_BINARY = $(BINDIR)/client/client

all: server client

server: $(SERVER_BINARY)

client: $(CLIENT_BINARY)

$(SERVER_BINARY): $(SERVER_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

$(CLIENT_BINARY): $(CLIENT_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(SERVER_BINARY)
	rm -f $(CLIENT_BINARY)
