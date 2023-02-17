#include <gba.h>
#include "sprite_handler.h"
#include "graphics_handler.h"
#include "print_system.h"
#include <stddef.h>

#include "sprite_cursor_bin.h"
#include "item_icon_bin.h"
#include "sprite_palettes_bin.h"
#include "item_icon_palette_bin.h"

#define CPUFASTSET_FILL (0x1000000)

#define ITEM_ICON_INC_X ((POKEMON_SPRITE_X_TILES-ITEM_SPRITE_X_TILES)<<3)
#define ITEM_ICON_INC_Y ((POKEMON_SPRITE_Y_TILES-ITEM_SPRITE_Y_TILES)<<3)

#define SPRITE_BASE_SIZE_MULTIPLIER NUM_POKEMON_SPRITES
#define SPRITE_BASE_TILE_SIZE (POKEMON_SPRITE_Y_TILES*POKEMON_SPRITE_X_TILES)
#define SPRITE_TILE_SIZE (SPRITE_BASE_SIZE_MULTIPLIER*SPRITE_BASE_TILE_SIZE)
#define SPRITE_ALT_DISTANCE (SPRITE_BASE_TILE_SIZE*TILE_SIZE)

#define SPRITE_SIZE (SPRITE_BASE_SIZE_MULTIPLIER*SPRITE_ALT_DISTANCE)
#define OVRAM_START ((uintptr_t)OBJ_BASE_ADR)
#define OVRAM_SIZE 0x8000
#define OVRAM_END (OVRAM_START+OVRAM_SIZE)
#define POSSIBLE_SPRITES (OVRAM_SIZE/SPRITE_SIZE)

#define OAM_ENTITIES 0x80

#define DISABLE_SPRITE (1<<9)
#define OFF_SCREEN_SPRITE SCREEN_HEIGHT

u8 check_for_same_address(const u8*);
uintptr_t get_vram_pos(void);
void set_updated_shadow_oam(void);
void inc_inner_sprite_counter(void);
u8 get_sprite_counter(void);
void inc_sprite_counter(void);
void set_attributes(u16, u16, u16);
u8 get_first_variable_palette(void);
u8 get_3bpp_palette(int);
void set_palette_3bpp(u8*, int, int);
u16 get_item_icon_tile(void);
u16 get_mail_icon_tile(void);
void set_item_icon(u16, u16);
void set_mail_icon(u16, u16);
void raw_update_cursor_x(u16);

const u16* sprite_cursor_gfx = (const u16*)sprite_cursor_bin;
const u16* item_icon_gfx = (const u16*)item_icon_bin;
const u16* sprite_palettes_bin_16 = (const u16*)sprite_palettes_bin;
const u16* item_icon_palette_bin_16 = (const u16*)item_icon_palette_bin;

u8 __sprite_counter;
u8 __inner_sprite_counter;
u8 __party_sprite_counter;
u8 __party_inner_sprite_counter;
u8 cursor_sprite;
u8 inner_cursor_sprite;
u16 cursor_base_x;
const u8* sprite_pointers[POSSIBLE_SPRITES];

u8 updated_shadow_oam;
u8 loaded_inner_sprite_counter;
u16 loaded_cursor_base_x;
OBJATTR shadow_oam[OAM_ENTITIES];

void init_sprite_counter(){
    __sprite_counter = 0;
    __inner_sprite_counter = 0;
    loaded_inner_sprite_counter = 0;
    loaded_cursor_base_x = 0;
}

void set_updated_shadow_oam() {
    wait_for_vblank_if_needed();
    updated_shadow_oam = 1;
}

IWRAM_CODE void update_normal_oam() {
    if(updated_shadow_oam) {
        u32* oam_ptr = (u32*)OAM;
        u32* shadow_oam_ptr = (u32*)shadow_oam;
        for(size_t i = 0; i < ((sizeof(OBJATTR)*OAM_ENTITIES)>>2); i++)
            oam_ptr[i] = shadow_oam_ptr[i];
        updated_shadow_oam = 0;
    }
    loaded_inner_sprite_counter = __inner_sprite_counter;
    loaded_cursor_base_x = cursor_base_x;
}

void set_party_sprite_counter(){
    __party_sprite_counter = __sprite_counter;
    __party_inner_sprite_counter = __inner_sprite_counter;
}

void init_sprites(){
    reset_sprites(0);
    update_normal_oam();
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

u8 get_next_sprite_index(){
    return __inner_sprite_counter;
}

u8 get_first_variable_palette(){
    return ((sprite_palettes_bin_size + item_icon_palette_bin_size)>>5);
}

uintptr_t get_vram_pos(){
    uintptr_t vram_pos = OVRAM_START+(__sprite_counter*SPRITE_SIZE);
    if(vram_pos >= OVRAM_END)
        vram_pos = OVRAM_END - SPRITE_SIZE;
    return vram_pos;
}

u8 get_3bpp_palette(int index) {
    return (index>>1) + get_first_variable_palette();
}

void set_palette_3bpp(u8* colors, int index, int palette) {
    u8 new_palette = get_3bpp_palette(index);
    const u8 num_colors = 1<<3;
    const u8 base = num_colors*(index & 1);
    
    for(int i = 0; i < num_colors; i++) {
        if(i)
            SPRITE_PALETTE[base+i+(new_palette<<4)] = SPRITE_PALETTE[colors[i]+(palette<<4)];
        else
            SPRITE_PALETTE[i+(new_palette<<4)] = SPRITE_PALETTE[colors[i]+(palette<<4)];
    }
}

void init_cursor(){
    sprite_pointers[__sprite_counter] = (const u8*)sprite_cursor_gfx;
    for(int i = 0; i < SPRITE_BASE_SIZE_MULTIPLIER; i++) {
        u16* vram_pos = (u16*)(get_vram_pos() + (SPRITE_ALT_DISTANCE*i));
        for(size_t j = 0; j < (sprite_cursor_bin_size>>1); j++)
            vram_pos[j] = sprite_cursor_gfx[j];
    }
    for(int i = 0; i < TOTAL_BG; i++) {
        set_attributes(OFF_SCREEN_SPRITE, 0, (SPRITE_TILE_SIZE*__sprite_counter) | ((3-i)<<10));
        if(i < TOTAL_BG-1)
            inc_inner_sprite_counter();
    }
    cursor_sprite = __sprite_counter;
    inner_cursor_sprite = __inner_sprite_counter;
    update_cursor_base_x(0);
    inc_sprite_counter();
}

void init_oam_palette(){
    for(size_t i = 0; i < (sprite_palettes_bin_size>>1); i++)
        SPRITE_PALETTE[i] = sprite_palettes_bin_16[i];
    for(size_t i = 0; i < (item_icon_palette_bin_size>>1); i++)
        SPRITE_PALETTE[i+(sprite_palettes_bin_size>>1)] = item_icon_palette_bin_16[i];
}

void init_item_icon(){
    for(int i = 0; i < SPRITE_BASE_SIZE_MULTIPLIER; i++) {
        u16* vram_pos = (u16*)(get_vram_pos() + sprite_cursor_bin_size + (SPRITE_ALT_DISTANCE*i));
        for(size_t j = 0; j < (item_icon_bin_size>>1); j++)
            vram_pos[j] = item_icon_gfx[j];
    }
}

u16 get_item_icon_tile(){
    return (SPRITE_TILE_SIZE*cursor_sprite) + (sprite_cursor_bin_size>>5);
}

u16 get_mail_icon_tile(){
    return get_item_icon_tile() + 1;
}

void set_item_icon(u16 y, u16 x){
    set_attributes(y + ITEM_ICON_INC_Y, x + ITEM_ICON_INC_X, get_item_icon_tile() | (get_curr_priority()<<10) | ((sprite_palettes_bin_size>>5)<<12));
    inc_inner_sprite_counter();
}

void set_mail_icon(u16 y, u16 x){
    set_attributes(y + ITEM_ICON_INC_Y, x + ITEM_ICON_INC_X, get_mail_icon_tile() | (get_curr_priority()<<10) | ((sprite_palettes_bin_size>>5)<<12));
    inc_inner_sprite_counter();
}

u8 check_for_same_address(const u8* address){
    u8 limit = __sprite_counter;
    if(__sprite_counter > POSSIBLE_SPRITES)
        limit = POSSIBLE_SPRITES;
    for(int i = 0; i < limit; i++)
        if(sprite_pointers[i] == address)
            return i;
    return POSSIBLE_SPRITES;
}

void set_pokemon_sprite(const u8* address, u8 palette, u8 info, u8 display_item, u8 display_mail, u16 y, u16 x){
    set_updated_shadow_oam();
    if(display_mail)
        set_mail_icon(y, x);
    else if(display_item)
        set_item_icon(y, x);
    u8 is_3bpp = info&2;
    u8 zero_fill = info&1;
    u8 position = check_for_same_address(address);
    if(position == POSSIBLE_SPRITES) {
        u8 colors[8];
        load_pokemon_sprite_gfx((const u32*)address, (u32*)get_vram_pos(), is_3bpp, zero_fill, __sprite_counter-(cursor_sprite+1), colors);
        if(is_3bpp)
            set_palette_3bpp(colors, __sprite_counter-(cursor_sprite+1), palette);
        position = __sprite_counter;
        __sprite_counter++;
    }
    if(is_3bpp)
        palette = get_3bpp_palette(position-(cursor_sprite+1));
    if(position < POSSIBLE_SPRITES)
        sprite_pointers[position] = address;
    set_attributes(y, x|ATTR1_SIZE_32, (SPRITE_TILE_SIZE*position)|(get_curr_priority()<<10)|(palette<<12));
    inc_inner_sprite_counter();
}

void update_cursor_y(u16 cursor_y){
    set_updated_shadow_oam();
    shadow_oam[inner_cursor_sprite-get_curr_priority()].attr0 = cursor_y;
}

void raw_update_cursor_x(u16 cursor_x){
    OAM[inner_cursor_sprite-get_loaded_priority()].attr1 = cursor_x;
    shadow_oam[inner_cursor_sprite-get_loaded_priority()].attr1 = cursor_x;
}

void raw_update_sprite_y(u8 index, u8 new_y){
    if(index >= OAM_ENTITIES)
        index = OAM_ENTITIES-1;
    OAM[index].attr0 &= ~0xFF;
    OAM[index].attr0 |= new_y;
    shadow_oam[index].attr0 &= ~0xFF;
    shadow_oam[index].attr0 |= new_y;
}

void fade_all_sprites_to_white(u16 fading_fraction){
    REG_BLDCNT &= ~(3<<6);
    REG_BLDCNT |= (1<<4) | (2<<6);
    REG_BLDY = fading_fraction;
}

void remove_fade_all_sprites(){
    REG_BLDCNT &= ~((1<<4) | (2<<6));
    REG_BLDY = 0;
}

void update_cursor_base_x(u16 cursor_x){
    wait_for_vblank_if_needed();
    cursor_base_x = cursor_x;
}

void disable_cursor(){
    update_cursor_y(OFF_SCREEN_SPRITE);
}

void disable_all_cursors(){
    set_updated_shadow_oam();
    for(int i = 0; i < TOTAL_BG; i++)
        shadow_oam[inner_cursor_sprite-i].attr0 = OFF_SCREEN_SPRITE;
}

void set_attributes(u16 obj_attr_0, u16 obj_attr_1, u16 obj_attr_2) {
    set_updated_shadow_oam();
    u8 position = __inner_sprite_counter;
    if(__inner_sprite_counter >= OAM_ENTITIES)
        position = OAM_ENTITIES-1;
    shadow_oam[position].attr0 = obj_attr_0;
    shadow_oam[position].attr1 = obj_attr_1;
    shadow_oam[position].attr2 = obj_attr_2;
}

void reset_sprites_to_cursor(){
    wait_for_vblank_if_needed();
    __sprite_counter = cursor_sprite+1;
    __inner_sprite_counter = inner_cursor_sprite+1;
    reset_sprites(__inner_sprite_counter);
}

void reset_sprites(u8 start){
    set_updated_shadow_oam();
    for(int i = start; i < OAM_ENTITIES; i++) {
        shadow_oam[i].attr0 = OFF_SCREEN_SPRITE;
        shadow_oam[i].attr1 = 0;
        shadow_oam[i].attr2 = 0;
    }
}

void disable_all_sprites(){
    set_updated_shadow_oam();
    for(int i = 0; i < OAM_ENTITIES; i++)
        shadow_oam[i].attr0 |= DISABLE_SPRITE;
}

void enable_all_sprites(){
    set_updated_shadow_oam();
    for(int i = 0; i < OAM_ENTITIES; i++)
        shadow_oam[i].attr0 &= ~DISABLE_SPRITE;
}

void reset_sprites_to_party(){
    wait_for_vblank_if_needed();
    __sprite_counter = __party_sprite_counter;
    __inner_sprite_counter = __party_inner_sprite_counter;
    reset_sprites(__party_inner_sprite_counter);
}

void move_sprites(u8 counter){
    u8 counter_kind = counter & 8;
    u8 limit = loaded_inner_sprite_counter;
    if(loaded_inner_sprite_counter > OAM_ENTITIES)
        limit = OAM_ENTITIES;
    for(int i = inner_cursor_sprite+1; i < limit; i++) {
        u16 obj_attr_2 = OAM[i].attr2 & ~SPRITE_BASE_TILE_SIZE;
        if(counter_kind)
            obj_attr_2 |= SPRITE_BASE_TILE_SIZE;
        OAM[i].attr2 = obj_attr_2;
    }
}

void move_cursor_x(u8 counter){
    counter = counter & 0x3F;
    u8 pos = counter >> 3;
    if(pos > 4)
        pos = 8-pos;
    raw_update_cursor_x(loaded_cursor_base_x + pos);
}
