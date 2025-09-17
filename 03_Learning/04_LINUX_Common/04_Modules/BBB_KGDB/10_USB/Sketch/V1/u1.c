#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h> // alloc_chrdev_region
#include <linux/pci.h> // ioremap
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/atomic.h>
#include <linux/delay.h>
#include "u1.h"

/* ================== Tmp variables ================= */
u8 Tx1_flag = 0, Rx1_flag = 0;
usb_t ret = NONE;

/* ================== Utils ===================== */
u32 fifo_offset(u8 epnum)
{
	return 0x20 + (epnum * 4);
}

int wait_register_update(struct usb_device_data *data, void __iomem *mem, u16 offset, u16 bit_offset, u8 bit_val, u16 delay_ms, u8* name_register){
    unsigned long timeout;
    timeout = jiffies + msecs_to_jiffies(delay_ms);

    while ((ioread32(mem + offset)&(1u << bit_offset)) != (bit_val << bit_offset))  // Wait register updated
    {
        if (time_after(jiffies, timeout)) {
            dev_err(data->dev, "Timeout %s\n", name_register);
            return -ETIMEDOUT;
        }
        cpu_relax();
    } 

    return 0;
}

int wait_val_update(struct usb_device_data *data, u16 var, u16 val, u16 delay_ms, u8* name_val){
    unsigned long timeout;
    timeout = jiffies + msecs_to_jiffies(delay_ms);

    while (var != val)  // Wait val updated
    {
        if (time_after(jiffies, timeout)) {
            dev_err(data->dev, "Timeout %s\n", name_val);
            return -ETIMEDOUT;
        }
        cpu_relax();
    } 

    return 0;
}

void USB1_SetToken(struct usb_device_data *data, const u8* TokenSet){
    data->InsReq.bRequestType = TokenSet[0];
    data->InsReq.bRequest = TokenSet[1];
    data->InsReq.wValue = (TokenSet[3] << 8 | TokenSet[2]);
    data->InsReq.wIndex = (TokenSet[5] << 8 | TokenSet[4]);
    data->InsReq.wLength = (TokenSet[7] << 8 | TokenSet[6]);
}

void USB1_ClrToken(struct usb_device_data *data){
    u16 i = 0;

    u8* p = (u8*)(&data->InsReq);
    u16 s = sizeof(data->InsReq);

    for (i = 0; i < s; i++){
        p[i] = 0;
    }
}

void USB1_ApplyToken(struct usb_device_data *data, u8 epnum){
    u32 FIFO0_offset = fifo_offset(epnum);
    u32 val[2]; // entry to FIFO has size of 8 bytes

    val[0] = (data->InsReq.wValue << 16) | (data->InsReq.bRequest << 8) | (data->InsReq.bRequestType);
    val[1] = (data->InsReq.wLength << 8) | (data->InsReq.wIndex);

    iowrite32(val[0], data->base_usb1core + FIFO0_offset);
    iowrite32(val[1], data->base_usb1core + FIFO0_offset);
}

u32 USB1_ReadFIFO(struct usb_device_data *data, u8 epnum){
    u32 FIFO0_offset = fifo_offset(epnum);
    return ioread32(data->base_usb1core + FIFO0_offset);
}

/* ================== Handler =================*/
irqreturn_t USB1_handler(int irq, void *d){
    struct usb_device_data *data = d;
    u32 irqsts;
    u32 h_csr0;

    irqsts = ioread32(data->base_usbss + USBSS_IRQSTAT);

    // Interrupt status for USB1 Tx CPPI DMA packet completion status
    if ((irqsts&(1u << 10)) == 1u << 10){
        Tx1_flag = 1;
    }
    // Interrupt status for USB1 Rx CPPI DMA packet completion status
    if ((irqsts&(1u << 11)) == 1u << 11){
        Rx1_flag = 1;
        h_csr0 = ioread32(data->base_usb1ep0 + MUSB_CSR0);
        if ((h_csr0&(MUSB_CSR0_H_RXSTALL)) == MUSB_CSR0_H_RXSTALL){
            ret = RXSTALL;
        }
        else if ((h_csr0&(MUSB_CSR0_H_ERROR)) == MUSB_CSR0_H_ERROR){
            ret = ERROR;
        }
        else if ((h_csr0&(MUSB_CSR0_H_NAKTIMEOUT)) == MUSB_CSR0_H_NAKTIMEOUT){
            ret = NAK_TIMEOUT;
        }
        else if ((h_csr0&(MUSB_CSR0_RXPKTRDY)) == MUSB_CSR0_RXPKTRDY){
            ret = RXPKTRDY;
        }
        else {
            ret = ACK;
        }
    }

    return IRQ_HANDLED;
}

/* ================ Init funcs ============= */
void USB1_init(struct usb_device_data *data){
    u32 usbcore_pwr = 0;

    iowrite32(1u << 7, data->base_usb1ctl + USB1CTL_MODE); // host mode by sw

    usbcore_pwr = MUSB_POWER_ISOUPDATE;
    usbcore_pwr &=~(MUSB_POWER_HSENAB); /* LOW/FULL speed */
    iowrite32(usbcore_pwr, data->base_usb1core + MUSB_POWER);

    iowrite16(0xFFFF, data->base_usb1core + MUSB_INTRTXE); // enable TX ep0 and 15 eps
    iowrite16(0xFFFE, data->base_usb1core + MUSB_INTRRXE); // enable RX 15 eps
    iowrite8(0xF7, data->base_usb1core + MUSB_INTRUSBE); // 

    iowrite32(MUSB_DEVCTL_SESSION, data->base_usb1core + MUSB_DEVCTL); // When the USB controller go into session, it will assume the role of a host
}

void PHY1_init(struct usb_device_data *data){
    u32 usb1_ctrl = 0;

    usb1_ctrl &= ~(USBPHY_CM_PWRDN | USBPHY_OTG_PWRDN | USBPHY_OTGVDET_EN); // power: normal mode, no Vbus detect as host mode
    usb1_ctrl |= USBPHY_OTGSESSEND_EN;

    iowrite32(usb1_ctrl, data->base_con_usb1ctrl1 + USB_CTRL1);
}

/* ================== API for Control Transfer ===================== */
int USB1_SETUP_Transaction_GetDesc(struct usb_device_data *data){
    u16 host_csr0 = 0;

    USB1_ClrToken(data);
    USB1_SetToken(data, GetDesc_pkt);
    USB1_ApplyToken(data, 0); // Load the 8 bytes of the required Device request command into the Endpoint 0 FIFO

    host_csr0 = ioread32(data->base_usb1ep0 + MUSB_CSR0);
    host_csr0 |= MUSB_CSR0_H_SETUPPKT | MUSB_CSR0_TXPKTRDY; // Set SETUPPKT and TXPKTRDY 
    iowrite32(host_csr0, data->base_usb1ep0 + MUSB_CSR0);

    // wait for Endpoint 0 interrupt (after TX transfer completion - Token + Data0/1 packet)
    wait_val_update(data, Tx1_flag, 1, 2000, "SETUP: Token + Data0/1");
    Tx1_flag = 0;

    // wait for Endpoint 0 interrupt (after RX transfer completion - Handshake packet)
    wait_val_update(data, Rx1_flag, 1, 2000, "SETUP: Handshake");
    Rx1_flag = 0;
    // Check error
    if (ret == RXSTALL) {
        dev_info(data->dev, "RXSTALL\n");
        return -1;
    }
    else if (ret == ERROR) {
        dev_info(data->dev, "ERROR\n"); // send additional 2 times
        return -1;
    }
    else if (ret == NAK_TIMEOUT) {
        dev_info(data->dev, "NAK_TIMEOUT\n"); // .... consider later
        return -1;
    } 
    else if (ret == ACK){
        dev_info(data->dev, "data packet received!\n");
        data->InsReq.leftLength = data->InsReq.wLength;
    }
    return 0;
}

int USB1_IN_Transaction_GetDesc(struct usb_device_data *data, u8* buffer, u16* outlen){
    u16 host_csr0 = 0;
    int i = 0;
    u32 tmp[2];

    // handle Length
    data->InsReq.maxLengthEntryFIFO = 8;
    if (data->InsReq.leftLength < 8){
        *outlen = data->InsReq.leftLength;
        data->InsReq.leftLength = 0;
    } 
    else {
        data->InsReq.leftLength = data->InsReq.leftLength - data->InsReq.maxLengthEntryFIFO;
        *outlen = data->InsReq.maxLengthEntryFIFO;
    }

    host_csr0 = ioread32(data->base_usb1ep0 + MUSB_CSR0);
    host_csr0 |= MUSB_CSR0_H_REQPKT; // Set REQPKT 
    iowrite32(host_csr0, data->base_usb1ep0 + MUSB_CSR0);

    // wait for Endpoint 0 interrupt (IN token packet)
    wait_val_update(data, Tx1_flag, 1, 2000, "IN: Token");
    Tx1_flag = 0;

    // wait for Endpoint 0 interrupt (Data packet)
    wait_val_update(data, Rx1_flag, 1, 2000, "IN: Data0/1");
    Rx1_flag = 0;

    // wait for Endpoint 0 interrupt (Handshake packet)
    wait_val_update(data, Tx1_flag, 1, 2000, "IN: Handshake");
    Tx1_flag = 0;

    // Check error
    if (ret == RXSTALL) {
        dev_info(data->dev, "RXSTALL\n");
        return -1;
    }
    else if (ret == ERROR) {
        dev_info(data->dev, "ERROR\n"); // the controller has tried to send the required IN token three times without getting any response
        return -1;
    }
    else if (ret == NAK_TIMEOUT) {
        dev_info(data->dev, "NAK_TIMEOUT\n"); // .... consider later
        return -1;
    } 
    else if (ret == RXPKTRDY) {
        dev_info(data->dev, "RXPKTRDY - read FIFO\n"); 
        tmp[0] = USB1_ReadFIFO(data, 0);
        tmp[1] = USB1_ReadFIFO(data, 0);

        // read buffer from FIFO
        for (i = 0; i < *outlen; i++){
            if (i < 4){
                buffer[i] = *((u8*)&(tmp[0]) + i);
            }
            else {
                buffer[i] = *((u8*)&(tmp[1]) + i - 4);
            }
        }

        // clear RXPKTRDY
        host_csr0 = ioread32(data->base_usb1ep0 + MUSB_CSR0);
        host_csr0 &=~ MUSB_CSR0_RXPKTRDY; 
        iowrite32(host_csr0, data->base_usb1ep0 + MUSB_CSR0);
    } 
    return 0;
}

int USB1_STATUS_Transaction_GetDesc(struct usb_device_data *data){
    u16 host_csr0 = 0;

    host_csr0 = ioread32(data->base_usb1ep0 + MUSB_CSR0);
    host_csr0 |= MUSB_CSR0_H_STATUSPKT | MUSB_CSR0_TXPKTRDY; // Set STATUSPKT and TXPKTRDY 
    iowrite32(host_csr0, data->base_usb1ep0 + MUSB_CSR0);

    // Wait while the controller sends the OUT token and a zero-length DATA1 packet
    // wait for Endpoint 0 interrupt (after TX transfer completion - Token + zero Data0/1 packet)
    wait_val_update(data, Tx1_flag, 1, 2000, "STATUS: Token + zero Data0/1");
    Tx1_flag = 0;

    // wait for Endpoint 0 interrupt (after RX transfer completion - Handshake packet)
    wait_val_update(data, Rx1_flag, 1, 2000, "STATUS: Handshake");
    Rx1_flag = 0;

    // Check error
    if (ret == RXSTALL) {
        dev_info(data->dev, "RXSTALL\n");
        return -1;
    }
    else if (ret == ERROR) {
        dev_info(data->dev, "ERROR\n"); // the controller has tried to send the required IN token three times without getting any response
        return -1;
    }
    else if (ret == NAK_TIMEOUT) {
        dev_info(data->dev, "NAK_TIMEOUT\n"); // .... consider later
        return -1;
    } 
    else if (ret == ACK){
        dev_info(data->dev, "status acked!\n");
    }
    return 0;
}

int USB1_GetDesc_Transfer(struct usb_device_data *data){
    int ret;

    ret = USB1_SETUP_Transaction_GetDesc(data);

    if (ret == 0){
        do {
            ret = USB1_IN_Transaction_GetDesc(data, data->TX, &data->TX_len);
        } while(data->InsReq.leftLength != 0);
    }

    if (ret == 0){
        ret = USB1_STATUS_Transaction_GetDesc(data);
    }

    return ret;
}

MODULE_LICENSE("GPL");   // <-- REQUIRED
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Test module with u1.c helper");