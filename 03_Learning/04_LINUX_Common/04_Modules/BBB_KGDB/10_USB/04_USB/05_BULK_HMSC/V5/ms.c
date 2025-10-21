#include "u1.h"
#include "ms.h"


/* Print result */
void USB1_Print_String(u8 *data, u16 len, u8* string) {
    u16 i = 0;
    u8 tmp[len+1];

    if (data == NULL || len == 0)
        return;

    for (i = 0; i < len; i++) {
        tmp[i] = data[i];
    }
    tmp[len-1] = '\0'; // add null terminator

    printk("# %s: '%s'\n", string, tmp);
}

void USB1_Print_SCSI_Inquiry(struct usb_device_data *data){
    printk("# ----------------------------------- #\n");
    printk("# additional_length = 0x%x\n", data->scsi_inquiry.additional_length);
    USB1_Print_String(data->scsi_inquiry.vendor_id, 8, "vendor_id");
    USB1_Print_String(data->scsi_inquiry.product_id, 16, "product_id");
    USB1_Print_String(data->scsi_inquiry.product_revision, 4, "product_revision");
    printk("# ----------------------------------- #\n");
}

void USB1_Print_CSW(struct usb_device_data *data){
    printk("# ----------------------------------- #\n");
    printk("# dCSWSignature = 0x%x\n", data->usb1_csw.dCSWSignature);
    printk("# dCSWTag = 0x%x\n", data->usb1_csw.dCSWTag);
    printk("# dCSWDataResidue = 0x%x\n", data->usb1_csw.dCSWDataResidue);
    printk("# bCSWStatus = 0x%x\n", data->usb1_csw.bCSWStatus);
    printk("# ----------------------------------- #\n");
}

/* ================== API for HMSC bulk Transfer ===================== */
int USB1_Send_INQUIRY(struct usb_device_data *data){
    int ret;
    u16 InDataLen;

    data->tag = 0x1;

    // Prepare CBW
    data->usb1_cbw.dCBWSignature = 0x43425355;
    data->usb1_cbw.dCBWTag = data->tag; // Increment per command
    data->usb1_cbw.dCBWDataTransferLength = 0x24;  // Data-IN length
    data->usb1_cbw.bmCBWFlags = 0x80;  // IN direction
    data->usb1_cbw.bCBWLUN = 0;
    data->usb1_cbw.bCBWCBLength = 6;
    data->usb1_cbw.CBWCB[0] = 0x12;  // INQUIRY
    data->usb1_cbw.CBWCB[1] = 0x00;  // LUN=0 (already in bCBWLUN)
    data->usb1_cbw.CBWCB[2] = 0x00;  // Page code=0
    data->usb1_cbw.CBWCB[3] = 0x00;  // Reserved
    data->usb1_cbw.CBWCB[4] = 0x00;    // Alloc length=36
    data->usb1_cbw.CBWCB[5] = 0x24;  // Control=0
    data->tag++;

    // 1. Command: Bulk OUT CBW (31 bytes)
    ret = USB1_OUT_Phase_Bulk(data, 0x2, Global_Address, (u8*)&data->usb1_cbw, sizeof(data->usb1_cbw));
    if (ret < 0) {
        printk("CBW OUT failed: %d\n", ret);
        return ret;
    }

    // 2. Data: Bulk IN (36 bytes)
    ret = USB1_IN_Phase_Bulk(data, 0x1, Global_Address, (u8*)&data->scsi_inquiry, &InDataLen);
    if (ret < 0) {
        printk("Data IN failed: %d (len=%d)\n", ret, InDataLen);
        return ret;
    }
    else {
       USB1_Print_SCSI_Inquiry(data);
    }

    // 3. Data: Bulk IN CSW (16 bytes)
    ret = USB1_IN_Phase_Bulk(data, 0x1, Global_Address, (u8*)&data->usb1_csw, &InDataLen);
    if (ret < 0) {
        printk("Data IN failed: %d (len=%d)\n", ret, InDataLen);
        return ret;
    }
    else {
       USB1_Print_CSW(data);
    }


    return 0;
}

