#define MAX_CLIENT_ID_LENGTH 10
#define MAX_CLIENTS 128
#define MAX_UDP_MSG_SIZE 1552
#define MAX_TOPIC_NAME_LENGTH 51
#define MAX_UDP_PAYLOAD_SIZE 1500
#define MAX_TCP_PAYLOAD_SIZE 1601
#define SF_ENABLED 1
#define SF_DISABLED 0

/**
 * Error checking macro.
 * Example:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */
#define DIE(assertion, call_description)	                    \
	do {									                    \
		if (assertion) {					                    \
			fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);	\
			perror(call_description);		                    \
			exit(EXIT_FAILURE);				                    \
		}									                    \
	} while(0)

/**
 * Structure representation for a udp message.
 */
struct udp_msg {
    char topic[MAX_TOPIC_NAME_LENGTH];
    unsigned char type;
	char data[MAX_UDP_PAYLOAD_SIZE];
};

/**
 * Structure representation for a tcp message.
 */
struct tcp_msg {
	char type;
	char data[MAX_TCP_PAYLOAD_SIZE];
};

/**
 * Structure representation for a subscribe/unsubscribe message
 * sent by the tcp clients.
 */
struct subscribe_msg {
	int type;
	char topic[MAX_TOPIC_NAME_LENGTH];
	int sf;
};
