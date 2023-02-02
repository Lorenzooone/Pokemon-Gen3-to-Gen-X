#ifndef SPRITE_HANDLER__
#define SPRITE_HANDLER__

#define BASE_Y_SPRITE_INFO_PAGE 0
#define BASE_X_SPRITE_INFO_PAGE 0

#define X_OFFSET_TRADE_MENU 0
#define Y_OFFSET_TRADE_MENU -3
#define BASE_Y_CURSOR_TRADING_MENU (16 - Y_OFFSET_TRADE_MENU)
#define BASE_X_CURSOR_TRADING_MENU (1 - X_OFFSET_TRADE_MENU)
#define BASE_Y_CURSOR_INCREMENT_TRADING_MENU 24
#define BASE_X_CURSOR_INCREMENT_TRADING_MENU 120
#define BASE_Y_SPRITE_TRADE_MENU (0 - Y_OFFSET_TRADE_MENU)
#define BASE_Y_SPRITE_INCREMENT_TRADE_MENU 24
#define BASE_X_SPRITE_TRADE_MENU (8 - X_OFFSET_TRADE_MENU)
#define CURSOR_Y_POS_CANCEL 152
#define CURSOR_X_POS_CANCEL 2

#define BASE_X_CURSOR_MAIN_MENU 2
#define BASE_Y_CURSOR_MAIN_MENU 8
#define BASE_Y_CURSOR_INCREMENT_MAIN_MENU 16

#define BASE_X_CURSOR_TRADE_OPTIONS 2
#define BASE_X_CURSOR_INCREMENT_TRADE_OPTIONS 120
#define BASE_Y_CURSOR_TRADE_OPTIONS 152

#define BASE_Y_SPRITE_OFFER_MENU 0
#define BASE_X_SPRITE_OFFER_MENU 0
#define BASE_X_CURSOR_OFFER_OPTIONS 10
#define BASE_Y_CURSOR_OFFER_OPTIONS 128
#define BASE_X_CURSOR_INCREMENT_OFFER_OPTIONS 64
#define BASE_Y_CURSOR_INCREMENT_OFFER_OPTIONS 16

void init_sprite_counter();
u8 get_sprite_counter();
void inc_sprite_counter();
u32 get_vram_pos();
void init_oam_palette();
void set_attributes(u16, u16, u16);

void init_sprites();
void init_item_icon();
void set_item_icon(u16, u16);
void set_pokemon_sprite(u32, u8, u8, u8, u8, u16, u16);
void set_party_sprite_counter();
void init_cursor();
void update_cursor_y(u16);
void update_cursor_x(u16);
void update_cursor_base_x(u16, u8);
void move_sprites(u8 counter);
void move_cursor_x(u8);
void disable_cursor();
void disable_all_cursors();
void reset_sprites(u8);
void disable_all_sprites();
void enable_all_valid_sprites();
void reset_sprites_to_cursor();
void reset_sprites_to_party();

#endif
