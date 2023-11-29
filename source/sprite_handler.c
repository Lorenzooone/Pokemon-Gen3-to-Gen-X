#include "base_include.h"
#include "sprite_handler.h"
#include "graphics_handler.h"
#include "print_system.h"
#include "config_settings.h"
#include <stddef.h>

#include "sprite_cursor_bin.h"
#include "sprite_palettes_bin.h"

#define CPUFASTSET_FILL (0x1000000)

#define SPRITE_BASE_SIZE_MULTIPLIER 2
#define SPRITE_BASE_TILE_SIZE 16
#define SPRITE_TILE_SIZE (SPRITE_BASE_SIZE_MULTIPLIER*SPRITE_BASE_TILE_SIZE)
#define SPRITE_ALT_DISTANCE (SPRITE_BASE_TILE_SIZE*TILE_SIZE)

#define CURSOR_SPRITE_PALETTE_INDEX 9
#define SPRITE_SIZE (SPRITE_BASE_SIZE_MULTIPLIER*SPRITE_ALT_DISTANCE)
#define OVRAM_SIZE 0x8000
#define OVRAM_END (OVRAM_START+OVRAM_SIZE)
#ifdef __NDS__
#define OVRAM_END_SUB (OVRAM_START_SUB+OVRAM_SIZE)
#endif
#define POSSIBLE_SPRITES (OVRAM_SIZE/SPRITE_SIZE)

#define OAM_ENTITIES 0x80
#define OAM_DATA ((OBJATTR *)OAM)
#ifdef __NDS__
#define OAM_DATA_SUB ((OBJATTR *)OAM_SUB)
#endif

#define DISABLE_SPRITE (1<<9)
#define OFF_SCREEN_SPRITE SCREEN_HEIGHT

u8 check_for_same_address(const u8*);
uintptr_t get_vram_pos(void);
#ifdef __NDS__
uintptr_t get_vram_pos_sub(void);
#endif
void set_updated_shadow_oam(void);
void inc_inner_sprite_counter(void);
u8 get_sprite_counter(void);
void inc_sprite_counter(void);
void set_attributes(u16, u16, u16);
u8 get_first_variable_palette(void);
u8 get_3bpp_palette(int);
void set_palette_3bpp(u8*, int, int);
void raw_update_cursor_x(u16);

const u16* sprite_cursor_gfx = (const u16*)sprite_cursor_bin;
const u16* sprite_palettes_bin_16 = (const u16*)sprite_palettes_bin;

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
        u32* oam_ptr = (u32*)OAM_DATA;
        #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
        u32* oam_ptr_sub = (u32*)OAM_DATA_SUB;
        #endif
        u32* shadow_oam_ptr = (u32*)shadow_oam;
        for(size_t i = 0; i < ((sizeof(OBJATTR)*OAM_ENTITIES)>>2); i++) {
            oam_ptr[i] = shadow_oam_ptr[i];
            #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
            oam_ptr_sub[i] = shadow_oam_ptr[i];
            #endif
        }
        updated_shadow_oam = 0;
    }
    loaded_inner_sprite_counter = __inner_sprite_counter;
    loaded_cursor_base_x = cursor_base_x;
}

void set_party_sprite_counter(){
    __party_sprite_counter = __sprite_counter;
    __party_inner_sprite_counter = __inner_sprite_counter;
}

void enable_sprites_rendering(){
    REG_DISPCNT |= OBJ_ON | OBJ_1D_MAP;
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    REG_DISPCNT_SUB |= OBJ_ON | OBJ_1D_MAP;
    #endif
}

void disable_sprites_rendering(){
    REG_DISPCNT &= ~(OBJ_ON);
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    REG_DISPCNT_SUB &= ~(OBJ_ON);
    #endif
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
    return ((sprite_palettes_bin_size)>>5);
}

uintptr_t get_vram_pos(){
    uintptr_t vram_pos = OVRAM_START+(__sprite_counter*SPRITE_SIZE);
    if(vram_pos >= OVRAM_END)
        vram_pos = OVRAM_END - SPRITE_SIZE;
    return vram_pos;
}

#ifdef __NDS__
uintptr_t get_vram_pos_sub(){
    uintptr_t vram_pos = OVRAM_START_SUB+(__sprite_counter*SPRITE_SIZE);
    if(vram_pos >= OVRAM_END_SUB)
        vram_pos = OVRAM_END_SUB - SPRITE_SIZE;
    return vram_pos;
}
#endif

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
        #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
        if(i)
            SPRITE_PALETTE_SUB[base+i+(new_palette<<4)] = SPRITE_PALETTE_SUB[colors[i]+(palette<<4)];
        else
            SPRITE_PALETTE_SUB[i+(new_palette<<4)] = SPRITE_PALETTE_SUB[colors[i]+(palette<<4)];
        #endif
    }
}

void init_cursor(){
    sprite_pointers[__sprite_counter] = (const u8*)sprite_cursor_gfx;
    for(int i = 0; i < SPRITE_BASE_SIZE_MULTIPLIER; i++) {
        u16* vram_pos = (u16*)(get_vram_pos() + (SPRITE_ALT_DISTANCE*i));
        for(size_t j = 0; j < (sprite_cursor_bin_size>>1); j++)
            vram_pos[j] = sprite_cursor_gfx[j];
        #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
        vram_pos = (u16*)(get_vram_pos_sub() + (SPRITE_ALT_DISTANCE*i));
        for(size_t j = 0; j < (sprite_cursor_bin_size>>1); j++)
            vram_pos[j] = sprite_cursor_gfx[j];
        #endif
    }
    for(int i = 0; i < TOTAL_BG; i++) {
        set_attributes(OFF_SCREEN_SPRITE, 0, (SPRITE_TILE_SIZE*__sprite_counter) | ((3-i)<<10) | ((sprite_palettes_bin_size>>5)<<12));
        if(i < TOTAL_BG-1)
            inc_inner_sprite_counter();
    }
    cursor_sprite = __sprite_counter;
    inner_cursor_sprite = __inner_sprite_counter;
    update_cursor_base_x(0);
    inc_sprite_counter();
}

void set_cursor_palette() {
    SPRITE_PALETTE[(sprite_palettes_bin_size>>1)+CURSOR_SPRITE_PALETTE_INDEX] = get_full_colour(SPRITE_COLOUR_POS);
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    SPRITE_PALETTE_SUB[(sprite_palettes_bin_size>>1)+CURSOR_SPRITE_PALETTE_INDEX] = get_full_colour(SPRITE_COLOUR_POS);
    #endif
}

void init_oam_palette(){
    for(size_t i = 0; i < (sprite_palettes_bin_size>>1); i++)
        SPRITE_PALETTE[i] = sprite_palettes_bin_16[i];
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    for(size_t i = 0; i < (sprite_palettes_bin_size>>1); i++)
        SPRITE_PALETTE_SUB[i] = sprite_palettes_bin_16[i];
    #endif
    set_cursor_palette();
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

void update_cursor_y(u16 cursor_y){
    set_updated_shadow_oam();
    shadow_oam[inner_cursor_sprite-get_curr_priority()].attr0 = cursor_y;
}

IWRAM_CODE void raw_update_cursor_x(u16 cursor_x){
    OAM_DATA[inner_cursor_sprite-get_loaded_priority()].attr1 = cursor_x;
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    OAM_DATA_SUB[inner_cursor_sprite-get_loaded_priority()].attr1 = cursor_x;
    #endif
    shadow_oam[inner_cursor_sprite-get_loaded_priority()].attr1 = cursor_x;
}

IWRAM_CODE void raw_update_sprite_y(u8 index, u8 new_y){
    if(index >= OAM_ENTITIES)
        index = OAM_ENTITIES-1;
    OAM_DATA[index].attr0 &= ~0xFF;
    OAM_DATA[index].attr0 |= new_y;
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    OAM_DATA_SUB[index].attr0 &= ~0xFF;
    OAM_DATA_SUB[index].attr0 |= new_y;
    #endif
    shadow_oam[index].attr0 &= ~0xFF;
    shadow_oam[index].attr0 |= new_y;
}

IWRAM_CODE void fade_all_sprites_to_white(u16 fading_fraction){
    REG_BLDCNT &= ~(3<<6);
    REG_BLDCNT |= (1<<4) | (2<<6);
    REG_BLDY = fading_fraction;
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    REG_BLDCNT_SUB &= ~(3<<6);
    REG_BLDCNT_SUB |= (1<<4) | (2<<6);
    REG_BLDY_SUB = fading_fraction;
    #endif
}

IWRAM_CODE void remove_fade_all_sprites(){
    REG_BLDCNT &= ~((1<<4) | (2<<6));
    REG_BLDY = 0;
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    REG_BLDCNT_SUB &= ~((1<<4) | (2<<6));
    REG_BLDY_SUB = 0;
    #endif
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

void reset_sprites_to_cursor(u8 reset_ovram){
    wait_for_vblank_if_needed();
    if(reset_ovram)
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

IWRAM_CODE void move_sprites(u8 counter){
    u8 counter_kind = counter & 8;
    u8 limit = loaded_inner_sprite_counter;
    if(loaded_inner_sprite_counter > OAM_ENTITIES)
        limit = OAM_ENTITIES;
    for(int i = inner_cursor_sprite+1; i < limit; i++) {
        u16 obj_attr_2 = OAM_DATA[i].attr2 & ~SPRITE_BASE_TILE_SIZE;
        if(counter_kind)
            obj_attr_2 |= SPRITE_BASE_TILE_SIZE;
        OAM_DATA[i].attr2 = obj_attr_2;
        #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
        obj_attr_2 = OAM_DATA_SUB[i].attr2 & ~SPRITE_BASE_TILE_SIZE;
        if(counter_kind)
            obj_attr_2 |= SPRITE_BASE_TILE_SIZE;
        OAM_DATA_SUB[i].attr2 = obj_attr_2;
        #endif
    }
}

IWRAM_CODE void move_cursor_x(u8 counter){
    counter = counter & 0x3F;
    u8 pos = counter >> 3;
    if(pos > 4)
        pos = 8-pos;
    raw_update_cursor_x(loaded_cursor_base_x + pos);
}
