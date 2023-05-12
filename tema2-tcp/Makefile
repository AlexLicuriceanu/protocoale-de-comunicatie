CC = g++
CFLAGS = -Wall -g

all: server subscriber

server:
	$(CC) $(CFLAGS) server.cpp -o server

subscriber:
	$(CC) $(CFLAGS) subscriber.cpp -o subscriber

.PHONY: clean

clean:
	rm -f server subscriber
