#ifndef MS_H
#define MS_H


// Forward declaration for function parameters
struct usb_device_data;

/* SCSI command */
typedef struct __attribute__((packed)) usb_msc_cbw {  // Command Block Wrapper
    u32 dCBWSignature;    // 0x43425355 ("USBC")
    u32 dCBWTag;          // Unique tag (increment per command)
    u32 dCBWDataTransferLength;  // 36 for INQUIRY
    u8 bmCBWFlags;        // 0x80 (IN direction)
    u8 bCBWLUN;           // 0 (LUN 0)
    u8 bCBWCBLength;      // 6 (SCSI cmd length)
    u8 CBWCB[16];         // SCSI INQUIRY: {0x12, 0x00, 0x00, 0x00, 36, 0, ...}
} msc_cbw;

typedef struct __attribute__((packed)) usb_msc_csw {  // Command Status Wrapper
    u32 dCSWSignature;    // 0x53425355 ("USBS")
    u32 dCSWTag;          // Matches CBW tag
    u32 dCSWDataResidue;  // Untransferred bytes (0 for success)
    u8 bCSWStatus;        // 0x00=Passed, 0x01=Failed, 0x02=Phase Error
} msc_csw;

/* SCSI data structure */
typedef struct __attribute__((packed)) scsi_inquiry_response {
    u8 peripheral_qualifier : 3;  // Bits 7-5: Qualifier (0=connected)
    u8 peripheral_device_type : 5;  // Bits 4-0: Type (0x00=direct-access block)
    u8 rmb : 1;  // Bit 7: Removable Media Bit (1=removable)
    u8 reserved1 : 7;  // Bits 6-0: Reserved
    u8 version;  // ANSI/ISO version (0x00=basic compliance)
    u8 response_data_format : 4;  // Bits 3-0: Format (0x01=SCSI-1 style)
    u8 reserved2 : 4;  // Bits 7-4: Reserved (includes AERC, Obsolete, NormACA, HiSup)
    u8 additional_length;  // Length of remaining data (0x1F=31 bytes, total 36)
    u8 sccs : 1;  // Bit 7: SCC Supported
    u8 addr16 : 1;  // Bit 6: 16-bit wide SCSI addresses
    u8 reserved3 : 6;  // Bits 5-0: Includes MChngr, MultiP, EncServ, etc.
    u8 reserved4;  // Byte 6: More flags (e.g., RelAdr, WBus32)
    u8 soft_reset : 1;  // Bit 7: Soft Reset support
    u8 cmdque : 1;  // Bit 6: Command Queuing
    u8 reserved5 : 6;  // Bits 5-0: Includes Linked, Sync
    char vendor_id[8];  // ASCII, space-padded: "Mass    "
    char product_id[16];  // ASCII, space-padded: "Storage Device  "
    char product_revision[4];  // ASCII: "1.00"
} inquiry_response;


/* ================== API for HMSC Bulk Transfer ===================== */ 
int USB1_Send_INQUIRY(struct usb_device_data *data);

#endif /* MS_H */
