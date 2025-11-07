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

/* Global array for holding dir name and dir cluster */
u8 names[MAX_DIRS][MAX_PATH_LEN]; // up to 10 directories
u32 Dir_cluster[MAX_DIRS];
u32 Dir_padding[MAX_DIRS];
u8 Dir_index = 0;

int num_direct[MAX_DIRS];
bool all_children_leaves[MAX_DIRS];
int max_subtree_padding[MAX_DIRS];
int parent[MAX_DIRS];
bool include_flags[MAX_DIRS];
int chain_indices[MAX_DIRS];

char path[MAX_FULL_PATH_LEN];
char stack[MAX_DIRS][MAX_PATH_LEN];
char full_paths[MAX_DIRS][MAX_FULL_PATH_LEN];
char prefix[MAX_PATH_LEN + 2];
char actual_dirs[MAX_DIRS][MAX_PATH_LEN];

int USB1_Scan_Cluster(struct usb_device_data *data, u8* cluster_data, u8 cluster_num, u8 padding_id, bool print_data, bool all_dir);

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

/* LFN_entry is a pointer to SFN dir entry */
void USB1_Read_NameFile(Root_Dir_Entry* LFN_entry, u8* Output_name, u16* Output_len){
    Root_Dir_Entry* entry_start;
    u16 tmp_len;
    int LFN_ret;

    *Output_len = 0;

    entry_start = LFN_entry;

    // =============== checking the LFN name
    entry_start -= 1;

    while(!((entry_start->id)&END_MARKER)){
        LFN_ret = USB1_Gather_LFN_String(((u8*)entry_start) + 1, 10, Output_name + *Output_len, &tmp_len);
        *Output_len += tmp_len;

        if (LFN_ret == 0){
            LFN_ret = USB1_Gather_LFN_String(((u8*)entry_start) + 14, 12, Output_name + *Output_len, &tmp_len);
            *Output_len += tmp_len;
        }

        if (LFN_ret == 0){
            LFN_ret = USB1_Gather_LFN_String(((u8*)entry_start) + 28, 4, Output_name + *Output_len, &tmp_len);
            *Output_len += tmp_len;
        }

        entry_start-= 1; // back the previous entry
    }

    if ((entry_start->id)&END_MARKER){
            //printk("hehe XXXXXXXXXXXXX\n");
        LFN_ret = USB1_Gather_LFN_String(((u8*)entry_start) + 1, 10, Output_name + *Output_len, &tmp_len);
        *Output_len += tmp_len;
        
        if (LFN_ret == 0){
            LFN_ret = USB1_Gather_LFN_String(((u8*)entry_start) + 14, 12, Output_name + *Output_len, &tmp_len);
            *Output_len += tmp_len;
            //USB1_Print_String(tmp_Root_Dir_Entry + 14, 12, "File name 2s:");
        }      
        if (LFN_ret == 0){
            LFN_ret = USB1_Gather_LFN_String(((u8*)entry_start) + 28, 4, Output_name + *Output_len, &tmp_len);
            *Output_len += tmp_len;
        }   
    }
}

void USB1_Scan_ClusterData(struct usb_device_data *data, u8* cluster_data, u8 cluster_num, bool print_data, bool all_dir){
    u32 cluster_len = (SECTOR_SIZE*8)/32;
    u32 i = 0, Cluster_data_len = 0;
    int ret = 0;
    u8* tmp_ptr = cluster_data;
    u8 tmp_file_name[50];
    u16 tmp_file_name_len = 0;

    u8* ptr8;
    bool hidden_folder = false;

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
                if ((valid_Dir_Entry[i]->File_attributes == LFN_TYPE) && (((valid_Dir_Entry[i]->id)&SEQ_NUM) == 0x01)) {
                    USB1_Read_NameFile(valid_Dir_Entry[i] + 1, tmp_file_name, &tmp_file_name_len);

                    // checking whether print file ('t'xt) or folder
                    if (tmp_file_name[tmp_file_name_len - 3] == 't') { // 't'xt

                        get_tree_padding(padding_index, padding_buffer, FILE_TYPE); 

                        USB1_Print_String(tmp_file_name, tmp_file_name_len, padding_buffer);
                    }
                    else {
#if (HIDDEN_FOLDERS)
                        get_tree_padding(padding_index, padding_buffer, DIR_TYPE); 
                        USB1_Print_String(tmp_file_name, tmp_file_name_len, padding_buffer);

                        /* Save Dir name into buffer */
                        USB1_Get_String(tmp_file_name, names[Dir_index], tmp_file_name_len);
                        Dir_padding[Dir_index] = padding_index;
#else
                        /* Checking hidden folders */
                        // as Hidden LFN Dir always has '.' at the start
                        if (tmp_file_name[0] != '.'){
                            get_tree_padding(padding_index, padding_buffer, DIR_TYPE); 
                            USB1_Print_String(tmp_file_name, tmp_file_name_len, padding_buffer);

                            /* Save Dir name into buffer */
                            // (valid_Dir_Entry[i]->High_first_cluster << 16) | (valid_Dir_Entry[i]->Low_first_cluster)
                            USB1_Get_String(tmp_file_name, names[Dir_index], tmp_file_name_len);
                            Dir_padding[Dir_index] = padding_index;
                        }
#endif
                    } 
                }
            
                // Name of dir (SFN) only else the name will be printed in the above if
                // valid_Dir_Entry[i]->id != '.' to prevent cluster N 0x2E at index 0 and 1
                else if (valid_Dir_Entry[i]->File_attributes == DIR_TYPE && valid_Dir_Entry[i]->id != '.'){
                    if (ret == 0){
                        u8 next_cluster = 0;
                        next_cluster = (valid_Dir_Entry[i]->High_first_cluster << 16) | (valid_Dir_Entry[i]->Low_first_cluster);

#if (!HIDDEN_FOLDERS)
                        /* DIR SFN only (except Hidden DIR) when dir entry is located at the third index of cluster (self + parent index), 
                           DIR SFN + LFN when dir entry is not located at the third index of cluster,
                           When not in root cluster (0x2), cluster starts with 2 dir entries (self + parent) */
                        ptr8 = (u8*)valid_Dir_Entry[i];
                        ptr8-=32;
                        /* Back the previous dir entry to check hidden folder ('.')
                        and make sure dir entry is not located at the first index of cluster (i != 0) */
                        if ((((ptr8[0])&SEQ_NUM) == 0x01) && (ptr8[1] == '.') && (i != 0)){
                            //printk("HIDDEN FILE\n");
                            hidden_folder = true;
                        }
#endif
                        // avoid printing SFN Dir as LFN Dir will be printed or only SFN Dir is printed
                        ptr8 = (u8*)valid_Dir_Entry[i];
                        ptr8-=32; // back to the previous entry to check LFN

                        get_tree_padding(padding_index, padding_buffer, DIR_TYPE); 
                        if (ptr8[11] != 0x0F){ // print SFN only when LFN is non-existent
                            USB1_Print_String((u8*)valid_Dir_Entry[i], 11, padding_buffer);

                            /* Save Dir name into buffer */
                            USB1_Get_String((u8*)valid_Dir_Entry[i], names[Dir_index], 11);
                            Dir_padding[Dir_index] = padding_index;
                        }

                        // only get next_cluster when SFN DIR
                        Dir_cluster[Dir_index+1] = (valid_Dir_Entry[i]->High_first_cluster << 16) | (valid_Dir_Entry[i]->Low_first_cluster);
                        Dir_index++;

                        padding_index++;
                        if (all_dir && !hidden_folder) ret = USB1_Scan_Cluster(data, (u8*)&Next_cluster_data, next_cluster, padding_index, 0, all_dir);
                        padding_index--;
                        hidden_folder = false;

                        /* Reading dir entry again for current_index_index use to optimize stack size */
                        if (ret == 0){
                            ret = USB1_Read_CLUSTER(data, cluster_num, (u8*)Next_cluster_data, &Cluster_data_len, "Reading dir entry again", 0);
                        }
                    }
                }
            }
            
        }
    }
}

int USB1_Scan_Cluster(struct usb_device_data *data, u8* cluster_data, u8 cluster_num, u8 padding_id, bool print_data, bool all_dir){
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
        USB1_Scan_ClusterData(data, (u8*)cluster_data, cluster_num, 1, all_dir);
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
                //printk("cluster %d doesn not end -> next: 0x%x\n", cluster_num, ptr32[cluster_num]);
            }
        }
    }

    return ret;
}

int USB1_f_Read_All(struct usb_device_data *data){
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
    Dir_cluster[Dir_index] = 0x02;
    /* Scan from Cluster 2 */
    if (ret == 0){
        ret = USB1_Scan_Cluster(data, (u8*)Next_cluster_data, 0x02, padding_index, 0, 1);
    }

    // /* Scan from Cluster 2 */
    // if (ret == 0){
    //     ret = USB1_Scan_Cluster(data, (u8*)Next_cluster_data, 49, padding_index, 1, 1);
    // }

    return ret;
}

/* ================ */
// Function to build the actual directory paths without heap allocation
void build_actual_dirs(u32* paddings, u32 len, u32* out_len) {
    u32 i, j;
    u32 stack_size;
    u32 actual_count;
    u32 idx;
    size_t pre_len;
    size_t remaining;
    size_t slen;
    size_t nlen;
    u8 *p;

    memset(num_direct, 0, sizeof(num_direct));
    memset(all_children_leaves, 0, sizeof(all_children_leaves));
    memset(max_subtree_padding, 0, sizeof(max_subtree_padding));
    memset(parent, -1, sizeof(parent)); // Sets all bytes to 0xFF, which for signed int is -1
    memset(include_flags, 0, sizeof(include_flags));
    memset(chain_indices, 0, sizeof(chain_indices));
    stack_size = 0;
    if (len <= 0) {
        *out_len = 1;
        strcpy(actual_dirs[0], "./");
        return;
    }
    for (i = 0; i < len; i++) {
        while (stack_size > paddings[i]) {
            stack_size--;
        }
        p = path;
        remaining = sizeof(path);
        *p = '\0';
        for (j = 0; j < stack_size; j++) {
            slen = strlen(stack[j]);
            if (slen + 1 >= remaining) {
                slen = remaining - 2;
            }
            memcpy(p, stack[j], slen);
            p += slen;
            *p++ = '/';
            *p = '\0';
            remaining -= (slen + 1);
        }
        nlen = strlen(names[i]);
        if (nlen >= remaining) {
            nlen = remaining - 1;
        }
        memcpy(p, names[i], nlen);
        p += nlen;
        *p = '\0';
        strcpy(full_paths[i], path);
        strcpy(stack[stack_size++], names[i]);
    }
    for (i = 0; i < len; i++) {
        snprintf(prefix, sizeof(prefix), "%s/", full_paths[i]);
        pre_len = strlen(prefix);
        for (j = 0; j < len; j++) {
            if (i != j && strncmp(full_paths[j], prefix, pre_len) == 0 && strchr(full_paths[j] + pre_len, '/') == NULL) {
                num_direct[i]++;
            }
        }
    }
    for (i = 0; i < len; i++) {
        all_children_leaves[i] = true;
        snprintf(prefix, sizeof(prefix), "%s/", full_paths[i]);
        pre_len = strlen(prefix);
        for (j = 0; j < len; j++) {
            if (i != j && strncmp(full_paths[j], prefix, pre_len) == 0 && strchr(full_paths[j] + pre_len, '/') == NULL) {
                if (num_direct[j] != 0) {
                    all_children_leaves[i] = false;
                }
            }
        }
    }
    for (i = 0; i < len; i++) {
        max_subtree_padding[i] = paddings[i];
        snprintf(prefix, sizeof(prefix), "%s/", full_paths[i]);
        pre_len = strlen(prefix);
        for (j = 0; j < len; j++) {
            if (strncmp(full_paths[j], prefix, pre_len) == 0) {
                if (paddings[j] > max_subtree_padding[i]) {
                    max_subtree_padding[i] = paddings[j];
                }
            }
        }
    }
    for (i = 0; i < len; i++) {
        snprintf(prefix, sizeof(prefix), "%s/", full_paths[i]);
        pre_len = strlen(prefix);
        for (j = 0; j < len; j++) {
            if (i != j && strncmp(full_paths[j], prefix, pre_len) == 0 && strchr(full_paths[j] + pre_len, '/') == NULL) {
                parent[j] = i;
            }
        }
    }
    // Set all include_flags to true to include every directory
    for (i = 0; i < len; i++) {
        include_flags[i] = true;
    }
    actual_count = 1;
    for (i = 0; i < len; i++) {
        if (include_flags[i]) {
            actual_count++;
        }
    }
    strcpy(actual_dirs[0], "./");
    idx = 1;
    for (i = 0; i < len; i++) {
        if (include_flags[i]) {
            snprintf(actual_dirs[idx], MAX_PATH_LEN, "./%s", full_paths[i]);
            idx++;
        }
    }
    *out_len = actual_count;
}

int USB1_f_Read_Dir(struct usb_device_data *data, u8* path_dir){
    // int i = 0;
    // u8* names[] = {"Countries", "BBB", "New_power", "AAA", "CCC", "VVV", "Languages", "Protocols", "USB", "BULK", "Ethernet"};
    // u32 paddings[] = {0, 1, 1, 1, 0, 1, 0, 0, 1, 2, 1};
    // u32 len = sizeof(names) / sizeof(names[0]);
    // u32 out_len;
    // build_actual_dirs(paddings, len, &out_len);

    // printk("out_len = %d\n", out_len);

    // for (i = 0; i < out_len; i++) {
    //     printk("%s\n", actual_dirs[i]);
    // }
    // return 0;



    // u32 out_len = 0, i = 0, j = 0;
    // bool dir_existed = false;
    // int ret;
    // build_actual_dirs(Dir_padding, Dir_index, &out_len);
    // printk("len = %d\n", out_len);

    // for (i = 0; i < out_len; i++) {
    //     printk("%s\n", actual_dirs[i]);
    // }
    // for (i = 0; i < out_len; i++) {
    //     printk("cluster_num = %d\n", Dir_cluster[i]);
    // }
    // return 0;



    u32 out_len = 0, i = 0, j = 0;
    bool dir_existed = false;
    int ret;
    build_actual_dirs(Dir_padding, Dir_index, &out_len);

    // printk("len = %d\n", out_len);

    // for (i = 0; i < out_len; i++) {
    //     printk("%s\n", actual_dirs[i]);
    // }

    for (i = 0; i < out_len; i++){
        while (actual_dirs[i][j] != '\0'){
            j++;
        }
        j++; // adding '\0'
        
        ret = USB1_Compare_String(path_dir, actual_dirs[i], j);
        j = 0;

        if (ret == 0){
            //printk("YES, index in String = %d\n", i);
            dir_existed = true;
            break;
        }
    }

    if (dir_existed){
        printk("'%s':\n", path_dir);
        padding_index = 1;
        if (ret == 0) ret = USB1_Scan_Cluster(data, (u8*)Next_cluster_data, Dir_cluster[i], 1, 0, 0);
        padding_index = 0;
    }
    else {
        printk("Invalid dir\n");
        ret = -1;
    }

    return ret;
}

int USB1_f_Read_File(struct usb_device_data *data, u8* path_file){
    u8 File_name[50];
    u8 Dir_name[100];
    u32 File_name_len = 0, Dir_name_len = 0;

    u32 out_len = 0, i = 0, j = 0;
    bool dir_existed = false;
    int ret;

    u8* ptr8;
    Root_Dir_Entry* entry;
    u16 next_cluster = 0;
    u32 Cluster_data_len;
    u32 size;


    u16 LFN_ret = 0;
    u8 tmp_file_name[50];
    u16 tmp_file_name_len = 0;

    USB1_Parse_TargetFile(path_file, Dir_name, &Dir_name_len, File_name, &File_name_len);
    // printk("Dir: '%s', File: '%s'\n", Dir_name, File_name);
    // printk("Dir_name_len: %d, File_name_len: %d\n", Dir_name_len, File_name_len);


    build_actual_dirs(Dir_padding, Dir_index, &out_len);
    // printk("len = %d\n", out_len);

    // for (i = 0; i < out_len; i++) {
    //     printk("%s\n", actual_dirs[i]);
    // }

    for (i = 0; i < out_len; i++){
        while (actual_dirs[i][j] != '\0'){
            j++;
        }
        j++; // adding '\0'
        
        ret = USB1_Compare_String(Dir_name, actual_dirs[i], j);
        j = 0;

        if (ret == 0){
            //printk("YES, index in String = %d\n", i);
            dir_existed = true;
            break;
        }
    }

    if (dir_existed){
        if (ret == 0) ret = USB1_Read_CLUSTER(data, Dir_cluster[i], (u8*)Next_cluster_data, &Cluster_data_len, "USB1_f_Read_File", 0);

        ptr8 = (u8*)Next_cluster_data;
        while (ptr8[0]){ // make sure the first char is not zero
            if (ptr8[11] == FILE_TYPE){
                entry = (Root_Dir_Entry*)ptr8; // save here
                //ntry_start = entry;

                // =============== checking the LFN name
                USB1_Read_NameFile(entry, tmp_file_name, &tmp_file_name_len);

                LFN_ret = USB1_Compare_String(File_name, tmp_file_name, tmp_file_name_len);
                // printk("tmp_file_name: '%s', File_name: '%s'\n", tmp_file_name, File_name);
                // printk("RET = %d\n", LFN_ret);
                tmp_file_name_len = 0;
                if (LFN_ret == 0){
                    next_cluster = (entry->High_first_cluster << 16) | (entry->Low_first_cluster);
                    size = entry->File_size;

                    //printk("next_cluster = %d, size = %d\n", next_cluster, size);
                    ret = USB1_Read_CLUSTER(data, next_cluster, (u8*)Cluster_data, &Cluster_data_len, "Reading dir entry", 0);

                    printk("%s\n", path_file);
                    USB1_Print_String((u8*)Cluster_data, size, "->"); 
                    
                    break;
                }
            }
            ptr8+=32;
        }
    }

    return 0;
}

int USB1_f_Make_Dir(struct usb_device_data *data, u8* path_dir){
    u8 Dir_name1[100];
    u8 Dir_name2[100];
    u32 Dir_name1_len = 0, Dir_name2_len = 0;

    u32 out_len = 0, i = 0, j = 0;
    bool dir_existed = false;
    int ret;

    USB1_Parse_TargetDir(path_dir, Dir_name1, &Dir_name1_len, Dir_name2, &Dir_name2_len);
    printk("Dir_name1: '%s', Dir_name2: '%s'\n", Dir_name1, Dir_name2);
    printk("Dir_name1_len: %d, Dir_name2_len: %d\n", Dir_name1_len, Dir_name2_len);

    build_actual_dirs(Dir_padding, Dir_index, &out_len);

    for (i = 0; i < out_len; i++){
        while (actual_dirs[i][j] != '\0'){
            j++;
        }
        j++; // adding '\0'
        
        ret = USB1_Compare_String(Dir_name1, actual_dirs[i], j);
        j = 0;

        if (ret == 0){
            printk("YES, index in String = %d\n", i);
            dir_existed = true;
            break;
        }
    }

    return 0;
}