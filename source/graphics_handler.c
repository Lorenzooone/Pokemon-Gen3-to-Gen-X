#include "base_include.h"
#include "graphics_handler.h"
#include "useful_qualifiers.h"
#include "print_system.h"
#include <stddef.h>

static void convert_3bpp_forward(const u8*, u32*, size_t, u8);
void convert_3bpp_forward_odd(const u8*, u32*, size_t);
void convert_3bpp_forward_even(const u8*, u32*, size_t);

void convert_xbpp(u8* src, u32* dst, size_t src_size, u8* colors, u8 is_forward, u8 num_bpp) {
    if(num_bpp == 0 || num_bpp > 4)
        return;
    size_t num_rows = Div(src_size, num_bpp);
    if(DivMod(src_size, num_bpp))
        num_rows += 1;
    for(size_t i = 0; i < num_rows; i++) {
        u32 src_data = 0;
        for(int j = 0; j < num_bpp; j++)
            src_data |= src[(i*num_bpp)+j]<<(8*j);
        u32 row = 0;
        for(int j = 0; j < 8; j++)
            if(!is_forward)
                row |= (colors[(src_data>>(num_bpp*j))&((1<<num_bpp)-1)]&0xF) << (4*j);
            else
                row |= (colors[(src_data>>(num_bpp*j))&((1<<num_bpp)-1)]&0xF) << (4*(7-j));
        dst[i] = row;
    }
}

MAX_OPTIMIZE ALWAYS_INLINE void convert_3bpp_forward(const u8* src, u32* dst, size_t src_size, u8 is_odd) {
    if(is_odd)
        convert_3bpp_forward_odd(src, dst, src_size);
    else
        convert_3bpp_forward_even(src, dst, src_size);
}

IWRAM_CODE MAX_OPTIMIZE void convert_3bpp_forward_even(const u8* src, u32* dst, size_t src_size) {
    // This is soooo slow, even with all of this. 50k cycles more than 4bpp    
    size_t num_rows = (size_t)Div(src_size, 3);
    if(DivMod(src_size, 3))
        num_rows += 1;
    for(size_t i = 0; i < num_rows; i++) {
        u32 src_data = 0;
        for(int j = 0; j < 3; j++)
            src_data |= src[(i*3)+j]<<(8*j);
        u32 row = 0;
        for(int j = 0; j < 8; j++) {
            u8 color = ((src_data>>(3*j))&((1<<3)-1));
            row |= color << (4*j);
        }
        dst[i] = row;
    }
}

IWRAM_CODE MAX_OPTIMIZE void convert_3bpp_forward_odd(const u8* src, u32* dst, size_t src_size) {
    // This is soooo slow, even with all of this. 50k cycles more than 4bpp
    size_t num_rows = (size_t)Div(src_size, 3);
    if(DivMod(src_size, 3))
        num_rows += 1;
    for(size_t i = 0; i < num_rows; i++) {
        u32 src_data = 0;
        for(int j = 0; j < 3; j++)
            src_data |= src[(i*3)+j]<<(8*j);
        u32 row = 0;
        for(int j = 0; j < 8; j++) {
            u8 color = ((src_data>>(3*j))&((1<<3)-1));
            if(color)
                color += 8;
            row |= color << (4*j);
        }
        dst[i] = row;
    }
}

void convert_1bpp(u8* src, u32* dst, size_t src_size, u8* colors, u8 is_forward) {
    convert_xbpp(src, dst, src_size, colors, is_forward, 1);
}

void convert_2bpp(u8* src, u32* dst, size_t src_size, u8* colors, u8 is_forward) {
    convert_xbpp(src, dst, src_size, colors, is_forward, 2);
}

void convert_3bpp(u8* src, u32* dst, size_t src_size, u8* colors, u8 is_forward) {
    convert_xbpp(src, dst, src_size, colors, is_forward, 3);
}
