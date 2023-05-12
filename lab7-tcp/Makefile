# Protocoale de comunicatii
# Laborator 7 - TCP
# Echo Server
# Makefile

CFLAGS = -Wall -g -Werror -Wno-error=unused-variable

# Portul pe care asculta serverul
PORT = 12345

# Adresa IP a serverului
IP_SERVER = 127.0.0.1

all: server client

common.o: common.c

# Compileaza server.c
server: server.c common.o

# Compileaza client.c
client: client.c common.o

.PHONY: clean run_server run_client

# Ruleaza serverul
run_server:
	./server ${IP_SERVER} ${PORT}

# Ruleaza clientul 	
run_client:
	./client ${IP_SERVER} ${PORT}

clean:
	rm -rf server client *.o *.dSYM
