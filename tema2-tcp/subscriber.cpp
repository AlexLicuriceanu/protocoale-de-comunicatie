#include <iostream>
#include <vector>
#include <iomanip>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <cctype>

#include "./structures.h"

using namespace std;

vector<string> split_command(string command, string delim) {

    // Initialize the empty vector for the tokens.
    vector<string> result;
    char *command_copy = (char *) command.c_str();
    char *delim_copy = (char *) delim.c_str();

    // Standard splitting algorithm.
    char *token = strtok(command_copy, delim_copy);
    while (token != nullptr) {
        result.emplace_back(token);
        token = strtok(nullptr, delim_copy);
    }

    return result;
}

bool is_string_number(string str) {
    int start_index = 0;

    if (str[0] == '-') {
        start_index = 1;
    }

    for (long unsigned int i = start_index; i < str.length(); i++) {
        if (!isdigit(str[i])) {
            return false;
        }
    }
    return true;
}

string lowercase(string str) {

    // Iterate all characters, transform all to lowercase.
    for (auto &c: str) {
        c = (char) tolower(c);
    }
    return str;
}

int main(int argc, char *argv[]) {

    // Handle argument errors.
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " id_client ip_server port_server" << endl;
        exit(0);
    }

    // Disable buffering.
    setvbuf(stdout, nullptr, _IONBF, BUFSIZ);

    // Convert the port to a number.
    int port = stoi(argv[3]);
    DIE(port == 0, "port");

    int server_socket, rc, flag;

    // Set up the socket.
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    DIE(server_socket < 0, "socket");

    // Populate the fields.
    auto server_addr = new sockaddr_in;
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(port);
    rc = inet_aton(argv[2], &server_addr->sin_addr);
    DIE(rc == 0, "inet_aton");

    // Connect to the server socket.
    rc = connect(server_socket, (struct sockaddr *) server_addr, sizeof(struct sockaddr_in));
    DIE(rc < 0, "connect");

    // Disable Nagle's algorithm.
    flag = 1;
    rc = setsockopt(server_socket, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
    DIE(rc < 0, "nagle");

    // Allocate memory for the client ID.
    auto *tcp_client_id = new char[MAX_CLIENT_ID_LENGTH + 1];
    memset(tcp_client_id, 0, MAX_CLIENT_ID_LENGTH + 1);
    // Copy the client ID.
    memcpy(tcp_client_id, argv[1], strlen(argv[1]));

    // Create a new tcp message.
    auto *tcp_client_connect_msg = new tcp_msg;
    memset(tcp_client_connect_msg, 0, sizeof(struct tcp_msg));
    tcp_client_connect_msg->type = 0;

    // Copy the client ID in the payload.
    memcpy(tcp_client_connect_msg->data, tcp_client_id, MAX_CLIENT_ID_LENGTH + 1);
    // Send the connect message to the server.
    rc = (int) send(server_socket, tcp_client_connect_msg, sizeof(tcp_msg), 0);
    DIE(rc < 0, "connect id");

    // Set file descriptors.
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(0, &read_fds);

    fd_set temp_fds;
    FD_ZERO(&temp_fds);

    FD_SET(server_socket, &read_fds);

    // Calculate the maximum file descriptor value.
    int max_fd = server_socket;
    while (true) {

        temp_fds = read_fds;

        // Monitor all file descriptors in range (0, max_fd).
        rc = select(max_fd + 1, &temp_fds, nullptr, nullptr, nullptr);
        DIE(rc < 0, "select");

        // Handle commands coming on stdin.
        if (FD_ISSET(STDIN_FILENO, &temp_fds)) {

            // Get the input.
            string command_line;
            getline(cin, command_line);
            // Split the input into command and arguments.
            auto command_args = split_command(command_line, " ");

            // Handle exit command.
            if (lowercase(command_args[0]) == "exit") {
                break;
            }

            // Handle subscribe command.
            if (lowercase(command_args[0]) == "subscribe") {

                // Allocate memory.
                auto *sub_msg = new subscribe_msg;
                memset(sub_msg, 0, sizeof(struct subscribe_msg));
                // Set type. 1 = subscribe, 0 = unsubscribe.
                sub_msg->type = 1;

                // Check if the command is executed with at least 2 arguments.
                if (command_args.size() < 3) {
                    continue;
                }

                // Check if the topic name is longer than the maximum length.
                char *topic_name = (char *) command_args[1].c_str();
                if (strlen(topic_name) > MAX_TOPIC_NAME_LENGTH - 1) {
                    continue;
                }

                // Get the second argument.
                string sf = command_args[2];

                // Check if the argument is a number.
                if (!is_string_number(sf)) {
                    continue;
                }

                // Set the sf field, wrapping values.
                if (sf[0] <= '0') {
                    sub_msg->sf = 0;
                } else {
                    sub_msg->sf = 1;
                }

                // Set the topic name.
                memcpy(sub_msg->topic, topic_name, strlen(topic_name));

                // Allocate memory for a new tcp message.
                auto *new_tcp_msg = new tcp_msg;
                memset(new_tcp_msg, 0, sizeof(struct tcp_msg));

                // Set the tcp message type.
                new_tcp_msg->type = 1;

                // Set the payload.
                memcpy(new_tcp_msg->data, sub_msg, sizeof(struct subscribe_msg));

                // Send.
                rc = (int) send(server_socket, new_tcp_msg, sizeof(struct tcp_msg), 0);
                DIE(rc < 0, "send");

                // Server sends subscribe confirmation.
                int reply;
                rc = (int) recv(server_socket, &reply, 1, 0);
                DIE(rc < 0, "receive");

                // Output success message.
                if (reply == 1) {
                    cout << "Subscribed to topic." << endl;
                }

                continue;
            }

            // Handle unsubscribe command.
            if (lowercase(command_args[0]) == "unsubscribe") {

                // Allocate memory.
                auto *unsub_msg = new subscribe_msg;
                memset(unsub_msg, 0, sizeof(struct subscribe_msg));
                // Set type. Type 0 = unsubscribe request.
                unsub_msg->type = 0;

                // Check if at least 1 argument was provided.
                if (command_args.size() < 2) {
                    continue;
                }

                // Get the topic name.
                char *topic_name = (char *) command_args[1].c_str();

                // Check if the topic name is longer that the maximum length.
                if (strlen(topic_name) > MAX_TOPIC_NAME_LENGTH - 1) {
                    continue;
                }

                // Copy the topic name to the unsubscribe message.
                memcpy(unsub_msg->topic, topic_name, strlen(topic_name));

                // Create a new tcp message.
                auto *new_tcp_msg = new tcp_msg;
                memset(new_tcp_msg, 0, sizeof(struct tcp_msg));

                // Set the payload and type.
                memcpy(new_tcp_msg->data, unsub_msg, sizeof(struct subscribe_msg));
                new_tcp_msg->type = 1;

                // Send the unsubscribe request.
                rc = (int) send(server_socket, new_tcp_msg, sizeof(struct tcp_msg), 0);
                DIE(rc < 0, "send");

                // Server sends unsubscribe confirmation.
                int reply;
                rc = (int) recv(server_socket, &reply, 1, 0);
                DIE(rc < 0, "receive");

                // Output success message.
                if (reply == 0) {
                    cout << "Unsubscribed from topic." << endl;
                }
            }
        } else {
            // Handle messages sent from the server.

            struct tcp_msg server_tcp_msg{};
            memset(&server_tcp_msg, 0, sizeof(struct tcp_msg));
            rc = (int) recv(server_socket, &server_tcp_msg, sizeof(struct tcp_msg), 0);
            DIE(rc < 0, "receive");

            // Guards.
            if (rc == 0) {
                break;
            } else if (server_tcp_msg.type != 1) {
                continue;
            }

            // Get the udp message sent from the udp clients.
            struct udp_msg server_udp_msg{};
            memset(&server_udp_msg, 0, sizeof(struct udp_msg));

            // Get the topic name.
            memcpy(server_udp_msg.topic, server_tcp_msg.data, MAX_TOPIC_NAME_LENGTH - 1);
            // Get the message type.
            memcpy(&server_udp_msg.type, server_tcp_msg.data + MAX_TOPIC_NAME_LENGTH - 1, sizeof(unsigned char));
            // Get the data.
            memcpy(server_udp_msg.data, server_tcp_msg.data + MAX_TOPIC_NAME_LENGTH, MAX_UDP_PAYLOAD_SIZE);

            // Set the end of the string.
            server_udp_msg.data[MAX_UDP_PAYLOAD_SIZE - 1] = 0;

            struct in_addr udp_addr{};
            memset(&udp_addr, 0, sizeof(struct in_addr));
            memcpy(&udp_addr, server_tcp_msg.data + sizeof(struct udp_msg), sizeof(struct in_addr));

            uint16_t udp_port;
            memcpy(&udp_port, server_tcp_msg.data + sizeof(struct udp_msg) + sizeof(in_addr), sizeof(uint16_t));
            // Convert to host order.
            udp_port = ntohs(udp_port);

            // Handle int number.
            if (server_udp_msg.type == 0) {
                uint8_t sign_byte;
                uint32_t value;

                // Get the sign.
                memcpy(&sign_byte, server_udp_msg.data, sizeof(uint8_t));
                // Get the number.
                memcpy(&value, server_udp_msg.data + sizeof(uint8_t), sizeof(uint32_t));
                // Convert to host order.
                value = ntohl(value);

                if (sign_byte == 1) {
                    value *= -1;
                }

                cout << inet_ntoa(udp_addr) << ":" << udp_port << " - " << server_udp_msg.topic
                     << " - INT - " << (int) value << endl;
            }

            // Handle short real number.
            if (server_udp_msg.type == 1) {
                uint16_t value;

                // Get the number.
                memcpy(&value, server_udp_msg.data, sizeof(uint16_t));
                // Convert to host order.
                value = ntohs(value);

                cout << inet_ntoa(udp_addr) << ":" << udp_port << " - " << server_udp_msg.topic
                     << " - SHORT_REAL - " << fixed << setprecision(2) << (float) value / 100.00 << endl;
            }

            // Handle float.
            if (server_udp_msg.type == 2) {
                uint8_t sign_byte;
                uint32_t concat_value;
                uint8_t power;

                // Get the sign byte.
                memcpy(&sign_byte, server_udp_msg.data, sizeof(uint8_t));
                // Get the number.
                memcpy(&concat_value, server_udp_msg.data + sizeof(uint8_t), sizeof(uint32_t));
                // Get the power.
                memcpy(&power, server_udp_msg.data + sizeof(uint8_t) + sizeof(uint32_t), sizeof(uint8_t));

                // Convert to host order.
                float value = ntohl(concat_value);

                for (int i = 0; i < power; i++) {
                    value /= 10;
                }

                if (sign_byte == 1) {
                    value *= -1;
                }

                cout << inet_ntoa(udp_addr) << ":" << udp_port << " - " << server_udp_msg.topic
                     << " - FLOAT - " << fixed << setprecision(power) << value << endl;
            }

            // Handle string.
            if (server_udp_msg.type == 3) {
                cout << inet_ntoa(udp_addr) << ":" << udp_port << " - " << server_udp_msg.topic
                     << " - STRING - " << (char *) server_udp_msg.data << endl;
            }
        }
    }

    // Close the connection.
    close(server_socket);

    return 0;
}
