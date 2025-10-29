#include "f.h"

BPB bpb_instance;
u8 Cluster_data[8][SECTOR_SIZE];
u8 Sector_data[SECTOR_SIZE];

int USB1_Read_CLUSTER(struct usb_device_data *data, u32 cluster_num, u8* cluster_data, u32* cluster_data_len, u8* name){
    int ret;
    u8 i = 0;
    u16 Sector_data_len;
    *cluster_data_len = 0;

    for (i = 0; i < 8; i++){
        ret = USB1_Read_SECTOR_DATA(data, 0x0, 1, (u8*)&Cluster_data[i][0], &Sector_data_len, "Sector N", 0, 0);
        if (ret < 0){
            printk("Fail at index: %d\n", i);
            return -1;
        }

        *cluster_data_len += SECTOR_SIZE;
    }

    return 0;
}

int USB1_Read(struct usb_device_data *data){
    int ret;
    u16 Sector_data_len;
    u32 Cluster_data_len;

    /* Read sector 0 */
    ret = USB1_Read_SECTOR_DATA(data, 0x0, 1, Sector_data, &Sector_data_len, "Sector 0", 0, 0);
    if (ret == 0){
        USB1_Print_HexVal(Sector_data + 0x1BE + 0x04, 1, "Partition type", LITTLE_ENDIAN);
        USB1_Print_HexVal(Sector_data + 0x1BE + 0x08, 4, "Starting LBA", LITTLE_ENDIAN);
        USB1_Print_HexVal(Sector_data + 0x1BE + 0x0C, 4, "Num of sectors in partition", LITTLE_ENDIAN);
        bpb_instance.Starting_LBA = USB1_Get_Bytes(Sector_data + 0x1BE + 0x08, 32, LITTLE_ENDIAN);
    }

    /* Read sector at the starting LBA */
    if (ret == 0){
        ret = USB1_Read_SECTOR_DATA(data, bpb_instance.Starting_LBA, 1, Sector_data, &Sector_data_len, "Sector starting LBA", 0, 0);
    }
    if (ret == 0){
        USB1_Print_HexVal(Sector_data + 13, 1, "Sectors per Cluster", LITTLE_ENDIAN);
        USB1_Print_HexVal(Sector_data + 14, 2, "Reserved Sectors", LITTLE_ENDIAN);
        bpb_instance.Sectors_per_Cluster = (u8)USB1_Get_Bytes(Sector_data + 13, 8, LITTLE_ENDIAN);
        bpb_instance.Reserved_Sectors = (u16)USB1_Get_Bytes(Sector_data + 14, 16, LITTLE_ENDIAN);

        USB1_Print_HexVal(Sector_data + 16, 2, "Number of FATs", LITTLE_ENDIAN);
        USB1_Print_HexVal(Sector_data + 36, 4, "Sectors per FAT table", LITTLE_ENDIAN);
        bpb_instance.Number_of_FATs = (u16)USB1_Get_Bytes(Sector_data + 16, 16, LITTLE_ENDIAN);
        bpb_instance.Sectors_per_FAT = USB1_Get_Bytes(Sector_data + 36, 32, LITTLE_ENDIAN);

        USB1_Print_HexVal(Sector_data + 44, 4, "Root Cluster", LITTLE_ENDIAN);
        USB1_Print_String(Sector_data + 82, 8, "FS type");
        bpb_instance.Root_Cluster = USB1_Get_Bytes(Sector_data + 44, 32, LITTLE_ENDIAN);

        bpb_instance.Data_Sector = bpb_instance.Reserved_Sectors + bpb_instance.Number_of_FATs * bpb_instance.Sectors_per_FAT;
        printk("Real data starts at sector: %d = 0x%x\n", bpb_instance.Data_Sector, bpb_instance.Data_Sector);
    }

    /* Read cluster 2 */
    if (ret == 0){
        ret = USB1_Read_CLUSTER(data, 0x02, (u8*)Cluster_data, &Cluster_data_len, "Cluster 0x2");
    }

    return ret;
}