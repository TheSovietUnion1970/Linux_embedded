#ifndef CPSW_H
#define CPSW_H

#include "mdio.h"

/* CPSW_BASE */
#define CPSW_CONTROL            0x04
    #define CPSW_VLAN_AWARE     1u << 1
    #define CPSW_RX_VLAN_ENCAP  1u << 2
#define CPSW_SOFT_RESET         0x08
#define CPSW_STAT_PORT_EN       0x0C
#define CPSW_PTYPE              0x10
#define CPSW_FLOW_CONTROL       0x24

/* SLx_BASE */
#define SLx_SOFT_RESET 0x0C

/* ALE_BASE */
#define ALE_IDVER		0x00
#define ALE_STATUS		0x04
#define ALE_CONTROL		0x08
    #define ALE_ENABLE_ALE		1u << 31
    #define ALE_CLEAR_TABLE		1u << 30
    #define ALE_P0_UNI_FLOOD    1u << 8
    #define ALE_VLAN_AWARE      1u << 2
#define ALE_PRESCALE		0x10
#define ALE_AGING_TIMER		0x14
#define ALE_UNKNOWNVLAN		0x18
#define ALE_TABLE_CONTROL	0x20
#define ALE_TABLE		0x34
#define ALE_PORTCTL		0x40

#define ALE_TABLE_SIZE_MULTIPLIER	1024
#define ALE_STATUS_SIZE_MASK		0x1f

/* CPDMA BASE */

/* PORT0 BASE */
#define P0_TX_IN_CTL            0x10
#define P0_PORT_VLAN            0x14
#define P0_TX_PRI_MAP           0x18
#define P0_CPDMA_TX_PRI_MAP     0x1C
#define P0_CPDMA_RX_CH_MAP      0x20

#define CPDMA_TX_PRIORITY_MAP	0x76543210
#define CPSW_FIFO_DUAL_MAC_MODE		(1 << 16)

#define tx_chan_num(chan)	(chan)
#define rx_chan_num(chan)	((chan) + CPDMA_MAX_CHANNELS)

int cpsw_init(struct ether_device_data *data);

#endif /* CPSW_H */