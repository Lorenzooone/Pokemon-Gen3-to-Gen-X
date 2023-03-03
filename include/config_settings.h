#ifndef CONFIG_SETTINGS__
#define CONFIG_SETTINGS

#include <stddef.h>

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
void set_sys_language(u8);
void set_target_int_language(u8);
void set_conversion_colo_xd(u8);
void set_default_conversion_game(u8);
void set_gen1_everstone(u8);
void set_allow_cross_gen_evos(u8);
void set_single_colour(u8, u8, u8);
void set_evolve_without_trade(u8);
void set_allow_undistributed_events(u8);
void set_fast_hatch_eggs(u8);
u8 get_sys_language(void);
u8 get_target_int_language(void);
u8 get_filtered_target_int_language(void);
u8 get_conversion_colo_xd(void);
u8 get_default_conversion_game(void);
u16 get_full_colour(u8);
u8 get_single_colour(u8, u8);
u8 get_gen1_everstone(void);
u8 get_allow_cross_gen_evos(void);
u8 get_evolve_without_trade(void);
u8 get_allow_undistributed_events(void);
u8 get_fast_hatch_eggs(void);

#endif
