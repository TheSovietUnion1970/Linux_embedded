#include "cpsw.h"

u8 mac_addr[6] = {0x24, 0x76, 0x25, 0xe7, 0x29, 0xf0};

int cpsw_ale_version(struct ether_device_data *data){
    u32 ale_id = 0;

    ale_id = ioread32(data->base_ale + ALE_IDVER);
    printk("ALE version: %d.%d\n", (ale_id >> 8)&0xFF, (ale_id)&0xFF);

    return 0;
}

int cpsw_soft_reset(struct ether_device_data *data){
    int ret;

    iowrite32(BIT_VAL_1, data->base_cpsw + CPSW_SOFT_RESET);
    ret = wait_register_update(data, data->base_cpsw, CPSW_SOFT_RESET, 0, BIT_VAL_0, 2000, "CPSW_SOFT_RESET");
    if (ret < 0) return -1;
    return 0;
}

/* cpsw_init_host_port funcs */
void cpsw_ale_start(struct ether_device_data *data)
{
    u32 ale_control = 0;

    /* soft reset the controller and initialize ale */
    ale_control = (ALE_ENABLE_ALE | ALE_CLEAR_TABLE);
    iowrite32(ale_control, data->base_ale + ALE_CONTROL);
}

void cpsw_ale_add_vlan_id0(struct ether_device_data *data){
    u8 i = 0;
    u8 idx = 0;
    u32 vlan_values[3] = {
        0x0, // MAC = 00:00:00:00:00:00 (or VLAN field)
        0x20000000, // Bit 61 = VLAN entry type
        0x07000007 // Ports 0, 1, 2 are members
    };
    for (i = 0; i < 3; i++){
        iowrite32(vlan_values[i], data->base_ale + ALE_TABLE + 4 * i);
    }

    iowrite32(ALE_TABLE_WRITE | idx, data->base_ale + ALE_TABLE_CONTROL);
}

void cpsw_init_host_port_dual_mac(struct ether_device_data *data){
    u32 ale_control = 0;

    /* host_port_dual_mac */
    iowrite32(CPSW_FIFO_DUAL_MAC_MODE, data->base_port0 + P0_TX_IN_CTL);

    /* unset P0_UNI_FLOOD */
    ale_control = ioread32(data->base_ale + ALE_CONTROL);
    ale_control &=~ (ALE_P0_UNI_FLOOD);
    iowrite32(ale_control, data->base_ale + ALE_CONTROL);

    /* TODO: default_vlan = 0 */
    iowrite32(0, data->base_port0 + P0_PORT_VLAN); 

    cpsw_ale_add_vlan_id0(data);

    /* learning make no sense in dual_mac mode in port 1 */
    ale_control = ioread32(data->base_ale + ALE_PORTCTL0);
    ale_control |= (ALE_NO_LEARN);
    iowrite32(ale_control, data->base_ale + ALE_PORTCTL0);

    ale_control = ioread32(data->base_ale + ALE_PORTCTL0);
    ale_control |= (ALE_PORT_STATE_FORWARD);
    iowrite32(ale_control, data->base_ale + ALE_PORTCTL0);  
}

void cpsw_init_host_port(struct ether_device_data *data){
    u32 ale_control = 0;
    u32 cpsw_control = 0;
    int ret;

    /* Soft reset */
    ret = cpsw_soft_reset(data);

    if (ret == 0){
        /* soft reset the controller and initialize ale */
        cpsw_ale_start(data);

        /* switch to vlan unaware mode */
        // Drop packet if VLAN not found
        ale_control = ioread32(data->base_ale + ALE_CONTROL);
        ale_control |= ALE_VLAN_AWARE;
        iowrite32(ale_control, data->base_ale + ALE_CONTROL);

        // Port 0 receive packets (from 3G) are VLAN encapsulated
        cpsw_control = ioread32(data->base_cpsw + CPSW_CONTROL);
        cpsw_control |= CPSW_VLAN_AWARE | CPSW_RX_VLAN_ENCAP;
        iowrite32(cpsw_control, data->base_cpsw + CPSW_CONTROL);

        /* setup host port priority mapping */
        iowrite32(CPDMA_TX_PRIORITY_MAP, data->base_port0 + P0_CPDMA_TX_PRI_MAP);
        iowrite32(0, data->base_port0 + P0_CPDMA_RX_CH_MAP);

        /* disable priority elevation */
        iowrite32(0, data->base_cpsw + CPSW_PTYPE);

        /* enable statistics collection only on all ports */
        iowrite32(0x7, data->base_cpsw + CPSW_STAT_PORT_EN);

        /* Enable internal fifo flow control */
        iowrite32(0x7, data->base_cpsw + CPSW_FLOW_CONTROL);  

        cpsw_init_host_port_dual_mac(data);
    }
}


/* cpsw_slave_open */
void cpsw_ale_add_vlan_id1(struct ether_device_data *data){
    u8 i = 0;
    u8 idx = 1; // Table Entry Pointer 
    u32 vlan_values[3] = {
        0x0, // (VLAN field)
        0x20010000, // Bit 61 = VLAN entry type, VLAN ID = 1 (bits 60:49)
        0x03030003 // Port mask = 0b011 → Port 0 (CPU) + Port 1 (RJ45)
    };
    for (i = 0; i < 3; i++){
        iowrite32(vlan_values[i], data->base_ale + ALE_TABLE + 4 * i);
    }

    iowrite32(ALE_TABLE_WRITE | idx, data->base_ale + ALE_TABLE_CONTROL);
}

void cpsw_ale_add_mcast_id2(struct ether_device_data *data){
    u8 i = 0;
    u8 idx = 2; // Table Entry Pointer 
    u32 vlan_values[3] = {
        0x4, // (VLAN field)
        0x3001ffff, // Bit 61 = VLAN entry type, VLAN ID = 1 (bits 60:49)
        0xffffffff // Port mask = 0b011 → Port 0 (CPU) + Port 1 (RJ45)
    };
    for (i = 0; i < 3; i++){
        iowrite32(vlan_values[i], data->base_ale + ALE_TABLE + 4 * i);
    }

    iowrite32(ALE_TABLE_WRITE | idx, data->base_ale + ALE_TABLE_CONTROL);
}

void cpsw_ale_add_ucast_id3(struct ether_device_data *data){
    u8 i = 0;
    u8 idx = 3; // Table Entry Pointer 
    u32 vlan_values[3] = {
        0x1, // (VLAN field)
        0x30012476, // Bit 61 = VLAN entry type, VLAN ID = 1 (bits 60:49)
        0x25e729f0 // Port mask = 0b011 → Port 0 (CPU) + Port 1 (RJ45)
    };
    for (i = 0; i < 3; i++){
        iowrite32(vlan_values[i], data->base_ale + ALE_TABLE + 4 * i);
    }

    iowrite32(ALE_TABLE_WRITE | idx, data->base_ale + ALE_TABLE_CONTROL);
}

void cpsw_port_add_dual_emac_def_ale_entries(struct ether_device_data *data){
    u32 ale_control = 0;

    // port VLAN ID = 1
    iowrite32(0x1, data->base_port1 + P1_PORT_VLAN);

    cpsw_ale_add_vlan_id1(data);
    cpsw_ale_add_mcast_id2(data);
    cpsw_ale_add_ucast_id3(data);

    ale_control = ioread32(data->base_ale + ALE_PORTCTL1);
    ale_control |= (ALE_DROP_UNKNOWN_VLAN);
    iowrite32(ale_control, data->base_ale + ALE_PORTCTL1);  

    /* learning make no sense in dual_mac mode */
    ale_control = ioread32(data->base_ale + ALE_PORTCTL1);
    ale_control |= (ALE_NO_LEARN);
    iowrite32(ale_control, data->base_ale + ALE_PORTCTL1);  
}

int cpsw_slave_open(struct ether_device_data *data){
    int ret;

    // soft reset
    iowrite32(BIT_VAL_1, data->base_cpsw_sl + P1_SOFTRESET);
    ret = wait_register_update(data, data->base_cpsw_sl, P1_SOFTRESET, 0, BIT_VAL_0, 2000, "CPSW_SL_P1_SOFT_RESET");
    if (ret < 0) return -1;
    if (ret == 0){
        // reset MACCONTROL
        iowrite32(BIT_VAL_0, data->base_cpsw_sl + P1_MACCONTROL);

        /* setup priority mapping */
        iowrite32(RX_PRIORITY_MAPPING, data->base_cpsw_sl + P1_RX_PRI_MAP);
    }
    if (ret == 0){
        // CPSW_VERSION_2:
        iowrite32(TX_PRIORITY_MAPPING, data->base_port1 + P1_TX_PRI_MAP);
		/* Increase RX FIFO size to 5 for supporting fullduplex
		 * flow control mode
		 */
        iowrite32((CPSW_MAX_BLKS_TX << 4) | CPSW_MAX_BLKS_RX, data->base_port1 + P1_MAX_BLKS);
    }
    if (ret == 0){
        /* setup max packet size, and mac address */
        iowrite32(0x5f6, data->base_cpsw_sl + P1_RX_MAXLEN);
        iowrite32(mac_hi(mac_addr), data->base_port1 + P1_SA_HI);
        iowrite32(mac_lo(mac_addr), data->base_port1 + P1_SA_LO);

        cpsw_port_add_dual_emac_def_ale_entries(data);
    }

    return ret;
}

int cpsw_init(struct ether_device_data *data){
    int ret;

    ret = cpsw_ale_version(data);

    if (ret == 0){
        printk("Host port\n");
        // ale, ss, 
        cpsw_init_host_port(data);

        cpsw_slave_open(data);
    }

    return ret;
}