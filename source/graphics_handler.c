#include <gba.h>
#include "graphics_handler.h"

#include "gender_symbols_bin.h"

#define GRAPHICS_BUFFER_SHIFT 10
#define GRAPHICS_BUFFER_SIZE (1 << GRAPHICS_BUFFER_SHIFT)

void init_gender_symbols(){
    u8 colors[] = {0,1};
    convert_1bpp(gender_symbols_bin, (u32*)(VRAM+(0x88<<5)), gender_symbols_bin_size, colors, 1);
}

void convert_xbpp(u8* src, u32* dst, u16 src_size, u8* colors, u8 is_forward, u8 num_bpp) {
    const u16 buffer_size = GRAPHICS_BUFFER_SIZE>>2;
    u32 buffer[buffer_size];
    if(num_bpp == 0 || num_bpp > 4)
        return;
    const u16 limit = buffer_size-1;
    u16 base = 0;
    u16 num_rows = Div(src_size, num_bpp);
    if(DivMod(src_size, 8*num_bpp))
        num_rows += 1;
    for(int i = 0; i < num_rows; i++) {
        u32 src_data = 0;
        for(int j = 0; j < num_bpp; j++)
            src_data |= src[(i*num_bpp)+j]<<(8*j);
        u32 row = 0;
        for(int j = 0; j < 8; j++)
            if(!is_forward)
                row |= (colors[(src_data>>(num_bpp*j))&((1<<num_bpp)-1)]&0xF) << (4*j);
            else
                row |= (colors[(src_data>>(num_bpp*j))&((1<<num_bpp)-1)]&0xF) << (4*(7-j));
        buffer[i-base] = row;
        if((i-base) == limit) {
            CpuFastSet(buffer, dst + (base<<2), buffer_size);
            base += buffer_size;
        }
    }
    u16 remaining = num_rows-base;
    if(remaining)
        CpuFastSet(buffer, dst + (base<<2), remaining);
}

void convert_1bpp(u8* src, u32* dst, u16 src_size, u8* colors, u8 is_forward) {
    convert_xbpp(src, dst, src_size, colors, is_forward, 1);
}

void convert_2bpp(u8* src, u32* dst, u16 src_size, u8* colors, u8 is_forward) {
    convert_xbpp(src, dst, src_size, colors, is_forward, 2);
}

void convert_3bpp(u8* src, u32* dst, u16 src_size, u8* colors, u8 is_forward) {
    convert_xbpp(src, dst, src_size, colors, is_forward, 3);
}