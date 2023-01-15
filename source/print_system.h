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
void set_screen(u8);
void init_text_system();

#endif