#ifndef GRAPHICS_HANDLER__
#define GRAPHICS_HANDLER__

#include <stddef.h>

#define NUM_POKEMON_SPRITES 2
#define POKEMON_SPRITE_X_TILES 4
#define POKEMON_SPRITE_Y_TILES 4

#define ITEM_SPRITE_X_TILES 1
#define ITEM_SPRITE_Y_TILES 1

void load_pokemon_sprite_gfx(const u32*, u32*, u8, u8, u8, u8*);

void convert_xbpp(u8*, u32*, size_t, u8*, u8, u8);
void convert_1bpp(u8*, u32*, size_t, u8*, u8);
void convert_2bpp(u8*, u32*, size_t, u8*, u8);
void convert_3bpp(u8*, u32*, size_t, u8*, u8);

#endif
