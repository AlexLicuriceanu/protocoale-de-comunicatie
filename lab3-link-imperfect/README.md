We have **two separate programs**, the sender and the receiver that use the implementation of our protocol.

Normally, we would implement our protocol as a library, but to make things
easier, we implement part of the protocol in send.c, and the receving code in
recv.c. Note that we will use a Maximum Transmission Unit (MTU) of 1500.

``send.c`` - the code that the sender will execute

``recv.c`` - the code that the receiver on the other end of the wire will execute

``common.h`` - header where we define the header of the protocol and other common functions.

Overview of the architecture
```
	sender		      receiver
	  |			 |   
	data link	      data link
	 protocol	      protocol 		(used for framing)
	  |                      |
   Physical layer	   Physical layer		
      protocol 		      protocol
	  |______________________|
		Physical (wire)
```

## C++

Note, if you want to use C++, simply change the extension of the `send.c` and
`recv.c` to `.cpp` and update the Makefile to use `g++`.

## API
```c
/* This is the API exposed by the datalink protocol we're building upon and
already deals with framing. Our extension introduces the support for error
detection*/
/* Send len bytes from buffer on the data link */
int link_recv(void *buf, int len);
/* Receives a maximum of len bytes and writes it to buff */
void link_send(void *buf, int len);

/* The structure of our protocol */
/* Layer 3 header */    
struct l3_msg_hdr {    
        uint16_t len;    
        uint32_t sum;    
};    
    
/* Layer 3 frame */    
struct l3_msg {
        struct l3_msg_hdr hdr;    
        /* Data */    
        /* MTU = 1500 */    
        char payload[1500 - sizeof(l3_msg_hdr)];    
};    

```
## Usage
To compile the code
```
make
```

We will run the sender and receiver in parallel using ``run_experiment.sh``.
This scripts first runs the receiver binary and then runs the sender binary.
Note that we use the `CORRUPTION` variable from the script to set the
corruption rate.
