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

/* ================ Const data packet =======================*/
// const u8 GetStat_pkt[8] = {
//     0x80,       // bmRequestType: Device-to-host, Standard, Device
//     0x00,       // bRequest: USB_REQ_GET_STATUS
//     0x00, 0x01, // wValue: Descriptor Index = 0 (LOW), Type = DEVICE (1) (HIGH)
//     0x00, 0x00, // wIndex: 0
//     0x12, 0x00  // wLength: 18 bytes (Device descriptor length)
// };

const u8 GetDesc_pkt[8] = {
    0x80,       // bmRequestType: Device-to-host, Standard, Device
    0x06,       // bRequest: USB_REQ_GET_DESCRIPTOR
    0x00, 0x01, // wValue: Descriptor Index = 0 (LOW), Type = DEVICE (1) (HIGH)
    0x00, 0x00, // wIndex: 0
    0x12, 0x00  // wLength: 18 bytes (Device descriptor length)
};

const u8 SetAddr_pkt[8] = {
    0x00,       // bmRequestType: Host-to-device, Standard, Device
    0x05,       // bRequest: USB_REQ_SET_ADDRESS
    0x01, 0x00, // wValue: Device address: 1
    0x00, 0x00, // wIndex: 0
    0x00, 0x00  // wLength: 0
};

/* ================== Tmp variables ================= */
u16 Tx1_flag = 0, Rx1_flag = 0;
usb_t ret = NONE;

/* MAP: indexed register
USBCORE1 0x47401C00:
@1C00-1C0F: faddr(1 byte), power(1), ..., index(1), testmode(1).
@1C10-1C1F: EPx control + status register.
@1C20-....: EP0_FIFO_entry(4)-...
 */

/* ================== Utils ===================== */
void setIndex(struct usb_device_data *data, u8 epnum){
    iowrite8(epnum, data->base_usb1core + MUSB_INDEX); // @1C0E
}
void setFifo(struct usb_device_data *data){
    u8 babblectl = 0;

    // TX
    iowrite8(0x03, data->base_usb1core + MUSB_TXFIFOSZ); // sz = 3 -> fifo size = 2^(sz+3) = 64 bytes for TX FIFO0
    iowrite16(0x00, data->base_usb1core + MUSB_TXFIFOADD); 

    // RX
    iowrite8(0x03, data->base_usb1core + MUSB_RXFIFOSZ); // sz = 3 -> fifo size = 2^(sz+3) = 64 bytes for RX FIFO0
    iowrite16(0x00, data->base_usb1core + MUSB_RXFIFOADD); 

    // fifo type0 (8-bit)
    iowrite8(0, data->base_usb1core + MUSB_INDEX); // @1C0E

    iowrite16(0x0200, data->base_usb1core + 0x10 + MUSB_CSR0);
    iowrite8(0x80, data->base_usb1core + 0x10 + MUSB_TYPE0);
    //iowrite8(0xDE, data->base_usb1core + 0x10 + MUSB_CONFIGDATA);

    babblectl = ioread8(data->base_usb1core + MUSB_BABBLE_CTL);
    if (babblectl&MUSB_BABBLE_RCV_DISABLE){
		babblectl |= MUSB_BABBLE_SW_SESSION_CTRL;
        iowrite8(babblectl, data->base_usb1core + MUSB_BABBLE_CTL);
	}
}
u32 fifo_offset(u8 epnum)
{
	return 0x20 + (epnum * 4); // @1C20
}

int wait_register_update(struct usb_device_data *data, void __iomem *mem, u16 reg_offset, u16 bit_offset, u8 bit_val, u16 delay_ms, u8* name_register){
    unsigned long timeout;
    timeout = jiffies + msecs_to_jiffies(delay_ms);

    while ((ioread32(mem + reg_offset)&(1u << bit_offset)) != (bit_val << bit_offset))  // Wait register updated
    {
        if (time_after(jiffies, timeout)) {
            dev_err(data->dev, "Timeout %s\n", name_register);
            return -ETIMEDOUT;
        }
        cpu_relax();
    } 

    return 0;
}

int wait_val_update(struct usb_device_data *data, u16* var, u16 val, u16 delay_ms, u8* name_val){
    unsigned long timeout;
    timeout = jiffies + msecs_to_jiffies(delay_ms);

    while (*var != val)  // Wait val updated
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

    printk("val[0] = 0x%x, val[1] = 0x%x\n", val[0], val[1]);
    iowrite32(val[0], data->base_usb1core + FIFO0_offset);
    iowrite32(val[1], data->base_usb1core + FIFO0_offset);

}

u32 USB1_ReadFIFO(struct usb_device_data *data, u8 epnum){
    u32 FIFO0_offset = fifo_offset(epnum);
    return ioread32(data->base_usb1core + FIFO0_offset);
}

void USB1_IRQ_clr(struct usb_device_data *data, u32 reg_offset, u32 val){
    u32 reg = ioread32(data->base_usb1ctl + reg_offset);
    reg |= val;
    iowrite32(reg, data->base_usb1ctl + reg_offset);
}
/* ================== Handler =================*/
irqreturn_t USB1_handler(int irq, void *d){
    struct usb_device_data *data = d;
    u16 irqtx, irqrx, irqusb;
    u32 irqsts;
    u32 irqsts0, irqsts1;

    // if ((data->base_usb1core == NULL) || (data->base_usb1ctl == NULL) || (data->base_usbss == NULL)){
    //     printk("Not yet init\n");
    //     return IRQ_HANDLED;
    // }

    irqtx = ioread16(data->base_usb1core + MUSB_INTRTX);
    irqrx = ioread16(data->base_usb1core + MUSB_INTRRX);
    irqusb = ioread16(data->base_usb1core + MUSB_INTRUSB)&0x00FF;

    irqsts0 = ioread32(data->base_usb1ctl + USB1CTL_IRQSTAT0);
    irqsts1 = ioread32(data->base_usb1ctl + USB1CTL_IRQSTAT1);
    printk("irqtx = 0x%x, irqrx = 0x%x, irqusb = 0x%x, irqsts0 = 0x%x, s1 = 0x%x", irqtx, irqrx, irqusb, irqsts0, irqsts1);

    irqsts = ioread32(data->base_usbss + USBSS_IRQSTAT);
    if ((irqsts1)&(1u << 7)){
        printk("< VBUS valid threshold>\n");
        // iowrite32(0x8, data->base_usb1ctl + USB1CTL_IRQENCLR1);
        USB1_IRQ_clr(data, USB1CTL_IRQENCLR1, 1u << 7);
    }
    // sts0
    if ((irqsts0&(1u << 0)) == (1u << 0)){
        Tx1_flag = 1;
        printk("TX endpoint 0\n");
        //iowrite32(1u << 0, data->base_usb1ctl + USB1CTL_IRQENCLR0);
        USB1_IRQ_clr(data, USB1CTL_IRQENCLR0, 1u << 0);
    }

    // sts1
    if ((irqsts1&(1u << 3)) == (1u << 3)){
        printk("SOF started\n");
        Tx1_flag = 1;
        //iowrite32(1u << 3, data->base_usb1ctl + USB1CTL_IRQENCLR1);
        USB1_IRQ_clr(data, USB1CTL_IRQENCLR1, 1u << 3);
    }
    if ((irqsts1&(1u << 2)) == (1u << 2)){
        printk("Babble detected\n");
        //iowrite32(1u << 2, data->base_usb1ctl + USB1CTL_IRQENCLR1);
        USB1_IRQ_clr(data, USB1CTL_IRQENCLR1, 1u << 2);
    }
    if ((irqsts1&(1u << 4)) == (1u << 4)){
        printk("Device connected\n");
        //iowrite32(1u << 4, data->base_usb1ctl + USB1CTL_IRQENCLR1);
        USB1_IRQ_clr(data, USB1CTL_IRQENCLR1, 1u << 4);
    }
    if ((irqsts1&(1u << 5)) == (1u << 5)){
        printk("Device disconnected\n");
        //iowrite32(1u << 5, data->base_usb1ctl + USB1CTL_IRQENCLR1);
        USB1_IRQ_clr(data, USB1CTL_IRQENCLR1, 1u << 5);
    }
    if ((irqsts1&(1u << 8)) == (1u << 8)){
        printk("DRVVBUS level change\n");
        //iowrite32(1u << 8, data->base_usb1ctl + USB1CTL_IRQENCLR1);
        USB1_IRQ_clr(data, USB1CTL_IRQENCLR1, 1u << 8);
    }

    data->count_many++;
    if (data->count_many > 100){
        printk("Too many interrupts\n");
        data->count_many = 0;

        iowrite8(0x0, data->base_usb1core + MUSB_DEVCTL);
        iowrite32(0x0, data->base_usb1ctl + USB1CTL_IRQENSET0);
        iowrite32(0x0, data->base_usb1ctl + USB1CTL_IRQENSET1);

        iowrite16(0x0, data->base_usb1core + MUSB_INTRTXE); // enable TX ep0 and 15 eps
        iowrite16(0x0, data->base_usb1core + MUSB_INTRRXE); // enable RX 15 eps
        iowrite8(0x0, data->base_usb1core + MUSB_INTRUSBE); // 

        USB1_reset(data);
    }

    return IRQ_HANDLED;
}

irqreturn_t USBS_handler(int irq, void *d){


    return IRQ_HANDLED;
}

/* ================ Init funcs ============= */
void USB1_reset(struct usb_device_data *data){
    u32 usb1ctl = 0;

    iowrite32((1u << 0) | (1u << 5), data->base_usb1ctl + USB1CTL_CTRL); // soft reset + isolation
    wait_register_update(data, data->base_usb1ctl, USB1CTL_CTRL, 0, 0, 2000, "RESET"); // wait reset

    // clear soft reset isolation
    usb1ctl = ioread32(data->base_usb1ctl + USB1CTL_CTRL);
    usb1ctl &=~ (1u << 5);
    iowrite32(usb1ctl, data->base_usb1ctl + USB1CTL_CTRL);

}

int USB1_init(struct usb_device_data *data){
    u32 usbcore_pwr = 0, usbcore_testmode = 0;
    u8 power = 0;
    power &= 0xf0;

    int ret = 0;

    // disable testmode
    usbcore_testmode = 0x00;
    iowrite32(usbcore_testmode, data->base_usb1core + MUSB_TESTMODE);

    // init high speed
    usbcore_pwr = MUSB_POWER_ISOUPDATE;
    usbcore_pwr |= (MUSB_POWER_HSENAB); /* full speed */
    iowrite32(usbcore_pwr, data->base_usb1core + MUSB_POWER);

    // host mode by sw
    iowrite32(1u << 7, data->base_usb1ctl + USB1CTL_MODE); // host mode by sw

    // test mode
    //iowrite8(MUSB_TEST_FORCE_HOST, data->base_usb1core + MUSB_TESTMODE);

    // session
    iowrite8(MUSB_DEVCTL_SESSION, data->base_usb1core + MUSB_DEVCTL); // When the USB controller go into session, it will assume the role of a host
    ret = wait_register_update(data, data->base_usb1core, MUSB_DEVCTL, 0, MUSB_DEVCTL_SESSION, 2000, "DEVCTL_SESSION"); // wait DEVCTL_SESSION is set to 1
    if (ret < 0) return -1;

    // enable all interrupts after session
    iowrite32(0xFFFEFFFF, data->base_usb1ctl + USB1CTL_IRQENSET0);
    iowrite32(0xFFFFFFFF, data->base_usb1ctl + USB1CTL_IRQENSET1);

    iowrite16(0xFFFF, data->base_usb1core + MUSB_INTRTXE); // enable TX ep0 and 15 eps
    iowrite16(0xFFFE, data->base_usb1core + MUSB_INTRRXE); // enable RX 15 eps
    iowrite8(0xF7, data->base_usb1core + MUSB_INTRUSBE); // 

    msleep(50);
    printk("Reset\n...");
    power |= MUSB_POWER_RESET;
    iowrite32(power , data->base_usb1ctl + MUSB_POWER);
    msleep(50); // stay 50 ms for reset

    printk("Stop reset\n...");
    power &=~ MUSB_POWER_RESET;
    power |= MUSB_POWER_SOFTCONN;
    iowrite32(power, data->base_usb1ctl + MUSB_POWER);
    
    // wait host mode (bit 2, val 1)
    ret = wait_register_update(data, data->base_usb1core, MUSB_DEVCTL, 2, 1, 2000, "HOST MODE"); // wait DEVCTL_SESSION is set to 1
    if (ret < 0) return -1;

    return 0;
}

void PHY1_init(struct usb_device_data *data){
    u32 usb1_ctrl = 0, usb_wkup = 0;

    // usb wkup
    usb_wkup = ioread32(data->base_con_usb1ctrl1 + USB_WKUP);
    usb_wkup |= (1u << 0) | (1u << 8);
    iowrite32(usb_wkup, data->base_con_usb1ctrl1 + USB_WKUP);

    // usb1 ctrl
    usb1_ctrl = ioread32(data->base_con_usb1ctrl1 + USB_CTRL1);
    usb1_ctrl &= ~(USBPHY_CM_PWRDN | USBPHY_OTG_PWRDN | USBPHY_OTGVDET_EN); // power: normal mode, no Vbus detect as host mode
    usb1_ctrl |= USBPHY_OTGSESSEND_EN;

    iowrite32(usb1_ctrl, data->base_con_usb1ctrl1 + USB_CTRL1);

    msleep(1); // Give the PHY ~1ms to complete the power up operation.
}

/* ================== API for Control Transfer ===================== */
int USB1_SETUP_Transaction_GetDesc(struct usb_device_data *data){
    u16 host_csr0 = 0;
    int ret = 0;
    int i = 0;

#if (INDEX_USED)
    setIndex(data, 0);
#endif
    setFifo(data);

    USB1_ClrToken(data);
    USB1_SetToken(data, GetDesc_pkt);
    USB1_ApplyToken(data, 0); // Load the 8 bytes of the required Device request command into the Endpoint 0 FIFO

    printk("0x%x 0x%x 0x%x 0x%x\n", ioread32(data->base_usb1core), ioread32(data->base_usb1core + 0x4), ioread32(data->base_usb1core + 0x8), ioread32(data->base_usb1core + 0xc));
    printk("0x%x 0x%x 0x%x 0x%x\n", ioread32(data->base_usb1core + 0x10), ioread32(data->base_usb1core + 0x14), ioread32(data->base_usb1core + 0x18), ioread32(data->base_usb1core + 0x1c));
    //printk("0x%x\n", ioread32(data->base_usb1core + 0x20));
    printk("devctl -> 0x%x\n", ioread8(data->base_usb1core + 0x60));
    printk("BABBLE -> 0x%x\n", ioread8(data->base_usb1core + 0x61));
    printk("FIFOSZ -> 0x%x\n", ioread16(data->base_usb1core + 0x62));
    printk("FIFOADDR -> 0x%x\n", ioread32(data->base_usb1core + 0x64));

    for (i = 0; i < 3; i++){
        host_csr0 = ioread16(data->base_usb1core + 0x10 + MUSB_CSR0);
        host_csr0 |= MUSB_CSR0_H_SETUPPKT | MUSB_CSR0_TXPKTRDY; // Set SETUPPKT and TXPKTRDY 
        iowrite16(host_csr0, data->base_usb1core + 0x10 + MUSB_CSR0);

        // wait for Endpoint 0 interrupt (after TX transfer completion - Token + Data0/1 packet)
        ret = wait_val_update(data, &Tx1_flag, 1, 2000, "SETUP: Token + Data0/1");
        if (ret < 0) return -1;
        Tx1_flag = 0;
    }


    // wait for Endpoint 0 interrupt (after RX transfer completion - Handshake packet)
    wait_val_update(data, &Rx1_flag, 1, 2000, "SETUP: Handshake");
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

    host_csr0 = ioread16(data->base_usb1core + 0x10 + MUSB_CSR0);
    host_csr0 |= MUSB_CSR0_H_REQPKT; // Set REQPKT 
    iowrite16(host_csr0, data->base_usb1core + 0x10 + MUSB_CSR0);

    // wait for Endpoint 0 interrupt (IN token packet)
    wait_val_update(data, &Tx1_flag, 1, 2000, "IN: Token");
    Tx1_flag = 0;

    // wait for Endpoint 0 interrupt (Data packet)
    wait_val_update(data, &Rx1_flag, 1, 2000, "IN: Data0/1");
    Rx1_flag = 0;

    // wait for Endpoint 0 interrupt (Handshake packet)
    wait_val_update(data, &Tx1_flag, 1, 2000, "IN: Handshake");
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
        host_csr0 = ioread32(data->base_usb1core + 0x10 + MUSB_CSR0);
        host_csr0 &=~ MUSB_CSR0_RXPKTRDY; 
        iowrite32(host_csr0, data->base_usb1core + 0x10 + MUSB_CSR0);
    } 
    return 0;
}

int USB1_STATUS_Transaction_GetDesc(struct usb_device_data *data){
    u16 host_csr0 = 0;

    host_csr0 = ioread16(data->base_usb1core + 0x10 + MUSB_CSR0);
    host_csr0 |= MUSB_CSR0_H_STATUSPKT | MUSB_CSR0_TXPKTRDY; // Set STATUSPKT and TXPKTRDY 
    iowrite16(host_csr0, data->base_usb1core + 0x10 + MUSB_CSR0);

    // Wait while the controller sends the OUT token and a zero-length DATA1 packet
    // wait for Endpoint 0 interrupt (after TX transfer completion - Token + zero Data0/1 packet)
    wait_val_update(data, &Tx1_flag, 1, 2000, "STATUS: Token + zero Data0/1");
    Tx1_flag = 0;

    // wait for Endpoint 0 interrupt (after RX transfer completion - Handshake packet)
    wait_val_update(data, &Rx1_flag, 1, 2000, "STATUS: Handshake");
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
// MODULE_LICENSE("GPL");   // <-- REQUIRED

