Physical layer simulator. We have **two separate programs**, the sender and the receiver.

Normally, we would implement our protocol as a library, but to make things
easier, we implement part of the protocol in send.c, and the receving code in
recv.c.

``send.c`` - the code that the sender will execute

``recv.c`` - the code that the receiver on the other end of the wire will execute

``common.h`` - header where we define the `Frame` structure and the API that the physical layers exports

Overview of the architecture
```
	sender                receiver
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
```
/* Sends one character to the other end via the Physical layer */
int send_byte(char c);
/* Receives one character from the other end, if nothing is sent by the other end, returns a random character */
char recv_byte();
```
## Usage
To compile the code
```
make
```

We will run the sender and receiver in parallel using ``run_experiment.sh``.
This scripts first runs the receiver binary and then runs the sender binary.
There are several parameters that can be used to modify the characteristics of
the link, but we don't need them for this lab.
