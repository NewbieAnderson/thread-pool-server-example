all: server client

server:
	gcc -o ./bin/server/server ./src/server/server.c

client:
	gcc -o ./bin/client/client ./src/client/client.c