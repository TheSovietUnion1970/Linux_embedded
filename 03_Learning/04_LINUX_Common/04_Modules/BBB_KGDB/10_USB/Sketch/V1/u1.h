#include <linux/module.h>
#include <linux/fs.h> // alloc_chrdev_region
#include <linux/pci.h> // ioremap
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/clk.h>

/* ============= CONTROL MODULE ===================== */
#define CONTROL_MODULE 0x44e10000
#define USB_CTRL1 0x628

#define USBPHY_CM_PWRDN		(1 << 0)
#define USBPHY_OTG_PWRDN	(1 << 1)
#define USBPHY_CHGDET_DIS	(1 << 2)
#define USBPHY_CHGDET_RSTRT	(1 << 3)
#define USBPHY_SRCONDM		(1 << 4)
#define USBPHY_SINKONDP		(1 << 5)
#define USBPHY_CHGISINK_EN	(1 << 6)
#define USBPHY_CHGVSRC_EN	(1 << 7)
#define USBPHY_DMPULLUP		(1 << 8)
#define USBPHY_DPPULLUP		(1 << 9)
#define USBPHY_CDET_EXTCTL	(1 << 10)
#define USBPHY_GPIO_MODE	(1 << 12)
#define USBPHY_DPOPBUFCTL	(1 << 13)
#define USBPHY_DMOPBUFCTL	(1 << 14)
#define USBPHY_DPINPUT		(1 << 15)
#define USBPHY_DMINPUT		(1 << 16)
#define USBPHY_DPGPIO_PD	(1 << 17)
#define USBPHY_DMGPIO_PD	(1 << 18)
#define USBPHY_OTGVDET_EN	(1 << 19)
#define USBPHY_OTGSESSEND_EN	(1 << 20)
#define USBPHY_DATA_POLARITY	(1 << 23)

/* ============= USBSS ===================== */
#define BASE_USBSS 0x47400000
#define USBSS_IRQSTAT 0x28

/* USB1 CTL */
#define BASE_USB1CTL 0x47401800
#define USB1CTL_IRQENSET0 0x1838
#define USB1CTL_MODE 0x18e3
/* USB1 PHY */
#define BASE_USB1PHY 0x47401b00

/* ============= USB1 CORE ================= */
#define BASE_USB1CORE 0x47401c00

/* POWER */
#define MUSB_POWER_ISOUPDATE	0x80
#define MUSB_POWER_SOFTCONN	0x40
#define MUSB_POWER_HSENAB	0x20
#define MUSB_POWER_HSMODE	0x10
#define MUSB_POWER_RESET	0x08
#define MUSB_POWER_RESUME	0x04
#define MUSB_POWER_SUSPENDM	0x02
#define MUSB_POWER_ENSUSPEND	0x01

/* DEVCTL */
#define MUSB_DEVCTL_BDEVICE	0x80
#define MUSB_DEVCTL_FSDEV	0x40
#define MUSB_DEVCTL_LSDEV	0x20
#define MUSB_DEVCTL_VBUS	0x18
#define MUSB_DEVCTL_VBUS_SHIFT	3
#define MUSB_DEVCTL_HM		0x04
#define MUSB_DEVCTL_HR		0x02
#define MUSB_DEVCTL_SESSION	0x01

/*
 * Common USB registers
 */

#define MUSB_FADDR		0x00	/* 8-bit */
#define MUSB_POWER		0x01	/* 8-bit */

#define MUSB_INTRTX		0x02	/* 16-bit */
#define MUSB_INTRRX		0x04
#define MUSB_INTRTXE		0x06
#define MUSB_INTRRXE		0x08
#define MUSB_INTRUSB		0x0A	/* 8 bit */
#define MUSB_INTRUSBE		0x0B	/* 8 bit */
#define MUSB_FRAME		0x0C
#define MUSB_INDEX		0x0E	/* 8 bit */
#define MUSB_TESTMODE		0x0F	/* 8 bit */

#define MUSB_DEVCTL		0x60	/* 8 bit */


/* ============ ENDPOINT registers =============== */
#define USB1EP0_base    0x47401D00

/* Offsets to endpoint registers */
#define MUSB_TXMAXP		0x00
#define MUSB_TXCSR		0x02
#define MUSB_CSR0		MUSB_TXCSR	/* Re-used for EP0 */
#define MUSB_RXMAXP		0x04
#define MUSB_RXCSR		0x06
#define MUSB_RXCOUNT		0x08
#define MUSB_COUNT0		MUSB_RXCOUNT	/* Re-used for EP0 */
#define MUSB_TXTYPE		0x0A
#define MUSB_TYPE0		MUSB_TXTYPE	/* Re-used for EP0 */
#define MUSB_TXINTERVAL		0x0B
#define MUSB_NAKLIMIT0		MUSB_TXINTERVAL	/* Re-used for EP0 */
#define MUSB_RXTYPE		0x0C
#define MUSB_RXINTERVAL		0x0D
#define MUSB_FIFOSIZE		0x0F
#define MUSB_CONFIGDATA		MUSB_FIFOSIZE	/* Re-used for EP0 */

/* CSR0 */
#define MUSB_CSR0_FLUSHFIFO	0x0100
#define MUSB_CSR0_TXPKTRDY	0x0002
#define MUSB_CSR0_RXPKTRDY	0x0001
/* CSR0 in Host mode */
#define MUSB_CSR0_H_DIS_PING		0x0800
#define MUSB_CSR0_H_WR_DATATOGGLE	0x0400	/* Set to allow setting: */
#define MUSB_CSR0_H_DATATOGGLE		0x0200	/* Data toggle control */
#define MUSB_CSR0_H_NAKTIMEOUT		0x0080
#define MUSB_CSR0_H_STATUSPKT		0x0040
#define MUSB_CSR0_H_REQPKT		0x0020
#define MUSB_CSR0_H_ERROR		0x0010
#define MUSB_CSR0_H_SETUPPKT		0x0008
#define MUSB_CSR0_H_RXSTALL		0x0004

/* CONFIGDATA */
#define MUSB_CONFIGDATA_MPRXE		0x80	/* Auto bulk pkt combining */
#define MUSB_CONFIGDATA_MPTXE		0x40	/* Auto bulk pkt splitting */
#define MUSB_CONFIGDATA_BIGENDIAN	0x20
#define MUSB_CONFIGDATA_HBRXE		0x10	/* HB-ISO for RX */
#define MUSB_CONFIGDATA_HBTXE		0x08	/* HB-ISO for TX */
#define MUSB_CONFIGDATA_DYNFIFO		0x04	/* Dynamic FIFO sizing */
#define MUSB_CONFIGDATA_SOFTCONE	0x02	/* SoftConnect */
#define MUSB_CONFIGDATA_UTMIDW		0x01	/* Data width 0/1 => 8/16bits */

/* =============== TOKEN ============ */
#define USB_REQ_GET_STATUS		0x00
#define USB_REQ_CLEAR_FEATURE		0x01
#define USB_REQ_SET_FEATURE		0x03
#define USB_REQ_SET_ADDRESS		0x05
#define USB_REQ_GET_DESCRIPTOR		0x06
#define USB_REQ_SET_DESCRIPTOR		0x07
#define USB_REQ_GET_CONFIGURATION	0x08
#define USB_REQ_SET_CONFIGURATION	0x09
#define USB_REQ_GET_INTERFACE		0x0A
#define USB_REQ_SET_INTERFACE		0x0B
#define USB_REQ_SYNCH_FRAME		0x0C
#define USB_REQ_SET_SEL			0x30
#define USB_REQ_SET_ISOCH_DELAY		0x31

/* ================== enum variables ================= */
typedef enum ret_usb {
    NONE,
    ACK,
    RXSTALL,
    ERROR,
    NAK_TIMEOUT,
    RXPKTRDY 
} usb_t;

/* ================ Main structure ============== */
struct usb_devRequest {
	u8 bRequestType;
	u8 bRequest;
	u16 wValue;
	u16 wIndex;
	u16 wLength;

    u16 maxLengthEntryFIFO;
    u16 actualLength;
    u16 leftLength;
};

struct usb_device_data {

    dev_t dev_num;
    struct cdev cdev;
    struct class *class;
    struct device *dev;

    void __iomem *base_usbss;  
    void __iomem *base_usb1ctl;
    void __iomem *base_usb1phy;
    void __iomem *base_usb1core;
    void __iomem *base_usb1ep0;
    void __iomem *base_con_usb1ctrl1;

    struct clk *clk;
    struct usb_devRequest InsReq;

    u8 TX[256];
    u16 TX_len;
    u8 RX[256];
    u16 RX_len;
};

/* ================== Utils ===================== */
u32 fifo_offset(u8 epnum);
int wait_register_update(struct usb_device_data *data, void __iomem *mem, u16 offset, u16 bit_offset, u8 bit_val, u16 delay_ms, u8* name_register);
int wait_val_update(struct usb_device_data *data, u16 var, u16 val, u16 delay_ms, u8* name_val);
void USB1_SetToken(struct usb_device_data *data, const u8* TokenSet);
void USB1_ClrToken(struct usb_device_data *data);
void USB1_ApplyToken(struct usb_device_data *data, u8 epnum);

/* ================ Init funcs ============= */
void USB1_init(struct usb_device_data *data);
void PHY1_init(struct usb_device_data *data);

/* ================== API for Control Transfer ===================== */
int USB1_SETUP_Transaction_GetDesc(struct usb_device_data *data);
int USB1_IN_Transaction_GetDesc(struct usb_device_data *data, u8* buffer, u16* outlen);
int USB1_STATUS_Transaction_GetDesc(struct usb_device_data *data);
int USB1_GetDesc_Transfer(struct usb_device_data *data); /* main */

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

