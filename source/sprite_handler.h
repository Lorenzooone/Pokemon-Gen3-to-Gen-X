#ifndef SPRITE_HANDLER__
#define SPRITE_HANDLER__

void init_sprite_counter();
u8 get_sprite_counter();
void inc_sprite_counter();
u32 get_vram_pos();
void init_oam_palette();
void set_attributes(u16, u16, u16);

void init_cursor(u8);
void update_cursor_y(u16);
void update_cursor_x(u16);
void update_cursor_base_x(u16);
void move_cursor_x(u8);
void disable_cursor();
void reset_sprites_to_cursor();

#endif