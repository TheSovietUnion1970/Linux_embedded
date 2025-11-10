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

Root_Dir_Entry* Glob_free_entry;

Root_Dir_Entry Ins_entry[10];

u8 dir_name[] = "power";
u8 dir_name1[] = "global_countries";
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

//void USB1_Create_Cluster_Dir_SFN(u8* dir_name, u32 dir_name_len, u32 next_cluster_num, Root_Dir_Entry* entry)

void main(){
    u32 free_dir_index = 0;

    free_dir_index = USB1_Find_Free_Entry(dataX, &Glob_free_entry);

    printf("free_dir_index = %d, Glob_free_entry = 0x%x - data = 0x%x\n", free_dir_index, Glob_free_entry, dataX);

    

    USB1_Create_Cluster_Dir_SFN(dir_name1, sizeof(dir_name1) - 1, 12, &Ins_entry[0]);
    USB1_Print_Hex((u8*)&Ins_entry[0], 32, "SFN");


}