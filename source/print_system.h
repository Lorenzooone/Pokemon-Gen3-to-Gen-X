#ifndef PRINT_SYSTEM__
#define PRINT_SYSTEM__

#define PRINT_FUNCTION fast_printf
#define X_LIMIT 0x1E
#define Y_LIMIT 0x14

int fast_printf(const char *, ...);

void init_numbers();
void default_reset_screen();
void reset_screen(u8);
void set_text_x(u8);
void set_text_y(u8);
void enable_screen(u8);
void disable_screen(u8);
void set_bg_pos(u8, int, int);
void set_screen(u8);
void init_text_system();
void create_window(u8, u8, u8, u8);
void reset_window(u8, u8, u8, u8);
u8 get_bg_priority(u8);
u8 get_curr_priority();

#endif