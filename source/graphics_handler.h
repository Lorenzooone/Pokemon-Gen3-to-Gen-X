#ifndef GRAPHICS_HANDLER__
#define GRAPHICS_HANDLER__

void init_gender_symbols();

void load_pokemon_sprite_gfx(u32, u32, u8);

void convert_xbpp(u8*, u32*, u16, u8*, u8, u8);
void convert_1bpp(u8*, u32*, u16, u8*, u8);
void convert_2bpp(u8*, u32*, u16, u8*, u8);
void convert_3bpp(u8*, u32*, u16, u8*, u8);

#endif