#ifndef GEN12_METHODS__
#define GEN12_METHODS__

#include "party_handler.h"

u8 has_legal_moves_gen12(u8*, u8);
u8 get_ivs_gen2(u16, u8);
u8 get_unown_letter_gen2(u16);
s32 get_proper_exp_gen2(u16, u8, u8, u8*);
u8 is_shiny_gen2(u8, u8, u8, u8);
u8 is_shiny_gen2_unfiltered(u16);
u8 is_shiny_gen2_raw(struct gen2_mon_data*);
u16 get_mon_index_gen1(int);
u16 get_mon_index_gen2_1(int);
u16 get_mon_index_gen2(int, u8);
u16 get_mon_index_gen1_to_3(u8);
u16 calc_stats_gen1(u16, u8, u8, u8, u16);
u16 calc_stats_gen2(u16, u32, u8, u8, u8, u16);
u8 get_gender_thresholds_gen12(u8);
u8 get_gender_useless_atk_ivs_gen12(u8);
u8 get_pokemon_gender_gen2(u8, u8, u8, u8);
u8 get_pokemon_gender_kind_gen2(u8, u8, u8);
const u8* get_pokemon_name_gen2(int, u8, u8, u8*);

#endif
