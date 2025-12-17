#include <stdio.h>
#include "parse.h"

void print_ether_header(const struct ether_header *eth) {
    printf("=== Ether Header ===\n");
    printf("Destination MAC    : %x:%x:%x:%x:%x:%x\n", eth->MAC_dst[0], eth->MAC_dst[1], eth->MAC_dst[2], eth->MAC_dst[3], eth->MAC_dst[4], eth->MAC_dst[5]);
    printf("Source MAC         : %x:%x:%x:%x:%x:%x\n", eth->MAC_src[0], eth->MAC_src[1], eth->MAC_src[2], eth->MAC_src[3], eth->MAC_src[4], eth->MAC_src[5]);
    printf("Type               : %u\n", eth->type);
}

void print_ipv4_header(const struct ipv4_header *ip) {
    printf("=== IPv4 Header ===\n");
    printf("Version            : %u\n", ip->version);
    printf("Header Length      : %u bytes (%u words)\n", ip->ihl * 4, ip->ihl);
    printf("DSCP               : %u\n", ip->dscp);
    printf("ECN                : %u\n", ip->ecn);
    printf("Total Length       : %u bytes\n", ntohs(ip->total_length));
    printf("Identification     : 0x%04x\n", ntohs(ip->identification));
    printf("Flags              : %s%s%s\n",
           (ip->flags & 0x4) ? "Reserved " : "",
           (ip->flags & 0x2) ? "DF " : "",
           (ip->flags & 0x1) ? "MF " : "");
    printf("Fragment Offset    : %u\n", ntohs(ip->fragment_offset));
    printf("TTL                : %u\n", ip->ttl);
    printf("Protocol           : %u ", ip->protocol);
    switch (ip->protocol) {
        case 1:  printf("(ICMP)\n"); break;
        case 6:  printf("(TCP)\n"); break;
        case 17: printf("(UDP)\n"); break;
        default: printf("(Other)\n"); break;
    }
    printf("Header Checksum    : 0x%04x\n", ntohs(ip->header_checksum));
    printf("Source IP          : %u.%u.%u.%u\n",
           ip->source_ip & 0xFF, 
           (ip->source_ip >> 8) & 0xFF,
           (ip->source_ip >> 16) & 0xFF,  
           (ip->source_ip >> 24) & 0xFF);
    printf("Destination IP     : %u.%u.%u.%u\n",
           ip->dest_ip & 0xFF, 
           (ip->dest_ip >> 8) & 0xFF,
           (ip->dest_ip >> 16) & 0xFF,  
           (ip->dest_ip >> 24) & 0xFF);
}

void print_icmp_header(const struct icmp_header *icmp) {
    printf("=== ICMP Header ===\n");
    printf("Type               : %u ", icmp->type);
    switch (icmp->type) {
        case 0:  printf("(Echo Reply)\n"); break;
        case 8:  printf("(Echo Request)\n"); break;
        case 3:  printf("(Destination Unreachable)\n"); break;
        default: printf("\n"); break;
    }
    printf("Code               : %u\n", icmp->code);
    printf("Checksum           : 0x%04x\n", ntohs(icmp->checksum));
    if (icmp->type == 0 || icmp->type == 8) {
        printf("Identifier         : %u\n", ntohs(icmp->rest_of_header >> 16));
        printf("Sequence Number    : %u\n", ntohs(icmp->rest_of_header & 0xFFFF));
    }
}

void print_tcp_header(const struct tcp_header *tcp) {
    printf("=== TCP Header ===\n");
    printf("Source Port        : %u\n", ntohs(tcp->source_port));
    printf("Destination Port   : %u\n", ntohs(tcp->dest_port));
    printf("Sequence Number    : %u\n", ntohl(tcp->sequence));
    printf("Acknowledgment     : %u\n", ntohl(tcp->ack_number));
    printf("Header Length      : %u bytes (%u words)\n", tcp->data_offset * 4, tcp->data_offset);
    printf("Flags              : ");
    if (tcp->flags & 0x01) printf("FIN ");
    if (tcp->flags & 0x02) printf("SYN ");
    if (tcp->flags & 0x04) printf("RST ");
    if (tcp->flags & 0x08) printf("PSH ");
    if (tcp->flags & 0x10) printf("ACK ");
    if (tcp->flags & 0x20) printf("URG ");
    if (tcp->flags & 0x40) printf("ECE ");
    if (tcp->flags & 0x80) printf("CWR ");
    printf("\n");
    printf("Window Size        : %u\n", ntohs(tcp->window));
    printf("Checksum           : 0x%04x\n", ntohs(tcp->checksum));
    printf("Urgent Pointer     : %u\n", ntohs(tcp->urgent_pointer));
}

// Example usage: parse a raw packet buffer (starting after Ethernet header)
void parse_and_print_packet(const uint8_t *packet, size_t len, uint8_t* name_packet) {
    if (len < sizeof(struct ipv4_header)) {
        printf("Packet too short for IPv4 header\n");
        return;
    }

    const struct ether_header *eth = (const struct ether_header *)((uint8_t*)packet);
    const struct ipv4_header *ip = (const struct ipv4_header *)((uint8_t*)packet + sizeof(struct ether_header));

    printf("================== %s ==================\n", name_packet);
    print_ether_header(eth);
    print_ipv4_header(ip);

    size_t ip_hdr_len = ip->ihl * 4;
    if (len < ip_hdr_len) {
        printf("Truncated IP header\n");
        return;
    }

    if (ip->protocol == 1) {  // ICMP
        if (len >= ip_hdr_len + sizeof(struct icmp_header)) {
            const struct icmp_header *icmp = (const struct icmp_header *)(packet + ip_hdr_len + 14);
            print_icmp_header(icmp);
        }
    } else if (ip->protocol == 6) {  // TCP
        if (len >= ip_hdr_len + sizeof(struct tcp_header)) {
            const struct tcp_header *tcp = (const struct tcp_header *)(packet + ip_hdr_len);
            print_tcp_header(tcp);
        }
    }

    printf("================== === === ==================\n");
}

