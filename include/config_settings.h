#ifndef CONFIG_SETTINGS__
#define CONFIG_SETTINGS__

#include <stddef.h>
#include "useful_qualifiers.h"

#define NUM_COLOURS 5
#define NUM_SUB_COLOURS 3
#define BACKGROUND_COLOUR_POS 0
#define FONT_COLOUR_POS 1
#define WINDOW_COLOUR_1_POS 2
#define WINDOW_COLOUR_2_POS 3
#define SPRITE_COLOUR_POS 4
#define R_SUB_INDEX 0
#define G_SUB_INDEX 1
#define B_SUB_INDEX 2

void set_default_settings(void);
void set_single_colour(u8, u8, u8);
u16 get_full_colour(u8);
u8 get_single_colour(u8, u8);

#endif
