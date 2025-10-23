#include "u1.h"
#include "ms.h"
#include <linux/math64.h>

cbw_EAA cbw_EAA_Instance;


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

// const u8 cbw_mode_sense_EAA[31] = {
//     0x55, 0x53, 0x42, 0x43, // dCBWSignature: 0x43425355 ("USBC" in little-endian)
//     0x04, 0x00, 0x00, 0x00, // dCBWTag: 0x00000004 (arbitrary, incremented as needed)
//     0x18, 0x00, 0x00, 0x00, // dCBWDataTransferLength: 0x00000018 (24 bytes Data-In)
//     0x80,                   // bmCBWFlags: 0x80 (Data-In direction)
//     0x00,                   // bCBWLUN: 0x00 (LUN 0)
//     0x06,                   // bCBWCBLength: 0x06 (6-byte SCSI command)
//     0x1A, 0x08, 0x1D, 0x00, // CBWCB: Opcode 0x1A, DBD=1 (0x08), Page Code 0x1D, Subpage 0x00
//     0x18, 0x00,             // CBWCB: Allocation Length 0x18 (24 bytes), Control 0x00
//     0x00, 0x00, 0x00, 0x00, // Padding zeros (ignored since length=6)
//     0x00, 0x00, 0x00, 0x00, // Padding zeros
//     0x00, 0x00              // Padding zeros (total CBWCB=16 bytes)
// };

const u8 cbw_mode_sense_EAA[31] = {
    0x55, 0x53, 0x42, 0x43, // dCBWSignature: 0x43425355 ("USBC" in little-endian)
    0x04, 0x00, 0x00, 0x00, // dCBWTag: 0x00000004 (arbitrary, incremented as needed)
    0x18, 0x00, 0x00, 0x00, // dCBWDataTransferLength: 0x00000018 (24 bytes Data-In)
    0x80,                   // bmCBWFlags: 0x80 (Data-In direction)
    0x00,                   // bCBWLUN: 0x00 (LUN 0)
    0x06,                   // bCBWCBLength: 0x06 (6-byte SCSI command)
    0x1A, 0x08, 0x3F, 0x00, // CBWCB: Opcode 0x1A, DBD=1 (0x08), Page Code 0x1D, Subpage 0x00
    0x18, 0x00,             // CBWCB: Allocation Length 0x18 (24 bytes), Control 0x00
    0x00, 0x00, 0x00, 0x00, // Padding zeros (ignored since length=6)
    0x00, 0x00, 0x00, 0x00, // Padding zeros
    0x00, 0x00              // Padding zeros (total CBWCB=16 bytes)
};

u32 swap_endian32(u32 val) {
    return ((val >> 24) & 0x000000FF) |
           ((val >> 8)  & 0x0000FF00) |
           ((val << 8)  & 0x00FF0000) |
           ((val << 24) & 0xFF000000);
}


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

void USB1_Print_hex_data(const uint8_t *data, size_t len, u8* name) {
    size_t i = 0;
    printk("%s[%d] =: ", name, len);
    for (i = 0; i < len; i++) {
        printk("%02X ", data[i]);  // Print each byte in 2-digit hex
        if ((i + 1) % 10 == 0)     // After 10 bytes, print newline
            printk("\n");
    }
    if (len % 10 != 0)
        printk("\n"); // Final newline if not exactly multiple of 10
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

void USB1_Print_SCSI_Inquiry(struct usb_device_data *data){
    u64 tmp[4];
    printk("# ----------------------------------- #\n");
    printk("# additional_length = 0x%x\n", data->scsi_inquiry.additional_length);
    USB1_Print_String(data->scsi_inquiry.vendor_id, 8, "vendor_id");
    USB1_Print_String(data->scsi_inquiry.product_id, 16, "product_id");
    USB1_Print_String(data->scsi_inquiry.product_revision, 4, "product_revision");

    data->scsi_inquiry.LBA = swap_endian32(data->scsi_inquiry.LBA);
    data->scsi_inquiry.Capacity = swap_endian32(data->scsi_inquiry.Capacity);
    printk("# Logical block address(LBA) = 0x%x\n", data->scsi_inquiry.LBA);
    printk("# block size = 0x%x\n", data->scsi_inquiry.Capacity);

    tmp[0] = data->scsi_inquiry.LBA;
    tmp[1] = data->scsi_inquiry.Capacity;
    tmp[2] = tmp[0]*tmp[1];
    tmp[3] = div_u64(tmp[2], 1000000000ULL);
    printk("# => Total = %lld bytes | ~ %lld GB\n", tmp[2], tmp[3]);
    printk("# ----------------------------------- #\n");
}

void USB1_Print_EAA_Instance(struct usb_device_data *data, u8* name){
    printk("# ----------------------------------- #\n");
    printk("# %s: \n", name);
    printk("# Mode_data_len = 0x%x\n", cbw_EAA_Instance.Mode_data_len);
    printk("# Reserved1 = 0x%x\n", cbw_EAA_Instance.Reserved1);
    printk("# Block_descriptor_len = 0x%x\n", cbw_EAA_Instance.Block_descriptor_len);

    if (cbw_EAA_Instance.Mode_data_len > 3){
        USB1_Print_hex_data(cbw_EAA_Instance.block_descriptors, 5, "block_descriptors");
        USB1_Print_hex_data(cbw_EAA_Instance.mode_pages, 61, "mode_pages");
    }
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
int USB1_Send_INQUIRY(struct usb_device_data *data, const u8* cbw, u8* data_inquiry, bool print_status, u8* name){
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
    //ret = USB1_IN_Phase_Bulk(data, 0x1, Global_Address, (u8*)&data->scsi_inquiry, &InDataLen);
    ret = USB1_IN_Phase_Bulk(data, 0x1, Global_Address, data_inquiry, &InDataLen);
    if (ret < 0) {
        printk("%s: DATA IN failed: %d (len=%d)\n", name, ret, InDataLen);
        return ret;
    }
    else {
       printk("InDataLen = %d\n", InDataLen);
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

    ret = USB1_Send_INQUIRY(data, cbw_initial, (u8*)&data->scsi_inquiry, 0, "CBW Initial");

    if (ret == 0){
        ret = USB1_Send_INQUIRY(data, cbw_capacity, (u8*)&data->scsi_inquiry + 0x24, 0, "CBW Capacity");
    }

    // cbw_EAA_Instance
    if (ret == 0){
        ret = USB1_Send_INQUIRY(data, cbw_mode_sense_EAA, (u8*)&cbw_EAA_Instance, 1, "cbw_EAA_Instance");
    }

    /* Print result */
    if (ret == 0){
        USB1_Print_SCSI_Inquiry(data);
        USB1_Print_EAA_Instance(data, "cbw_EAA_Instance");
    }

    return ret;
}
