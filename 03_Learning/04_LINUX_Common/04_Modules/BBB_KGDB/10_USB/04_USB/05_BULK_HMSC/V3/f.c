#include "f.h"

BPB bpb_instance;
Root_Dir_Entry rde_instance;

/* Global buffer for reading content of file / directory */
u8 Cluster_data[8][SECTOR_SIZE];

/* Global buffer for mounting */
u8 Glob_Sector_data[SECTOR_SIZE];

/* Global buffer for next cluster */
u8 Next_cluster_data[8][SECTOR_SIZE];

/* Global padding index */
u8 padding_index = 0;

int USB1_Scan_Cluster(struct usb_device_data *data, u8* cluster_data, u8 cluster_num, u8 padding_id, bool print_data);

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

void get_tree_padding(int padding_index, u8* buf, u8 type_file) {
    int i;

    if (padding_index < 0) {
        strcpy(buf, "");  // Invalid: empty
        //return buf;
        return;
    }

    // Clear buffer
    memset(buf, 0, 200);

    // Add (padding_index) spaces
    for (i = 0; i < padding_index*3; i++) {
        buf[i] = ' ';
    }

    // Append "|-"
    if (type_file == FILE_TYPE) strcpy(buf + i, "|----\0");
    else if (type_file == CONTENT_TYPE) strcpy(buf + i, "|-[.]\0");
    else strcpy(buf + i, "|-[ ]\0");
}

void USB1_Scan_ClusterData(struct usb_device_data *data, u8* cluster_data, u8 cluster_num, bool print_data){
    u32 cluster_len = (SECTOR_SIZE*8)/32;
    u32 i = 0, Cluster_data_len = 0;
    int ret = 0, LFN_ret = 0;
    u8* tmp_ptr = cluster_data;
    u8* tmp_Root_Dir_Entry;
    u8 tmp_file_name[50];
    u16 tmp_file_name_len = 0;
    u16 tmp_len = 0;

    Root_Dir_Entry* valid_Dir_Entry[100];
    u8 valid_Dir_Entry_index = 0;

    /* Local buffer for padding */
    u8 padding_buffer[200];

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
            //printk("i = %d\n", i);
            if (tmp_ptr[i*32 + 11] == FILE_TYPE) {
                valid_Dir_Entry[valid_Dir_Entry_index++] = (Root_Dir_Entry*)(cluster_data + i*32); // save ptr to valid root dir entry
                //printk("index FILE = %d\n", i*32);
            }
            else if ((tmp_ptr[i*32 + 11] == LFN_TYPE) && ((tmp_ptr[i*32 + 0])&SEQ_NUM) == 0x1) { // get the first first entry of LFN
                valid_Dir_Entry[valid_Dir_Entry_index++] = (Root_Dir_Entry*)(cluster_data + i*32); // save ptr to valid root dir entry

                //printk("index = %d\n", i);
            }
            else if (tmp_ptr[i*32 + 11] == DIR_TYPE) {
                valid_Dir_Entry[valid_Dir_Entry_index++] = (Root_Dir_Entry*)(cluster_data + i*32); // save ptr to valid root dir entry
            }
        }
    }

    if (print_data){
        for (i = 0; i < valid_Dir_Entry_index; i++){
            if (!valid_Dir_Entry[i]) {
                printk("valid_Dir_Entry is NULL");
                break;
            }
            else {
#if (PRINT_CONTENT)
                // content of file
                if (valid_Dir_Entry[i]->File_attributes == FILE_TYPE) { 
                    ret = USB1_Read_CLUSTER(data, (valid_Dir_Entry[i]->High_first_cluster << 16) | (valid_Dir_Entry[i]->Low_first_cluster), (u8*)Cluster_data, &Cluster_data_len, "Cluster next", 0);
                    if ((ret == 0)){
                        get_tree_padding(padding_index + 1, padding_buffer, CONTENT_TYPE); 
                        USB1_Print_String((u8*)Cluster_data, valid_Dir_Entry[i]->File_size, padding_buffer);
                    }       
                }
#endif
                // Name of file (SFN + LFN) / dir (LFN)
                if (valid_Dir_Entry[i]->File_attributes == LFN_TYPE) {
                    tmp_Root_Dir_Entry = (u8*)valid_Dir_Entry[i];
                    while(!((*tmp_Root_Dir_Entry)&END_MARKER)){
                        LFN_ret = USB1_Gather_LFN_String(tmp_Root_Dir_Entry + 1, 10, tmp_file_name + tmp_file_name_len, &tmp_len);
                        tmp_file_name_len += tmp_len;

                        if (LFN_ret == 0){
                            LFN_ret = USB1_Gather_LFN_String(tmp_Root_Dir_Entry + 14, 12, tmp_file_name + tmp_file_name_len, &tmp_len);
                            tmp_file_name_len += tmp_len;
                        }

                        if (LFN_ret == 0){
                            LFN_ret = USB1_Gather_LFN_String(tmp_Root_Dir_Entry + 28, 4, tmp_file_name + tmp_file_name_len, &tmp_len);
                            tmp_file_name_len += tmp_len;
                        }
        
                        tmp_Root_Dir_Entry-= 32; // reverse the previous entry
                    }

                    //printk("tmp_Root_Dir_Entry = 0x%x, 0x%x | 0x%x\n", *tmp_Root_Dir_Entry, tmp_Root_Dir_Entry[9], tmp_Root_Dir_Entry[10]);
                    if (*tmp_Root_Dir_Entry&END_MARKER){
                            //printk("hehe XXXXXXXXXXXXX\n");
                        LFN_ret = USB1_Gather_LFN_String(tmp_Root_Dir_Entry + 1, 10, tmp_file_name + tmp_file_name_len, &tmp_len);
                        tmp_file_name_len += tmp_len;
                        
                        if (LFN_ret == 0){
                            LFN_ret = USB1_Gather_LFN_String(tmp_Root_Dir_Entry + 14, 12, tmp_file_name + tmp_file_name_len, &tmp_len);
                            tmp_file_name_len += tmp_len;
                            //USB1_Print_String(tmp_Root_Dir_Entry + 14, 12, "File name 2s:");
                        }      
                        if (LFN_ret == 0){
                            LFN_ret = USB1_Gather_LFN_String(tmp_Root_Dir_Entry + 28, 4, tmp_file_name + tmp_file_name_len, &tmp_len);
                            tmp_file_name_len += tmp_len;
                            //USB1_Print_String(tmp_Root_Dir_Entry + 28, 4, "File name 3s:");
                        }   
                        
                        //printk("tmp_file_name_len = %d\n", tmp_file_name_len);
                        if (tmp_file_name[tmp_file_name_len - 4] == '.') {

                            get_tree_padding(padding_index, padding_buffer, FILE_TYPE); 

                            USB1_Print_String(tmp_file_name, tmp_file_name_len, padding_buffer);
                            //printk("AAAAAAAAAAAAAA\n");
                        }
                        else {
                            get_tree_padding(padding_index, padding_buffer, DIR_TYPE); 
                            USB1_Print_String(tmp_file_name, tmp_file_name_len, padding_buffer);
                            //printk("***********\n");
                        } 
                        tmp_file_name_len = 0;
                    }
                    
                }
            
                // Name of dir (SFN)
                else if (valid_Dir_Entry[i]->File_attributes == DIR_TYPE && valid_Dir_Entry[i]->id != '.'){
                    if (ret == 0){
                        u8 next_cluster = 0;
                        next_cluster = (valid_Dir_Entry[i]->High_first_cluster << 16) | (valid_Dir_Entry[i]->Low_first_cluster);

                        //printk("DIR TYPE, next_cluster = %d\n\n", next_cluster);

                        if ((valid_Dir_Entry[i]->File_name)[5] != '~') {
                            //printk("\n >>> ========= [Dir] ==========\n");
                            get_tree_padding(padding_index, padding_buffer, DIR_TYPE); 
                            USB1_Print_String((u8*)valid_Dir_Entry[i], 11, padding_buffer);
                        }
                        padding_index++;
                        ret = USB1_Scan_Cluster(data, (u8*)&Next_cluster_data, next_cluster, padding_index, 0);
                        padding_index--;

                        /* Reading dir entry again for current use to optimize stack size */
                        if (ret == 0){
                            ret = USB1_Read_CLUSTER(data, cluster_num, (u8*)Next_cluster_data, &Cluster_data_len, "Reading dir entry again", 0);
                        }
                    }
                }
            }
            
        }
    }
}

int USB1_Scan_Cluster(struct usb_device_data *data, u8* cluster_data, u8 cluster_num, u8 padding_id, bool print_data){
    int ret;
    u32 Cluster_data_len = 0;
    u32* ptr32;

    /* Local buffer for checking FAT */
    u8 FAT_Sector_data[SECTOR_SIZE];
    u16 FAT_Sector_data_len = 0;

    /* Read cluster num for root directory entry */
    ret = USB1_Read_CLUSTER(data, cluster_num, (u8*)cluster_data, &Cluster_data_len, "Reading dir entry", print_data);

    //printk(">>> === [cluster %d starts] === \n", cluster_num);
    // /* Scan root dir */
    if (ret == 0){
        USB1_Scan_ClusterData(data, (u8*)cluster_data, cluster_num, 1);
    }

    /* Check FAT */
    if (ret == 0){
        ret = USB1_Read_SECTOR_DATA(data, bpb_instance.Starting_LBA + bpb_instance.Reserved_Sectors, 1, FAT_Sector_data, &FAT_Sector_data_len, "Sector starting LBA", 0, 0);
        if (ret == 0){
            ptr32 = (u32*)FAT_Sector_data;

            /* Check if end of cluster */
            if (ptr32[cluster_num] == 0x0FFFFFFF){
                //printk(" === [cluster %d ends] === <<<\n", cluster_num);
            }
            else {
                printk("cluster %d is not end -> next: 0x%x\n", cluster_num, ptr32[cluster_num]);
            }
        }
    }

    return ret;
}

int USB1_f_Read(struct usb_device_data *data){
    int ret;
    u16 Sector_data_len;

    /* Read sector 0 */
    ret = USB1_Read_SECTOR_DATA(data, 0x0, 1, Glob_Sector_data, &Sector_data_len, "Sector 0", 0, 0);
    if (ret == 0){
        USB1_Print_HexVal(Glob_Sector_data + 0x1BE + 0x04, 1, "Partition type", LITTLE_ENDIAN);
        USB1_Print_HexVal(Glob_Sector_data + 0x1BE + 0x08, 4, "Starting LBA", LITTLE_ENDIAN);
        USB1_Print_HexVal(Glob_Sector_data + 0x1BE + 0x0C, 4, "Num of sectors in partition", LITTLE_ENDIAN);
        bpb_instance.Starting_LBA = USB1_Get_Bytes(Glob_Sector_data + 0x1BE + 0x08, 32, LITTLE_ENDIAN);
    }

    /* Read sector at the starting LBA */
    if (ret == 0){
        ret = USB1_Read_SECTOR_DATA(data, bpb_instance.Starting_LBA, 1, Glob_Sector_data, &Sector_data_len, "Sector starting LBA", 0, 0);
    }
    if (ret == 0){
        USB1_Print_HexVal(Glob_Sector_data + 13, 1, "Sectors per Cluster", LITTLE_ENDIAN);
        USB1_Print_HexVal(Glob_Sector_data + 14, 2, "Reserved Sectors", LITTLE_ENDIAN);
        bpb_instance.Sectors_per_Cluster = (u8)USB1_Get_Bytes(Glob_Sector_data + 13, 8, LITTLE_ENDIAN);
        bpb_instance.Reserved_Sectors = (u16)USB1_Get_Bytes(Glob_Sector_data + 14, 16, LITTLE_ENDIAN);

        USB1_Print_HexVal(Glob_Sector_data + 16, 2, "Number of FATs", LITTLE_ENDIAN);
        USB1_Print_HexVal(Glob_Sector_data + 36, 4, "Sectors per FAT table", LITTLE_ENDIAN);
        bpb_instance.Number_of_FATs = (u16)USB1_Get_Bytes(Glob_Sector_data + 16, 16, LITTLE_ENDIAN);
        bpb_instance.Sectors_per_FAT = USB1_Get_Bytes(Glob_Sector_data + 36, 32, LITTLE_ENDIAN);

        USB1_Print_HexVal(Glob_Sector_data + 44, 4, "Root Cluster", LITTLE_ENDIAN);
        USB1_Print_String(Glob_Sector_data + 82, 8, "FS type");
        bpb_instance.Root_Cluster = USB1_Get_Bytes(Glob_Sector_data + 44, 32, LITTLE_ENDIAN);

        bpb_instance.Data_Sector = bpb_instance.Reserved_Sectors + bpb_instance.Number_of_FATs * bpb_instance.Sectors_per_FAT;
        printk("Real data starts at sector: %d = 0x%x\n", bpb_instance.Data_Sector, bpb_instance.Data_Sector);
    }

    printk("================= [Files and Folders] ==================\n");
    /* Scan from Cluster 2 */
    if (ret == 0){
        ret = USB1_Scan_Cluster(data, (u8*)Next_cluster_data, 0x02, padding_index, 0);
    }

    /* Scan from Cluster 2 */
    if (ret == 0){
        ret = USB1_Scan_Cluster(data, (u8*)Next_cluster_data, 19, padding_index, 1);
    }

    return ret;
}
