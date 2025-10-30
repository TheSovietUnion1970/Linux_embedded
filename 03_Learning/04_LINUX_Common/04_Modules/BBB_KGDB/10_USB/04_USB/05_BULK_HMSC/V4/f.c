#include "f.h"

BPB bpb_instance;
Root_Dir_Entry rde_instance;
Root_Dir_Entry* valid_Root_Dir_Entry[100];
u8 valid_Root_Dir_Entry_index = 0;
u8 Sector_data_Root_Dir_Entry[8][SECTOR_SIZE];

u8 Cluster_data[8][SECTOR_SIZE];
u8 Sector_data[SECTOR_SIZE];

int USB1_Read_CLUSTER(struct usb_device_data *data, u32 cluster_num, u8* cluster_data, u32* cluster_data_len, u8* name, bool print_data){
    int ret;
    u8 i = 0;
    u16 Sector_data_len;
    u32 sector_num;
    *cluster_data_len = 0;

    /* Caculate sector num from setor num */
    sector_num = bpb_instance.Starting_LBA + bpb_instance.Data_Sector + (cluster_num - 2)*bpb_instance.Sectors_per_Cluster;

    for (i = 0; i < 8; i++){
        ret = USB1_Read_SECTOR_DATA(data, sector_num + i, 1, (u8*)(cluster_data + i*SECTOR_SIZE), &Sector_data_len, "Sector N", 0, print_data);
        //printk("Addr: = 0x%x\n", (u8*)&Cluster_data[i][0]);
        if (ret < 0){
            printk("Fail at index: %d\n", i);
            return -1;
        }

        *cluster_data_len += SECTOR_SIZE;
    }

    return 0;
}

void SetMemf(u8* dst, u8* src, u16 size){
    u16 i = 0;
    for (i = 0; i < size; i++){
        dst[i] = src[i];
    }
}

void USB1_Scan_Root_Dir(struct usb_device_data *data, u8* cluster_data, bool print_data){
    u32 cluster_len = (SECTOR_SIZE*8)/32;
    u32 i = 0, Cluster_data_len = 0;
    int ret;
    u8* tmp_ptr = cluster_data;
    u8* tmp_Root_Dir_Entry;

    // make 2D into 1D
    for (i = 0; i < cluster_len; i++){
        if ((tmp_ptr[i*32] == 0xE5U)) // deleted or unallocated file
        {
            // do nothing as notthing should be shown
        }
        else if (tmp_ptr[i*32] == 0x00U){
            break; // as no entry anymore
        }
        else // the name of read data
        {
            if (tmp_ptr[i*32 + 11] == FILE_TYPE) {
                valid_Root_Dir_Entry[valid_Root_Dir_Entry_index++] = (Root_Dir_Entry*)(cluster_data + i*32); // save ptr to valid root dir entry
            }
            else if ((tmp_ptr[i*32 + 11] == LFN_TYPE) && ((tmp_ptr[i*32 + 0])&SEQ_NUM) == 0x1) { // get the first first entry of LFN
                valid_Root_Dir_Entry[valid_Root_Dir_Entry_index++] = (Root_Dir_Entry*)(cluster_data + i*32); // save ptr to valid root dir entry
            }

            // SetMemf((u8*)&rde_instance, tmp_ptr + i*32, 32);
            // USB1_Print_String(rde_instance.File_name, 11, "Root dir entry");
        }
    }


    if (print_data){
        for (i = 0; i < valid_Root_Dir_Entry_index; i++){
            if (!valid_Root_Dir_Entry[i]) {
                printk("valid_Root_Dir_Entry is NULL");
                break;
            }
            else {
                if (valid_Root_Dir_Entry[i]->File_attributes == FILE_TYPE) { // SFN
                    USB1_Print_String((u8*)valid_Root_Dir_Entry[i], 11, "File name:");
                    ret = USB1_Read_CLUSTER(data, (valid_Root_Dir_Entry[i]->High_first_cluster << 16) | (valid_Root_Dir_Entry[i]->Low_first_cluster), (u8*)Cluster_data, &Cluster_data_len, "Cluster next", 0);
                    if (ret == 0){
                        USB1_Print_String((u8*)Cluster_data, valid_Root_Dir_Entry[i]->File_size, "Content String");
                    }       
                }
                else if (valid_Root_Dir_Entry[i]->File_attributes == LFN_TYPE) { // LFN
                    tmp_Root_Dir_Entry = (u8*)valid_Root_Dir_Entry[i];
                    while(!((*tmp_Root_Dir_Entry)&END_MARKER)){
                        USB1_Print_String(tmp_Root_Dir_Entry, 10, "File name 1:");
                        USB1_Print_String(tmp_Root_Dir_Entry + 14, 12, "File name 2:");
                        USB1_Print_String(tmp_Root_Dir_Entry + 28, 4, "File name 3:");

                        tmp_Root_Dir_Entry-= 32; // reverse the previous entry
                    }
                    if (*tmp_Root_Dir_Entry&END_MARKER){
                        if ((tmp_Root_Dir_Entry[9] != 0xFF) && tmp_Root_Dir_Entry[10] != 0xFF){
                            USB1_Print_String(tmp_Root_Dir_Entry, 10, "File name 1:");
                        }
                        if ((tmp_Root_Dir_Entry[24] != 0xFF) && tmp_Root_Dir_Entry[25] != 0xFF){
                            USB1_Print_String(tmp_Root_Dir_Entry + 14, 12, "File name 2:");
                        }      
                        if ((tmp_Root_Dir_Entry[30] != 0xFF) && tmp_Root_Dir_Entry[31] != 0xFF){
                            USB1_Print_String(tmp_Root_Dir_Entry + 28, 4, "File name 3:");
                        }                    
                    }
                    
                }
            }
            
        }
    }
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

    /* Read cluster 2 for root directory entry */
    if (ret == 0){
        ret = USB1_Read_CLUSTER(data, 0x02, (u8*)Sector_data_Root_Dir_Entry, &Cluster_data_len, "Cluster 0x2", 1);
    }

    // /* Scan root dir */
    if (ret == 0){
        USB1_Scan_Root_Dir(data, (u8*)Sector_data_Root_Dir_Entry, 1);
    }

    return ret;
}
