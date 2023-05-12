#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <chrono>
#include "link_emulator/lib.h"

using namespace std;

/* We don't touch this */
#define HOST "127.0.0.1"
#define PORT 10000

/* Here we have the Frame structure */
#include "common.h"

/* Our unqiue layer 2 ID */
static int ID = 123131;

/* Function which our protocol implementation will provide to the upper layer. */
int send_frame(const char *buf, int size)
{

	/* 1.1: Create a new frame. */
    auto *new_frame = new Frame;
    new_frame->frame_delim_start[0] = DLE;
    new_frame->frame_delim_start[1] = STX;
    new_frame->frame_delim_end[0] = DLE;
    new_frame->frame_delim_end[1] = ETX;

	/* 1.2: Copy the data from buffer to our frame structure */
    memcpy(new_frame->payload, buf, size);

	/* 2.1: Set the destination and source */
    new_frame->dest = TEST_DEST_ID;
    new_frame->source = TEST_SOURCE_ID;

	/* We can cast the frame to a char *, and iterate through sizeof(struct Frame) byte calling send_bytes. */
    char *cast_frame = (char *)new_frame;

    send_byte(cast_frame[0]);
    send_byte(cast_frame[1]);

    for (int i=2; i<2+2*sizeof(int); i++) {
        send_byte(cast_frame[i]);
    }

    for (int i = 2+2*sizeof(int); i < size+2+2*sizeof(int); i++) {
        if (cast_frame[i] == DLE) {
            send_byte(DLE);
        }
        send_byte(cast_frame[i]);
    }

    send_byte(cast_frame[sizeof(struct Frame)-2]);
    send_byte(cast_frame[sizeof(struct Frame)-1]);

	/* if all went all right, return 0 */
	return 0;
}

int main(int argc, const char** argv){
	// Don't touch this
	init(HOST,PORT);

	/* Send Hello */
	send_byte(DLE);
	send_byte(STX);
	send_byte('H');
	send_byte('e');
	send_byte('l');
	send_byte('l');
	send_byte('o');
	send_byte('!');
	send_byte(DLE);
	send_byte(ETX);

	/* 1.0: Get some input in a buffer and call send_frame with it */
    char buffer[MAX_PAYLOAD];
    ifstream fin("./testfile.txt");

    char c;
    int length = 0;
    while (fin.get(c)) {
        if (length == MAX_PAYLOAD) {
            send_frame(buffer, MAX_PAYLOAD);
            length = 0;
        }

        buffer[length] = c;
        length++;
    }

    if (length > 0) {
        length--;
    }
    send_frame(buffer, length);

	/* 3.1: Get a timestamp of the current time copy it in the the payload */
    auto begin_time = std::chrono::high_resolution_clock::now();

	return 0;
}
