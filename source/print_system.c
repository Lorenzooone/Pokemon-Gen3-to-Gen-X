#include "base_include.h"
#include <stdarg.h>
#include "optimized_swi.h"
#include "print_system.h"
#include "text_handler.h"
#include "graphics_handler.h"
#include "sprite_handler.h"
#include "useful_qualifiers.h"
#include "config_settings.h"
#include <stddef.h>

#include "text_gen3_to_general_int_bin.h"
#include "font_bin.h"
#include "window_graphics_bin.h"

struct {
    union {
        struct {
            u8 l1;
            u8 l2;
            u8 l3;
            u8 l4;
            u8 l5;
            u8 l6;
            u8 l7;
            u8 l8;
        };
        struct {
            u8 line[8];
        };
        struct {
            u32 p1;
            u32 p2;
        };
    }
} typedef character_bitpattern_t;


#define UNSIGNED_MIN(a, b) ((b) ^ (((a) ^ (b)) & -((a) < (b))))
#define UNSIGNED_MAX(a, b) ((a) ^ (((a) ^ (b)) & -((a) < (b))))

#define VRAM_SIZE 0x10000 // 2kb
#define VRAM_END (((uintptr_t)VRAM_0)+VRAM_SIZE)
#define PALETTE 0xF
#define PALETTE_SIZE 0x10
#define PALETTE_BASE 0x5000000
#define FONT_TILES 0x200 // Both western and japanese characters in the same font
#define DYNAMIC_TILE_INDEX_OFFSET 32
#define DYNAMIC_TILE_MAX_SLOTS (1024 - DYNAMIC_TILE_INDEX_OFFSET)
#define DYNAMIC_TILE_BUFFER_START_ADR ((uintptr_t)VRAM_0 + DYNAMIC_TILE_INDEX_OFFSET*TILE_SIZE)
#define DYNAMIC_TILE_BUFFER_BYTE_SIZE (DYNAMIC_TILE_MAX_SLOTS*TILE_SIZE)
#define FONT_SIZE (FONT_TILES*TILE_SIZE)
#define FONT_1BPP_SIZE (FONT_SIZE>>2)
#define VRAM_START (((uintptr_t)VRAM_0) + 0)
#define ARRANGEMENT_POS (DYNAMIC_TILE_BUFFER_START_ADR+DYNAMIC_TILE_BUFFER_BYTE_SIZE)
#define NUMBERS_POS (VRAM_END - ((1000 + 24)*2)) // 24 bytes wasted in VRAM, might get alignment wins?? :D
#define DYNAMIC_TILE_1BBP_BUFFER_ADR ((character_bitpattern_t*)NUMBERS_POS - (FONT_TILES))
#define DYNAMIC_TILE_WIDTH_BUFFER_ADR ((u32*)DYNAMIC_TILE_1BBP_BUFFER_ADR - (FONT_TILES / 8))
#define DYNAMIC_TILE_ALLOC_BUFFER_ADR ((u32*)DYNAMIC_TILE_WIDTH_BUFFER_ADR - (TOTAL_BG * (DYNAMIC_TILE_MAX_SLOTS / 32)))
#ifdef __NDS__
#define DYNAMIC_TILE_BUFFER_START_ADR_SUB ((uintptr_t)VRAM_0_SUB + DYNAMIC_TILE_INDEX_OFFSET*TILE_SIZE)
#define VRAM_START_SUB (((uintptr_t)VRAM_0_SUB) + 0)
#define ARRANGEMENT_POS_SUB (DYNAMIC_TILE_BUFFER_START_ADR_SUB+DYNAMIC_TILE_BUFFER_BYTE_SIZE)
#endif
#define X_OFFSET_POS 0
#define Y_OFFSET_POS 1
#define COLOR_PALETTE_INDEX_TRANSPARENT 0
#define COLOR_PALETTE_INDEX_BACKGROUND 2
#define COLOR_PALETTE_INDEX_WINDOW_1 3
#define COLOR_PALETTE_INDEX_WINDOW_2 4
#define COLOR_PALETTE_INDEX_FONT_1 15
#define COLOR_PALETTE_INDEX_FONT_2 7

#define SCREEN_SIZE 0x800

//#define OVERWRITE_SPACING

#define CPUFASTSET_FILL (0x1000000)

#define NUM_DIGITS 12

void set_arrangements(u8);
void process_arrangements(void);
void enable_screens(void);
void set_screens_positions(void);
static void base_flush(void);
void new_line(void);
void write_above_char(u16);
int sub_printf(u8*);
int sub_printf_gen3(u8*, size_t, u8);
int prepare_base_10(int, u8*);
int prepare_base_16(u32, u8*);
int digits_print(u8*, int, u8, u8);
int write_base_10(int, int, u8);
int write_base_16(u32, int, u8);
void swap_buffer_screen_internal(u8);
u8 is_screen_disabled(u8);
void add_requested_spacing(u8, u8, u8);

#define font_1bpp_buffer ((character_bitpattern_t*)DYNAMIC_TILE_1BBP_BUFFER_ADR)
#define font_1bpp_width ((u32*)DYNAMIC_TILE_WIDTH_BUFFER_ADR)
#define dynamic_tile_alloc_buffer ((u32*)DYNAMIC_TILE_ALLOC_BUFFER_ADR)
u32 dynamic_tile_alloc_index = 0;
character_bitpattern_t tile_canvas_bitpattern;
u32 tile_canvas_index;
u8 tile_canvas_free_column_cnt;
u8 x_pos;
u8 y_pos;
screen_t* screen;
#ifdef __NDS__
screen_t* screen_sub;
#endif
u8 screen_num;
u8 loaded_screen_num;

u8 screens_flush;
u8 enabled_screen[TOTAL_BG];
u8 updated_screen[TOTAL_BG];
u8 buffer_screen[TOTAL_BG];
u8 screen_positions[TOTAL_BG][2];

const u32 BIT_STEP_LENGTHS[4] = {16, 8, 4, 2};
const u32 BIT_STEP_LENGTHS_MASK[4] = {0xFFFF, 0xFF, 0xF, 0x3};
const u32 EMPTY_TILE = 0;
const character_bitpattern_t EMPTY_CHAR_BITPATTERN = {
    .p1 = 0,
    .p2 = 0,
};

// Get a free bg tile index that can be used for temporary tiles.
// Don't forget to free tiles after use!
// Used by variable width font rendering, but could be used by other things as well.
// The maximum tiles to have allocated at the same time is DYNAMIC_TILE_MAX_SLOTS.
// Put allocated indices in tile_ids_out
// Returns the number of allocated tiles
IWRAM_CODE MAX_OPTIMIZE u32 allocate_tiles(u32 count, u32* tile_ids_out){
    u32 start_count = count;
    u32 tile_id = DYNAMIC_TILE_MAX_SLOTS;
    // Search linearly through tile alloc mask buffer
    for(u32 i = 0; (i < (DYNAMIC_TILE_MAX_SLOTS / 32)) & (count != 0); i++) {
        u32 block_index = dynamic_tile_alloc_index % (DYNAMIC_TILE_MAX_SLOTS / 32);
        u32 tile_mask = 
            dynamic_tile_alloc_buffer[0 * DYNAMIC_TILE_MAX_SLOTS / 32 + block_index]|
            dynamic_tile_alloc_buffer[1 * DYNAMIC_TILE_MAX_SLOTS / 32 + block_index]|
            dynamic_tile_alloc_buffer[2 * DYNAMIC_TILE_MAX_SLOTS / 32 + block_index]|
            dynamic_tile_alloc_buffer[3 * DYNAMIC_TILE_MAX_SLOTS / 32 + block_index];

        allocate_more_in_same_block:
        while ((tile_mask != 0xFFFFFFFF) & (count != 0)) {
            // Once a block is found, do divide and conquer to find which tile ids bits are free
            tile_id = block_index * 32;
            u32 bit_step_len = 32;
            // Halve bit_step_len 4 times down to 2-bit, to search for first free tile-bit
            for (u32 depth_index = 0; depth_index < 4; depth_index++){
                // Part of block is empty, do several tile allocs in a succession.
                if(tile_mask == 0){
                    u32 tiles_left = UNSIGNED_MIN(bit_step_len, count);
                    tile_mask = dynamic_tile_alloc_buffer[screen_num * DYNAMIC_TILE_MAX_SLOTS / 32 + block_index];
                    u32 mask = 1 << (tile_id & 0x1F);
                    while (tiles_left-- > 0) {
                        tile_mask |= mask;
                        tile_ids_out[--count] = tile_id++;
                        mask <<= 1;
                    }
                    dynamic_tile_alloc_buffer[screen_num * DYNAMIC_TILE_MAX_SLOTS / 32 + block_index] = tile_mask;
                    goto allocate_more_in_same_block;
                }
                // Search block for free tiles
                bit_step_len = BIT_STEP_LENGTHS[depth_index];
                u32 bits_to_step_mask = BIT_STEP_LENGTHS_MASK[depth_index];
                u32 bits_to_step = -((tile_mask & bits_to_step_mask) == bits_to_step_mask) & bit_step_len;
                tile_mask = (tile_mask >> bits_to_step) & bits_to_step_mask;
                tile_id += bits_to_step;
            }
            // Found single (fragmented) tile to allocate in block.
            u32 bits_to_step_mask = 0x1;
            u32 bits_to_step = (tile_mask & bits_to_step_mask) == bits_to_step_mask;
            tile_id += bits_to_step;
            u32 mask = 1 << (tile_id & 0x1F);
            tile_mask = dynamic_tile_alloc_buffer[screen_num * DYNAMIC_TILE_MAX_SLOTS / 32 + block_index] | mask;
            dynamic_tile_alloc_buffer[screen_num * DYNAMIC_TILE_MAX_SLOTS / 32 + block_index] = tile_mask;
            tile_ids_out[--count] = tile_id;
        }
        dynamic_tile_alloc_index += (tile_mask == 0xFFFFFFFF);
    }
    return start_count - count;
}

IWRAM_CODE void reset_allocated_tiles_bg(u8 bg_num) {
    u32* addr = &(dynamic_tile_alloc_buffer[bg_num * DYNAMIC_TILE_MAX_SLOTS / 32]);
    *addr = 0;
    CpuFastSet(addr, addr, CPUFASTSET_FILL | (DYNAMIC_TILE_MAX_SLOTS / 32));
    tile_canvas_bitpattern = EMPTY_CHAR_BITPATTERN;
    tile_canvas_free_column_cnt = 8;
    dynamic_tile_alloc_index = (dynamic_tile_alloc_index + 1) % (DYNAMIC_TILE_MAX_SLOTS / 32);
    allocate_tiles(1, &tile_canvas_index);
}

IWRAM_CODE void set_arrangements(u8 bg_num){
    if(bg_num >= TOTAL_BG)
        bg_num = TOTAL_BG-1;
    BGCTRL[bg_num] = get_bg_priority(bg_num) | ((((uintptr_t)get_screen(bg_num))-((uintptr_t)VRAM_0))>>3);
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    BGCTRL_SUB[bg_num] = get_bg_priority(bg_num) | ((((uintptr_t)get_screen(bg_num))-((uintptr_t)VRAM_0))>>3);
    #endif
}

IWRAM_CODE void process_arrangements() {
    for(int i = 0; i < TOTAL_BG; i++)
        if(updated_screen[i]) {
            set_arrangements(i);
            updated_screen[i] = 0;
            swap_buffer_screen_internal(i);
        }
}

IWRAM_CODE void swap_buffer_screen_internal(u8 bg_num) {
    if(bg_num >= TOTAL_BG)
        bg_num = TOTAL_BG-1;
    buffer_screen[bg_num] ^= 1;
    if(bg_num == screen_num) {
        screen = get_screen(bg_num);
        #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
        screen_sub = get_screen_sub(bg_num);
        #endif
    }
}

IWRAM_CODE void enable_screens() {
    for(int i = 0; i < TOTAL_BG; i++)
        if(enabled_screen[i]) {
            REG_DISPCNT |= (0x100)<<i;
            #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
            REG_DISPCNT_SUB |= (0x100)<<i;
            #endif
        }
        else {
            REG_DISPCNT &= ~((0x100)<<i);
            #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
            REG_DISPCNT_SUB &= ~((0x100)<<i);
            #endif
        }
}

IWRAM_CODE u8 is_screen_disabled(u8 bg_num) {
    if(bg_num >= TOTAL_BG)
        bg_num = TOTAL_BG-1;
    if(REG_DISPCNT & ((0x100)<<bg_num))
        return 0;
    return 1;
}

IWRAM_CODE void set_screens_positions() {
    for(int i = 0; i < TOTAL_BG; i++) {
        BG_OFFSET[i].x = screen_positions[i][X_OFFSET_POS];
        BG_OFFSET[i].y = screen_positions[i][Y_OFFSET_POS];
        #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
        BG_OFFSET_SUB[i].x = screen_positions[i][X_OFFSET_POS];
        BG_OFFSET_SUB[i].y = screen_positions[i][Y_OFFSET_POS];
        #endif
    }
}

IWRAM_CODE ALWAYS_INLINE void base_flush() {
    process_arrangements();
    set_screens_positions();
    enable_screens();
    loaded_screen_num = screen_num;
    screens_flush = 0;
}

IWRAM_CODE void flush_screens() {
    if(screens_flush) {
        base_flush();
        update_normal_oam();
    }
}

void init_numbers() {
    u16* numbers_storage = (u16*)NUMBERS_POS;
    
    for(int j = 0; j < 10; j++)
        for(int k = 0; k < 10; k++)
            for(int l = 0; l < 10; l++)
                numbers_storage[l + (k*10) + (j*100)] = l | (k<<4) | (j<<8);
}

void set_text_palettes() {
    BG_PALETTE[0]=get_full_colour(BACKGROUND_COLOUR_POS);
    BG_PALETTE[(PALETTE*PALETTE_SIZE)+COLOR_PALETTE_INDEX_TRANSPARENT]     = get_full_colour(BACKGROUND_COLOUR_POS);
    BG_PALETTE[(PALETTE*PALETTE_SIZE)+COLOR_PALETTE_INDEX_FONT_1]          = get_full_colour(FONT_COLOUR_1_POS);
    BG_PALETTE[(PALETTE*PALETTE_SIZE)+COLOR_PALETTE_INDEX_FONT_2]          = get_full_colour(FONT_COLOUR_2_POS);
    BG_PALETTE[(PALETTE*PALETTE_SIZE)+COLOR_PALETTE_INDEX_BACKGROUND]      = get_full_colour(BACKGROUND_COLOUR_POS);
    BG_PALETTE[(PALETTE*PALETTE_SIZE)+COLOR_PALETTE_INDEX_WINDOW_1]        = get_full_colour(WINDOW_COLOUR_1_POS);
    BG_PALETTE[(PALETTE*PALETTE_SIZE)+COLOR_PALETTE_INDEX_WINDOW_2]        = get_full_colour(WINDOW_COLOUR_2_POS);
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    BG_PALETTE_SUB[0]=get_full_colour(BACKGROUND_COLOUR_POS);
    BG_PALETTE_SUB[(PALETTE*PALETTE_SIZE)+COLOR_PALETTE_INDEX_TRANSPARENT] = get_full_colour(BACKGROUND_COLOUR_POS);
    BG_PALETTE_SUB[(PALETTE*PALETTE_SIZE)+COLOR_PALETTE_INDEX_FONT_1]      = get_full_colour(FONT_COLOUR_1_POS);
    BG_PALETTE_SUB[(PALETTE*PALETTE_SIZE)+COLOR_PALETTE_INDEX_FONT_2]      = get_full_colour(FONT_COLOUR_2_POS);
    BG_PALETTE_SUB[(PALETTE*PALETTE_SIZE)+COLOR_PALETTE_INDEX_BACKGROUND]  = get_full_colour(BACKGROUND_COLOUR_POS);
    BG_PALETTE_SUB[(PALETTE*PALETTE_SIZE)+COLOR_PALETTE_INDEX_WINDOW_1]    = get_full_colour(WINDOW_COLOUR_1_POS);
    BG_PALETTE_SUB[(PALETTE*PALETTE_SIZE)+COLOR_PALETTE_INDEX_WINDOW_2]    = get_full_colour(WINDOW_COLOUR_2_POS);
    #endif
}

void compute_font_character_width() {
    for (size_t i = 0; i < FONT_TILES / 8; i++) {
        font_1bpp_width[i] = 0;
    }
    for (size_t i = 0; i < FONT_TILES; i++) {
        character_bitpattern_t character = font_1bpp_buffer[i];
        u8 character_width = 8;
        for (u8 clear_row = true; (character_width > 0) && clear_row; character_width--){
            clear_row = !(character.l1 & 1) &
                        !(character.l2 & 1) &
                        !(character.l3 & 1) &
                        !(character.l4 & 1) &
                        !(character.l5 & 1) &
                        !(character.l6 & 1) &
                        !(character.l7 & 1) &
                        !(character.l8 & 1);
            character.l1 = character.l1 >> 1;
            character.l2 = character.l2 >> 1;
            character.l3 = character.l3 >> 1;
            character.l4 = character.l4 >> 1;
            character.l5 = character.l5 >> 1;
            character.l6 = character.l6 >> 1;
            character.l7 = character.l7 >> 1;
            character.l8 = character.l8 >> 1;
        }
        character_width++;
        if (i == ' '){
            character_width = 3;
        }
        font_1bpp_width[i / 8] |= character_width << ((i % 8) * 4);
    }
}

void init_text_system() {
    REG_DISPCNT = 0 | TILE_1D_MAP | ACTIVATE_SCREEN_HW;
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    REG_DISPCNT_SUB = 0 | TILE_1D_MAP | ACTIVATE_SCREEN_HW;
    #endif
    screens_flush = 0;
    for(int i = 0; i < TOTAL_BG; i++) {
        enabled_screen[i] = 0;
        updated_screen[i] = 0;
        buffer_screen[i] = 0;
        set_arrangements(i);
        set_bg_pos(i, 0, 0);
    }
    set_text_palettes();
    set_screen(0);
    
    // This is for the first frame
    u32 magic = 0x22222222;
    CpuFastSet(&magic, VRAM_START, CPUFASTSET_FILL | (TILE_SIZE / 4));
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    CpuFastSet(&magic, VRAM_START_SUB, CPUFASTSET_FILL | (TILE_SIZE / 4));
    #endif
    
    default_reset_screen();
    enable_screen(0);
    
    // Load font
    CpuFastSet(font_bin, font_1bpp_buffer, FONT_TILES * sizeof(character_bitpattern_t) / sizeof(size_t));
    compute_font_character_width();
    // TODO: Import font as compressed font:
    // LZ77UnCompWram(font_c_bin, font_1bpp_buffer);
    
    for (u16 i = 0; i < TOTAL_BG; i++){
        reset_allocated_tiles_bg(i);
    }
    PRINT_FUNCTION("\n  Loading...");
    base_flush();
    
    // Set empty tile
    u32 zero = 0;
    CpuFastSet(&zero, VRAM_START+TILE_SIZE, CPUFASTSET_FILL | (TILE_SIZE / 4));
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    CpuFastSet(&zero, VRAM_START_SUB+TILE_SIZE, CPUFASTSET_FILL | (TILE_SIZE / 4));
    #endif

    // Set window tiles
    CpuFastSet((u8*)window_graphics_bin, VRAM_START+2*TILE_SIZE, window_graphics_bin_size / 4);
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    CpuFastSet((u8*)window_graphics_bin, VRAM_START_SUB+2*TILE_SIZE, window_graphics_bin_size / 4);
    #endif
}

IWRAM_CODE void set_updated_screen() {
    wait_for_vblank_if_needed();
    updated_screen[screen_num] = 1;
}

IWRAM_CODE void swap_buffer_screen(u8 bg_num, u8 effective_swap) {
    wait_for_vblank_if_needed();
    if(bg_num >= TOTAL_BG)
        bg_num = TOTAL_BG-1;
    if(effective_swap && is_screen_disabled(bg_num))
        swap_buffer_screen_internal(bg_num);
    updated_screen[bg_num] = 1;
}

IWRAM_CODE void prepare_flush() {
    screens_flush = 1;
}

IWRAM_CODE void wait_for_vblank_if_needed() {
    // Avoid writing where you shouldn't
    if(screens_flush)
        VBlankIntrWait();
}

IWRAM_CODE void swap_screen_enabled_state(u8 bg_num){
    wait_for_vblank_if_needed();
    if(bg_num >= TOTAL_BG)
        bg_num = TOTAL_BG-1;
    enabled_screen[bg_num] ^= 1;
}

IWRAM_CODE void enable_screen(u8 bg_num){
    wait_for_vblank_if_needed();
    if(bg_num >= TOTAL_BG)
        bg_num = TOTAL_BG-1;
    enabled_screen[bg_num] = 1;
}

IWRAM_CODE void disable_screen(u8 bg_num){
    wait_for_vblank_if_needed();
    if(bg_num >= TOTAL_BG)
        bg_num = TOTAL_BG-1;
    enabled_screen[bg_num] = 0;
}

IWRAM_CODE void disable_all_screens_but_current(){
    for(int i = 0; i < TOTAL_BG; i++) {
        if(i != screen_num)
            disable_screen(i);
    }
}

IWRAM_CODE u8 get_bg_priority(u8 bg_num) {
    if(bg_num >= TOTAL_BG)
        bg_num = TOTAL_BG-1;
    return TOTAL_BG-1-bg_num;
}

IWRAM_CODE u8 get_curr_priority() {
    return get_bg_priority(screen_num);
}

IWRAM_CODE u8 get_loaded_priority() {
    return get_bg_priority(loaded_screen_num);
}

IWRAM_CODE void set_bg_pos(u8 bg_num, int x, int y){
    wait_for_vblank_if_needed();
    if(bg_num >= TOTAL_BG)
        bg_num = TOTAL_BG-1;
    if(x < 0)
        x = SCREEN_REAL_WIDTH + x;
    if(y < 0)
        y = SCREEN_REAL_HEIGHT + y;
    screen_positions[bg_num][X_OFFSET_POS] = x;
    screen_positions[bg_num][Y_OFFSET_POS] = y;
}

IWRAM_CODE u8 get_screen_num(){
    return screen_num;
}

IWRAM_CODE screen_t* get_screen(u8 bg_num){
    if(bg_num >= TOTAL_BG)
        bg_num = TOTAL_BG-1;
    return (screen_t*)(ARRANGEMENT_POS+(SCREEN_SIZE*(bg_num+(TOTAL_BG*buffer_screen[bg_num]))));
}

#ifdef __NDS__
screen_t* get_screen_sub(u8 bg_num){
    if(bg_num >= TOTAL_BG)
        bg_num = TOTAL_BG-1;
    return (screen_t*)(ARRANGEMENT_POS_SUB+(SCREEN_SIZE*(bg_num+(TOTAL_BG*buffer_screen[bg_num]))));
}
#endif

IWRAM_CODE void set_screen(u8 bg_num){
    if(bg_num >= TOTAL_BG)
        bg_num = TOTAL_BG-1;
    screen_num = bg_num;
    screen = get_screen(bg_num);
    #ifdef __NDS__
    screen_sub = get_screen_sub(bg_num);
    #endif
}

IWRAM_CODE void reset_screen(u8 blank_fill){
    set_updated_screen();
    *((u32*)screen) = (PALETTE<<28) | (PALETTE<<12) | 0;
    if(blank_fill)
        *((u32*)screen) = (PALETTE<<28) | (PALETTE<<12) | 0x00010001;
    CpuFastSet(screen, screen, CPUFASTSET_FILL | (SCREEN_SIZE>>2));
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    *((u32*)screen_sub) = (PALETTE<<28) | (PALETTE<<12) | 0;
    if(blank_fill)
        *((u32*)screen_sub) = (PALETTE<<28) | (PALETTE<<12) | 0x00010001;
    CpuFastSet(screen_sub, screen_sub, CPUFASTSET_FILL | (SCREEN_SIZE>>2));
    #endif
    x_pos = 0;
    y_pos = 0;
    reset_allocated_tiles_bg(screen_num);
}

IWRAM_CODE void default_reset_screen(){
    reset_screen(REGULAR_FILL);
}

IWRAM_CODE void set_text_x(u8 new_x){
    finish_tile();
    new_x %= X_SIZE;
    if(new_x < X_LIMIT)
        x_pos = new_x;
    else
        x_pos = X_LIMIT-1;
}

IWRAM_CODE void set_text_y(u8 new_y){
    finish_tile();
    new_y %= Y_SIZE;
    y_pos = new_y;
    x_pos = 0;
}

IWRAM_CODE u8 get_text_x(){
    return x_pos;
}

IWRAM_CODE u8 get_text_y(){
    return y_pos;
}

IWRAM_CODE u8 flush_char(u16 character) {
    screen[x_pos+(y_pos*X_SIZE)] = character | (PALETTE << 12);
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    screen_sub[x_pos+(y_pos*X_SIZE)] = character | (PALETTE << 12);
    #endif
}

IWRAM_CODE u8 advance_cursor() {
    x_pos++;
    if(x_pos >= X_LIMIT) {
        new_line();
        return 1;
    }
    return 0;
}

IWRAM_CODE u8 write_char_and_advance_cursor(u16 character) {
    flush_char(character);
    return advance_cursor();
}

IWRAM_CODE u8 flush_tile() {
    if (tile_canvas_bitpattern.p1 == EMPTY_CHAR_BITPATTERN.p1 && tile_canvas_bitpattern.p2 == EMPTY_CHAR_BITPATTERN.p2){
        return 1;
    }
    u8 colors[] = {
        COLOR_PALETTE_INDEX_BACKGROUND,
        COLOR_PALETTE_INDEX_FONT_1
    };
    u16* tile_vram_ptr = (u16*)DYNAMIC_TILE_BUFFER_START_ADR + (tile_canvas_index * TILE_SIZE >> 1);
    convert_1bpp((u8*)&tile_canvas_bitpattern, (u32*)tile_vram_ptr, 8, colors, 1, 1);
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    u16* tile_vram_ptr_sub = (u16*)DYNAMIC_TILE_BUFFER_START_ADR_SUB + (tile_canvas_index * TILE_SIZE >> 1);
    convert_1bpp((u8*)&tile_canvas_bitpattern, (u32*)tile_vram_ptr_sub, 8, colors, 1, 1);
    #endif
    flush_char(DYNAMIC_TILE_INDEX_OFFSET + tile_canvas_index);
    return 0;
}

IWRAM_CODE void finish_tile() {
    u8 didnt_write_tile = flush_tile();
    tile_canvas_free_column_cnt = 8;
    if (didnt_write_tile) {
        return;
    }
    tile_canvas_bitpattern = EMPTY_CHAR_BITPATTERN;
    allocate_tiles(1, &tile_canvas_index);
}

IWRAM_CODE u8 write_char_variable_width(u16 character) {
    character_bitpattern_t character_bitpattern = font_1bpp_buffer[character];
    u32 width = (font_1bpp_width[character / 8] >> ((character % 8) * 4)) & 0xF;
    width++;
    if(tile_canvas_free_column_cnt > width){
        u32 cols_first = 8 - tile_canvas_free_column_cnt;
        tile_canvas_bitpattern.l1 |= (character_bitpattern.l1 >> cols_first);
        tile_canvas_bitpattern.l2 |= (character_bitpattern.l2 >> cols_first);
        tile_canvas_bitpattern.l3 |= (character_bitpattern.l3 >> cols_first);
        tile_canvas_bitpattern.l4 |= (character_bitpattern.l4 >> cols_first);
        tile_canvas_bitpattern.l5 |= (character_bitpattern.l5 >> cols_first);
        tile_canvas_bitpattern.l6 |= (character_bitpattern.l6 >> cols_first);
        tile_canvas_bitpattern.l7 |= (character_bitpattern.l7 >> cols_first);
        tile_canvas_bitpattern.l8 |= (character_bitpattern.l8 >> cols_first);
        tile_canvas_free_column_cnt -= width;
        return 0;
    } else {
        u32 cols_first = 8 - tile_canvas_free_column_cnt;
        u32 cols_last = 8 - cols_first;
        tile_canvas_bitpattern.l1 |= (character_bitpattern.l1 >> cols_first);
        tile_canvas_bitpattern.l2 |= (character_bitpattern.l2 >> cols_first);
        tile_canvas_bitpattern.l3 |= (character_bitpattern.l3 >> cols_first);
        tile_canvas_bitpattern.l4 |= (character_bitpattern.l4 >> cols_first);
        tile_canvas_bitpattern.l5 |= (character_bitpattern.l5 >> cols_first);
        tile_canvas_bitpattern.l6 |= (character_bitpattern.l6 >> cols_first);
        tile_canvas_bitpattern.l7 |= (character_bitpattern.l7 >> cols_first);
        tile_canvas_bitpattern.l8 |= (character_bitpattern.l8 >> cols_first);
        u8 err = 0;
        // VRAM mem optimization. If the tile to write happens to be empty
        // use a special hardcoded empty tile and reuse the current tile next character
        if (tile_canvas_bitpattern.p1 == EMPTY_CHAR_BITPATTERN.p1 && tile_canvas_bitpattern.p2 == EMPTY_CHAR_BITPATTERN.p2) {
            err = advance_cursor();
        } else {
            u8 fg_colors[] = {
                COLOR_PALETTE_INDEX_BACKGROUND,
                COLOR_PALETTE_INDEX_FONT_1
            };
            u8 bg_colors[] = {
                COLOR_PALETTE_INDEX_TRANSPARENT,
                COLOR_PALETTE_INDEX_FONT_2
            };
            u16* tile_vram_ptr = (u16*)DYNAMIC_TILE_BUFFER_START_ADR + (tile_canvas_index * TILE_SIZE >> 1);
            character_bitpattern_t character_shadow = tile_canvas_bitpattern;
            character_shadow.l1 = (character_shadow.l1 >> 1); // | (character_bitpattern.l1 << 7);
            character_shadow.l2 = (character_shadow.l2 >> 1); // | (character_bitpattern.l2 << 7);
            character_shadow.l3 = (character_shadow.l3 >> 1); // | (character_bitpattern.l2 << 7);
            character_shadow.l4 = (character_shadow.l4 >> 1); // | (character_bitpattern.l2 << 7);
            character_shadow.l5 = (character_shadow.l5 >> 1); // | (character_bitpattern.l2 << 7);
            character_shadow.l6 = (character_shadow.l6 >> 1); // | (character_bitpattern.l2 << 7);
            character_shadow.l7 = (character_shadow.l7 >> 1); // | (character_bitpattern.l2 << 7);
            character_shadow.l8 = (character_shadow.l8 >> 1); // | (character_bitpattern.l2 << 7);
            convert_1bpp((u8*)&tile_canvas_bitpattern, (u32*)tile_vram_ptr, 8, fg_colors, 1, 1);
            convert_1bpp((u8*)&character_shadow, (u32*)tile_vram_ptr, 8, bg_colors, 1, 0);
            #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
            u16* tile_vram_ptr_sub = (u16*)DYNAMIC_TILE_BUFFER_START_ADR_SUB + (tile_canvas_index * TILE_SIZE >> 1);
            convert_1bpp((u8*)&tile_canvas_bitpattern, (u32*)tile_vram_ptr_sub, 8, fg_colors, 1, 1);
            convert_1bpp((u8*)&character_shadow, (u32*)tile_vram_ptr_sub, 8, bg_colors, 1, 0);
            #endif
            err = write_char_and_advance_cursor((u16)(DYNAMIC_TILE_INDEX_OFFSET + tile_canvas_index));
        }
        tile_canvas_bitpattern.l1 = (character_bitpattern.l1 << cols_last);
        tile_canvas_bitpattern.l2 = (character_bitpattern.l2 << cols_last);
        tile_canvas_bitpattern.l3 = (character_bitpattern.l3 << cols_last);
        tile_canvas_bitpattern.l4 = (character_bitpattern.l4 << cols_last);
        tile_canvas_bitpattern.l5 = (character_bitpattern.l5 << cols_last);
        tile_canvas_bitpattern.l6 = (character_bitpattern.l6 << cols_last);
        tile_canvas_bitpattern.l7 = (character_bitpattern.l7 << cols_last);
        tile_canvas_bitpattern.l8 = (character_bitpattern.l8 << cols_last);
        if (!err) {
            if (allocate_tiles(1, &tile_canvas_index) == 0) {
                return 1;
            }
            tile_canvas_free_column_cnt = 8 + cols_last - width;
        }
        return err;
    }
}

IWRAM_CODE ALWAYS_INLINE u8 write_char(u16 character) {
    return write_char_variable_width(character);
}

IWRAM_CODE void new_line(){
    finish_tile();
    x_pos = 0;
    y_pos += 1;
    if(y_pos >= Y_SIZE)
        y_pos = 0;
}

IWRAM_CODE void write_above_char(u16 character) {
    // Save old tile data for writing characters
    u8 old_y_pos = y_pos;
    u8 old_x_pos = x_pos;
    u8 old_tile_canvas_free_column_cnt = tile_canvas_free_column_cnt;
    character_bitpattern_t old_tile_canvas_bitpattern = tile_canvas_bitpattern;
    u32 old_tile_canvas_index = tile_canvas_index;

    // Setup tile and reposition for writing above current char
    tile_canvas_bitpattern = EMPTY_CHAR_BITPATTERN;
    allocate_tiles(1, &tile_canvas_index);
    y_pos = y_pos-1;
    if(!y_pos)
        y_pos = Y_SIZE-1;

    // Write tile
    write_char_variable_width(character);
    flush_tile();

    // Restore old values to continue to write in ordinary position
    y_pos = old_y_pos;
    x_pos = old_x_pos;
    tile_canvas_free_column_cnt = old_tile_canvas_free_column_cnt;
    tile_canvas_bitpattern = old_tile_canvas_bitpattern;
    tile_canvas_index = old_tile_canvas_index;
}

IWRAM_CODE MAX_OPTIMIZE int sub_printf(u8* string) {
    while((*string) != '\0')
        if(write_char(*(string++)))
            break;
    return 0;
}

IWRAM_CODE MAX_OPTIMIZE int sub_printf_gen3(u8* string, size_t size_max, u8 is_jp) {
    size_t curr_pos = 0;
    while((*string) != GEN3_EOL) {
        if(is_jp) {
            u8 character = *(string++);
            size_t jp_font_offset = FONT_TILES / 2;
            if((character >= GEN3_FIRST_TICKS_START && character <= GEN3_FIRST_TICKS_END) || (character >= GEN3_SECOND_TICKS_START && character <= GEN3_SECOND_TICKS_END))
                write_above_char(GENERIC_TICKS_CHAR + jp_font_offset);
            else if((character >= GEN3_FIRST_CIRCLE_START && character <= GEN3_FIRST_CIRCLE_END) || (character >= GEN3_SECOND_CIRCLE_START && character <= GEN3_SECOND_CIRCLE_END))
                write_above_char(GENERIC_CIRCLE_CHAR + jp_font_offset);
            if(write_char(character + jp_font_offset))
                return 0;
        }
        else if(write_char(text_gen3_to_general_int_bin[*(string++)]))
            return 0;
        curr_pos++;
        if(curr_pos == size_max)
            return 0;
    }
    return 0;
}

IWRAM_CODE int prepare_base_10(int number, u8* digits) {
    u16* numbers_storage = (u16*)NUMBERS_POS;

    for(int i = 0; i < NUM_DIGITS; i++)
        digits[i] = 0;
    u8 pos = NUM_DIGITS-1;
    
    u8 minus = 0;
    u8 special = 0;
    if(number < 0){
        minus = 1;
        number = -number;
        if(number < 0) {
            number -= 1;
            special = 1;
        }
    }
    
    int mod;
    while(number >= 1000) {
        number = SWI_DivDivMod(number, 1000, &mod);
        for(int i = 0; i < 3; i++)
            digits[pos--] = ((numbers_storage[mod]>>(i*4)) & 0xF) + '0';
    }
    u8 found = 0;
    u8 found_pos = 0;
    for(int i = 0; i < 3; i++) {
        u8 digit = ((numbers_storage[number]>>((2-i)*4)) & 0xF);
        if(digit || found) {
            digits[pos-(2-i)] = digit + '0';
            if(!found)
                found_pos = (2-i);
            found = 1;
        }
    }
    
    if(!found)
        digits[pos] = '0';
    
    pos -= found_pos;
    
    if(minus) {
        pos -= 1;
        digits[pos] = '-';
        if(special)
            digits[NUM_DIGITS-1] += 1;
    }
    
    return pos;
}

IWRAM_CODE int prepare_base_16(u32 number, u8* digits) {
    for(int i = 0; i < NUM_DIGITS; i++)
        digits[i] = 0;
    
    u8 pos = NUM_DIGITS-1;
    u8 digit;
    while(number >= 0x10) {
        digit = number & 0xF;
        if(digit >= 0xA)
            digit += 'A' - 0xA;
        else
            digit += '0';
        digits[pos--] = digit;
        number >>= 4;
    }
    
    digit = number & 0xF;
    if(digit >= 0xA)
        digit += 'A' - 0xA;
    else
        digit += '0';
    digits[pos] = digit;

    return pos;
}

IWRAM_CODE int digits_print(u8* digits, int max, u8 sub, u8 start_pos) {
    for(int i = NUM_DIGITS-max; i < start_pos; i++)
        if(write_char(sub))
            return 0;
    for(int i = start_pos; i < NUM_DIGITS; i++)
        if(write_char(digits[i]))
            return 0;
    return 0;
}

IWRAM_CODE int write_base_10(int number, int max, u8 sub) {
    u8 digits[NUM_DIGITS];
    u8 start_pos = prepare_base_10(number, digits);
    
    return digits_print(digits, max, sub, start_pos);
}

IWRAM_CODE int write_base_16(u32 number, int max, u8 sub) {
    u8 digits[NUM_DIGITS];
    u8 start_pos = prepare_base_16(number, digits);
    
    return digits_print(digits, max, sub, start_pos);
}

IWRAM_CODE void add_requested_spacing(u8 prev_x_pos, u8 prev_y_pos, u8 space_increase) {
    finish_tile();
    #ifdef OVERWRITE_SPACING
    y_pos = prev_y_pos;
    #endif
    if(prev_y_pos != y_pos)
        return;
    u16 new_x_pos = prev_x_pos + space_increase;
    #ifndef OVERWRITE_SPACING
    if(new_x_pos < x_pos)
        new_x_pos = x_pos;
    #endif
    if(new_x_pos >= X_LIMIT)
        new_line();
    else
        x_pos = new_x_pos;
}

IWRAM_CODE int fast_printf(const char * format, ... ) {
    set_updated_screen();
    va_list va;
    va_start(va, format);
    int consumed = 0;
    while((*format) != '\0') {
        u8 curr_x_pos = x_pos;
        u8 curr_y_pos = y_pos;
        u8 character = *format;
        switch(character) {
            case '\x01':
                sub_printf(va_arg(va, u8*));
                break;
            case '\x02':
                write_char((u8)va_arg(va, int));
                break;
            case '\x03':
                write_base_10(va_arg(va, int), 0, 0);
                break;
            case '\x04':
                write_base_16(va_arg(va, u32), 0, 0);
                break;
            case '\x05':
                sub_printf_gen3(va_arg(va, u8*), va_arg(va, size_t), va_arg(va, int));
                break;
            case '\x09':
                write_base_10(va_arg(va, int), va_arg(va, int), ' ');
                break;
            case '\x0B':
                write_base_10(va_arg(va, int), va_arg(va, int), '0');
                break;
            case '\x0C':
                write_base_16(va_arg(va, u32), va_arg(va, int), ' ');
                break;
            case '\x0D':
                write_base_16(va_arg(va, u32), va_arg(va, int), '0');
                break;
            case '\x11':
                sub_printf(va_arg(va, u8*));
                add_requested_spacing(curr_x_pos, curr_y_pos, va_arg(va, int));
                break;
            case '\x13':
                write_base_10(va_arg(va, int), 0, 0);
                add_requested_spacing(curr_x_pos, curr_y_pos, va_arg(va, int));
                break;
            case '\x14':
                write_base_16(va_arg(va, u32), 0, 0);
                add_requested_spacing(curr_x_pos, curr_y_pos, va_arg(va, int));
                break;
            case '\x15':
                sub_printf_gen3(va_arg(va, u8*), va_arg(va, size_t), va_arg(va, int));
                add_requested_spacing(curr_x_pos, curr_y_pos, va_arg(va, int));
                break;
            case '\n':
                new_line();
                break;
            default:
                write_char(character);
                break;
        }
        format++;
    }
    
    va_end(va);
    // Ensure that the last text character is written to screen.
    flush_tile();
    return consumed;
}
