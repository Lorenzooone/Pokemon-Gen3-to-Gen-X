#include <gba.h>
#include "sprite_handler.h"

#define OAM 0x7000000
#define CPUFASTSET_FILL (0x1000000)

const u8 sprite_cursor_bin[];
const u32 sprite_cursor_bin_size;
const u8 item_icon_bin[];
const u32 item_icon_bin_size;
const u8 sprite_palettes_bin[];
const u32 sprite_palettes_bin_size;
const u8 item_icon_palette_bin[];
const u32 item_icon_palette_bin_size;
const u16* sprite_cursor_gfx = (const u16*)sprite_cursor_bin;
const u16* item_icon_gfx = (const u16*)item_icon_bin;
const u16* sprite_palettes_bin_16 = (const u16*)sprite_palettes_bin;
const u16* item_icon_palette_bin_16 = (const u16*)item_icon_palette_bin;

u8 __sprite_counter;
u8 __inner_sprite_counter;
u8 cursor_sprite;
u8 inner_cursor_sprite;
u16 cursor_base_x;

void init_sprite_counter(){
    __sprite_counter = 0;
    __inner_sprite_counter = 0;
}

u8 get_sprite_counter(){
    return __sprite_counter;
}

void inc_sprite_counter(){
    __sprite_counter++;
    __inner_sprite_counter++;
}

void inc_inner_sprite_counter(){
    __inner_sprite_counter++;
}

u8 get_first_variable_palette(){
    return ((sprite_palettes_bin_size + item_icon_palette_bin_size)>>5);
}

u32 get_vram_pos(){
    return VRAM+0x10000+(__sprite_counter*0x400);
}

int set_palette_3bpp(u8* colors, int index, int palette) {
    u8 new_palette = (index>>1) + get_first_variable_palette();
    const u8 num_colors = 1<<3;
    const u8 base = num_colors*(index & 1);
    
    for(int i = 0; i < num_colors; i++) {
        if(i)
            SPRITE_PALETTE[base+i+(new_palette<<4)] = SPRITE_PALETTE[colors[i]+(palette<<4)];
        else
            SPRITE_PALETTE[i+(new_palette<<4)] = SPRITE_PALETTE[colors[i]+(palette<<4)];
    }
    
    return new_palette;
}

void init_oam_palette(){
    for(int i = 0; i < (sprite_palettes_bin_size>>1); i++)
        SPRITE_PALETTE[i] = sprite_palettes_bin_16[i];
    for(int i = 0; i < (item_icon_palette_bin_size>>1); i++)
        SPRITE_PALETTE[i+(sprite_palettes_bin_size>>1)] = item_icon_palette_bin_16[i];
}

void set_attributes(u16 obj_attr_0, u16 obj_attr_1, u16 obj_attr_2) {
    *((u16*)(OAM + (8*__inner_sprite_counter) + 0)) = obj_attr_0;
    *((u16*)(OAM + (8*__inner_sprite_counter) + 2)) = obj_attr_1;
    *((u16*)(OAM + (8*__inner_sprite_counter) + 4)) = obj_attr_2;
}

void init_item_icon(){
    u16* vram_pos = (u16*)(get_vram_pos() + 0x20 + (sprite_cursor_bin_size));
    for(int i = 0; i < (item_icon_bin_size>>1); i++)
        vram_pos[i] = item_icon_gfx[i];
    vram_pos = (u16*)(get_vram_pos() + 0x220 + (sprite_cursor_bin_size));
    for(int i = 0; i < (item_icon_bin_size>>1); i++)
        vram_pos[i] = item_icon_gfx[i];
}

u16 get_item_icon_tile(){
    return (32*cursor_sprite) + 1 + (sprite_cursor_bin_size>>5);
}

#define ITEM_ICON_INC_Y 24
#define ITEM_ICON_INC_X 24

void set_item_icon(u16 y, u16 x){
    set_attributes(y + ITEM_ICON_INC_Y, x + ITEM_ICON_INC_X, get_item_icon_tile() | ((sprite_palettes_bin_size>>5)<<12));
    inc_inner_sprite_counter();
}

void init_cursor(u8 cursor_y_pos){
    u16* vram_pos = (u16*)(get_vram_pos() + 0x20);
    for(int i = 0; i < (sprite_cursor_bin_size>>1); i++)
        vram_pos[i] = sprite_cursor_gfx[i];
    vram_pos = (u16*)(get_vram_pos() + 0x220);
    for(int i = 0; i < (sprite_cursor_bin_size>>1); i++)
        vram_pos[i] = sprite_cursor_gfx[i];
    if(cursor_y_pos > 4)
        cursor_y_pos = 4;
    set_attributes(BASE_Y_CURSOR_MAIN_MENU + (cursor_y_pos*BASE_Y_CURSOR_INCREMENT_MAIN_MENU), BASE_X_CURSOR_MAIN_MENU, (32*__sprite_counter)+1);
    cursor_sprite = __sprite_counter;
    inner_cursor_sprite = __inner_sprite_counter;
    update_cursor_base_x(BASE_X_CURSOR_MAIN_MENU, 0);
    inc_sprite_counter();
}

void update_cursor_y(u16 cursor_y){
    *((u16*)(OAM + (8*inner_cursor_sprite) + 0)) = cursor_y;
}

void update_cursor_x(u16 cursor_x){
    *((u16*)(OAM + (8*inner_cursor_sprite) + 2)) = cursor_x;
}

void update_cursor_base_x(u16 cursor_x, u8 counter){
    cursor_base_x = cursor_x;
    move_cursor_x(counter);
}

void disable_cursor(){
    update_cursor_y(1 << 9);
}

void reset_sprites_to_cursor(){
    __sprite_counter = cursor_sprite+1;
    __inner_sprite_counter = inner_cursor_sprite+1;
    for(int i = __inner_sprite_counter; i < 0x80; i++) {
        *((u16*)(OAM + (8*i) + 0)) = 0;
        *((u16*)(OAM + (8*i) + 2)) = 0;
        *((u16*)(OAM + (8*i) + 4)) = 0;
    }
}

void move_sprites(u8 counter){
    if(!(counter & 7)) {
        for(int i = inner_cursor_sprite+1; i < __inner_sprite_counter; i++) {
            u16 obj_attr_2 = *((u16*)(OAM + (8*i) + 4));
            if(obj_attr_2 & 0x10)
                obj_attr_2 &= ~0x10;
            else
                obj_attr_2 |= 0x10;
            *((u16*)(OAM + (8*i) + 4)) = obj_attr_2;
        }
    }
}

void move_cursor_x(u8 counter){
    counter = counter & 0x3F;
    if(counter >= 0x20)
        counter = 0x40 - counter;
    u8 pos = counter >> 3;
    update_cursor_x(cursor_base_x + pos);
}