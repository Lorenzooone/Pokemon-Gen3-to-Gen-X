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
#include "amiga_font_c_bin.h"
#include "jp_font_c_bin.h"
#include "window_graphics_bin.h"

#define VRAM_SIZE 0x10000
#define VRAM_END (((uintptr_t)VRAM_0)+VRAM_SIZE)
#define PALETTE 0xF
#define PALETTE_SIZE 0x10
#define PALETTE_BASE 0x5000000
#define FONT_TILES 0x100
#define FONT_SIZE (FONT_TILES*TILE_SIZE)
#define FONT_1BPP_SIZE (FONT_SIZE>>2)
#define FONT_POS (((uintptr_t)VRAM_0) + 0)
#define JP_FONT_POS (FONT_POS + FONT_SIZE)
#define NUMBERS_POS (VRAM_END - (10000*2))
#define ARRANGEMENT_POS (JP_FONT_POS+FONT_SIZE)
#ifdef __NDS__
#define FONT_POS_SUB (((uintptr_t)VRAM_0_SUB) + 0)
#define JP_FONT_POS_SUB (FONT_POS_SUB + FONT_SIZE)
#define ARRANGEMENT_POS_SUB (JP_FONT_POS_SUB+FONT_SIZE)
#endif
#define X_OFFSET_POS 0
#define Y_OFFSET_POS 1

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
u8 write_char(u16);
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

u8 is_screen_disabled(u8 bg_num) {
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

ALWAYS_INLINE void base_flush() {
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
    
    for(int i = 0; i < 10; i++)
        for(int j = 0; j < 10; j++)
            for(int k = 0; k < 10; k++)
                for(int l = 0; l < 10; l++)
                    numbers_storage[l + (k*10) + (j*100) + (i*1000)] = l | (k<<4) | (j<<8) | (i<<12);
}

void set_text_palettes() {
    BG_PALETTE[0]=get_full_colour(BACKGROUND_COLOUR_POS);
    BG_PALETTE[(PALETTE*PALETTE_SIZE)]=get_full_colour(BACKGROUND_COLOUR_POS);
    BG_PALETTE[(PALETTE*PALETTE_SIZE)+1]=get_full_colour(FONT_COLOUR_POS);
    BG_PALETTE[(PALETTE*PALETTE_SIZE)+2]=get_full_colour(BACKGROUND_COLOUR_POS);
    BG_PALETTE[(PALETTE*PALETTE_SIZE)+3]=get_full_colour(WINDOW_COLOUR_1_POS);
    BG_PALETTE[(PALETTE*PALETTE_SIZE)+4]=get_full_colour(WINDOW_COLOUR_2_POS);
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    BG_PALETTE_SUB[0]=get_full_colour(BACKGROUND_COLOUR_POS);
    BG_PALETTE_SUB[(PALETTE*PALETTE_SIZE)]=get_full_colour(BACKGROUND_COLOUR_POS);
    BG_PALETTE_SUB[(PALETTE*PALETTE_SIZE)+1]=get_full_colour(FONT_COLOUR_POS);
    BG_PALETTE_SUB[(PALETTE*PALETTE_SIZE)+2]=get_full_colour(BACKGROUND_COLOUR_POS);
    BG_PALETTE_SUB[(PALETTE*PALETTE_SIZE)+3]=get_full_colour(WINDOW_COLOUR_1_POS);
    BG_PALETTE_SUB[(PALETTE*PALETTE_SIZE)+4]=get_full_colour(WINDOW_COLOUR_2_POS);
    #endif
}

void init_text_system() {
    REG_DISPCNT = 0 | TILE_1D_MAP | ACTIVATE_SCREEN_HW;
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    REG_DISPCNT_SUB = 0 | TILE_1D_MAP | ACTIVATE_SCREEN_HW;
    #endif

    #ifdef __NDS__
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankB(VRAM_B_MAIN_SPRITE);
    vramSetBankC(VRAM_C_SUB_BG);
    vramSetBankD(VRAM_D_SUB_SPRITE);
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
    for(size_t i = 0; i < (TILE_SIZE>>2); i++)
        *((u32*)(FONT_POS+(i<<2))) = 0x22222222;
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    for(size_t i = 0; i < (TILE_SIZE>>2); i++)
        *((u32*)(FONT_POS_SUB+(i<<2))) = 0x22222222;
    #endif
    
    default_reset_screen();
    enable_screen(0);
    
    // Load english font
    u32 buffer[FONT_1BPP_SIZE>>2];
    u8 colors[] = {2, 1};
    LZ77UnCompWram(amiga_font_c_bin, buffer);
    convert_1bpp((u8*)buffer, (u32*)FONT_POS, FONT_1BPP_SIZE, colors, 1);
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    convert_1bpp((u8*)buffer, (u32*)FONT_POS_SUB, FONT_1BPP_SIZE, colors, 1);
    #endif
    
    PRINT_FUNCTION("\n  Loading...");
    base_flush();
    
    // Set empty tile
    for(size_t i = 0; i < (TILE_SIZE>>2); i++)
        *((u32*)(FONT_POS+TILE_SIZE+(i<<2))) = 0;
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    for(size_t i = 0; i < (TILE_SIZE>>2); i++)
        *((u32*)(FONT_POS_SUB+TILE_SIZE+(i<<2))) = 0;
    #endif

    // Load japanese font
    LZ77UnCompWram(jp_font_c_bin, buffer);
    convert_1bpp((u8*)buffer, (u32*)JP_FONT_POS, FONT_1BPP_SIZE, colors, 0);
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    convert_1bpp((u8*)buffer, (u32*)JP_FONT_POS_SUB, FONT_1BPP_SIZE, colors, 0);
    #endif
    
    // Set window tiles
    for(size_t i = 0; i < (window_graphics_bin_size>>2); i++)
        *((u32*)(FONT_POS+(2*TILE_SIZE)+(i<<2))) = ((const u32*)window_graphics_bin)[i];
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    for(size_t i = 0; i < (window_graphics_bin_size>>2); i++)
        *((u32*)(FONT_POS_SUB+(2*TILE_SIZE)+(i<<2))) = ((const u32*)window_graphics_bin)[i];
    #endif
}

void set_updated_screen() {
    wait_for_vblank_if_needed();
    updated_screen[screen_num] = 1;
}

void swap_buffer_screen(u8 bg_num, u8 effective_swap) {
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

void wait_for_vblank_if_needed() {
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

void disable_all_screens_but_current(){
    for(int i = 0; i < TOTAL_BG; i++) {
        if(i != screen_num)
            disable_screen(i);
    }
}

u8 get_bg_priority(u8 bg_num) {
    if(bg_num >= TOTAL_BG)
        bg_num = TOTAL_BG-1;
    return TOTAL_BG-1-bg_num;
}

u8 get_curr_priority() {
    return get_bg_priority(screen_num);
}

u8 get_loaded_priority() {
    return get_bg_priority(loaded_screen_num);
}

void set_bg_pos(u8 bg_num, int x, int y){
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

u8 get_screen_num(){
    return screen_num;
}

screen_t* get_screen(u8 bg_num){
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

void set_screen(u8 bg_num){
    if(bg_num >= TOTAL_BG)
        bg_num = TOTAL_BG-1;
    screen_num = bg_num;
    screen = get_screen(bg_num);
    #ifdef __NDS__
    screen_sub = get_screen_sub(bg_num);
    #endif
}

void reset_screen(u8 blank_fill){
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
}

void default_reset_screen(){
    reset_screen(REGULAR_FILL);
}

void set_text_x(u8 new_x){
    new_x %= X_SIZE;
    if(new_x < X_LIMIT)
        x_pos = new_x;
    else
        x_pos = X_LIMIT-1;
}

void set_text_y(u8 new_y){
    new_y %= Y_SIZE;
    y_pos = new_y;
    x_pos = 0;
}

u8 get_text_x(){
    return x_pos;
}

u8 get_text_y(){
    return y_pos;
}

void new_line(){
    x_pos = 0;
    y_pos += 1;
    if(y_pos >= Y_SIZE)
        y_pos = 0;
}

u8 write_char(u16 character) {
    screen[x_pos+(y_pos*X_SIZE)] = character | (PALETTE << 12);
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    screen_sub[x_pos+(y_pos*X_SIZE)] = character | (PALETTE << 12);
    #endif
    x_pos++;
    if(x_pos >= X_LIMIT) {
        new_line();
        return 1;
    }
    return 0;
}

void write_above_char(u16 character) {
    u8 y_pos_altered = y_pos-1;
    if(!y_pos)
        y_pos_altered = Y_SIZE-1;
    screen[x_pos+(y_pos_altered*X_SIZE)] = character | (PALETTE << 12);
    #if defined (__NDS__) && (SAME_ON_BOTH_SCREENS)
    screen_sub[x_pos+(y_pos_altered*X_SIZE)] = character | (PALETTE << 12);
    #endif
}

int sub_printf(u8* string) {
    while((*string) != '\0')
        if(write_char(*(string++)))
            return 0;
    return 0;
}

int sub_printf_gen3(u8* string, size_t size_max, u8 is_jp) {
    size_t curr_pos = 0;
    while((*string) != GEN3_EOL) {
        if(is_jp) {
            u8 character = *(string++);
            if((character >= GEN3_FIRST_TICKS_START && character <= GEN3_FIRST_TICKS_END) || (character >= GEN3_SECOND_TICKS_START && character <= GEN3_SECOND_TICKS_END))
                write_above_char(GENERIC_TICKS_CHAR+FONT_TILES);
            else if((character >= GEN3_FIRST_CIRCLE_START && character <= GEN3_FIRST_CIRCLE_END) || (character >= GEN3_SECOND_CIRCLE_START && character <= GEN3_SECOND_CIRCLE_END))
                write_above_char(GENERIC_CIRCLE_CHAR+FONT_TILES);
            if(write_char(character+FONT_TILES))
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

int prepare_base_10(int number, u8* digits) {
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
    while(number >= 10000) {
        number = SWI_DivDivMod(number, 10000, &mod);
        for(int i = 0; i < 4; i++)
            digits[pos--] = ((numbers_storage[mod]>>(i*4)) & 0xF) + '0';
    }
    u8 found = 0;
    u8 found_pos = 0;
    for(int i = 0; i < 4; i++) {
        u8 digit = ((numbers_storage[number]>>((3-i)*4)) & 0xF);
        if(digit || found) {
            digits[pos-(3-i)] = digit + '0';
            if(!found)
                found_pos = (3-i);
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

int prepare_base_16(u32 number, u8* digits) {
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

int digits_print(u8* digits, int max, u8 sub, u8 start_pos) {
    for(int i = NUM_DIGITS-max; i < start_pos; i++)
        if(write_char(sub))
            return 0;
    for(int i = start_pos; i < NUM_DIGITS; i++)
        if(write_char(digits[i]))
            return 0;
    return 0;
}

int write_base_10(int number, int max, u8 sub) {
    u8 digits[NUM_DIGITS];
    u8 start_pos = prepare_base_10(number, digits);
    
    return digits_print(digits, max, sub, start_pos);
}

int write_base_16(u32 number, int max, u8 sub) {
    u8 digits[NUM_DIGITS];
    u8 start_pos = prepare_base_16(number, digits);
    
    return digits_print(digits, max, sub, start_pos);
}

void add_requested_spacing(u8 prev_x_pos, u8 prev_y_pos, u8 space_increase) {
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

int fast_printf(const char * format, ... ) {
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
    return consumed;
}
