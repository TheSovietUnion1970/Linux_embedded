#include <stdint.h>
#include <arpa/inet.h>  // for ntohs(), ntohl()

// ICMP Header (minimum 8 bytes)
struct ether_header {
    uint8_t MAC_dst[6];
    uint8_t MAC_src[6];
    uint16_t type;
} __attribute__((packed));

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

    uint8_t reserved1:4;   // header length in 32-bit words
    uint8_t data_offset:4;

    uint8_t flags:6;           // CWR, ECE, URG, ACK, PSH, RST, SYN, FIN
    uint8_t reserved2:2;
    

    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_pointer;

    struct {
        uint8_t kind;
        uint8_t length;
        uint16_t value;  // for MSS
    } mss;

    struct {
        uint8_t kind;
        uint8_t length;
    } sack_permitted;

    struct {
        uint8_t kind;
        uint8_t length;
        uint32_t tsval;
        uint32_t tsecr;
    } timestamp;

    struct {
        uint8_t kind;
        uint8_t length;
        uint8_t shift;
    } window_scale;
    
} __attribute__((packed));

/* Common TCP options as a union for easy parsing */
union tcp_options {
    //uint8_t raw[0];  // Raw access

    struct {
        uint8_t kind;
        uint8_t length;
        uint16_t value;  // for MSS
    } mss;

    struct {
        uint8_t kind;
        uint8_t length;
    } sack_permitted;

    struct {
        uint8_t kind;
        uint8_t length;
        uint32_t tsval;
        uint32_t tsecr;
    } timestamp;

    struct {
        uint8_t kind;
        uint8_t length;
        uint8_t shift;
    } window_scale;

    // Add more as needed...
};

// void print_ipv4_header(const struct ipv4_header *ip);
// void print_icmp_header(const struct icmp_header *icmp);
// void print_tcp_header(const struct tcp_header *tcp);
void parse_and_print_packet(const uint8_t *packet, size_t len, uint8_t* name_packet, uint8_t eth_h, uint8_t ipv4_h);

// ===================
/* SSH Message: KEXINIT (message code 20) */
/* This is sent by both client and server during key exchange */
struct ssh_kexinit {
    uint8_t  message_code;     /* Always 20 for KEXINIT */

    uint8_t  cookie[16];       /* 16 random bytes */

    /* Name lists - each is: uint32 length + string (comma-separated algorithms) */
    uint32_t kex_algorithms_len;          /* Network byte order (big-endian) */
    char    *kex_algorithms;              /* Pointer to data (or parse manually) */

    uint32_t server_host_key_algorithms_len;
    char    *server_host_key_algorithms;

    uint32_t encryption_client_to_server_len;
    char    *encryption_client_to_server;

    uint32_t encryption_server_to_client_len;
    char    *encryption_server_to_client;

    uint32_t mac_client_to_server_len;
    char    *mac_client_to_server;

    uint32_t mac_server_to_client_len;
    char    *mac_server_to_client;

    uint32_t compression_client_to_server_len;
    char    *compression_client_to_server;

    uint32_t compression_server_to_client_len;
    char    *compression_server_to_client;

    uint32_t languages_client_to_server_len;
    char    *languages_client_to_server;

    uint32_t languages_server_to_client_len;
    char    *languages_server_to_client;

    uint8_t  first_kex_packet_follows;    /* Boolean: 0 or 1 */
    uint32_t reserved;                    /* Always 0 */
} __attribute__((packed));

void parse_ssh_kexinit(const uint8_t *data, size_t len);
void parse_and_print_ssh_kexinit(uint8_t *packet, size_t len, uint8_t* name_packet);
void parse_ssh_kex_reply(const uint8_t *payload, size_t payload_len);

