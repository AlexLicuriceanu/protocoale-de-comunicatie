#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cstring>

#include "./structures.h"

using namespace std;

string lowercase(string str) {

    // Iterate all characters, transform all to lowercase.
    for (auto& c : str) {
        c = (char) tolower(c);
    }
    return str;
}

int main(int argc, char *argv[]) {

    // Handle error if no arguments were passed.
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " server_port" << endl;
        exit(0);
    }

    int rc, flag;

    // Disable buffering.
    setvbuf(stdout, nullptr, _IONBF, BUFSIZ);

    // Convert port number to int.
    int port = stoi(argv[1]);
    DIE(port == 0, "port");

    // Set up UDP socket.
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(udp_socket < 0, "udp socket");

    // Fill the details on what destination port should the
    // datagrams have to be sent to our process.
    auto *udp_addr = new sockaddr_in;
    memset((char *) udp_addr, 0, sizeof(struct sockaddr_in));

    // Populate the fields.
    udp_addr->sin_family = AF_INET;
    udp_addr->sin_addr.s_addr = INADDR_ANY;
    udp_addr->sin_port = htons(port);

    // Bind the socket with the address.
    rc = bind(udp_socket, (const struct sockaddr *) udp_addr, sizeof(struct sockaddr_in));
    DIE(rc < 0, "udp bind");

    // Set up TCP socket.
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcp_socket < 0, "tcp socket");

    // Fill the details on what destination port should the
    // datagrams have to be sent to our process.
    auto *tcp_addr = new sockaddr_in;
    memset((char *) tcp_addr, 0, sizeof(struct sockaddr_in));

    // Populate the fields.
    tcp_addr->sin_family = AF_INET;
    tcp_addr->sin_addr.s_addr = INADDR_ANY;
    tcp_addr->sin_port = htons(port);

    // Bind the socket with the address.
    rc = bind(tcp_socket, (struct sockaddr *)tcp_addr, sizeof(struct sockaddr));
    DIE(rc < 0, "tcp bind");

    rc = listen(tcp_socket, MAX_CLIENTS);
    DIE(rc < 0, "listen");

    // Disable Nagle's algorithm.
    flag = 1;
    rc = setsockopt(tcp_socket, IPPROTO_TCP, TCP_NODELAY, (const void*) &flag, sizeof(int));
    DIE(rc < 0, "nagle");

    // Set file descriptors.
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(0, &read_fds);

    fd_set temp_fds;
    FD_ZERO(&temp_fds);

    FD_SET(udp_socket, &read_fds);
    FD_SET(tcp_socket, &read_fds);

    // Calculate the maximum file descriptor value.
    int max_fd = max(udp_socket, tcp_socket);

    // Unordered map linking client ID and socket.
    unordered_map<string, int> id_socket;

    // Unordered map linking socket and client ID.
    unordered_map<int, string> socket_id;

    // Unordered map linking topic with subscribed clients.
    unordered_map<string, vector<string>> topic_clients;

    // Unordered map linking clients with the SF topics.
    unordered_map<string, vector<string>> sf_client_topics;

    // Unordered map working as a message queue for every client
    // that has at least 1 SF enabled topic.
    unordered_map<string, vector<struct udp_msg *>> client_waiting_msgs;

    // Setup complete, start the infinite loop.
    bool loop = true;

    while (loop) {

        temp_fds = read_fds;

        // Monitor all file descriptors in range (0, max_fd).
        rc = select(max_fd + 1, &temp_fds, nullptr, nullptr, nullptr);
        DIE(rc < 0, "select");

        // Iterate all file descriptors and handle those which are ready for reading.
        for (int i = 0; i <= max_fd; i++) {

            // Check if file descriptor "i" is ready for reading.
            if (FD_ISSET(i, &temp_fds) == true) {

                // Handle messages coming on the UDP socket.
                if (i == udp_socket) {

                    // Prepare to store the message.
                    char buf[MAX_UDP_MSG_SIZE];
                    memset(buf, 0, sizeof(buf));

                    auto *udp_client = new sockaddr_in;
                    socklen_t udp_len = sizeof(struct sockaddr_in);

                    // Receive the message from the UDP socket.
                    rc = (int) recvfrom(udp_socket, buf, sizeof(buf), 0, (struct sockaddr *) udp_client, &udp_len);
                    DIE(rc < 0, "udp receive");

                    // Create a new TCP message.
                    auto *new_tcp_msg = new tcp_msg;
                    memset(new_tcp_msg, 0, sizeof(struct tcp_msg));

                    // Set type 1, copy the UDP message in the data field of the TCP message.
                    new_tcp_msg->type = 1;
                    memcpy(new_tcp_msg->data, buf, sizeof(struct udp_msg));

                    // Get the topic name.
                    char *topic_name = new char[MAX_TOPIC_NAME_LENGTH];
                    memset(topic_name, 0, MAX_TOPIC_NAME_LENGTH);
                    memcpy(topic_name, (struct udp_msg *) buf, MAX_TOPIC_NAME_LENGTH-1);

                    // If no clients are subscribed to the topic, continue.
                    if (topic_clients.find(topic_name) == topic_clients.end()){
                        continue;
                    }

                    // Get the subscribed clients.
                    vector<string> subscribed_clients = topic_clients[topic_name];

                    // Iterate all subscribed clients.
                    for (auto client : subscribed_clients) {

                        // Get the client's socket.
                        int client_socket = id_socket[client];

                        // If the socket of the client is -1, the client was previously connected.
                        if (id_socket[client] == -1) {

                            // Check if client has any SF subscriptions.
                            if (sf_client_topics.find(client) != sf_client_topics.end()) {

                                auto sf_topics = sf_client_topics[client];

                                // Client already has some SF enabled topics. Check if the current topic
                                // is contained.
                                if (find(sf_topics.begin(), sf_topics.end(), topic_name) != sf_topics.end()) {

                                    // Client is offline but has enabled SF for this topic, add the message
                                    // to the waiting list.
                                    auto *new_waiting_msg = new udp_msg;
                                    memset(new_waiting_msg, 0, sizeof(struct udp_msg));
                                    memcpy(new_waiting_msg, buf, sizeof(struct udp_msg));

                                    // Check if client has other waiting messages.
                                    if (client_waiting_msgs.find(client) != client_waiting_msgs.end()) {

                                        // Get the list of messages.
                                        auto waiting_msgs = client_waiting_msgs[client];
                                        // Add the new message to the list.
                                        waiting_msgs.push_back(new_waiting_msg);
                                        // Update the list.
                                        client_waiting_msgs[client] = waiting_msgs;

                                    }
                                    else {

                                        // Client doesn't have any other waiting messages.
                                        vector<struct udp_msg *> waiting_msgs;
                                        // Add the new message to the list.
                                        waiting_msgs.push_back(new_waiting_msg);
                                        // Update the list.
                                        client_waiting_msgs[client] = waiting_msgs;
                                    }
                                }

                                continue;
                            }

                        }

                        if (client_socket >= 0) {
                            // Send.
                            rc = (int) send(client_socket, new_tcp_msg, sizeof(struct tcp_msg), 0);
                            DIE(rc < 0, "tcp send");
                        }
                    }

                    break;
                }

                // Handle messages coming on the TCP socket.
                if (i == tcp_socket) {

                    // Disable Nagle's algorithm.
                    flag = 1;
                    rc = setsockopt(tcp_socket, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
                    DIE(rc < 0, "nagle");

                    // Get the client's address.
                    auto *client_address = new sockaddr_in;
                    socklen_t client_length = sizeof(struct sockaddr_in);

                    // Accept the new TCP connection and assign the file descriptor to new_client.
                    int new_client = accept(tcp_socket, (struct sockaddr *) client_address, &client_length);
                    DIE(new_client < 0, "accept");

                    // Allocate memory.
                    auto *client_tcp_msg = new tcp_msg;
                    memset(client_tcp_msg, 0, sizeof(tcp_msg));

                    // Get the actual message's data.
                    rc = (int) recv(new_client, client_tcp_msg, sizeof(tcp_msg), 0);
                    DIE(rc < 0, "tcp receive");

                    // This will take all characters until first '\0', representing the client's ID.
                    auto *client_tcp_id = (char *) client_tcp_msg->data;

                    // Check if a client with the same ID is already connected.
                    if (id_socket.find(client_tcp_id) != id_socket.end() && id_socket[client_tcp_id] != -1) {

                        // Close the connection.
                        close(new_client);

                        // Output an error message.
                        cout << "Client " << client_tcp_id << " already connected." << endl;
                        continue;
                    }

                    // This checks if an entry in the id_socket map already exists from a previous
                    // client that was connected with the same ID. When the previous client disconnected,
                    // its id_socket entry was set to -1, not deleted entirely.
                    if (id_socket.find(client_tcp_id) == id_socket.end()) {

                        // If there wasn't a previous client that had this ID, create a new entry in the map.
                        id_socket.insert(make_pair(client_tcp_id, new_client));
                        socket_id.insert(make_pair(new_client, client_tcp_id));
                    }
                    else {

                        // There was a previous client using this ID.
                        id_socket[client_tcp_id] = new_client;
                        socket_id.insert(make_pair(new_client, client_tcp_id));
                    }


                    // Update the set of file descriptors.
                    FD_SET(new_client, &read_fds);

                    // Update the max file descriptor value.
                    max_fd = max(new_client, max_fd);

                    // Print the "new client connected" message.
                    auto *string_address = inet_ntoa(client_address->sin_addr);
                    auto string_port = ntohs(client_address->sin_port);

                    cout << "New client " << client_tcp_id << " connected from " <<
                         string_address << ":" << string_port << endl;

                    // Check if the new client has any SF enabled topics.
                    if (client_waiting_msgs.find(client_tcp_id) != client_waiting_msgs.end()) {

                        // Client has some sf enabled topics, send the waiting messages.
                        for (auto message : client_waiting_msgs[client_tcp_id]) {

                            // Create a new TCP message.
                            auto *new_tcp_msg = new tcp_msg;
                            memset(new_tcp_msg, 0, sizeof(struct tcp_msg));

                            // Set type 1, copy the UDP message in the data field of the TCP message.
                            new_tcp_msg->type = 1;
                            memcpy(new_tcp_msg->data, message, sizeof(struct udp_msg));

                            // Send to the client.
                            rc = (int) send(id_socket[client_tcp_id], new_tcp_msg, sizeof(struct tcp_msg), 0);
                            DIE(rc < 0, "sf send");
                        }

                        // All waiting messages for this client have been sent, delete its entry
                        // in the waiting list.
                        client_waiting_msgs.erase(client_tcp_id);
                    }

                    continue;
                }

                // Handle commands coming on stdin.
                if (i == STDIN_FILENO) {

                    // Read the command.
                    string command;
                    cin >> command;

                    // Convert all character to lowercase.
                    command = lowercase(command);

                    // Exit command was entered.
                    if (command == "exit") {

                        // Close all connected clients.
                        for (int j = 1; j <= max_fd; j++) {

                            if (FD_ISSET(j, &read_fds) && j != tcp_socket && j != udp_socket) {
                                // Remove the "j" file descriptor from the list.
                                FD_CLR(j, &read_fds);
                                // Close the connection.
                                close(j);
                            }
                        }

                        // Stop the while loop.
                        loop = false;
                        break;
                    }

                    continue;
                }

                // Handle client disconnects.

                // Allocate memory for the tcp message.
                auto *client_tcp_msg = new tcp_msg;
                memset(client_tcp_msg, 0, sizeof(struct tcp_msg));

                // Receive the data.
                rc = (int) recv(i, client_tcp_msg, sizeof(tcp_msg), 0);
                DIE(rc < 0, "tcp receive");

                if (rc == 0) {

                    // Client has disconnected.
                    cout << "Client " << socket_id[i] << " disconnected." << endl;

                    // Remove the client from the maps. Set the id_socket entry to -1
                    // to mark what clients were previously connected.
                    id_socket[socket_id[i]] = -1;
                    socket_id.erase(i);

                    // Remove the client's file descriptor from the file descriptor sets.
                    FD_CLR(i, &read_fds);
                    FD_CLR(i, &temp_fds);

                    // Close the connection.
                    close(i);
                    continue;
                }

                // Handle subscribe/unsubscribe requests from the clients.
                if (client_tcp_msg->type != 1) {
                    continue;
                }

                // Allocate memory for the subscribe/unsubscribe message.
                auto *sub_msg = new subscribe_msg;
                memset(sub_msg, 0, sizeof(struct subscribe_msg));

                // Copy the data.
                memcpy(sub_msg, client_tcp_msg->data, sizeof(struct subscribe_msg));


                // Type is 0, client wants to unsubscribe.
                if (sub_msg->type == 0) {

                    // Check if the topic has any subscribers.
                    if (topic_clients.find(sub_msg->topic) != topic_clients.end()) {

                        // Get the subscribed clients to the topic.
                        auto subscribers = topic_clients[sub_msg->topic];

                        // Find the client's ID in the subscribers vector.
                        auto client_id = find(subscribers.begin(), subscribers.end(), socket_id[i]);

                        // If the client can be found in the list of subscribers.
                        if (client_id != subscribers.end()) {
                            // Remove the client from the list of subscribers.
                            subscribers.erase(client_id);
                        }

                        // Update the topic's subscribers with the new list.
                        topic_clients[sub_msg->topic] = subscribers;
                    }

                    // Send unsubscribe success to the client.
                    flag = 0;
                    rc = (int) send(i, &flag, 1, 0);
                    DIE(rc < 0, "unsubscribe send");
                }

                // Type is 1, client wants to subscribe.
                if (sub_msg->type == 1) {

                    // Check if the topic has any subscribers.
                    if (topic_clients.find(sub_msg->topic) != topic_clients.end()) {

                        // Get the subscribed clients to the topic.
                        auto subscribers = topic_clients[sub_msg->topic];

                        // Find the client's ID in the subscribers vector.
                        auto client_id = find(subscribers.begin(), subscribers.end(), socket_id[i]);

                        // If the client is not subscribed, add it to the list of subscribers.
                        if (client_id == subscribers.end()) {
                            subscribers.push_back(socket_id[i]);
                        }

                        // Update the topic's subscribers with the new list.
                        topic_clients[sub_msg->topic] = subscribers;
                    }
                    else {
                        // Topic does not have any subscribers, create a new list.
                        vector<string> subscribers;
                        // Add the client to the new list of subscribers.
                        subscribers.push_back(socket_id[i]);

                        // Convert the topic's name to string.
                        auto string_topic = string(sub_msg->topic);
                        // Add the new topic and list of subscribers to the topic_clients map.
                        topic_clients.insert(make_pair(string_topic, subscribers));
                    }

                    // Handle SF.
                    if (sub_msg->sf == SF_ENABLED) {

                        // Check if the client has other SF enabled topics.
                        if (sf_client_topics.find(socket_id[i]) != sf_client_topics.end()) {

                            // Client has other SF enabled topics, get the list.
                            auto topics = sf_client_topics[socket_id[i]];
                            // Add the new topic.
                            auto string_topic = string(sub_msg->topic);
                            topics.push_back(string_topic);
                            // Update the list of SF enabled topics.
                            sf_client_topics[socket_id[i]] = topics;
                        }
                        else {

                            // Client doesn't have any other sf enabled topics.
                            vector<string> topics;
                            // Add the new topic.
                            auto string_topic = string(sub_msg->topic);
                            topics.push_back(string_topic);
                            // Update the list of SF enabled topics.
                            sf_client_topics[socket_id[i]] = topics;
                        }
                    }

                    // Send subscribe success to client.
                    flag = 1;
                    rc = (int) send(i, &flag, 1, 0);
                    DIE(rc < 0, "subscribe send");

                }
            }
        }
    }

    // Close the server's udp and tcp sockets.
    close(udp_socket);
    close(tcp_socket);

    return 0;
}