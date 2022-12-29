#include <gba.h>
#include "bin_table_handler.h"

u8* get_table_pointer(u8* table, u16 entry){
    u16* table_16 = (u16*)table;
    u16 initial_offset;
    u16 offset = 0;
    u8 offset_table_pos = 2;
    u8 size_initial_offset = ((table[0] >> 0)&1)+1;
    u8 size_offsets = ((table[0] >> 1)&1)+1;
    u8 offset_shifter = table[0] >> 2;
    if(size_initial_offset == 1)
        initial_offset = table[1];
    else {
        initial_offset = table_16[1];
        offset_table_pos = 4;
    }
    if(size_offsets == 1)
        offset = (table[(offset_table_pos)+entry]) << offset_shifter;
    else
        offset = (table_16[(offset_table_pos>>1)+entry]) << offset_shifter;
    return table + initial_offset + offset;
}