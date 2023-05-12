#include "common.h"

#include <sys/socket.h>
#include <sys/types.h>

/*
    1.1: Rescrieți funcția de mai jos astfel încât ea să facă primirea
    a exact len octeți din buffer.
*/
int recv_all(int sockfd, void *buffer, size_t len) {

    size_t bytes_received = 0;
    size_t bytes_remaining = len;
    char *buff = buffer;

    while (bytes_remaining) {

        size_t bytes = recv(sockfd, buff, bytes_remaining, 0);

        if (bytes <= 0) {
            return bytes; // return the number of bytes received or a negative value in case of error
        }

        bytes_received += bytes;
        buff += bytes;
        bytes_remaining -= bytes;
    }

    return bytes_received;
}

/*
    1.2: Rescrieți funcția de mai jos astfel încât ea să facă trimiterea
    a exact len octeți din buffer.
*/

int send_all(int sockfd, void *buffer, size_t len) {

    size_t bytes_sent = 0;
    size_t bytes_remaining = len;
    char *buff = buffer;

    while (bytes_remaining) {

        int bytes = send(sockfd, buff, bytes_remaining, 0);

        if (bytes < 0) {
            return bytes; // return the error code
        }

        bytes_sent += bytes;
        buff += bytes;
        bytes_remaining -= bytes;
    }

    return bytes_sent;
}