#ifndef SPRITE_HANDLER__
#define SPRITE_HANDLER__

#define BASE_Y_CURSOR_TRADING_MENU 16
#define BASE_X_CURSOR_TRADING_MENU 1
#define BASE_Y_CURSOR_INCREMENT_TRADING_MENU 24
#define BASE_X_CURSOR_INCREMENT_TRADING_MENU 120
#define CURSOR_Y_POS_CANCEL 152
#define CURSOR_X_POS_CANCEL 2

#define BASE_X_CURSOR_MAIN_MENU 2
#define BASE_Y_CURSOR_MAIN_MENU 8
#define BASE_Y_CURSOR_INCREMENT_MAIN_MENU 16

void init_sprite_counter();
u8 get_sprite_counter();
void inc_sprite_counter();
u32 get_vram_pos();
void init_oam_palette();
void set_attributes(u16, u16, u16);

void init_item_icon();
void set_item_icon(u16, u16);
void init_cursor(u8);
void update_cursor_y(u16);
void update_cursor_x(u16);
void update_cursor_base_x(u16, u8);
void move_sprites(u8 counter);
void move_cursor_x(u8);
void disable_cursor();
void reset_sprites_to_cursor();

#endif