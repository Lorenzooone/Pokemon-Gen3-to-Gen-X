#ifndef SPRITE_HANDLER__
#define SPRITE_HANDLER__

#include "print_system.h"
#include "graphics_handler.h"

#define TOP_SCREEN_SPRITE_POS SCREEN_REAL_HEIGHT
#define LEFT_SCREEN_SPRITE_POS SCREEN_REAL_WIDTH

#define BASE_X_CURSOR_MAIN_MENU 2
#define BASE_Y_CURSOR_MAIN_MENU 0
#define BASE_Y_CURSOR_INCREMENT_MAIN_MENU 8

void init_sprites(void);
void enable_sprites_rendering(void);
void disable_sprites_rendering(void);
void init_sprite_counter(void);
void init_oam_palette(void);
void init_item_icon(void);
void set_party_sprite_counter(void);
void init_cursor(void);
void update_cursor_y(u16);
void update_cursor_base_x(u16);
void raw_update_sprite_y(u8, u8);
void move_sprites(u8 counter);
void move_cursor_x(u8 counter);
void disable_cursor(void);
void disable_all_cursors(void);
u8 get_next_sprite_index(void);
void reset_sprites(u8);
void disable_all_sprites(void);
void enable_all_sprites(void);
void update_normal_oam(void);
void reset_sprites_to_cursor(u8);
void reset_sprites_to_party(void);
void fade_all_sprites_to_white(u16);
void remove_fade_all_sprites(void);
void set_cursor_palette(void);

#endif
