#include "cpsw.h"

int cpsw_ale_version(struct ether_device_data *data){
    u32 ale_id = 0;

    ale_id = ioread32(data->base_ale + ALE_IDVER);
    printk("ALE version: %d.%d\n", (ale_id >> 8)&0xFF, (ale_id)&0xFF);

    return 0;
}

int cpsw_soft_reset(struct ether_device_data *data){
    int ret;

    iowrite32(0x1, data->base_cpsw + CPSW_SOFT_RESET);
    ret = wait_register_update(data, data->base_cpsw, CPSW_SOFT_RESET, 0, BIT_VAL_0, 2000, "CPSW_SOFT_RESET");
    if (ret < 0) return -1;
    return 0;
}

void cpsw_ale_start(struct ether_device_data *data)
{
    u32 ale_control = 0;

    /* soft reset the controller and initialize ale */
    ale_control = (ALE_ENABLE_ALE | ALE_CLEAR_TABLE);
    iowrite32(ale_control, data->base_ale + ALE_CONTROL);
}

void cpsw_init_host_port_dual_mac(struct ether_device_data *data){
    u32 ale_control = 0;

    /* host_port_dual_mac */
    iowrite32(CPSW_FIFO_DUAL_MAC_MODE, data->base_port0 + P0_TX_IN_CTL);

    /* unset P0_UNI_FLOOD */
    ale_control = ioread32(data->base_ale + ALE_CONTROL);
    ale_control &=~ (ALE_P0_UNI_FLOOD);
    iowrite32(ale_control, data->base_ale + ALE_CONTROL);

    /* TODO: default_vlan = 1 */
    iowrite32(1, data->base_port0 + P0_PORT_VLAN); 
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

int cpsw_cpdma_init(struct ether_device_data *data){
    return 0;
}

int cpsw_init(struct ether_device_data *data){
    int ret;

    ret = cpsw_ale_version(data);

    if (ret == 0){
        cpsw_init_host_port(data);
    }

    return ret;
}