#include "t1.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

void USB1_Print_Hex(u8 *data, u16 len, u8 *name)
{
    u8 line[3 * 8 + 1]; // "XX " * 8 bytes + null terminator = 25 chars
    u16 i;

    if (!data || len == 0)
        return;

    printf("# %s (len=%u bytes):\n", name, len);

    for (i = 0; i < len; i++) {
        u32 pos = (i % 8) * 3;
        snprintf(&line[pos], sizeof(line) - pos, "%02X ", data[i]);

        // Print every 8 bytes, or at the end of data
        if ((i % 8) == 7 || i == len - 1) {
            printf("  %s\n", line);
            memset(line, 0, sizeof(line));
        }
    }
}

void USB1_Print_String(u8 *data, u16 len, u8* string) {
    u8 tmp[len+1];
    u16 i = 0;

    if (data == NULL || len == 0)
        return;

    for (i = 0; i < len; i++) {
        tmp[i] = data[i];
    }
    tmp[len] = '\0'; // add null terminator

    printf("%s '%s'\n", string, tmp);
}

int USB1_Gather_LFN_String(u8 *data, u16 len, u8 *output_buf, u16 *buf_size) {
    u16 i = 0;
    u16 out_idx = 0;
    char *tmp = output_buf;
    int ret = 0;

    if (data == NULL || len == 0 || tmp == NULL) {
        *tmp = '\0';  // Null-terminate empty buffer
        return -1;
    }

    out_idx = len/2;
    for (i = 0; i < out_idx; i++){
        tmp[i] = (char)data[i*2];
        //printk("data[%d] = %c. - %c\n", i*2, data[i*2], tmp[i]);
        if (tmp[i] == 0x00){
            // stop here
            //printk("i = %d\n", i);
            ret = 1;
            break;
        }
    }

    *buf_size = i;
    tmp[*buf_size] = '\0';  // Null-terminate the string

    return ret;
}

/* Compute VFAT LFN checksum for an 8.3 short name (11 bytes) */
u8 vfat_lfn_checksum(u8* sfn, u16 len) {
    u8 chk = 0;
    u16 i = 0;

    for (i = 0; i < len; ++i) {
        /* rotate-right by 1, then add next byte */
        chk = ((chk & 1) ? 0x80 : 0) + (chk >> 1) + sfn[i];
    }
    return chk;
}

// Root_Dir_Entry* Glob_free_entry;

Root_Dir_Entry Ins_entry[10];

u8 dir_name[] = "wifi";
u8 dir_name1[] = "global_countries_hahahahaha";
u8 dir_name11[] = "global_countries_hahahahaha";
u8 dir_name2[] = "power123";


u32 USB1_Find_Free_Entry(u8* cluster_data, Root_Dir_Entry** entry){
    u32 free_dir_index = 0;
    (*entry) = (Root_Dir_Entry*)cluster_data;

    //printf("entry = 0x%x - data = 0x%x\n", *entry, cluster_data);

    while ((*entry)->id != 0x00){
        (*entry)++;
        //printf("entry = 0x%x - data = 0x%x\n", *entry, cluster_data);
        free_dir_index++;
    }

    return free_dir_index;
}

int USB1_Compare_String(u8* input, u8* output, u16 len){
    u16 i = 0;
    for (i = 0; i < len; i++){
        if (output[i] != input[i]){
            return -1;
        }
    }
    return 0;

}

bool IsLowercase(u8 c){
    if ((c >= 0x61) && (c <= 0x7A)) return true;
    else if ((c >= 0x41) && (c <= 0x5A)) return false;
    else return false;
}

// dir_name_len not including '\0'
void USB1_Create_Cluster_Dir_SFN(u8* dir_name, u32 dir_name_len, u32 next_cluster_num, Root_Dir_Entry* entry){
    u32 i = 0, j = 0;
    bool is;

    u8 tmp[8];

    printf("dir_name_len = %d\n", dir_name_len);

    // fulfill 0x20
    for (i = 0; i < 8; i++){
        if (i < dir_name_len){
            tmp[i] = dir_name[i];
        }
        else {
            tmp[i] = 0x20;
        }
    }

    entry->id = tmp[0];
    is = IsLowercase(tmp[0]);
    if (is == true) entry->id -= 0x20; // make it become uppercase

    for (i = 1; i < 6; i++){
        entry->File_name[i-1] = tmp[i];

        is = IsLowercase(tmp[i]);
        if (is == true) entry->File_name[i-1] -= 0x20; // make it become uppercase
    }

    if (dir_name_len > 8) {
        entry->File_name[5] = '~';
        entry->File_name[6] = '1';
    }
    else {
        entry->File_name[5] = tmp[i++];
        entry->File_name[6] = tmp[i++];
    }

    // instead of txt in file type
    entry->File_name[7] = 0x20;
    entry->File_name[8] = 0x20;
    entry->File_name[9] = 0x20;

    entry->File_attributes = DIR_TYPE;

    // next cluster num
    entry->High_first_cluster = (next_cluster_num&0xFFFF0000)>>16;
    entry->Low_first_cluster = (next_cluster_num&0xFFFF);
}

// dir_name_len not including '\0'
void USB1_Create_Cluster_Dir_LFN(u8* dir_name, u32 dir_name_len, LFN_Root_Dir_Entry* entry, u16* entry_num){
    u16 i = 0, j = 0;
    u16 dir_name_index = 0;
    bool is;
    u8 tmp[11];
    
    *entry_num = (dir_name_len + 1)/13;
    if ((dir_name_len + 1)%13) *entry_num+=1;

    memset((u8*)entry, 0x00, 32*(*entry_num));

    //printf("dir_name_len = %d, entry_num = %d\n", dir_name_len, *entry_num);

    // Get SFN for checksum
    //printf("dir_name_len = %d\n", dir_name_len);

    // fulfill 0x20
    for (i = 0; i < 8; i++){
        if (i < dir_name_len){
            tmp[i] = dir_name[i];
        }
        else {
            tmp[i] = 0x20;
        }

        is = IsLowercase(tmp[i]);
        if (is == true) tmp[i] -= 0x20; // make it become uppercase
    }

    if (dir_name_len > 8) {
        tmp[6] = '~';
        tmp[7] = '1';
    }

    // instead of txt in file type
    tmp[8] = 0x20;
    tmp[9] = 0x20;
    tmp[10] = 0x20;

    USB1_Print_String(tmp, 11, "SFN");

    for (i = *entry_num; i > 1 ; i--){
        (entry + i - 1)->id = 0x00&END_MARKER;
        (entry + i - 1)->id |= (*entry_num - i + 1)&SEQ_NUM;

        (entry + i - 1)->File_attributes = LFN_TYPE;
        (entry + i - 1)->checksum = vfat_lfn_checksum(tmp, 11);

        for (j = 0; j < 10; j+=2){
            (entry + i - 1)->File_name1[j] = dir_name[dir_name_index++];
        }

        for (j = 0; j < 12; j+=2){
            (entry + i - 1)->File_name2[j] = dir_name[dir_name_index++];
        }

        for (j = 0; j < 4; j+=2){
            (entry + i - 1)->File_name3[j] = dir_name[dir_name_index++];
        }
    }

    (entry)->id = END_MARKER;
    (entry)->id |= (*entry_num)&SEQ_NUM;
    (entry)->File_attributes = LFN_TYPE;
    (entry)->checksum = vfat_lfn_checksum(tmp, 11);

    //dir_name_len += 1;
    //printf("dir_name_index = %d\n", dir_name_index);

    for (j = 0; j < 10; j+=2){
        if (dir_name_index < dir_name_len + 1) (entry)->File_name1[j] = dir_name[dir_name_index++];
        else {
            (entry)->File_name1[j] = 0xFF;
            (entry)->File_name1[j+1] = 0xFF;
        }
    }

    for (j = 0; j < 12; j+=2){
        if (dir_name_index < dir_name_len + 1) (entry)->File_name2[j] = dir_name[dir_name_index++];
        else {
            (entry)->File_name2[j] = 0xFF;
            (entry)->File_name2[j+1] = 0xFF;
        }
    }

    for (j = 0; j < 4; j+=2){
        if (dir_name_index < dir_name_len + 1) (entry)->File_name3[j] = dir_name[dir_name_index++];
        else {
            (entry)->File_name3[j] = 0xFF;
            (entry)->File_name3[j+1] = 0xFF;
        }
    }

}

// dir_name_len not including '\0'
void USB1_Create_Cluster_Dir(u8* cluster_data, u8* dir_name, u32 dir_name_len, u32 next_cluster_num, u16* bytes_occupied){
    u32 free_dir_index = 0;
    u16 Ins_entry_len = 0;
    Root_Dir_Entry* Glob_free_entry;

    free_dir_index = USB1_Find_Free_Entry(cluster_data, &Glob_free_entry);

    USB1_Create_Cluster_Dir_LFN(dir_name, dir_name_len, (LFN_Root_Dir_Entry*)Ins_entry, &Ins_entry_len);
    USB1_Create_Cluster_Dir_SFN(dir_name, dir_name_len, next_cluster_num, &Ins_entry[Ins_entry_len]);
    //USB1_Print_Hex((u8*)&Ins_entry[0], 32*(Ins_entry_len + 1) , "LFN + SFN");  
    
    memcpy(Glob_free_entry, (u8*)&Ins_entry[0], 32*(Ins_entry_len + 1));
    *bytes_occupied = 32*(Ins_entry_len + 1);
}

// ================== rm dir
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

/* entry_start is a pointer to SFN dir entry */
void USB1_Clear_Cluster_Dir_LFN(Root_Dir_Entry* entry_start){
    entry_start--;
    while(!((entry_start->id)&END_MARKER)){
        memset((u8*)entry_start, 0x00, 32);

        entry_start-= 1; // back the previous entry
    }

    if ((entry_start->id)&END_MARKER){
        memset((u8*)entry_start, 0x00, 32);
    }
}

// dir_name_len including '\0'
int USB1_Clear_Cluster_Dir(u8* cluster_data, u8* dir_name, u32 dir_name_len){
    Root_Dir_Entry* SFN_entry;
    SFN_entry = (Root_Dir_Entry*)cluster_data;
    u8 tmp_dir[100];
    u16 tmp_dir_len;
    int ret;
    bool folder_existed = false;
    Root_Dir_Entry* Target_SFN_entry;

    while(SFN_entry->id){
        if (SFN_entry->File_attributes == DIR_TYPE){
            USB1_Read_NameFile(SFN_entry, tmp_dir, &tmp_dir_len);

            ret = USB1_Compare_String(dir_name, tmp_dir, dir_name_len);
            if (ret == 0){
                folder_existed = true;
                Target_SFN_entry = SFN_entry;
            }
        }

        SFN_entry++;
    }

    //printf("out\n");

    if (folder_existed == true){
        printf("Yes\n");
        memset((u8*)Target_SFN_entry, 0x00, 32);
        USB1_Clear_Cluster_Dir_LFN(Target_SFN_entry);
    }
    else {
        printf("Dir invalid\n");
    }
}

void main(){
    u16 bytes_occupied = 0;

    USB1_Create_Cluster_Dir(dataX, dir_name, sizeof(dir_name) - 1, 12, &bytes_occupied);
    USB1_Print_Hex(dataX, 512, "LFN + SFN");  

    printf("========== rm dir =========\n");
    USB1_Clear_Cluster_Dir(dataX, dir_name, sizeof(dir_name));
    USB1_Print_Hex(dataX, 512, "LFN + SFN");  
}