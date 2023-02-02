#ifndef PRINT_SYSTEM__
#define PRINT_SYSTEM__

#include "gba.h"

#define PRINT_FUNCTION fast_printf
#define X_LIMIT (SCREEN_WIDTH>>3)
#define Y_LIMIT (SCREEN_HEIGHT>>3)
#define SCREEN_REAL_WIDTH 0x100
#define SCREEN_REAL_HEIGHT 0x100
#define X_SIZE (SCREEN_REAL_WIDTH>>3)
#define Y_SIZE (SCREEN_REAL_HEIGHT>>3)

#define PALETTE 0xF

#define HSWAP_TILE 0x400
#define VSWAP_TILE 0x800

#define TOTAL_BG 4

#define REGULAR_FILL 0
#define BLANK_FILL 1

int fast_printf(const char *, ...);

void init_numbers();
void default_reset_screen();
void reset_screen(u8);
void set_text_x(u8);
void set_text_y(u8);
void enable_screen(u8);
void disable_screen(u8);
void disable_all_screens_but_current();
void set_bg_pos(u8, int, int);
u16* get_screen(u8 bg_num);
void set_screen(u8);
void init_text_system();
void prepare_flush();
void set_updated_screen();
void flush_screens();
u8 get_bg_priority(u8);
u8 get_curr_priority();

#endif
