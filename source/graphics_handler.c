#include <gba.h>
#include "graphics_handler.h"
#include "useful_qualifiers.h"
#include "print_system.h"
#include <stddef.h>

#define POKEMON_SPRITE_ROW_SIZE (TILE_SIZE*POKEMON_SPRITE_X_TILES)
#define SINGLE_POKEMON_SPRITE_SIZE (POKEMON_SPRITE_ROW_SIZE*POKEMON_SPRITE_Y_TILES)
#define TOTAL_POKEMON_SPRITE_SIZE (SINGLE_POKEMON_SPRITE_SIZE*NUM_POKEMON_SPRITES)

#define BPP3_TILE_ROW_SIZE ((3*TILE_X_PIXELS)/BITS_PER_BYTE)
#define BPP3_TILE_SIZE (BPP3_TILE_ROW_SIZE*TILE_Y_PIXELS)
#define BPP3_POKEMON_SPRITE_ROW_SIZE (BPP3_TILE_SIZE*POKEMON_SPRITE_X_TILES)
#define BPP3_SINGLE_POKEMON_SPRITE_SIZE (BPP3_POKEMON_SPRITE_ROW_SIZE*POKEMON_SPRITE_Y_TILES)
#define BPP3_TOTAL_POKEMON_SPRITE_SIZE (BPP3_SINGLE_POKEMON_SPRITE_SIZE*NUM_POKEMON_SPRITES)

#define GRAPHICS_BUFFER_SHIFT 10
#define GRAPHICS_BUFFER_SIZE (1 << GRAPHICS_BUFFER_SHIFT)

#define BUFFER_SIZE (TOTAL_POKEMON_SPRITE_SIZE>>2)

#define CPUFASTSET_FILL (0x1000000)

static void pokemon_sprite_zero_fill(u32*, u32*, u8, u8);
static void convert_3bpp_forward(const u8*, u32*, size_t, u8);
void convert_3bpp_forward_odd(const u8*, u32*, size_t);
void convert_3bpp_forward_even(const u8*, u32*, size_t);

MAX_OPTIMIZE void load_pokemon_sprite_gfx(const u32* src, u32* dst, u8 is_3bpp, u8 zero_fill, u8 index, u8* colors){
    
    u32 buffer[BUFFER_SIZE];
    LZ77UnCompWram(src, buffer);
    size_t processed_size = TOTAL_POKEMON_SPRITE_SIZE;
    
    if(is_3bpp) {
        for(int i = 0; i < 8; i++)
            colors[i] = (buffer[0]>>(4*i))&0xF;
        processed_size = BPP3_TOTAL_POKEMON_SPRITE_SIZE;
        if(zero_fill) 
            pokemon_sprite_zero_fill(buffer, dst, index&1, 1);
        else
            convert_3bpp_forward((u8*)(buffer+1), dst, processed_size, index&1);
    }
    else {
        if(zero_fill)
            pokemon_sprite_zero_fill(buffer, dst, 0, 0);
        else
            CpuFastSet(buffer, dst, TOTAL_POKEMON_SPRITE_SIZE>>2);
    }
}

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

MAX_OPTIMIZE ALWAYS_INLINE void pokemon_sprite_zero_fill(u32* src_buffer, u32* dst, u8 is_odd, u8 is_3bpp) {
    u32 zero = 0;
    u32 mid_buffer[BUFFER_SIZE];
    size_t processed_size = SINGLE_POKEMON_SPRITE_SIZE-POKEMON_SPRITE_ROW_SIZE;
    if(is_3bpp)
        processed_size = BPP3_SINGLE_POKEMON_SPRITE_SIZE-BPP3_POKEMON_SPRITE_ROW_SIZE;

    CpuFastSet(&zero, mid_buffer, (TOTAL_POKEMON_SPRITE_SIZE>>2)|CPUFASTSET_FILL);

    for(int i = 0; i < NUM_POKEMON_SPRITES; i++) {
        const u32* new_src = src_buffer+((i*processed_size)>>2);
        u32* new_dst = mid_buffer+((POKEMON_SPRITE_ROW_SIZE+(i*SINGLE_POKEMON_SPRITE_SIZE))>>2);
        
        if(is_3bpp)
            convert_3bpp_forward((const u8*)(new_src+1), new_dst, processed_size, is_odd);
        else
            CpuFastSet(new_src, new_dst, processed_size>>2);
    }
    
    CpuFastSet(mid_buffer, dst, (TOTAL_POKEMON_SPRITE_SIZE>>2));
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
