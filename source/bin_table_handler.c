#include "base_include.h"
#include "bin_table_handler.h"
#include <stddef.h>

const u8* get_table_pointer(const u8* table, u16 entry){
    const u16* table_16 = (const u16*)table;
    size_t initial_offset;
    size_t offset = 0;
    u8 offset_table_pos = 2;
    u8 size_initial_offset = ((table[0] >> 0)&1)+1;
    u8 size_offsets = ((table[0] >> 1)&1)+1;
    u8 offset_shifter = (table[0] >> 2) & 7;
    u8 offset_shifter_initial_offset = (table[0] >> 5);
    if(size_initial_offset == 1)
        initial_offset = table[1]<<offset_shifter_initial_offset;
    else {
        initial_offset = table_16[1]<<offset_shifter_initial_offset;
        offset_table_pos = 4;
    }
    u16 entries = initial_offset-offset_table_pos;
    if(size_offsets == 2)
        entries >>= 1;
    // Sanity check
    if(entry >= entries)
        entry = 0;
    if(size_offsets == 1)
        offset = (table[(offset_table_pos)+entry]) << offset_shifter;
    else
        offset = (table_16[(offset_table_pos>>1)+entry]) << offset_shifter;
    return table + initial_offset + offset;
}

const u8* search_table_for_index(const u8* table, u16 index){
    const u16* table_16 = (const u16*)table;
    size_t initial_offset;
    size_t offset = 0;
    u8 offset_table_pos = 2;
    u8 size_initial_offset = ((table[0] >> 0)&1)+1;
    u8 size_offsets = ((table[0] >> 1)&1)+1;
    u8 offset_shifter = (table[0] >> 2) & 7;
    u8 offset_shifter_initial_offset = (table[0] >> 5);
    if(size_initial_offset == 1)
        initial_offset = table[1]<<offset_shifter_initial_offset;
    else {
        initial_offset = table_16[1]<<offset_shifter_initial_offset;
        offset_table_pos = 4;
    }
    u16 entries = initial_offset-offset_table_pos;
    if(size_offsets == 2)
        entries >>= 1;
    
    for(int i = 0; i < entries; i++) {
        if(size_offsets == 1)
            offset = (table[(offset_table_pos)+i]) << offset_shifter;
        else
            offset = (table_16[(offset_table_pos>>1)+i]) << offset_shifter;
        u16 read_value = table[initial_offset + offset] | (table[initial_offset + offset + 1]<<8);
        if(read_value == index)
            return table + initial_offset + offset + 2;
    }
    return NULL;
}
