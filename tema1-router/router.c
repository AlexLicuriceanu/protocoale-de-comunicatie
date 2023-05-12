#include "queue.h"
#include "lib.h"
#include "protocols.h"
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>

#define ETHERTYPE_IP 0x0800
#define ETHERTYPE_ARP 0x0806
#define RTABLE_MAXSIZE 100000
#define ICMP 1
#define ICMP_ECHO_REQUEST 8
#define ICMP_ECHO_REPLY 0
#define ICMP_TIME_EXCEEDED 11
#define ICMP_DESTINATION_UNREACHABLE 3
#define ARP_OP_REQUEST 1
#define ARP_OP_REPLY 2
#define ARP_HTYPE 1
#define ARP_PTYPE 0x0800
#define ARP_HLEN 6
#define ARP_PLEN 4
#define MAX_TTL 64

static int rtable_size;
static int arp_table_size;
struct route_table_entry *rtable;
struct arp_entry *arp_table;
queue q;

struct packet {
    char *payload;
    size_t len;
    int interface;
};

/**
 * @brief Converts a IP in char[] format to a decimal representation.
 *
 * @param s string IP.
 * @return Decimal IP.
 */
uint32_t convert_string_ip(char *s) {
    char *token = strtok(s, ".");
    char bytes[4];
    int k = 0;

    while (token != NULL) {
        bytes[k] = atoi(token);
        k++;
        token = strtok(NULL, ".");
    }

    return (int)((unsigned char)(bytes[3]) << 24 |
                 (unsigned char)(bytes[2]) << 16 |
                 (unsigned char)(bytes[1]) << 8 |
                 (unsigned char)(bytes[0]));
}

/**
 * @brief Extracts the ethernet header from a buffer.
 * @param buf
 * @return
 */
struct ether_header *get_ether_header(char *buf) {
    return (struct ether_header *)(buf);
}

/**
 * @brief Extracts the IP header from a buffer.
 * @param buf
 * @return
 */
struct iphdr *get_ip_header(char *buf) {
    return (struct iphdr *)(buf + sizeof(struct ether_header));
}

/**
 * @brief Extracts the ICMP header from a buffer.
 * @param buf
 * @return
 */
struct icmphdr *get_icmp_header(void *buf){
   return (struct icmphdr *)(buf + sizeof(struct iphdr) + sizeof(struct ether_header));
}

/**
 * @brief Extracts the ARP header from a buffer.
 * @param buf
 * @return
 */
struct arp_header *get_arp_header(void *buf) {
    return (struct arp_header *)(buf + sizeof(struct ether_header));
}

/**
 * @brief Comparator function for the routing table. First sort by mask length
 * descending then by prefix length also descending.
 *
 * @param a First element to compare.
 * @param b Second element to compare.
 * @return -1 if a should come before b, 0 if the order doesn't matter, 1 if b should come before a.
 */
int comparator(const void *a, const void *b) {
    const struct route_table_entry *entry_a = (const struct route_table_entry *) a;
    const struct route_table_entry *entry_b = (const struct route_table_entry *) b;

    if (entry_a->mask > entry_b->mask) {
        // a has a larger mask than b, so a should come first.
        return -1;
    }
    else if (entry_a->mask < entry_b->mask) {
        // a has a smaller mask than b, so a should come later.
        return 1;
    }
    else {
        // Then sort by prefix in descending order.
        if (entry_a->prefix > entry_b->prefix) {
            return -1;
        }
        else if (entry_a->prefix < entry_b->prefix) {
            return 1;
        }
        else return 0;
    }
}

/**
 * @brief Algorithm to determine the longest prefix match of a target IP implemented
 * with binary search in O(log n) time.
 *
 * @param target_ip The IP to search for.
 * @param left Left index.
 * @param right Right index.
 * @return Routing table entry containing the best route for the target IP.
 */
struct route_table_entry *get_best_route(uint32_t target_ip, uint32_t left, uint32_t right) {
    struct route_table_entry *best_match = NULL;

    // Start the binary search.
    while (left <= right) {
        uint32_t mid = (left + right) / 2;
        uint32_t mask = rtable[mid].mask;
        uint32_t prefix = rtable[mid].prefix;
        uint32_t masked_dest_ip = target_ip & mask;

        if (masked_dest_ip == prefix) {

            // Found a match, check if there are any longer prefixes.
            if (left == right) {
                // Found the longest match.
                return &rtable[mid];
            } else {
                // There might be longer prefixes, the routing table is sorted in descending order both by
                // mask and prefix, so the longest prefix is now somewhere between index 0 and mid.
                // Start binary search from 0 to mid.
                return get_best_route(target_ip, 0, mid);
            }


        }
        else if (masked_dest_ip > prefix) {
            // A match may be somewhere in the left part from the current midpoint.
            right = mid - 1;
        }
        else {
            // A match may be somewhere in the right part from the current midpoint.
            left = mid + 1;
        }
    }

    return best_match;
}

/**
 * @brief Linear search the ARP table for the target_ip.
 *
 * @param target_ip
 * @return The ARP entry corresponding to the IP, NULL if the IP cannot be found.
 */
struct arp_entry *get_arp_entry(uint32_t target_ip) {
    for (int i=0; i<arp_table_size; i++) {
        if (target_ip == arp_table[i].ip) {
            return &arp_table[i];
        }
    }
    return NULL;
}

/**
 * @brief Sends an ICMP packet on the interface.
 *
 * @param packet The packet that generated the ICMP message.
 * @param icmp_type The type of the ICMP message.
 * @param icmp_code The code of the ICMP message.
 * @param interface The interface on which to send it on.
 */
void send_icmp(struct packet *packet, uint8_t icmp_type, uint8_t icmp_code, int interface) {
    // Setup, unpack.
    struct ether_header *eth_hdr = get_ether_header(packet->payload);
    struct iphdr *ip_hdr = get_ip_header(packet->payload);
    struct icmphdr *icmp_hdr = get_icmp_header(packet->payload);
    size_t len = packet->len;
    char *buf = packet->payload;

    struct ether_header new_eth_hdr;
    struct iphdr new_ip_hdr;
    struct icmphdr new_icmp_hdr;

    // Construct the ETHERNET header
    memcpy(&new_eth_hdr.ether_dhost, eth_hdr->ether_shost, sizeof(eth_hdr->ether_shost));
    memcpy(&new_eth_hdr.ether_shost, eth_hdr->ether_dhost, sizeof(eth_hdr->ether_dhost));
    new_eth_hdr.ether_type = ntohs(ETHERTYPE_IP);

    // Construct the IP header
    memcpy(&new_ip_hdr, ip_hdr, sizeof(struct iphdr));
    new_ip_hdr.tot_len = htons(len - sizeof(struct ether_header));
    new_ip_hdr.ttl = MAX_TTL;
    new_ip_hdr.protocol = ICMP;
    new_ip_hdr.saddr = ip_hdr->daddr;
    new_ip_hdr.daddr = ip_hdr->saddr;
    new_ip_hdr.check = 0;
    new_ip_hdr.check = ntohs(checksum((uint16_t *)&new_ip_hdr, sizeof(struct iphdr)));
    
    // Construct the ICMP header
    memcpy(&new_icmp_hdr, icmp_hdr, sizeof(struct icmphdr));
    new_icmp_hdr.type = icmp_type;
    new_icmp_hdr.code = icmp_code;
    new_icmp_hdr.checksum = 0;
    new_icmp_hdr.checksum = ntohs(checksum((uint16_t *) &new_icmp_hdr,
                                           len - sizeof(struct ether_header) - sizeof(struct iphdr)));

    // Create the new packet
    char *new_packet = malloc(len * sizeof(char));

    memcpy(new_packet, buf, len);
    memcpy(new_packet, &new_eth_hdr, sizeof(struct ether_header));
    memcpy(new_packet + sizeof(struct ether_header), &new_ip_hdr, sizeof(struct iphdr));
    memcpy(new_packet + sizeof(struct ether_header) + sizeof(struct iphdr), &new_icmp_hdr, sizeof(struct icmphdr));

    // Send the created packet
    send_to_link(interface, new_packet, len);
}

/**
 * @brief Sends an ICMP error message.
 *
 * @param packet The packet that generated the error.
 * @param icmp_type The ICMP error type.
 * @param icmp_code The ICMP error code.
 * @param interface The interface on which to send the ICMP on.
 */
void send_icmp_error(struct packet *packet, uint8_t icmp_type, uint8_t icmp_code, int interface) {
    // Setup, unpack.
    struct ether_header *eth_hdr = get_ether_header(packet->payload);
    struct iphdr *ip_hdr = get_ip_header(packet->payload);
    struct icmphdr *icmp_hdr = get_icmp_header(packet->payload);

    struct ether_header new_eth_hdr;
    struct iphdr new_ip_hdr;
    struct icmphdr *new_icmp_hdr;

    // Construct the ETHERNET header
    memcpy(&new_eth_hdr.ether_dhost, eth_hdr->ether_shost, sizeof(eth_hdr->ether_shost));
    memcpy(&new_eth_hdr.ether_shost, eth_hdr->ether_dhost, sizeof(eth_hdr->ether_dhost));
    new_eth_hdr.ether_type = ntohs(ETHERTYPE_IP);

    // Construct the IP header
    memcpy(&new_ip_hdr, ip_hdr, sizeof(struct iphdr));
    uint16_t new_tot_len = sizeof(struct iphdr) + sizeof(uint64_t) + sizeof(struct iphdr) + sizeof(uint64_t);
    new_ip_hdr.tot_len = htons(new_tot_len);
    new_ip_hdr.ttl = MAX_TTL;
    new_ip_hdr.protocol = ICMP;
    new_ip_hdr.saddr = ip_hdr->daddr;
    new_ip_hdr.daddr = ip_hdr->saddr;
    new_ip_hdr.check = 0;
    new_ip_hdr.check = ntohs(checksum((uint16_t *)&new_ip_hdr, sizeof(struct iphdr)));

    // Create the new ICMP header
    uint32_t new_icmp_hdr_size = sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t);
    new_icmp_hdr = malloc(new_icmp_hdr_size);

    // Copy data.
    new_icmp_hdr->type = icmp_type;
    new_icmp_hdr->code = icmp_code;
    new_icmp_hdr->checksum = 0;
    new_icmp_hdr->un.echo.sequence = 0;
    new_icmp_hdr->un.echo.sequence = 0;
    new_icmp_hdr->un.gateway = 0;

    // Create the new packet.
    size_t new_packet_len = sizeof(struct ether_header) + new_tot_len;
    char *new_packet = malloc(new_packet_len);

    // Copy the ETHERNET header.
    size_t offset = 0;
    memcpy(new_packet, &new_eth_hdr, sizeof(struct ether_header));
    offset += sizeof(struct ether_header);

    // Copy the IP header.
    memcpy(new_packet + offset, &new_ip_hdr, sizeof(struct iphdr));
    offset += sizeof(struct iphdr);

    // Copy the ICMP header.
    memcpy(new_packet + offset, new_icmp_hdr, new_icmp_hdr_size);
    offset += new_icmp_hdr_size;

    // Copy the old IP header.
    memcpy(new_packet + offset, ip_hdr, sizeof(struct iphdr));
    offset += sizeof(struct iphdr);

    // Copy 8 bytes of the old ICMP header.
    memcpy(new_packet + offset, icmp_hdr, sizeof(uint64_t));

    // Recompute checksum.
    struct icmphdr *temp = get_icmp_header(new_packet);
    temp->checksum = 0;
    temp->checksum = ntohs(checksum((uint16_t *) temp, new_icmp_hdr_size + sizeof(struct iphdr) + sizeof(uint64_t)));

    // Send the packet
    send_to_link(interface, new_packet, new_packet_len);
}

/**
 * @brief Sends an ARP message.
 *
 * @param daddr Destination IP address.
 * @param saddr Sender IP address.
 * @param eth_hdr Ethernet header.
 * @param interface Interface ID.
 * @param op ARP OPCODE.
 */
void send_arp(uint32_t daddr, uint32_t saddr, struct ether_header *eth_hdr, int interface, uint16_t op) {
    struct arp_header new_arp_hdr;

    // Construct the ARP header.
    new_arp_hdr.op = op;
    new_arp_hdr.hlen = ARP_HLEN;
    new_arp_hdr.plen = ARP_PLEN;
    new_arp_hdr.htype = htons(ARP_HTYPE);
    new_arp_hdr.ptype = htons(ARP_PTYPE);

    memcpy(&new_arp_hdr.sha, eth_hdr->ether_shost, sizeof(new_arp_hdr.sha));
    memcpy(&new_arp_hdr.tha, eth_hdr->ether_dhost, sizeof(new_arp_hdr.tha));

    new_arp_hdr.spa = saddr;
    new_arp_hdr.tpa = daddr;

    // Create the packet
    size_t len = sizeof(struct ether_header) + sizeof(struct arp_header);
    char *payload = malloc(len);

    // Copy the data.
    memcpy(payload, eth_hdr, sizeof(struct ether_header));
    memcpy(payload + sizeof(struct ether_header), &new_arp_hdr, sizeof(struct arp_header));

    struct packet *new_packet = malloc(sizeof(struct packet));
    new_packet->payload = payload;
    new_packet->interface = interface;
    new_packet->len = len;

    // Send the packet.
    send_to_link(interface, payload, len);
}

/**
 * @brief Forwards a packet on the network.
 *
 * @param packet
 */
void forward_packet(struct packet *packet) {
    // Setup
    struct ether_header *eth_hdr = get_ether_header(packet->payload);
    struct iphdr *ip_hdr = get_ip_header(packet->payload);
    int interface = packet->interface;
    size_t len = packet->len;
    char *buf = packet->payload;

    // Verify checksum.
    uint16_t old_check = ip_hdr->check;
    ip_hdr->check = 0;
    uint16_t new_check = ntohs(checksum((uint16_t *) ip_hdr, sizeof(struct iphdr)));
    ip_hdr->check = new_check;

    // Drop the packet if the checksum is incorrect.
    if (old_check != new_check) {
        return;
    }

    // Check the packet's TTL
    if (ip_hdr->ttl <= 1) {
        // TTL expired, send time exceeded.
        send_icmp_error(packet, ICMP_TIME_EXCEEDED, 0, interface);
        return;
    }

    // Find the best route.
    struct route_table_entry *best_route = get_best_route(ip_hdr->daddr, 0, rtable_size - 1);

    // Check if a route was found.
    if (best_route == NULL) {

        // Send destination unreachable ICMP
        send_icmp_error(packet, ICMP_DESTINATION_UNREACHABLE, 0, interface);
        return;
    }

    // A route was found, prepare to forward the packet

    // Decrement TTL, recompute checksum
    ip_hdr->ttl--;
    ip_hdr->check = 0;
    new_check = ntohs(checksum((uint16_t *) ip_hdr, sizeof(struct iphdr)));
    ip_hdr->check = new_check;

    struct arp_entry *arp_table_entry = get_arp_entry(best_route->next_hop);

    // If no ARP entry was found.
    if (arp_table_entry == NULL) {

        // Enqueue the packet.
        struct packet *new_packet = malloc(sizeof(struct packet));
        memcpy(new_packet, packet, sizeof(struct packet));
        queue_enq(q, new_packet);


        char *target_ip_char = get_interface_ip(interface);
        uint32_t target_ip = convert_string_ip(target_ip_char);
        struct ether_header new_eth_hdr;
        uint8_t best_route_mac[6];
        uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

        get_interface_mac(best_route->interface, best_route_mac);

        memcpy(new_eth_hdr.ether_dhost, broadcast_mac, sizeof(new_eth_hdr.ether_dhost));
        memcpy(new_eth_hdr.ether_shost, best_route_mac, sizeof(new_eth_hdr.ether_shost));
        new_eth_hdr.ether_type = htons(ETHERTYPE_ARP);

        send_arp(best_route->next_hop, target_ip, &new_eth_hdr, best_route->interface, htons(ARP_OP_REQUEST));

        return;
    }

    memcpy(eth_hdr->ether_dhost, arp_table_entry->mac, sizeof(eth_hdr->ether_dhost));
    get_interface_mac(best_route->interface, eth_hdr->ether_shost);

    // Send the packet
    send_to_link(best_route->interface, buf, len);
}

/**
 * @brief Updates the ARP table, adding a new entry in the table.
 *
 * @param arp_hdr ARP header.
 */
void update_arp_table(struct arp_header *arp_hdr) {
    // Allocate memory.
    arp_table = realloc(arp_table, sizeof(struct arp_entry) * (arp_table_size+1));

    // Add the new entry.
    arp_table[arp_table_size].ip = arp_hdr->spa;
    memcpy(arp_table[arp_table_size].mac, arp_hdr->sha, sizeof(arp_hdr->sha));

    // Update the size of the table.
    arp_table_size++;
}

/**
 * @brief Creates and returns a new packet.
 *
 * @param payload The packet's payload.
 * @param len The packet's length.
 * @param interface
 * @return
 */
struct packet *create_packet(char *payload, size_t len, int interface) {
    struct packet *new_packet = malloc(sizeof(struct packet));
    new_packet->payload = payload;
    new_packet->len = len;
    new_packet->interface = interface;

    return new_packet;
}

int main(int argc, char *argv[])
{
    char buf[MAX_PACKET_LEN];

    // Do not modify this line.
    init(argc - 2, argv + 2);

    // Read the routing table and sort it.
    rtable = malloc(sizeof(struct route_table_entry) * RTABLE_MAXSIZE);
    rtable_size = read_rtable(argv[1], rtable);
    qsort((void *) rtable, rtable_size, sizeof(struct route_table_entry), comparator);

    // Set arp table values.
    arp_table = NULL;
    arp_table_size = 0;

    // Initialize the queue.
    q = queue_create();

    while (1) {

        int interface;
        size_t len;

        interface = recv_from_any_link(buf, &len);
        DIE(interface < 0, "recv_from_any_links");

        /* Note that packets received are in network order,
        any header field which has more than 1 byte will need to be converted to
        host order. For example, ntohs(eth_hdr->ether_type). The opposite is needed when
        sending a packet on the link, */

        struct ether_header *eth_hdr = (struct ether_header *) buf;
        struct iphdr *ip_hdr = (struct iphdr *)(buf + sizeof(struct ether_header));
        struct icmphdr *icmp_hdr = get_icmp_header(eth_hdr);
        struct arp_header *arp_hdr = get_arp_header(eth_hdr);

        // Check the encapsulated protocol
        uint16_t eth_type = eth_hdr->ether_type;

        if (ntohs(eth_type) == ETHERTYPE_IP) {
            // Handle IP packet

            // Verify checksum.
            uint16_t old_check = ip_hdr->check;
            ip_hdr->check = 0;
            uint16_t new_check = ntohs(checksum((uint16_t *) ip_hdr, sizeof(struct iphdr)));

            // Drop the packet if the checksum is incorrect.
            if (old_check != new_check) {
                continue;
            }

            // Checksum is correct, continue with the execution.
            ip_hdr->check = old_check;

            // Check if the destination is the router.
            char *router_ip_address = get_interface_ip(interface);
            if (ip_hdr->daddr == convert_string_ip(router_ip_address)) {

                if (ip_hdr->protocol == ICMP) {
                    // Check the ICMP type. Looking for echo request (type 8).
                    if (icmp_hdr->type == ICMP_ECHO_REQUEST) {
                        // Check the packet's TTL
                        if (ip_hdr->ttl <= 1) {
                            // TTL expired, send time exceeded.
                            struct packet *new_packet = create_packet(buf, len, interface);
                            send_icmp_error(new_packet, ICMP_TIME_EXCEEDED, 0, interface);
                        }
                        else {
                            // Send echo reply.
                            struct packet *new_packet = create_packet(buf, len, interface);
                            send_icmp(new_packet, ICMP_ECHO_REPLY, 0, interface);
                        }

                    }
                    else {
                        // ICMP packet is for the router, but it is not an ICMP request,
                        // do not respond to it.
                        continue;
                    }
                }
                else {
                    // Forward the packet
                    struct packet *new_packet = create_packet(buf, len, interface);
                    forward_packet(new_packet);
                }
            }
            else {
                // Forward the packet
                struct packet *new_packet = create_packet(buf, len, interface);
                forward_packet(new_packet);
            }

        }
        else if (ntohs(eth_type) == ETHERTYPE_ARP) {
            // Received ARP packet, check the opcode.

            if (ntohs(arp_hdr->op) == ARP_OP_REQUEST) {
                // Received ARP request.

                char *target_ip_char = get_interface_ip(interface);
                uint32_t target_ip = convert_string_ip(target_ip_char);

                if (target_ip == arp_hdr->tpa) {
                    uint8_t *mac = malloc(sizeof(eth_hdr->ether_shost));
                    get_interface_mac(interface, mac);

                    memcpy(eth_hdr->ether_dhost, eth_hdr->ether_shost, sizeof(eth_hdr->ether_dhost));
                    memcpy(eth_hdr->ether_shost, mac, sizeof(eth_hdr->ether_shost));

                    send_arp(arp_hdr->spa, arp_hdr->tpa, eth_hdr, interface, htons(ARP_OP_REPLY));
                }
                else {
                    struct packet *new_packet = create_packet(buf, len, interface);
                    forward_packet(new_packet);
                }
            }
            else if (ntohs(arp_hdr->op) == ARP_OP_REPLY) {
                // Update the ARP table.
                update_arp_table(arp_hdr);

                // Dequeue the packet and send.
                if (!queue_empty(q)) {
                    struct packet *new_packet = queue_deq(q);
                    forward_packet(new_packet);
                }
            }
        }
    }
}
