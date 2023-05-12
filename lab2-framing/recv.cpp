#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include "link_emulator/lib.h"

/* Do not touch these two */
#define HOST "127.0.0.1"
#define PORT 10001

#include "common.h"

using namespace std;

/* Our unqiue layer 2 ID */
static int ID = 123131;

/* Function which our protocol implementation will provide to the upper layer. */
int recv_frame(char *buf, int size)
{
	/* 1.1: Call recv_byte() until we receive the frame start
	 * delimitator. This operation makes this function blocking until it
	 * receives a frame. */
    char c1,c2;
    c1 = recv_byte();
    c2 = recv_byte();

    while(c1 != DLE && c2 != STX) {
        c1 = c2;
        c2 = recv_byte();
    }

	/* 2.1: The first 2 * sizeof(int) bytes represent sender and receiver ID */
    int source_id = 0, dest_id = 0;
    char source_byte_buffer[4];
    char dest_byte_buffer[4];

    for (int i=0; i<sizeof(int); i++) {
        source_byte_buffer[i] = recv_byte();
    }

    for (int i=0; i<sizeof(int); i++) {
        dest_byte_buffer[i] = recv_byte();
    }

	/* 2.2: Check that the frame was sent to me */
    dest_id = (int)((unsigned char)(dest_byte_buffer[3]) << 24 |
                    (unsigned char)(dest_byte_buffer[2]) << 16 |
                    (unsigned char)(dest_byte_buffer[1]) << 8 |
                    (unsigned char)(dest_byte_buffer[0]));

    source_id = (int)((unsigned char)(source_byte_buffer[3]) << 24 |
                    (unsigned char)(source_byte_buffer[2]) << 16 |
                    (unsigned char)(source_byte_buffer[1]) << 8 |
                    (unsigned char)(source_byte_buffer[0]));

    if (dest_id != TEST_DEST_ID) {
        return -1;
    }

	/* 1.2: Read bytes and copy them to buff until we receive the end of the frame */
    for (int i=0; i<size; i++) {
        char byte = recv_byte();

        if (byte == DLE) {
            byte = recv_byte();

            if (byte == ETX) {
                return i;
            } else if (byte != DLE) {
                return -1;
            }
        }

        cout << byte << endl;
        buf[i] = byte;
    }

    recv_byte();
    return size;
}

int main(int argc,char** argv){
	/* Do not touch this */
	init(HOST,PORT);

	char c;

	/* Wait for the start of a frame */
	char c1,c2;
	c1 = recv_byte();
	c2 = recv_byte();

	/* Cat timp nu am primit DLE STX citim bytes */
	while((c1 != DLE) && (c2 != STX)) {
		c1 = c2;
		c2 = recv_byte();
	}


	printf("%d ## %d\n",c1, c2);
	c = recv_byte();
	printf("%c\n", c);

	c = recv_byte();
	printf("%c\n", c);

	c = recv_byte();
	printf("%c\n", c);

	c = recv_byte();
	printf("%c\n", c);

	c = recv_byte();
	printf("%c\n", c);

	c = recv_byte();
	printf("%c\n", c);

    /* DLE ETX */
    c = recv_byte();
    c = recv_byte();

	/* 1.0: Allocate a buffer and call recv_frame */
    char buffer[MAX_PAYLOAD];

    int nr_recv_bytes = recv_frame(buffer, MAX_PAYLOAD);

    while (nr_recv_bytes == MAX_PAYLOAD) {
        nr_recv_bytes = recv_frame(buffer, MAX_PAYLOAD);
    }

	/* TODO 3: Measure latency in a while loop for any frame that contains
	 * a timestamp we receive, print frame_size and latency */

	printf("[RECEIVER] Finished transmission\n");
	return 0;
}
