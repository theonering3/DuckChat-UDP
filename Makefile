CC=g++

CFLAGS=-Wall -W -g  



all: client server

client: client.cc raw.cc
	$(CC) client.cc raw.cc $(CFLAGS) -o client

server: server.cc
	$(CC) server.cc $(CFLAGS) -o server

clean:
	rm -f client server *.o

