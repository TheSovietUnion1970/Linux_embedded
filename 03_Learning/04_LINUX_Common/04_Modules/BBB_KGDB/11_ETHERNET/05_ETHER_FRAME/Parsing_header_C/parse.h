#include <stdint.h>
#include <arpa/inet.h>  // for ntohs(), ntohl()


// IPv4 Header (20 bytes fixed, no options)
struct ipv4_header {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint8_t ihl:4;
    uint8_t version:4;
#else
    uint8_t version:4;
    uint8_t ihl:4;
#endif
    uint8_t dscp:6;
    uint8_t ecn:2;
    uint16_t total_length;
    uint16_t identification;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint16_t fragment_offset:13;
    uint16_t flags:3;
#else
    uint16_t flags:3;
    uint16_t fragment_offset:13;
#endif
    uint8_t ttl;
    uint8_t protocol;
    uint16_t header_checksum;
    uint32_t source_ip;
    uint32_t dest_ip;
} __attribute__((packed));

// ICMP Header (minimum 8 bytes)
struct icmp_header {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint32_t rest_of_header;  // identifier + sequence for Echo, varies otherwise
} __attribute__((packed));

// TCP Header (minimum 20 bytes)
struct tcp_header {
    uint16_t source_port;
    uint16_t dest_port;
    uint32_t sequence;
    uint32_t ack_number;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint8_t data_offset:4;   // header length in 32-bit words
    uint8_t reserved:4;
#else
    uint8_t reserved:4;
    uint8_t data_offset:4;
#endif
    uint8_t flags;           // CWR, ECE, URG, ACK, PSH, RST, SYN, FIN
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_pointer;
} __attribute__((packed));

void print_ipv4_header(const struct ipv4_header *ip);
void print_icmp_header(const struct icmp_header *icmp);
void print_tcp_header(const struct tcp_header *tcp);
void parse_and_print_packet(const uint8_t *packet, size_t len);
