#ifndef F_H
#define F_H

#include "u1.h"
#include "ms.h"


// Forward declaration for function parameters
struct usb_device_data;

/* BIOS Parameter Block (BPB) Details */
typedef struct BPB {  
    u16 Starting_LBA;
    u16 Bytes_per_Sector;
    u8 Sectors_per_Cluster;
    u16 Reserved_Sectors;
    u16 Number_of_FATs;
    u32 Sectors_per_FAT;
    u32 Root_Cluster;
    u32 Data_Sector;
} BPB;

int USB1_Read_CLUSTER(struct usb_device_data *data, u32 cluster_num, u8* cluster_data, u32* cluster_data_len, u8* name);

int USB1_Read(struct usb_device_data *data);


#endif /* F_H */