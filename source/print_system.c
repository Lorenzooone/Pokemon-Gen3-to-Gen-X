#include <stdarg.h>
#include <gba.h>
#include "optimized_swi.h"
#include "print_system.h"
#include "text_handler.h"
#include "graphics_handler.h"

#include "text_gen3_to_general_int_bin.h"
#include "amiga_font_c_bin.h"
#include "jp_font_c_bin.h"

#define BASE_COLOUR RGB8(58,110,165)
#define FONT_COLOUR RGB5(31,31,31)

#define VRAM_START 0x6000000
#define VRAM_END (VRAM_START+0x10000)
#define PALETTE 0xF
#define PALETTE_SIZE 0x10
#define PALETTE_BASE 0x5000000
#define ARRANGEMENT_POS 0x6004000
#define TILE_SIZE 0x20
#define FONT_TILES 0x100
#define FONT_SIZE (FONT_TILES*TILE_SIZE)
#define FONT_1BPP_SIZE (FONT_SIZE>>2)
#define FONT_POS VRAM_START
#define JP_FONT_POS (FONT_POS + FONT_SIZE)
#define NUMBERS_POS (VRAM_END - (10000*2))
#define X_SIZE 0x20
#define Y_SIZE 0x20
#define X_LIMIT 0x1E

#define SCREEN_SIZE 0x800

#define CPUFASTSET_FILL (0x1000000)

#define NUM_DIGITS 12

u8 x_pos;
u8 y_pos;
u16* screen;
u8 screen_num;

void init_numbers() {
    u16* numbers_storage = (u16*)NUMBERS_POS;
    
    for(int i = 0; i < 10; i++)
        for(int j = 0; j < 10; j++)
            for(int k = 0; k < 10; k++)
                for(int l = 0; l < 10; l++)
                    numbers_storage[l + (k*10) + (j*100) + (i*1000)] = l | (k<<4) | (j<<8) | (i<<12);
}

void init_text_system() {
    REG_DISPCNT = 0;
    REG_BG0CNT = 0 | (((ARRANGEMENT_POS-VRAM_START)>>11)<<8);
    REG_BG0HOFS = 0;
    REG_BG0VOFS = 0;
    BG_COLORS[0]=BASE_COLOUR;
	BG_COLORS[(PALETTE*PALETTE_SIZE)]=BASE_COLOUR;
	BG_COLORS[(PALETTE*PALETTE_SIZE)+1]=FONT_COLOUR;
	BG_COLORS[(PALETTE*PALETTE_SIZE)+2]=BASE_COLOUR;
    set_screen(0);
    
    // This is for the first frame
    for(int i = 0; i < (TILE_SIZE>>2); i++)
        *((u32*)(FONT_POS+(i<<2))) = 0x22222222;
    
    default_reset_screen();
    enable_screen(0);
    
    u32 buffer[FONT_1BPP_SIZE>>2];
    u8 colors[] = {2, 1};
    LZ77UnCompWram(amiga_font_c_bin, buffer);
    convert_1bpp(buffer, (u32*)FONT_POS, FONT_1BPP_SIZE, colors, 1);
    PRINT_FUNCTION("\n  Loading...");
    for(int i = 0; i < (TILE_SIZE>>2); i++)
        *((u32*)(FONT_POS+TILE_SIZE+(i<<2))) = 0;
    LZ77UnCompWram(jp_font_c_bin, buffer);
    convert_1bpp(buffer, (u32*)JP_FONT_POS, FONT_1BPP_SIZE, colors, 0);
}

void enable_screen(u8 bg_num){
    REG_DISPCNT |= (0x100)<<bg_num;
}

void set_screen(u8 bg_num){
    screen_num = bg_num;
    screen = (u16*)(ARRANGEMENT_POS+(SCREEN_SIZE*bg_num));
}

void reset_screen(u8 blank_fill){
    *((u32*)screen) = (PALETTE<<28) | (PALETTE<<12) | 0;
    if(blank_fill)
        *((u32*)screen) =(PALETTE<<28) | (PALETTE<<12) | 0x00010001;
    CpuFastSet(screen, screen, CPUFASTSET_FILL | (SCREEN_SIZE>>2));
    x_pos = 0;
    y_pos = 0;
}

void default_reset_screen(){
    reset_screen(0);
}

void set_text_x(u8 new_x){
    if(new_x < X_LIMIT)
        x_pos = new_x;
}

void set_text_y(u8 new_y){
    if(new_y < Y_SIZE) {
        y_pos = new_y;
        x_pos = 0;
    }
}

void new_line(){
    x_pos = 0;
    y_pos += 1;
    if(y_pos >= Y_SIZE)
        y_pos = 0;
}

u8 write_char(u16 character) {
    screen[x_pos+(y_pos*X_SIZE)] = character | (PALETTE << 12);
    x_pos++;
    if(x_pos >= X_LIMIT) {
        new_line();
        return 1;
    }
    return 0;
}

int sub_printf(u8* string) {
    while((*string) != '\0')
        if(write_char(*(string++)))
            return 0;
}

int sub_printf_gen3(u8* string, u8 size_max, u8 is_jp) {
    u8 curr_pos = 0;
    while((*string) != GEN3_EOL) {
        if(is_jp) {
            if(write_char((*(string++))+FONT_TILES))
                return 0;
        }
        else if(write_char(text_gen3_to_general_int_bin[*(string++)]))
            return 0;
        curr_pos++;
        if(curr_pos == size_max)
            return 0;
    }
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
    for(int i = 0; i < 4; i++) {
        u8 digit = ((numbers_storage[number]>>((3-i)*4)) & 0xF);
        if(digit || found) {
            digits[pos+i-3] = digit + '0';
            found = 1;
        }
    }
    
    if(!found)
        digits[pos] = '0';
    
    if(minus) {
        digits[0] = '-';
        if(special)
            digits[NUM_DIGITS-1] += 1;
    }
    
    return 0;
}

int prepare_base_16(int number, u8* digits) {
    for(int i = 0; i < NUM_DIGITS; i++)
        digits[i] = 0;
    
    u8 pos = NUM_DIGITS-1;
    u8 digit;
    while(number >= 0x10) {
        digit = number & 0xF;
        if(digit >= 0xA)
            digit += 'A';
        else
            digit += '0';
        digits[pos--] = digit;
        number >>= 4;
    }
    
    digit = number & 0xF;
    if(digit >= 0xA)
        digit += 'A';
    else
        digit += '0';
    digits[pos--] = digit;
    number >>= 4;

    return 0;
}

int write_base_10(int number) {
    u8 digits[NUM_DIGITS];
    prepare_base_10(number, digits);
    
    for(int i = 0; i < NUM_DIGITS; i++)
        if(digits[i])
            write_char(digits[i]);
    
    return 0;
}

int write_base_16(int number) {
    u8 digits[NUM_DIGITS];
    prepare_base_16(number, digits);
    
    for(int i = 0; i < NUM_DIGITS; i++)
        if(digits[i])
            write_char(digits[i]);
    
    return 0;
}

int write_base_10_subs(int number, int max, u8 sub) {
    u8 digits[NUM_DIGITS];
    prepare_base_10(number, digits);
    
    for(int i = 0; i < NUM_DIGITS; i++)
        if(digits[i])
            write_char(digits[i]);
        else if(i >= NUM_DIGITS-max)
            write_char(sub);
    
    return 0;
}

int write_base_16_subs(int number, int max, u8 sub) {
    u8 digits[NUM_DIGITS];
    prepare_base_16(number, digits);
    
    for(int i = 0; i < NUM_DIGITS; i++)
        if(digits[i])
            write_char(digits[i]);
        else if(i >= NUM_DIGITS-max)
            write_char(sub);
    
    return 0;
}

int fast_printf(const char * format, ... ) {
    va_list va;
    va_start(va, format);
    int consumed = 0;
    u8 curr_x_pos = 0;
    while((*format) != '\0') {
        u8 character = *format;
        switch(character) {
            case '\x01':
                sub_printf(va_arg(va, u8*));
                break;
            case '\x02':
                write_char((u8)va_arg(va, int));
                break;
            case '\x03':
                write_base_10(va_arg(va, int));
                break;
            case '\x04':
                write_base_16(va_arg(va, int));
                break;
            case '\x05':
                sub_printf_gen3(va_arg(va, u8*), va_arg(va, int), va_arg(va, int));
                break;
            case '\x09':
                write_base_10_subs(va_arg(va, int), va_arg(va, int), ' ');
                break;
            case '\x0B':
                write_base_10_subs(va_arg(va, int), va_arg(va, int), '0');
                break;
            case '\x0C':
                write_base_16_subs(va_arg(va, int), va_arg(va, int), ' ');
                break;
            case '\x0D':
                write_base_16_subs(va_arg(va, int), va_arg(va, int), '0');
                break;
            case '\x11':
                curr_x_pos = x_pos;
                sub_printf(va_arg(va, u8*));
                x_pos = curr_x_pos + va_arg(va, int);
                if(x_pos >= X_LIMIT)
                    new_line();
                break;
            case '\x13':
                curr_x_pos = x_pos;
                write_base_10(va_arg(va, int));
                x_pos = curr_x_pos + va_arg(va, int);
                if(x_pos >= X_LIMIT)
                    new_line();
                break;
            case '\x14':
                curr_x_pos = x_pos;
                write_base_16(va_arg(va, int));
                x_pos = curr_x_pos + va_arg(va, int);
                if(x_pos >= X_LIMIT)
                    new_line();
                break;
            case '\x15':
                curr_x_pos = x_pos;
                sub_printf(va_arg(va, u8*));
                x_pos = curr_x_pos + va_arg(va, int);
                if(x_pos >= X_LIMIT)
                    new_line();
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
