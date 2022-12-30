#include <gba.h>
#include "graphics_handler.h"

#include "gender_symbols_bin.h"

#define MAX_SIZE_POKEMON_SPRITE 0x400
#define GRAPHICS_BUFFER_SHIFT 10
#define GRAPHICS_BUFFER_SIZE (1 << GRAPHICS_BUFFER_SHIFT)

#define CPUFASTSET_FILL (0x1000000)

void convert_3bpp_forward(u8* src, u32* dst, u16 src_size, u8* colors);

void init_gender_symbols(){
    u8 colors[] = {0,1};
    convert_1bpp(gender_symbols_bin, (u32*)(VRAM+(0x88<<5)), gender_symbols_bin_size, colors, 1);
}

__attribute__ ((optimize(3))) void load_pokemon_sprite_gfx(u32 src, u32 dst, u8 info){
    u8 is_3bpp = info&2;
    u8 zero_fill = info&1;
    u32 zero = 0;
    
    u32 buffer[2][MAX_SIZE_POKEMON_SPRITE>>2];
    LZ77UnCompWram(src, buffer[0]);
    
    if(is_3bpp) {
        u8 colors[8];
        for(int i = 0; i < 8; i++)
            colors[i] = (buffer[0][0]>>(4*i))&0xF;
        if(zero_fill) {
            CpuFastSet(&zero, buffer[1], (MAX_SIZE_POKEMON_SPRITE>>2)|CPUFASTSET_FILL);
            convert_3bpp_forward(buffer[0]+1, (u32*)(((u32)buffer[1])+0x80), (((MAX_SIZE_POKEMON_SPRITE>>2)*3)>>1)-0x60, colors);
            convert_3bpp_forward(buffer[0]+1+(((((MAX_SIZE_POKEMON_SPRITE>>2)*3)>>1)-0x60)>>2), (u32*)(((u32)buffer[1])+0x280), (((MAX_SIZE_POKEMON_SPRITE>>2)*3)>>1)-0x60, colors);
            CpuFastSet(buffer[1], dst, (MAX_SIZE_POKEMON_SPRITE>>2));
        }
        else
            convert_3bpp_forward(buffer[0]+1, dst, (MAX_SIZE_POKEMON_SPRITE>>2)*3, colors);
    }
    else {
        if(zero_fill) {
            CpuFastSet(&zero, buffer[1], (MAX_SIZE_POKEMON_SPRITE>>2)|CPUFASTSET_FILL);
            CpuFastSet(buffer[0], ((u32)buffer[1])+0x80, ((MAX_SIZE_POKEMON_SPRITE>>1)-0x80)>>2);
            CpuFastSet(buffer[0]+(((MAX_SIZE_POKEMON_SPRITE>>1)-0x80)>>2), ((u32)buffer[1])+0x280, ((MAX_SIZE_POKEMON_SPRITE>>1)-0x80)>>2);
            CpuFastSet(buffer[1], dst, (MAX_SIZE_POKEMON_SPRITE>>2));
        }
        else
            CpuFastSet(buffer[0], dst, MAX_SIZE_POKEMON_SPRITE>>2);
    }
}

void convert_xbpp(u8* src, u32* dst, u16 src_size, u8* colors, u8 is_forward, u8 num_bpp) {
    if(num_bpp == 0 || num_bpp > 4)
        return;
    u16 num_rows = Div(src_size, num_bpp);
    if(DivMod(src_size, num_bpp))
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
        dst[i] = row;
    }
}

__attribute__ ((optimize(3))) void convert_3bpp_forward(u8* src, u32* dst, u16 src_size, u8* colors) {
    // This is soooo slow, even with all of this. 50k cycles more than 4bpp
    const u8 num_bpp = 3;
    u16 num_rows = Div(src_size, num_bpp);
    if(DivMod(src_size, num_bpp))
        num_rows += 1;
    for(int i = 0; i < num_rows; i++) {
        u32 src_data = 0;
        for(int j = 0; j < num_bpp; j++)
            src_data |= src[(i*num_bpp)+j]<<(8*j);
        u32 row = 0;
        for(int j = 0; j < 8; j++)
                row |= (colors[(src_data>>(num_bpp*j))&((1<<num_bpp)-1)]&0xF) << (4*j);
        dst[i] = row;
    }
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