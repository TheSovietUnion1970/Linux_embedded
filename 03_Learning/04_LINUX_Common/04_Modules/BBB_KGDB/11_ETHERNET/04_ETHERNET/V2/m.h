#ifndef M_H
#define M_H

/* Control */
#define PRINT_DATA 1
#define VID_USED 3

/* Use it for identifying type of pkt (VLAN tagged or untagged) */
#define VLAN_ENCAP_USED 1

/* DATA */
/*
CPSW_MAX_PACKET_SIZE = 0x5f6
cpsw_rxbuf_total_len(CPSW_MAX_PACKET_SIZE) = 0x800
skb->offload_fwd_mark = 0

XDP_PACKET_HEADROOM = 0x100
NET_SKB_PAD = 0x40
NET_IP_ALIGN = 0x2
*/

#endif /* M_H */