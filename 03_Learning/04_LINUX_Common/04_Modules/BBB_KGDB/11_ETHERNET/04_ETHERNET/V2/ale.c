#include "ale.h"
#include <linux/types.h>  /* for u8, u32, u64, etc */

u8 get_bit_position(u8 value)
{
    u8 pos = 0;
    if (value == 0)
        return -1;
        
    while ((value & 1) == 0) {
        value >>= 1;
        pos++;
    }
    return pos;
}

/* ALE read/write */
void ale_write(struct ether_device_data *data, u32* ale_entry, u16 idx){
    u8 i = 0;

	printk("ale_write > idx = %d, ale_entry[0][1][2] = 0x%x, 0x%x, 0x%x\n", idx,
										ale_entry[0], ale_entry[1], ale_entry[2]);

    for (i = 0; i < 3; i++){
        iowrite32(ale_entry[i], data->base_ale + ALE_TABLE + 4 * i);
    }

    iowrite32(ALE_TABLE_WRITE | (idx&0x3F), data->base_ale + ALE_TABLE_CONTROL);   
}

void ale_read(struct ether_device_data *data, u32* ale_entry, u16 idx){
    u8 i = 0;

    iowrite32((idx&0x3F), data->base_ale + ALE_TABLE_CONTROL);   

    for (i = 0; i < 3; i++){
        ale_entry[i] = ioread32(data->base_ale + ALE_TABLE + 4 * i);
    } 
}

/* ALE entry utils */
void ale_set_bit_val(u32 ale[3], u8 bit_position, u8 bits, u32 val)
{
    u8 i = 0;
    u8 byte_position = 0, reverse_byte_position = 0;
    u8 fix_bit = 0;
    if (bits == 0 || bit_position >= 96)
        return;

    // in case overflow
    if (bit_position + bits > 96)
        bits = 96 - bit_position;

    byte_position = bit_position/32;
    if (byte_position == 2) reverse_byte_position = 0;
    else if (byte_position == 0) reverse_byte_position = 2;
    else reverse_byte_position = 1;

    fix_bit = bit_position%32;
    for (i = 0; i < bits; i++){
        ale[reverse_byte_position] |= (val << fix_bit)&(1u << bit_position%32);
        //printf("ale[%d] = 0x%x\n", reverse_byte_position, (val << fix_bit)&(1u << bit_position%32));
        bit_position++;
        if (bit_position >= byte_position*32 + 32){
            bit_position = 0;
            byte_position--;

            if (byte_position == 2) reverse_byte_position = 0;
            else if (byte_position == 0) reverse_byte_position = 2;
            else reverse_byte_position = 1;

            // update
            fix_bit = bit_position%32;
            val = val >> (i+1);
        }
    }

    //printf("[0][1][2] = 0x%x, 0x%x, 0x%x\n", ale[0], ale[1], ale[2]);
}

void ale_get_bit_val(u32 ale[3], u8 bit_position, u8 bits, u32 *val)
{
    u32 result = 0;
    u8 i;

    if (!val)      // safety
        return;

    *val = 0;      // default

    if (bits == 0 || bit_position >= 96)
        return;

    /* clamp to ALE size and to u32 width */
    if (bit_position + bits > 96)
        bits = 96 - bit_position;
    if (bits > 32)
        bits = 32;    // can't return more than 32 bits in u32

    for (i = 0; i < bits; i++) {
        u8 abs_bit = bit_position + i;    // global bit index 0..95
        u8 word_idx = abs_bit / 32;       // 0..2 from LSB side
        u8 bit_in_word = abs_bit % 32;    // 0..31

        /* convert 0..2 (LSB->MSB) to ale[] index (2..0) */
        u8 ale_idx = 2 - word_idx;

        u32 bit_val = (ale[ale_idx] >> bit_in_word) & 1u;

        /* place it into result at position i */
        result |= (bit_val << i);
    }

    *val = result;
}

void ale_set_byte_val(u32 ale[3], u8 byte_position, u8 bytes, u8 *val)
{
    u8 i;
    u8 index_u32 = 0, byte_in_u32 = 0, reverse_byte_in_u32 = 0, reverse_byte_position = 0;

    reverse_byte_position = bytes - byte_position;

    index_u32 = reverse_byte_position/4;
    byte_in_u32 = reverse_byte_position - index_u32*4;

    for (i = 0; i < bytes; i++) {
        if (byte_in_u32 == 2){
            reverse_byte_in_u32 = 1;
        }
        else if (byte_in_u32 == 3){
            reverse_byte_in_u32 = 0;
        }
        else if (byte_in_u32 == 1){
            reverse_byte_in_u32 = 2;
        }
        else {
            reverse_byte_in_u32 = 3;
        }

        //printf("reverse_byte_in_u32 = %d, byte_in_u32 = %d, index_u32 = %d\n", reverse_byte_in_u32, byte_in_u32, index_u32);
        ale[index_u32] |= (0xff << reverse_byte_in_u32*8)&(val[i] << reverse_byte_in_u32*8);

        byte_in_u32++;
        if (byte_in_u32 > 3){
            index_u32++;
            byte_in_u32 = 0;
        } 
    }
    //printf("[0][1][2] = 0x%x, 0x%x, 0x%x\n", ale[0], ale[1], ale[2]);
}

void ale_get_byte_val(u32 ale[3], u8 byte_position, u8 bytes, u8 *val)
{
    u8 i;
    u8 index_u32 = 0, byte_in_u32 = 0, reverse_byte_in_u32 = 0, reverse_byte_position = 0;

    if (!val || bytes == 0)
        return;

    /* Same starting point math as ale_set_byte_val() */
    reverse_byte_position = bytes - byte_position;

    index_u32    = reverse_byte_position / 4;
    byte_in_u32  = reverse_byte_position - index_u32 * 4;

    for (i = 0; i < bytes; i++) {
        /* Same byte-remap as in set function */
        if (byte_in_u32 == 2) {
            reverse_byte_in_u32 = 1;
        } else if (byte_in_u32 == 3) {
            reverse_byte_in_u32 = 0;
        } else if (byte_in_u32 == 1) {
            reverse_byte_in_u32 = 2;
        } else {
            reverse_byte_in_u32 = 3;
        }

        if (index_u32 > 2) {
            /* out of ale[3] range, avoid overflow */
            val[i] = 0;
        } else {
            /* Extract the byte we previously stored there */
            val[i] = (ale[index_u32] >> (reverse_byte_in_u32 * 8)) & 0xFF;
        }

        byte_in_u32++;
        if (byte_in_u32 > 3) {
            index_u32++;
            byte_in_u32 = 0;
        }
    }
}

/* ALE match addr? */
u8 Is_the_same(u8* op1, u8* op2, u8 size){
    u8 i = 0;
    for (i = 0; i < size; i++){
        if (op1[i] != op2[i]) return 0;
    }
    return 1;
}

int ale_match_addr(struct ether_device_data *data, u8 *addr, u16 vid){
    u32 idx = 0;
    u32 ale_entry[3];
    u32 type = 0, vidX = 0;

    for (idx = 0; idx < data->ale_entries; idx++){
        u8 entry_addr[6];

        ale_read(data, ale_entry, idx);

        // get type (only take care of ALE_TYPE_ADDR or ALE_TYPE_VLAN_ADDR)
        ale_get_bit_val(ale_entry, ENTRY_TYPE_START, ENTRY_TYPE_BITS, &type);
        if (type != ALE_TYPE_ADDR && type != ALE_TYPE_VLAN_ADDR) continue;

        // get vid, must match the same
        ale_get_bit_val(ale_entry, VLAN_ID_START, VLAN_ID_BITS, &vidX);
        if (((u16)vidX) != vid) continue;

        // get addr
        ale_set_byte_val(ale_entry, MAC_ADDR_START, MAC_ADDR_BYTES, entry_addr);
        if (Is_the_same(entry_addr, addr, 6)) return idx;
    }

    return -1;
}

/* ALE match free? */
int ale_match_free(struct ether_device_data *data){
    u32 idx = 0;
    u32 ale_entry[3];
    u32 type = 0;

    for (idx = 0; idx < data->ale_entries; idx++){
        ale_read(data, ale_entry, idx);

        // get type (only take care of ALE_TYPE_ADDR or ALE_TYPE_VLAN_ADDR)
        ale_get_bit_val(ale_entry, ENTRY_TYPE_START, ENTRY_TYPE_BITS, &type);
        if (type == ALE_TYPE_FREE) return idx;
    }

    return -1;
}

/* ALE match free? */
int ale_match_vlan(struct ether_device_data *data, u16 vid){
    u32 idx = 0;
    u32 ale_entry[3];
    u32 type = 0, vidX = 0;

    for (idx = 0; idx < data->ale_entries; idx++){
        ale_read(data, ale_entry, idx);

        // get type (only take care of ALE_TYPE_VLAN)
        ale_get_bit_val(ale_entry, ENTRY_TYPE_START, ENTRY_TYPE_BITS, &type);
        if (type != ALE_TYPE_VLAN) continue;

        // get vid, must match the same
        ale_get_bit_val(ale_entry, VLAN_ID_START, VLAN_ID_BITS, &vidX);
        if (((u16)vidX) == vid) return idx;
    }

    return -1;
}

/* add vlan */
u8 idx_0 = 0;
void ale_add_vlan_id0(struct ether_device_data *data, u16 vid)
{
    int idx = 0;

    // idx = ale_match_vlan(data, vid);
    // if (idx >=0 ) ale_read(data, ale, idx);

    u32 ale[3] = {
        0x0, // MAC = 00:00:00:00:00:00 (or VLAN field)
        0x20000000, // Bit 61 = VLAN entry type
        0x07000007 // Ports 0, 1, 2 are members
    };

    // if (idx < 0){
    //     data->ale_entries++; // update the number of entries

    //     idx = ale_match_free(data); // get the new entry for config above
    // }

    // if (idx < 0) {
    //     printk("No more ALE entry free\n");
    //     return;
    // }

    if (idx_0 == 0) {
        data->ale_entries++; // update the number of entries
        idx_0++;
    }
    ale_write(data, ale, idx);
}

u8 idx_1 = 0;
void ale_add_vlan_id1(struct ether_device_data *data, u16 vid)
{
    int idx = 1;

    // idx = ale_match_vlan(data, vid);
    // if (idx >=0 ) ale_read(data, ale, idx);

    u32 ale[3] = {
        0x0, // (VLAN field)
        0x20010000, // Bit 61 = VLAN entry type, VLAN ID = 1 (bits 60:49)
        0x03030003 // Port mask = 0b011 → Port 0 (CPU) + Port 1 (RJ45)
    };

    // if (idx < 0){
    //     data->ale_entries++; // update the number of entries

    //     idx = ale_match_free(data); // get the new entry for config above
    // }

    // if (idx < 0) {
    //     printk("No more ALE entry free\n");
    //     return;
    // }
    if (idx_1 == 0) {
        data->ale_entries++; // update the number of entries
        idx_1++;
    }
    ale_write(data, ale, idx);
}

/* Add ucast */
void ale_add_ucast(struct ether_device_data *data, u16 vid, u8* addr, u32 flags, u32 port)
{
    u32 ale[3] = {0, 0, 0};
    u8 ucast_type = ALE_UCAST_PERSISTANT;
    u8 secure = (flags & ALE_SECURE);
    u8 blocked = (flags & ALE_BLOCKED);
    int idx = 0;

    /* entry_type */
    if (flags&ALE_VLAN){
        ale_set_bit_val(ale, ENTRY_TYPE_START, ENTRY_TYPE_BITS, ALE_TYPE_VLAN_ADDR);
        ale_set_bit_val(ale, VLAN_ID_START, VLAN_ID_BITS, (u32)vid);
    }
    else {
        ale_set_bit_val(ale, ENTRY_TYPE_START, ENTRY_TYPE_BITS, ALE_TYPE_ADDR);
    }

    /* set addr */
    ale_set_byte_val(ale, MAC_ADDR_START, MAC_ADDR_BYTES, addr);
    
    /* ucast_type */
    ale_set_bit_val(ale, UCAST_TYPE_START, UCAST_TYPE_BITS, (u32)ucast_type);

    /* secure */
    ale_set_bit_val(ale, SECURE_START, SECURE_BITS, (u32)(secure >> get_bit_position(ALE_SECURE)));
    //printk("[0][1][2] = 0x%x, 0x%x, 0x%x\n", ale[0], ale[1], ale[2]);
    /* blocked */
    ale_set_bit_val(ale, BLOCKED_START, BLOCKED_BITS, (u32)(blocked >> get_bit_position(ALE_BLOCKED)));

    /* port num */
    ale_set_bit_val(ale, PORT_NUM_START, PORT_NUM_BITS, (u32)port);

    // printk("[0][1][2] = 0x%x, 0x%x, 0x%x\n", ale[0], ale[1], ale[2]);

    // check whether we have this ALE entry, then just modify it
    idx = ale_match_addr(data, addr, vid);
    if (idx < 0){
        data->ale_entries++; // update the number of entries

        idx = ale_match_free(data); // get the new entry for config above
    }

    if (idx < 0) {
        printk("No more ALE entry free\n");
        return;
    }

    ale_write(data, ale, idx);
}

/* Add mcast */
void ale_add_mcast(struct ether_device_data *data, u16 vid, u8* addr, u32 flags, u32 port, u32 mcast_state)
{
    u32 ale[3] = {0, 0, 0};
    u32 mask = 0;
    int idx = 0;

    // check whether we have this ALE entry, then just modify it
    idx = ale_match_addr(data, addr, vid);
    if (idx >=0 ) ale_read(data, ale, idx);

    /* entry_type */
    if (flags&ALE_VLAN){
        ale_set_bit_val(ale, ENTRY_TYPE_START, ENTRY_TYPE_BITS, ALE_TYPE_VLAN_ADDR);
        ale_set_bit_val(ale, VLAN_ID_START, VLAN_ID_BITS, (u32)vid);
    }
    else {
        ale_set_bit_val(ale, ENTRY_TYPE_START, ENTRY_TYPE_BITS, ALE_TYPE_ADDR);
    }

    /* set addr */
    ale_set_byte_val(ale, MAC_ADDR_START, MAC_ADDR_BYTES, addr);
    
    /* set super */
    ale_set_bit_val(ale, SUPER_START, SUPER_BITS, (flags & ALE_SUPER) ? 1 : 0);

    /* mcast_type */
    ale_set_bit_val(ale, UCAST_TYPE_START, UCAST_TYPE_BITS, mcast_state);

    /* port num */
    ale_get_bit_val(ale, PORT_NUM_START, PORT_NUM_BITS, &mask);
    port |= mask; // still save the current value
    ale_set_bit_val(ale, PORT_NUM_START, PORT_NUM_BITS, port);

    // printk("[0][1][2] = 0x%x, 0x%x, 0x%x\n", ale[0], ale[1], ale[2]);

    if (idx < 0){
        data->ale_entries++; // update the number of entries

        idx = ale_match_free(data); // get the new entry for config above
    }

    if (idx < 0) {
        printk("No more ALE entry free\n");
        return;
    }
    ale_write(data, ale, idx);
}
