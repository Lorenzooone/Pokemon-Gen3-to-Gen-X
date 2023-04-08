#ifndef PRINT_SYSTEM__
#define PRINT_SYSTEM__

#define PRINT_FUNCTION fast_printf
#define X_LIMIT (SCREEN_WIDTH>>3)
#define Y_LIMIT (SCREEN_HEIGHT>>3)
#define SCREEN_REAL_WIDTH 0x100
#define SCREEN_REAL_HEIGHT 0x100
#define X_SIZE (SCREEN_REAL_WIDTH>>3)
#define Y_SIZE (SCREEN_REAL_HEIGHT>>3)

#define SCREEN_HALF_X (X_LIMIT>>1)

#define PALETTE 0xF

#define HSWAP_TILE 0x400
#define VSWAP_TILE 0x800

#define TOTAL_BG 4

#define DEFAULT_BPP 4
#define BITS_PER_BYTE 8
#define TILE_X_PIXELS 8
#define TILE_Y_PIXELS 8
#define DEFAULT_TILE_ROW_SIZE ((DEFAULT_BPP*TILE_X_PIXELS)/BITS_PER_BYTE)
#define TILE_SIZE (DEFAULT_TILE_ROW_SIZE*TILE_Y_PIXELS)

#define REGULAR_FILL 0
#define BLANK_FILL 1

int fast_printf(const char *, ...);

typedef u16 screen_t;

void init_numbers(void);
void default_reset_screen(void);
void reset_screen(u8);
void set_text_x(u8);
void set_text_y(u8);
u8 get_text_x(void);
u8 get_text_y(void);
void swap_screen_enabled_state(u8);
void enable_screen(u8);
void disable_screen(u8);
void swap_buffer_screen(u8, u8);
void disable_all_screens_but_current(void);
void set_bg_pos(u8, int, int);
screen_t* get_screen(u8 bg_num);
#ifdef __NDS__
screen_t* get_screen_sub(u8 bg_num);
#endif
u8 get_screen_num(void);
void set_screen(u8);
void init_text_system(void);
void prepare_flush(void);
void set_updated_screen(void);
void flush_screens(void);
void wait_for_vblank_if_needed(void);
u8 get_bg_priority(u8);
u8 get_curr_priority(void);
u8 get_loaded_priority(void);
void set_text_palettes(void);

#endif
