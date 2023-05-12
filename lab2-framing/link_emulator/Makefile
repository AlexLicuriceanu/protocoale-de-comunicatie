all: link lib.o

link: link.o queue.o
	g++ -g link.o queue.o -o link -lpthread

.c.o: 
	g++ -Wall -g -c $? -lpthread

clean:
	-rm *.o link
