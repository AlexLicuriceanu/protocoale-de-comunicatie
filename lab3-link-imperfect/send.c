#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "common.h"
#include "link_emulator/lib.h"
#include <arpa/inet.h>

#define HOST "127.0.0.1"
#define PORT 10000


int main(int argc,char** argv) {
	init(HOST,PORT);

	/* Look in common.h for the definition of l3_msg */
	struct l3_msg t;
	t.hdr.type = 0;

	/* We set the payload */
	sprintf(t.payload, "Hello my World of PC! ");
	t.hdr.len = strlen(t.payload) + 1;

	/* Add the checksum */
	/* Note that we compute the checksum for both header and data. Thus
	 * we set the checksum equal to 0 when computing it */
	t.hdr.sum = 0;

	/* Since sum is on 32 bits, we have to convert it to network order */
	// t.hdr.sum = htonl(simple_csum((void *) &t, sizeof(struct l3_msg)));

	/* 2.0: Call crc32 function */
	t.hdr.sum = htonl(crc32((void *) &t, sizeof(struct l3_msg)));

	/* Send the message */
	link_send(&t, sizeof(struct l3_msg));

	/* TODO 3.1: Receive the confirmation */
	while (1) {

		struct l3_msg temp;
		int temp_len = link_recv(&temp, sizeof(struct l3_msg));

		if (temp.hdr.type == 2) {
			printf("[SEND] Received NACK, retransmitting frame.\n");

			link_send(&t, sizeof(struct l3_msg));
		}
	}

	/* TODO 3.2: If we received a NACK, retransmit the previous frame */

	/* TODO 3.3: Update this to read the content of a file and send it as
	 * chunks of that file given a MTU of 1500 bytes */

	return 0;
}
