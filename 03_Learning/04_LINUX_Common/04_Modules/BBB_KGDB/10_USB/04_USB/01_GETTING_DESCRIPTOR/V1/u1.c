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
u16 count;

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
    iowrite8(3, data->base_usb1core + MUSB_TXFIFOSZ); // sz = 3 -> fifo size = 2^(sz+3) = 64 bytes for TX FIFO0
    iowrite16(0x00, data->base_usb1core + MUSB_TXFIFOADD); 

    // RX
    iowrite8(3, data->base_usb1core + MUSB_RXFIFOSZ); // sz = 3 -> fifo size = 2^(sz+3) = 64 bytes for RX FIFO0
    iowrite16(0x00, data->base_usb1core + MUSB_RXFIFOADD); 

    // fifo type0 (8-bit)
    iowrite8(0, data->base_usb1core + MUSB_INDEX); // @1C0E

    //iowrite16(0x0200, data->base_usb1core + 0x10 + MUSB_CSR0);
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

void USB1_IRQ_clr8(struct usb_device_data *data, u8 reg_offset, u16 val){
    // u8 reg = ioread8(data->base_usb1core + reg_offset);
    // reg |= val;
    iowrite8(val, data->base_usb1core + reg_offset);
}

void USB1_IRQ_clr32(struct usb_device_data *data, u32 reg_offset, u32 val){
    // u32 reg = ioread32(data->base_usb1ctl + reg_offset);
    // reg |= val;
    iowrite32(val, data->base_usb1ctl + reg_offset);
}
/* ================== Handler =================*/
void temp_reset(struct usb_device_data *data, u8 int_tx){
    u32 usb1_ctrl = 0;
    u8 musb1_babble = 0;

    iowrite32(1 << 0, data->base_usb1ctl + USB1CTL_CTRL); // soft reset

    // /* reset the otgdisable bit, needed for host mode to work */
	// usb1_ctrl = ioread32(data->base_usb1ctl + USB1CTL_UTMI);
	// usb1_ctrl &= ~(1 << 21); // wrp->otg_disable
	// iowrite32(usb1_ctrl, data->base_usb1ctl + USB1CTL_UTMI);

    // // MUSB_BABBLE_CTL
    // musb1_babble = ioread8(data->base_usb1core + MUSB_BABBLE_CTL);
    // if (musb1_babble&MUSB_BABBLE_RCV_DISABLE){
    //     musb1_babble |= MUSB_BABBLE_SW_SESSION_CTRL;
    //     iowrite8(musb1_babble, data->base_usb1core + MUSB_BABBLE_CTL);
    // }


    // // usb1 ctrl
    // usb1_ctrl = ioread32(data->base_con_usb1ctrl1 + USB_CTRL1);
    // usb1_ctrl &= ~(USBPHY_CM_PWRDN | USBPHY_OTG_PWRDN | USBPHY_OTGVDET_EN); // power: normal mode, no Vbus detect as host mode
    // usb1_ctrl |= USBPHY_OTGSESSEND_EN;

    // iowrite32(usb1_ctrl, data->base_con_usb1ctrl1 + USB_CTRL1);

    // /* disable interrupts */
    // iowrite8(0, data->base_usb1core + MUSB_INTRUSBE);
    // iowrite16(0, data->base_usb1core + MUSB_INTRTXE);
    // iowrite16(0, data->base_usb1core + MUSB_INTRRXE);

    // /*  flush pending interrupts */
    // iowrite8(0xff, data->base_usb1core + MUSB_INTRUSB);
    // iowrite16(0xffff, data->base_usb1core + MUSB_INTRTX);
    // iowrite16(0xffff, data->base_usb1core + MUSB_INTRRX);

    // iowrite32(0x1ff, data->base_usb1ctl + USB1CTL_COREINT_CLR);
    // iowrite32(0xfffeffff, data->base_usb1ctl + USB1CTL_EPINT_CLR);

    // usb1_ctrl = ioread32(data->base_usb1ctl + USB1CTL_MODE);
    // usb1_ctrl &=~ (1u << 8); // A type
    // usb1_ctrl |= (1u << 7); // iddig_mux
    // iowrite32(usb1_ctrl, data->base_usb1ctl + USB1CTL_MODE);

    // iowrite32(0x2, data->base_usb1ctl + USB1CTL_UTMI);


    // ===
    u8 power = 0;

    // tell the SoC to forward the USB core IRQ to the CPU.
    iowrite32(int_tx, data->base_usb1ctl + USB1CTL_IRQEN0);
    iowrite32(0x0, data->base_usb1ctl + USB1CTL_IRQEN1);

    // tell the USB core which events matter.
    iowrite16(0xFFFF, data->base_usb1core + MUSB_INTRTXE); // enable TX ep0 and 15 eps
    iowrite16(0xFFFE, data->base_usb1core + MUSB_INTRRXE); // enable RX 15 eps
    iowrite8(0xF7, data->base_usb1core + MUSB_INTRUSBE); // 

    // go into session
    iowrite8(MUSB_DEVCTL_SESSION, data->base_usb1core + MUSB_DEVCTL); // When the USB controller go into session, it will assume the role of a host
    ret = wait_register_update(data, data->base_usb1core, MUSB_DEVCTL, 0, MUSB_DEVCTL_SESSION, 2000, "DEVCTL_SESSION"); // wait DEVCTL_SESSION is set to 1

    // release reset
    power &=~ MUSB_POWER_RESET;
    //power |= MUSB_POWER_SOFTCONN;
    power |= MUSB_POWER_ISOUPDATE | MUSB_POWER_HSENAB; // init high speed
    iowrite8(power, data->base_usb1core + MUSB_POWER);
}

void temp_reset0(struct usb_device_data *data, u32 int_val){
    u32 usb1_ctrl = 0;
    u8 musb1_babble = 0;

    iowrite32(1 << 0, data->base_usb1ctl + USB1CTL_CTRL); // soft reset

    // MUSB_BABBLE_CTL
    musb1_babble = ioread8(data->base_usb1core + MUSB_BABBLE_CTL);
    if (musb1_babble&MUSB_BABBLE_RCV_DISABLE){
        musb1_babble |= MUSB_BABBLE_SW_SESSION_CTRL;
        iowrite8(musb1_babble, data->base_usb1core + MUSB_BABBLE_CTL);
    }
    // ===
    u8 power = 0;

    // tell the SoC to forward the USB core IRQ to the CPU.
    // iowrite32(0xFFFEFFFF, data->base_usb1ctl + USB1CTL_IRQEN0);
    // iowrite32(0xFFFE03F7, data->base_usb1ctl + USB1CTL_IRQEN1);
    iowrite32(int_val, data->base_usb1ctl + USB1CTL_IRQEN0);

    // tell the USB core which events matter.
    iowrite16(0xFFFF, data->base_usb1core + MUSB_INTRTXE); // enable TX ep0 and 15 eps
    iowrite16(0xFFFE, data->base_usb1core + MUSB_INTRRXE); // enable RX 15 eps
    iowrite8(0xF7, data->base_usb1core + MUSB_INTRUSBE); // 

    // go into session
    iowrite8(MUSB_DEVCTL_SESSION, data->base_usb1core + MUSB_DEVCTL); // When the USB controller go into session, it will assume the role of a host
    ret = wait_register_update(data, data->base_usb1core, MUSB_DEVCTL, 0, MUSB_DEVCTL_SESSION, 2000, "DEVCTL_SESSION"); // wait DEVCTL_SESSION is set to 1

    // release reset
    power &=~ MUSB_POWER_RESET;
    //power |= MUSB_POWER_SOFTCONN;
    power |= MUSB_POWER_ISOUPDATE | MUSB_POWER_HSENAB; // init high speed
    iowrite8(power, data->base_usb1core + MUSB_POWER);
}
void temp_reset1(struct usb_device_data *data, u32 int_val){
    u32 usb1_ctrl = 0;
    u8 musb1_babble = 0;

    iowrite32(1 << 0, data->base_usb1ctl + USB1CTL_CTRL); // soft reset

    // ===
    u8 power = 0;

    // tell the SoC to forward the USB core IRQ to the CPU.
    //iowrite32(int_val, data->base_usb1ctl + USB1CTL_IRQEN0);
    iowrite32(int_val, data->base_usb1ctl + USB1CTL_IRQEN1);

    // tell the USB core which events matter.
    iowrite16(0xFFFF, data->base_usb1core + MUSB_INTRTXE); // enable TX ep0 and 15 eps
    iowrite16(0xFFFE, data->base_usb1core + MUSB_INTRRXE); // enable RX 15 eps
    iowrite8(0xF7, data->base_usb1core + MUSB_INTRUSBE); // 

    // go into session
    iowrite8(MUSB_DEVCTL_SESSION, data->base_usb1core + MUSB_DEVCTL); // When the USB controller go into session, it will assume the role of a host
    ret = wait_register_update(data, data->base_usb1core, MUSB_DEVCTL, 0, MUSB_DEVCTL_SESSION, 2000, "DEVCTL_SESSION"); // wait DEVCTL_SESSION is set to 1

    // release reset
    power &=~ MUSB_POWER_RESET;
    //power |= MUSB_POWER_SOFTCONN;
    power |= MUSB_POWER_ISOUPDATE | MUSB_POWER_HSENAB; // init high speed
    iowrite8(power, data->base_usb1core + MUSB_POWER);
}


void stop_reset(struct usb_device_data *data);
void reset(struct usb_device_data *data);
int USB1_SETUP_temp(struct usb_device_data *data);

void IntoSuspendMode(struct usb_device_data *data){
    u8 pwr = 0;

    pwr = ioread8(data->base_usb1core + MUSB_POWER);
    pwr |= MUSB_POWER_SUSPENDM;
    iowrite8(pwr, data->base_usb1core + MUSB_POWER);
}

void OutSuspendMode(struct usb_device_data *data){
    u8 pwr = 0;

    pwr = ioread8(data->base_usb1core + MUSB_POWER);
    pwr &=~ MUSB_POWER_SUSPENDM;
    iowrite8(pwr, data->base_usb1core + MUSB_POWER);

    //msleep(50);
}

void IntoResumeMode(struct usb_device_data *data){
    u8 pwr = 0;

    pwr = ioread8(data->base_usb1core + MUSB_POWER);
    pwr |= MUSB_POWER_RESUME;
    iowrite8(pwr, data->base_usb1core + MUSB_POWER);
}

void OutResumeMode(struct usb_device_data *data){
    u8 pwr = 0;

    pwr = ioread8(data->base_usb1core + MUSB_POWER);
    pwr &=~ MUSB_POWER_RESUME;
    iowrite8(pwr, data->base_usb1core + MUSB_POWER);

    msleep(50);
}

irqreturn_t USB1_handler(int irq, void *d){
    struct usb_device_data *data = d;
    u16 irqtx, irqrx, irqusb;
    u32 irqst0, irqst1;
    u16 int_tx, int_rx;
    u16 tmp = 0;


    irqst0 = ioread32(data->base_usb1ctl + USB1CTL_IRQST0);
    irqst1 = ioread32(data->base_usb1ctl + USB1CTL_IRQST1);
    int_tx = irqst0&0xFF;
    int_rx = (irqst0 >> 16)&0xFE;

    if (irqst0)
        iowrite32(irqst0, data->base_usb1ctl + USB1CTL_IRQST0);
    if (irqst1)
        iowrite32(irqst1, data->base_usb1ctl + USB1CTL_IRQST1);

    // //printk("TX = 0x%x, RX = 0x%x, USB = 0x%x", irqtx, irqrx, irqusb);
    // printk("st0 = 0x%x, st1 = 0x%x", irqst0, irqst1);

    if (irqst0){
        if (int_tx){
            Tx1_flag = 1;
            printk("ISR -> TX[%d]\n", int_tx);
        }
        if (int_rx){
            Rx1_flag = 1;
            printk("ISR -> RX[%d]\n", int_rx);
        }
    }
    if (irqst1){
        if ((irqst1)&(1u << 7)){
            printk("< VBUS valid threshold>\n");
        } 
        if ((irqst1)&(1u << 2)){
            printk("Babble detected\n");
        } 
        if ((irqst1)&(1u << 4)){
            printk("Device connected\n");
        }   
        if ((irqst1)&(1u << 5)){
            printk("Device disconnected\n");
        }     
        if ((irqst1)&(1u << 8)){
            printk("DRVVBUS level change\n");
        }   
        if ((irqst1)&(1u << 6)){
            printk("SRF ...\n");
        }    
    }

    data->count_many++;
    if (data->count_many > 100){
        printk("Too many interrupts\n");
        data->count_many = 0;

        USB1_reset(data);
    }

    return IRQ_HANDLED;
}

/* ================ Init funcs ============= */
void USB1_reset(struct usb_device_data *data){
    u32 usb1ctl = 0;

    iowrite32((1u << 0) | (1u << 5), data->base_usb1ctl + USB1CTL_CTRL); // soft reset + isolation
    wait_register_update(data, data->base_usb1ctl, USB1CTL_CTRL, 0, 0, 2000, "RESET"); // wait reset
    wait_register_update(data, data->base_usb1ctl, USB1CTL_CTRL, 5, 0, 2000, "RESET ISOLATION"); // wait reset

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
    // iowrite32(0xFFFEFFFF, data->base_usb1ctl + USB1CTL_IRQENSET0);
    // iowrite32(0xFFFFFFFF, data->base_usb1ctl + USB1CTL_IRQENSET1);

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
    u32 usb1_ctlreg = 0;
    u32 usb1_ctrl = 0;

    // return 0 if not clocked
    usb1_ctrl = ioread32(data->base_usb1ctl + USB1CTL_REV);
    if (!usb1_ctrl){
        printk("REV = 0 -> error\n");
        return;
    }

    // usb1 ctrl
    usb1_ctrl = ioread32(data->base_con_usb1ctrl1 + USB_CTRL1);
    usb1_ctrl &= ~(USBPHY_CM_PWRDN | USBPHY_OTG_PWRDN | USBPHY_OTGVDET_EN); // power: normal mode, no Vbus detect as host mode
    usb1_ctrl |= USBPHY_OTGSESSEND_EN;

    iowrite32(usb1_ctrl, data->base_con_usb1ctrl1 + USB_CTRL1);

    msleep(1); // Give the PHY ~1ms to complete the power up operation.
}

// ====== helper func =======
/* dsps_musb_disable - disable HDRC and flush interrupts */
void dsps_musb_disable_V(struct usb_device_data *data){
    iowrite32(0x1ff, data->base_usb1ctl + USB1CTL_COREINT_CLR);
    iowrite32(0xfffeffff, data->base_usb1ctl + USB1CTL_EPINT_CLR);

    // consider ...
    // del_timer_sync(&musb->dev_timer);
}
void musb_disable_interrupts_V(struct usb_device_data *data){
    /* disable interrupts */
    iowrite8(0, data->base_usb1core + MUSB_INTRUSBE);
    iowrite16(0, data->base_usb1core + MUSB_INTRTXE);
    iowrite16(0, data->base_usb1core + MUSB_INTRRXE);

    /*  flush pending interrupts */
    iowrite8(0xff, data->base_usb1core + MUSB_INTRUSB);
    iowrite16(0xffff, data->base_usb1core + MUSB_INTRTX);
    iowrite16(0xffff, data->base_usb1core + MUSB_INTRRX);
}
void fifo0_setup_V(struct usb_device_data *data){
    iowrite8(0, data->base_usb1core + MUSB_INDEX); // @1C0E

    // TX
    iowrite8(3, data->base_usb1core + MUSB_TXFIFOSZ); // sz = 3 -> fifo size = 2^(sz+3) = 64 bytes for TX FIFO0
    iowrite16(0x00, data->base_usb1core + MUSB_TXFIFOADD); 

    // RX
    iowrite8(3, data->base_usb1core + MUSB_RXFIFOSZ); // sz = 3 -> fifo size = 2^(sz+3) = 64 bytes for RX FIFO0
    iowrite16(0x00, data->base_usb1core + MUSB_RXFIFOADD); 
}
void dsps_musb_set_mode_V(struct usb_device_data *data){
    u32 usb1_ctrl = 0;

    usb1_ctrl = ioread32(data->base_usb1ctl + USB1CTL_MODE);
    usb1_ctrl &=~ (1u << 8); // A type
    usb1_ctrl |= (1u << 7); // iddig_mux
    iowrite32(usb1_ctrl, data->base_usb1ctl + USB1CTL_MODE);

    iowrite32(0x2, data->base_usb1ctl + USB1CTL_UTMI);
}
void restart_next_round(struct usb_device_data *data){
    u8 power = 0;
    
    /* MUSB_POWER_SOFTCONN might be already set, JZ4740 does this. */
    iowrite8(0, data->base_usb1core + MUSB_POWER);
    //msleep(20);
    power = ioread8(data->base_usb1core + MUSB_POWER);
    // release reset
    power &=~ MUSB_POWER_RESET;
    //power |= MUSB_POWER_SOFTCONN;
    power |= MUSB_POWER_ISOUPDATE | MUSB_POWER_HSENAB; // init high speed
    iowrite8(power, data->base_usb1core + MUSB_POWER);
}
void stop_reset(struct usb_device_data *data){
    u32 power = 0;

    power = ioread32(data->base_usb1core + MUSB_POWER);
    // release reset
    power &=~ MUSB_POWER_RESET;
    iowrite8(power, data->base_usb1core + MUSB_POWER);
}
void reset(struct usb_device_data *data){
    u32 power = 0;

    power = ioread32(data->base_usb1core + MUSB_POWER);
    // release reset
    power |= MUSB_POWER_RESET;
    iowrite8(power, data->base_usb1core + MUSB_POWER);
}
void musb_start_V(struct usb_device_data *data){
    // tell the USB core which events matter.
    iowrite16(0xFFFF, data->base_usb1core + MUSB_INTRTXE); // enable TX ep0 and 15 eps
    iowrite16(0xFFFE, data->base_usb1core + MUSB_INTRRXE); // enable RX 15 eps
    iowrite8(0xF7, data->base_usb1core + MUSB_INTRUSBE); // 

    iowrite32(0, data->base_usb1core + MUSB_TESTMODE); // no test mode

    // init high speed
    // iowrite8(MUSB_POWER_ISOUPDATE | MUSB_POWER_HSENAB, data->base_usb1core + MUSB_POWER);
    iowrite8(MUSB_POWER_ISOUPDATE, data->base_usb1core + MUSB_POWER);

    // tell the SoC to forward the USB core IRQ to the CPU.
    iowrite32(0xFFFEFFFF, data->base_usb1ctl + USB1CTL_IRQEN0);
    iowrite32(0xFFFE03F7, data->base_usb1ctl + USB1CTL_IRQEN1);

    // go into session
    iowrite8(MUSB_DEVCTL_SESSION, data->base_usb1core + MUSB_DEVCTL); // When the USB controller go into session, it will assume the role of a host
}
void musb_ep_program_V(struct usb_device_data *data){
    u16 int_tx = 0, csr;
    u8 i = 0;

    /* disable interrupt in case we flush */
    int_tx = ioread16(data->base_usb1core + MUSB_INTRTXE);
    int_tx &=~ (1u << 0);
    iowrite16(int_tx, data->base_usb1core + MUSB_INTRTXE); // no test mode

    for (i = 0; i < 5; i++){
        csr = ioread16(data->base_usb1core + 0x10 + MUSB_TXCSR);

        iowrite16(MUSB_CSR0_FLUSHFIFO, data->base_usb1core + 0x10 + MUSB_TXCSR);

        csr = ioread16(data->base_usb1core + 0x10 + MUSB_TXCSR);

        udelay(10);
    }
    /* and reset for the next transfer */
    iowrite16(0, data->base_usb1core + 0x10 + MUSB_TXCSR);

    /* target addr and (for multipoint) hub addr/port */
    // ...
    iowrite32(0x1000680, data->base_usb1core + 0x20);
    iowrite32(0x00120000, data->base_usb1core + 0x20);
    //iowrite32(0x1200, data->base_usb1core + 0x20);

    /* re-enable interrupt */
    int_tx = ioread16(data->base_usb1core + MUSB_INTRTXE);
    int_tx |= (1u << 0);
    iowrite16(int_tx, data->base_usb1core + MUSB_INTRTXE); // no test mode
}

// 1st
void dsps_musb_init_V(struct usb_device_data *data){
    u32 usb1_ctrl = 0;
    u8 musb1_babble = 0;

    // return 0 if not clocked
    usb1_ctrl = ioread32(data->base_usb1ctl + USB1CTL_REV);
    if (!usb1_ctrl){
        printk("REV = 0 -> error\n");
        return;
    }

    iowrite32(1 << 0, data->base_usb1ctl + USB1CTL_CTRL); // soft reset

    /* reset the otgdisable bit, needed for host mode to work */
	usb1_ctrl = ioread32(data->base_usb1ctl + USB1CTL_UTMI);
	usb1_ctrl &= ~(1 << 21); // wrp->otg_disable
	iowrite32(usb1_ctrl, data->base_usb1ctl + USB1CTL_UTMI);

    // MUSB_BABBLE_CTL
    musb1_babble = ioread8(data->base_usb1core + MUSB_BABBLE_CTL);
    if (musb1_babble&MUSB_BABBLE_RCV_DISABLE){
        musb1_babble |= MUSB_BABBLE_SW_SESSION_CTRL;
        iowrite8(musb1_babble, data->base_usb1core + MUSB_BABBLE_CTL);
    }

    //msleep(2000); // dsps_mod_timer(glue, -1);
}
// 2nd
void am335x_phy_power_V(struct usb_device_data *data){
    u32 usb1_ctrl = 0;

    // usb1 ctrl
    usb1_ctrl = ioread32(data->base_con_usb1ctrl1 + USB_CTRL1);
    usb1_ctrl &= ~(USBPHY_CM_PWRDN | USBPHY_OTG_PWRDN | USBPHY_OTGVDET_EN); // power: normal mode, no Vbus detect as host mode
    usb1_ctrl |= USBPHY_OTGSESSEND_EN;

    iowrite32(usb1_ctrl, data->base_con_usb1ctrl1 + USB_CTRL1);

    msleep(1); // Give the PHY ~1ms to complete the power up operation.
}
// 3rd
void dsps_musb_init_start_V(struct usb_device_data *data){
    u8 power = 0;

    // tell the SoC to forward the USB core IRQ to the CPU.
    iowrite32(0xFFFEFFFF, data->base_usb1ctl + USB1CTL_IRQEN0);
    iowrite32(0x01F7, data->base_usb1ctl + USB1CTL_IRQEN1);

    // tell the USB core which events matter.
    iowrite16(0xFFFF, data->base_usb1core + MUSB_INTRTXE); // enable TX ep0 and 15 eps
    iowrite16(0xFFFE, data->base_usb1core + MUSB_INTRRXE); // enable RX 15 eps
    iowrite8(0xF7, data->base_usb1core + MUSB_INTRUSBE); // 

    // // release reset
    // power &=~ MUSB_POWER_RESET;
    // power |= MUSB_POWER_SOFTCONN;
    // power |= MUSB_POWER_ISOUPDATE | MUSB_POWER_HSENAB; // init high speed
    // iowrite8(power, data->base_usb1core + MUSB_POWER);

    // go into session
    iowrite8(MUSB_DEVCTL_SESSION, data->base_usb1core + MUSB_DEVCTL); // When the USB controller go into session, it will assume the role of a host
    ret = wait_register_update(data, data->base_usb1core, MUSB_DEVCTL, 0, MUSB_DEVCTL_SESSION, 2000, "DEVCTL_SESSION"); // wait DEVCTL_SESSION is set to 1

    // release reset
    power &=~ MUSB_POWER_RESET;
    //power |= MUSB_POWER_SOFTCONN;
    power |= MUSB_POWER_ISOUPDATE | MUSB_POWER_HSENAB; // init high speed
    iowrite8(power, data->base_usb1core + MUSB_POWER);
}
// final
void musb_init_controller_V(struct usb_device_data *data){
    /* status = musb_platform_init(musb); */
    dsps_musb_init_V(data);

    /* status = usb_phy_init(musb->xceiv); */
    am335x_phy_power_V(data);

    /* be sure interrupts are disabled before connecting ISR */
    dsps_musb_disable_V(data);
    musb_disable_interrupts_V(data);
    iowrite8(0, data->base_usb1core + MUSB_DEVCTL);

    /* MUSB_POWER_SOFTCONN might be already set, JZ4740 does this. */
    iowrite8(0, data->base_usb1core + MUSB_POWER);

    /* status = ep_config_from_table(musb); */
    fifo0_setup_V(data);

    /* status = musb_host_setup(musb, plat->power); */
    // .. consider to find code that enables int
    musb_start_V(data);

    /* status = musb_platform_set_mode(musb, MUSB_HOST); */
    dsps_musb_set_mode_V(data);


    //dsps_musb_init_start_V(data);
    // musb_start_V(data);
}

void musb_exit_V(struct usb_device_data *data){
    dsps_musb_disable_V(data);
    musb_disable_interrupts_V(data);

    iowrite8(0, data->base_usb1core + MUSB_DEVCTL);
}
/* ================== API for Control Transfer ===================== */
int USB1_SETUP_temp(struct usb_device_data *data){
    u16 host_csr0 = 0;
    int ret = 0;
    int i = 0;

    reset(data);
    msleep(10);
    stop_reset(data);
    // musb_ep_program_V(data);

    // set index region
    iowrite8(0, data->base_usb1core + MUSB_INDEX); // @1C0E

    // TX
    iowrite8(0x03, data->base_usb1core + MUSB_TXFIFOSZ); // sz = 3 -> fifo size = 2^(sz+3) = 64 bytes for TX FIFO0
    iowrite16(0x00, data->base_usb1core + MUSB_TXFIFOADD); 
    // RX
    iowrite8(0x03, data->base_usb1core + MUSB_RXFIFOSZ); // sz = 3 -> fifo size = 2^(sz+3) = 64 bytes for RX FIFO0
    iowrite16(0x00, data->base_usb1core + MUSB_RXFIFOADD); 

    // fifo type0 (8-bit)
    //iowrite16(0x0200, data->base_usb1core + 0x10 + MUSB_CSR0);
    iowrite8(0x80, data->base_usb1core + 0x10 + MUSB_TYPE0);

    // val[0] = 0x1000680, val[1] = 0x1200
    // iowrite32(0x1000680, data->base_usb1core + 0x20);
    // iowrite32(0x1200, data->base_usb1core + 0x20);
    musb_ep_program_V(data);



    // printk("0x%x 0x%x 0x%x 0x%x\n", ioread32(data->base_usb1core), ioread32(data->base_usb1core + 0x4), ioread32(data->base_usb1core + 0x8), ioread32(data->base_usb1core + 0xc));
    // printk("0x%x 0x%x 0x%x 0x%x\n", ioread32(data->base_usb1core + 0x10), ioread32(data->base_usb1core + 0x14), ioread32(data->base_usb1core + 0x18), ioread32(data->base_usb1core + 0x1c));
    // //printk("0x%x\n", ioread32(data->base_usb1core + 0x20));
    // printk("devctl -> 0x%x\n", ioread8(data->base_usb1core + 0x60));
    // printk("BABBLE -> 0x%x\n", ioread8(data->base_usb1core + 0x61));
    // printk("FIFOSZ -> 0x%x\n", ioread16(data->base_usb1core + 0x62));
    // printk("FIFOADDR -> 0x%x\n", ioread32(data->base_usb1core + 0x64));


        host_csr0 = ioread16(data->base_usb1core + 0x10 + MUSB_CSR0);
        host_csr0 = MUSB_CSR0_H_SETUPPKT | MUSB_CSR0_TXPKTRDY; // Set SETUPPKT and TXPKTRDY 
        iowrite16(host_csr0, data->base_usb1core + 0x10 + MUSB_CSR0);

    //msleep(100);

        //printk("0 = %x, 1 = %x, 3 = %x\n", ioread16(data->base_usb1core + MUSB_INTRTX), ioread16(data->base_usb1core + MUSB_INTRRX), ioread8(data->base_usb1core + MUSB_INTRUSB) );




    // wait for Endpoint 0 interrupt (after TX transfer completion - Token + Data0/1 packet)
    ret = wait_val_update(data, &Tx1_flag, 1, 2000, "SETUP: Token + Data0/1");
    if (ret < 0) return -1;
    Tx1_flag = 0;

    // ret = ioread16(data->base_usb1core + 0x10 + MUSB_CSR0)&0xFF;
    // printk("ret = 0x%x\n", ret);

    // Check error
    if (ret == MUSB_CSR0_H_RXSTALL) {
        dev_info(data->dev, "RXSTALL\n");
        return -1;
    }
    if (ret == MUSB_CSR0_H_ERROR) {
        dev_info(data->dev, "ERROR\n"); // send additional 2 times
        return -1;
    }
    if (ret == MUSB_CSR0_H_NAKTIMEOUT) {
        dev_info(data->dev, "NAK_TIMEOUT\n"); // .... consider later
        return -1;
    } 
    else{
        dev_info(data->dev, "ACKed!\n");
        //dev_info(data->dev, "data1 = 0x%x\n", ioread32(data->base_usb1core + 0x20));
        //dev_info(data->dev, "data2 = 0x%x\n", ioread32(data->base_usb1core + 0x20));
        data->InsReq.leftLength = data->InsReq.wLength;
    }
    return 0;
}


int USB1_SETUP_Transaction_GetDesc(struct usb_device_data *data){
    u16 host_csr0 = 0;
    int ret = 0;
    int i = 0;

    reset(data);
    msleep(5);
    stop_reset(data);

#if (INDEX_USED)
    setIndex(data, 0);
#endif
    setFifo(data);

    musb_ep_program_V(data);

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

    host_csr0 = ioread16(data->base_usb1core + 0x10 + MUSB_CSR0);
    host_csr0 |= MUSB_CSR0_H_SETUPPKT | MUSB_CSR0_TXPKTRDY; // Set SETUPPKT and TXPKTRDY 
    iowrite16(host_csr0, data->base_usb1core + 0x10 + MUSB_CSR0);

    printk("0 = %x, 1 = %x, 3 = %x\n", ioread16(data->base_usb1core + MUSB_INTRTX), ioread16(data->base_usb1core + MUSB_INTRRX), ioread8(data->base_usb1core + MUSB_INTRUSB) );

    // wait for Endpoint 0 interrupt (after TX transfer completion - Token + Data0/1 packet)
    ret = wait_val_update(data, &Tx1_flag, 1, 2000, "SETUP: Token + Data0/1");
    if (ret < 0) return -1;
    Tx1_flag = 0;



    // // wait for Endpoint 0 interrupt (after RX transfer completion - Handshake packet)
    // wait_val_update(data, &Rx1_flag, 1, 2000, "SETUP: Handshake");
    // Rx1_flag = 0;

    ret = ioread16(data->base_usb1core + 0x10 + MUSB_CSR0)&0xFF;
    printk("ret = 0x%x\n", ret);
    i++;

    // Check error
    if (ret == MUSB_CSR0_H_RXSTALL) {
        dev_info(data->dev, "RXSTALL\n");
        return -1;
    }
    if (ret == MUSB_CSR0_H_ERROR) {
        dev_info(data->dev, "ERROR\n"); // send additional 2 times
        return -1;
    }
    if (ret == MUSB_CSR0_H_NAKTIMEOUT) {
        dev_info(data->dev, "NAK_TIMEOUT\n"); // .... consider later
        return -1;
    } 
    else{
        dev_info(data->dev, "ACKed!\n");
        // dev_info(data->dev, "data1 = 0x%x\n", ioread32(data->base_usb1core + 0x20));
        // dev_info(data->dev, "data2 = 0x%x\n", ioread32(data->base_usb1core + 0x20));
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

    iowrite16(MUSB_CSR0_H_REQPKT, data->base_usb1core + 0x10 + MUSB_CSR0);

    // wait for Endpoint 0 interrupt (Data packet)
    wait_val_update(data, &Tx1_flag, 1, 2000, "IN: Data0/1 - A");
    Tx1_flag = 0;
    ret = ioread16(data->base_usb1core + 0x10 + MUSB_CSR0)&0xFF;
    count = ioread16(data->base_usb1core + 0x10 + MUSB_COUNT0)&0xFFFF;
    printk("ret = 0x%x, count = 0x%x\n", ret, count);

    // Check error
    if (ret == MUSB_CSR0_H_RXSTALL) {
        dev_info(data->dev, "RXSTALL\n");
        return -1;
    }
    else if (ret == MUSB_CSR0_H_ERROR) {
        dev_info(data->dev, "ERROR\n"); // the controller has tried to send the required IN token three times without getting any response
        return -1;
    }
    else if (ret == MUSB_CSR0_H_NAKTIMEOUT) {
        dev_info(data->dev, "NAK_TIMEOUT\n"); // .... consider later
        return -1;
    } 
    else if (ret == MUSB_CSR0_RXPKTRDY) {
        //dev_info(data->dev, "RXPKTRDY - read FIFO\n"); 
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

        printk("d0 = 0x%x, d1 = 0x%x\n", tmp[0], tmp[1]);

        // clear RXPKTRDY
        host_csr0 = ioread32(data->base_usb1core + 0x10 + MUSB_CSR0);
        host_csr0 &=~ MUSB_CSR0_RXPKTRDY; 
        iowrite32(host_csr0, data->base_usb1core + 0x10 + MUSB_CSR0);
    } 
    
    iowrite16(MUSB_CSR0_H_REQPKT, data->base_usb1core + 0x10 + MUSB_CSR0);

    // wait for Endpoint 0 interrupt (Data packet)
    wait_val_update(data, &Tx1_flag, 1, 2000, "IN: Data0/1 - B");
    Tx1_flag = 0;
    ret = ioread16(data->base_usb1core + 0x10 + MUSB_CSR0)&0xFF;
    count = ioread16(data->base_usb1core + 0x10 + MUSB_COUNT0)&0xFFFF;
    printk("ret = 0x%x, count = 0x%x\n", ret, count);

    // Check error
    // Check error
    if (ret == MUSB_CSR0_H_RXSTALL) {
        dev_info(data->dev, "RXSTALL\n");
        return -1;
    }
    else if (ret == MUSB_CSR0_H_ERROR) {
        dev_info(data->dev, "ERROR\n"); // the controller has tried to send the required IN token three times without getting any response
        return -1;
    }
    else if (ret == MUSB_CSR0_H_NAKTIMEOUT) {
        dev_info(data->dev, "NAK_TIMEOUT\n"); // .... consider later
        return -1;
    } 
    else if (ret == MUSB_CSR0_RXPKTRDY) {
        //dev_info(data->dev, "RXPKTRDY - read FIFO\n"); 
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

        printk("d0 = 0x%x, d1 = 0x%x\n", tmp[0], tmp[1]);

        // clear RXPKTRDY
        host_csr0 = ioread32(data->base_usb1core + 0x10 + MUSB_CSR0);
        host_csr0 &=~ MUSB_CSR0_RXPKTRDY; 
        iowrite32(host_csr0, data->base_usb1core + 0x10 + MUSB_CSR0);
    } 
    

    iowrite16(MUSB_CSR0_H_REQPKT, data->base_usb1core + 0x10 + MUSB_CSR0);

    // wait for Endpoint 0 interrupt (Data packet)
    wait_val_update(data, &Tx1_flag, 1, 2000, "IN: Data0/1 - C");
    Tx1_flag = 0;
    ret = ioread16(data->base_usb1core + 0x10 + MUSB_CSR0)&0xFF;
    count = ioread16(data->base_usb1core + 0x10 + MUSB_COUNT0)&0xFFFF;
    printk("ret = 0x%x, count = 0x%x\n", ret, count);

    // Check error
    // Check error
    if (ret == MUSB_CSR0_H_RXSTALL) {
        dev_info(data->dev, "RXSTALL\n");
        return -1;
    }
    else if (ret == MUSB_CSR0_H_ERROR) {
        dev_info(data->dev, "ERROR\n"); // the controller has tried to send the required IN token three times without getting any response
        return -1;
    }
    else if (ret == MUSB_CSR0_H_NAKTIMEOUT) {
        dev_info(data->dev, "NAK_TIMEOUT\n"); // .... consider later
        return -1;
    } 
    else if (ret == MUSB_CSR0_RXPKTRDY) {
        //dev_info(data->dev, "RXPKTRDY - read FIFO\n"); 
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

        printk("d0 = 0x%x, d1 = 0x%x\n", tmp[0], tmp[1]);

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

    // // wait for Endpoint 0 interrupt (after RX transfer completion - Handshake packet)
    // wait_val_update(data, &Rx1_flag, 1, 2000, "STATUS: Handshake");
    // Rx1_flag = 0;

    // Check error
    if (ret == MUSB_CSR0_H_RXSTALL) {
        dev_info(data->dev, "RXSTALL\n");
        return -1;
    }
    else if (ret == MUSB_CSR0_H_ERROR) {
        dev_info(data->dev, "ERROR\n"); // the controller has tried to send the required IN token three times without getting any response
        return -1;
    }
    else if (ret == MUSB_CSR0_H_NAKTIMEOUT) {
        dev_info(data->dev, "NAK_TIMEOUT\n"); // .... consider later
        return -1;
    } 
    else if (ret == MUSB_CSR0_RXPKTRDY){
        dev_info(data->dev, "status acked!\n");
    }
    return 0;
}

int USB1_GetDesc_Transfer(struct usb_device_data *data){
    int ret;
    int i = 0;


    ret = USB1_SETUP_temp(data);

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




