#ifndef CPDMA_H
#define CPDMA_H

#include <linux/dma-mapping.h>
#include<linux/interrupt.h>

#include "mdio.h"


#define chan_linear(chan_num)	((chan_num) & (32 - 1))
#define CPSW_MAX_QUEUES 8
#define CPSW_HEADROOM_NA (max(XDP_PACKET_HEADROOM, NET_SKB_PAD) + NET_IP_ALIGN)

/* CPSW_CPDMA */
#define CPDMA_RXTHRESH		0x0c0
#define CPDMA_RXFREE		0x0e0

/* CPDMA_STATERAM */
#define CPDMA_TXHDP		0x00
#define CPDMA_RXHDP		0x20
#define CPDMA_TXCP		0x40
#define CPDMA_RXCP		0x60

/* DMA Registers */
#define CPDMA_TXIDVER		0x00
#define CPDMA_TXCONTROL		0x04
#define CPDMA_TXTEARDOWN	0x08
#define CPDMA_RXIDVER		0x10
#define CPDMA_RXCONTROL		0x14
#define CPDMA_SOFTRESET		0x1c
#define CPDMA_RXTEARDOWN	0x18
#define CPDMA_CONTROL   	0x20
    #define TX_PTYPE    1u << 0
#define CPDMA_STATUS       	0x24
#define CPDMA_RX_BUFF_OFFSET       	0x28
#define CPDMA_TX_PRI0_RATE	0x30
#define CPDMA_TXINTSTATRAW	0x80
#define CPDMA_TXINTSTATMASKED	0x84
#define CPDMA_TXINTMASKSET	0x88
#define CPDMA_TXINTMASKCLEAR	0x8c
#define CPDMA_MACINVECTOR	0x90
#define CPDMA_MACEOIVECTOR	0x94
#define CPDMA_RXINTSTATRAW	0xa0
#define CPDMA_RXINTSTATMASKED	0xa4
#define CPDMA_RXINTMASKSET	0xa8
#define CPDMA_RXINTMASKCLEAR	0xac
#define CPDMA_DMAINTSTATRAW	0xb0
#define CPDMA_DMAINTSTATMASKED	0xb4
#define CPDMA_DMAINTMASKSET	0xb8
#define CPDMA_DMAINTMASKCLEAR	0xbc
#define CPDMA_DMAINT_HOSTERR	BIT(1)

#define CPDMA_EOI_RX_THRESH	0x0
#define CPDMA_EOI_RX		0x1
#define CPDMA_EOI_TX		0x2
#define CPDMA_EOI_MISC		0x3

/* WRAPPER */
#define WR_C0_RX_THRESH_EN  0x10
#define WR_C0_RX_EN         0x14
#define WR_C0_TX_EN         0x18
#define WR_C0_MISC_EN       0x1c
#define TX_INT_SET          0x88
#define TX_INT_CLEAR        0x8c
#define RX_INT_SET          0xa8
#define RX_INT_CLEAR        0xac

/* Descriptor mode bits */
#define CPDMA_DESC_SOP		BIT(31)
#define CPDMA_DESC_EOP		BIT(30)
#define CPDMA_DESC_OWNER	BIT(29)
#define CPDMA_DESC_EOQ		BIT(28)
#define CPDMA_DESC_TO_PORT_EN	BIT(20)

int cpdma_ctlr_start(struct ether_device_data* data);
int cpdma_ctlr_stop(struct ether_device_data* data);
void cpdma_intr_enable(struct ether_device_data* data);
void cpdma_intr_disable(struct ether_device_data* data);

irqreturn_t rx_thresh_handler(int irq, void *dev_id);
irqreturn_t rx_handler(int irq, void *dev_id);
irqreturn_t tx_handler(int irq, void *dev_id);
irqreturn_t misc_handler(int irq, void *dev_id);

int p_create_ports(struct ether_device_data *data);
int p_register_ports(struct ether_device_data *data);
int p_create_xdp_rxqs(struct ether_device_data *data);
void p_destroy_xdp_rxqs(struct ether_device_data *data);

void run_test(struct ether_device_data *data);

#endif /* CPDMA_H */