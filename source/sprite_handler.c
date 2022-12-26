#include <gba.h>
#include "sprite_handler.h"

#define OAM 0x7000000
#define BASE_CURSOR_X 2
#define BASE_CURSOR_Y 8

const u8 sprite_cursor_bin[];
const u32 sprite_cursor_bin_size;
const u8 sprite_palettes_bin[];
const u32 sprite_palettes_bin_size;
const u16* sprite_cursor_gfx = (const u16*)sprite_cursor_bin;
const u16* sprite_palettes_bin_16 = (const u16*)sprite_palettes_bin;

u8 __sprite_counter;
u8 cursor_sprite;
u16 cursor_base_x;

void init_sprite_counter(){
    __sprite_counter = 0;
}

u8 get_sprite_counter(){
    return __sprite_counter;
}

void inc_sprite_counter(){
    __sprite_counter++;
}

u32 get_vram_pos(){
    return VRAM+0x10000+(__sprite_counter*0x400);
}

void init_oam_palette(){
    for(int i = 0; i < (sprite_palettes_bin_size>>1); i++)
        SPRITE_PALETTE[i] = sprite_palettes_bin_16[i];
}

void set_attributes(u16 obj_attr_0, u16 obj_attr_1, u16 obj_attr_2) {
    *((u16*)(OAM + (8*__sprite_counter) + 0)) = obj_attr_0;
    *((u16*)(OAM + (8*__sprite_counter) + 2)) = obj_attr_1;
    *((u16*)(OAM + (8*__sprite_counter) + 4)) = obj_attr_2;
}

void init_cursor(u8 cursor_y_pos){
    u8 sprite_counter = get_sprite_counter();
    u16* vram_pos = (u16*)(get_vram_pos() + 0x20);
    for(int i = 0; i < (sprite_cursor_bin_size>>1); i++)
        vram_pos[i] = sprite_cursor_gfx[i];
    if(cursor_y_pos > 3)
        cursor_y_pos = 3;
    set_attributes(BASE_CURSOR_Y + (cursor_y_pos*16), BASE_CURSOR_X, (32*sprite_counter)+1);
    cursor_sprite = sprite_counter;
    update_cursor_base_x(BASE_CURSOR_X);
    inc_sprite_counter();
}

void update_cursor_y(u16 cursor_y){
    *((u16*)(OAM + (8*cursor_sprite) + 0)) = cursor_y;
}

void update_cursor_x(u16 cursor_x){
    *((u16*)(OAM + (8*cursor_sprite) + 2)) = cursor_x;
}

void update_cursor_base_x(u16 cursor_x){
    cursor_base_x = cursor_x;
}

void disable_cursor(){
    update_cursor_y(1 << 9);
}

void reset_sprites_to_cursor(){
    __sprite_counter = cursor_sprite+1;
    for(int i = __sprite_counter; i < 0x80; i++) {
        *((u16*)(OAM + (8*i) + 0)) = 0;
        *((u16*)(OAM + (8*i) + 2)) = 0;
        *((u16*)(OAM + (8*i) + 4)) = 0;
    }
}

void move_cursor_x(u8 counter){
    counter = counter & 0x3F;
    if(counter >= 0x20)
        counter = 0x40 - counter;
    u8 pos = counter >> 3;
    update_cursor_x(cursor_base_x + pos);
}