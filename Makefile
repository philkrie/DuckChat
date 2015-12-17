CC=g++

CFLAGS=-Wall -W -g



all: client server

client: client.cpp
	g++ client.cpp $(CFLAGS) -o client

server: server.cpp 
	g++ server.cpp linked_list.cpp $(CFLAGS) -o server

clean:
	rm -f client server *.o

