#ifndef SPRITE_HANDLER__
#define SPRITE_HANDLER__

#define BASE_Y_SPRITE_INFO_PAGE 0
#define BASE_X_SPRITE_INFO_PAGE 0

#define BASE_Y_SPRITE_NATURE_PAGE 248
#define BASE_X_SPRITE_NATURE_PAGE 0

#define BASE_Y_SPRITE_IV_FIX_PAGE 248
#define BASE_X_SPRITE_IV_FIX_PAGE 0

#define X_OFFSET_TRADE_MENU 0
#define Y_OFFSET_TRADE_MENU -3
#define Y_OFFSET_TRADE_MENU_SPRITES (Y_OFFSET_TRADE_MENU + 2)
#define BASE_Y_CURSOR_TRADING_MENU (16 - Y_OFFSET_TRADE_MENU_SPRITES + 1)
#define BASE_X_CURSOR_TRADING_MENU (1 - X_OFFSET_TRADE_MENU)
#define BASE_Y_CURSOR_INCREMENT_TRADING_MENU 24
#define BASE_X_CURSOR_INCREMENT_TRADING_MENU 120
#define BASE_Y_SPRITE_TRADE_MENU (0 - Y_OFFSET_TRADE_MENU_SPRITES)
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

void init_sprites(void);
void init_sprite_counter(void);
void init_oam_palette(void);
void init_item_icon(void);
void set_pokemon_sprite(const u8*, u8, u8, u8, u8, u16, u16);
void set_party_sprite_counter(void);
void init_cursor(void);
void update_cursor_y(u16);
void update_cursor_base_x(u16);
void move_sprites(u8 counter);
void move_cursor_x(u8 counter);
void disable_cursor(void);
void disable_all_cursors(void);
void reset_sprites(u8);
void disable_all_sprites(void);
void enable_all_sprites(void);
void update_normal_oam(void);
void reset_sprites_to_cursor(void);
void reset_sprites_to_party(void);

#endif
