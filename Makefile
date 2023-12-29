CC = gcc
CFLAGS = -pthread
SRCDIR = ./src
BINDIR = ./bin

SERVER_SRCS = $(SRCDIR)/server/main.c \
              $(SRCDIR)/server/server.c \
			  $(SRCDIR)/server/session_buffer.c \
			  $(SRCDIR)/server/worker_thread.c

TEST_SRCS = $(SRCDIR)/test/test_local.c

SERVER_BINARY = $(BINDIR)/server/server

TEST_BINARY = $(BINDIR)/test/test

all: server test

server: $(SERVER_BINARY)

test: $(TEST_BINARY)

$(SERVER_BINARY): $(SERVER_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_BINARY): $(TEST_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(SERVER_BINARY)
	rm -f $(TEST_BINARY)