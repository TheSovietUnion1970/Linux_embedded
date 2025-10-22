#include "u1.h"
#include "ms.h"

/* const CBW */
const u8 cbw_initial[31] = {
    0x55, 0x53, 0x42, 0x43,  // dCBWSignature: 0x43425355 (LE "USBC")
    0x01, 0x00, 0x00, 0x00,  // dCBWTag: 0x00000001
    0x24, 0x00, 0x00, 0x00,  // dCBWDataTransferLength: 0x00000024 (36 bytes, Data-In)
    0x80,                    // bmCBWFlags: 0x80 (IN direction)
    0x00,                    // bCBWLUN: 0x00
    0x06,                    // bCBWCBLength: 0x06 (6-byte SCSI cmd)
    0x12, 0x00, 0x00, 0x00, 0x24, 0x00,  // CBWCB: INQUIRY (0x12) + params (LUN=0x00, page=0x00, reserved=0x00, alloc len=0x0024)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Padding (10 zeros to reach 31 bytes)
    0x00, 0x00  // Final padding zeros
};

const u8 cbw_capacity[31] = {
    0x55, 0x53, 0x42, 0x43,  // dCBWSignature: 0x43425355 (LE "USBC")
    0x02, 0x00, 0x00, 0x00,  // dCBWTag: 0x00000002 (increment from previous)
    0x08, 0x00, 0x00, 0x00,  // dCBWDataTransferLength: 0x00000008 (8 bytes Data-In)
    0x80,                    // bmCBWFlags: 0x80 (Data-In direction)
    0x00,                    // bCBWLUN: 0x00 (LUN 0)
    0x0A,                    // bCBWCBLength: 0x0A (10-byte SCSI command)
    0x25, 0x00, 0x00, 0x00,  // CBWCB: Opcode 0x25 (READ CAPACITY (10))
    0x00, 0x00, 0x00, 0x00,  // CBWCB: Logical Block Address (0x00000000 for full capacity)
    0x00, 0x00,              // CBWCB: Reserved (0x00) and PMI (0x00, no partial media info)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Padding zeros to reach 31 bytes
};

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

void USB1_Print_CSW(struct usb_device_data *data, u8* name){
    printk("# ----------------------------------- #\n");
    printk("# %s: \n", name);
    printk("# dCSWSignature = 0x%x\n", data->usb1_csw.dCSWSignature);
    printk("# dCSWTag = 0x%x\n", data->usb1_csw.dCSWTag);
    printk("# dCSWDataResidue = 0x%x\n", data->usb1_csw.dCSWDataResidue);
    printk("# bCSWStatus = 0x%x\n", data->usb1_csw.bCSWStatus);
    printk("# ----------------------------------- #\n");
}

/* Utils */
void USB1_Clear_CBW(struct usb_device_data *data){
    u16 i = 0;
    u8* ptr = (u8*)&data->usb1_cbw;
    u16 size = sizeof(data->usb1_cbw);

    for (i = 0; i < size; i++){
        ptr[i] = 0;
    }
}
void USB1_Set_CBW(struct usb_device_data *data, const u8* d){
    u16 i = 0;
    u8* ptr = (u8*)&data->usb1_cbw;
    u16 size = sizeof(data->usb1_cbw);

    for (i = 0; i < size; i++){
        ptr[i] = d[i];
    }
}
void USB1_Apply_CBW(struct usb_device_data *data, const u8* d){
    USB1_Clear_CBW(data);
    USB1_Set_CBW(data, d);
}

/* ================== API for HMSC bulk Transfer ===================== */
int USB1_Send_INQUIRY(struct usb_device_data *data, const u8* cbw, bool print_status, u8* name){
    int ret;
    u16 InDataLen;

    // 1. Command: Bulk OUT CBW (31 bytes)
    USB1_Apply_CBW(data, cbw);
    ret = USB1_OUT_Phase_Bulk(data, 0x2, Global_Address, (u8*)&data->usb1_cbw, sizeof(data->usb1_cbw));
    if (ret < 0) {
        printk("%s: CBW OUT failed: %d\n", name, ret);
        return ret;
    }

    // 2. Data: Bulk IN (36 bytes)
    ret = USB1_IN_Phase_Bulk(data, 0x1, Global_Address, (u8*)&data->scsi_inquiry, &InDataLen);
    if (ret < 0) {
        printk("%s: DATA IN failed: %d (len=%d)\n", name, ret, InDataLen);
        return ret;
    }
    else {
       //USB1_Print_SCSI_Inquiry(data);
    }

    // 3. Data: Bulk IN CSW (16 bytes)
    ret = USB1_IN_Phase_Bulk(data, 0x1, Global_Address, (u8*)&data->usb1_csw, &InDataLen);
    if (ret < 0) {
        printk("%s: CSW IN failed: %d (len=%d)\n", name, ret, InDataLen);
        return ret;
    }
    else {
        if (print_status) USB1_Print_CSW(data, name);
    }


    return 0;
}

int USB1_CBW(struct usb_device_data *data){
    int ret;

    ret = USB1_Send_INQUIRY(data, cbw_initial, 1, "CBW Initial");

    if (ret == 0){
        ret = USB1_Send_INQUIRY(data, cbw_capacity, 1, "CBW Capacity");
    }

    return ret;
}