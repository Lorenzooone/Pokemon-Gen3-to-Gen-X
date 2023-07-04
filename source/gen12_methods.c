#include "base_include.h"
#include "gen12_methods.h"
#include "party_handler.h"
#include "text_handler.h"
#include "fast_pokemon_methods.h"

#include "gen3_to_1_conv_table_bin.h"
#include "gen1_to_3_conv_table_bin.h"
#include "pokemon_gender_bin.h"
#include "pokemon_stats_gen1_bin.h"
#include "pokemon_stats_bin.h"

const u8* set_buffer_with_gen2(const u8*, u8, u8*);
u16 get_stat_exp_contribution(u16);

u8 stat_index_conversion_gen2[] = {0, 1, 2, 5, 3, 4};
u8 gender_thresholds_gen12[TOTAL_GENDER_KINDS] = {8, 0, 2, 4, 12, 14, 16, 17, 0, 16};
u8 gender_useful_atk_ivs[TOTAL_GENDER_KINDS] = {3, 4, 1, 2, 2, 1, 4, 4, 4, 4};

const struct stats_gen_23* stats_table_gen2 = (const struct stats_gen_23*)pokemon_stats_bin;
const struct stats_gen_1* stats_table_gen1 = (const struct stats_gen_1*)pokemon_stats_gen1_bin;

u8 has_legal_moves_gen12(u8* moves, u8 is_gen2) {
    u8 last_valid_move = LAST_VALID_GEN_1_MOVE;
    if(is_gen2)
        last_valid_move = LAST_VALID_GEN_2_MOVE;

    for(size_t i = 0; i < MOVES_SIZE; i++)
        if(is_move_valid(moves[i], last_valid_move))
            return 1;

    return 0;
}

u8 get_ivs_gen2(u16 ivs, u8 stat_index) {
    u8 atk_ivs = (ivs >> 4) & 0xF;
    u8 def_ivs = ivs & 0xF;
    u8 spe_ivs = (ivs >> 12) & 0xF;
    u8 spa_ivs = (ivs >> 8) & 0xF;
    switch(stat_index) {
        case HP_STAT_INDEX:
            return ((atk_ivs & 1) << 3) | ((def_ivs & 1) << 2) | ((spe_ivs & 1) << 1) | (spa_ivs & 1);
        case ATK_STAT_INDEX:
            return atk_ivs;
        case DEF_STAT_INDEX:
            return def_ivs;
        case SPE_STAT_INDEX:
            return spe_ivs;
        default:
            return spa_ivs;
    }
}

u8 get_unown_letter_gen2(u16 ivs){
    return get_unown_letter_gen2_fast(ivs);
}

u8 to_valid_level_gen12(u8 level) {
    if(level < MIN_LEVEL_GEN12)
        return MIN_LEVEL_GEN12;
    if(level > MAX_LEVEL_GEN12)
        return MAX_LEVEL_GEN12;
    return level;
}

u8 is_shiny_gen2(u8 atk_ivs, u8 def_ivs, u8 spa_ivs, u8 spe_ivs) {
    if((atk_ivs & 2) == 2 && def_ivs == 10 && spa_ivs == 10 && spe_ivs == 10)
        return 1;
    return 0;
}

u8 is_shiny_gen2_unfiltered(u16 ivs) {
    return is_shiny_gen2((ivs >> 4) & 0xF, ivs & 0xF, (ivs >> 8) & 0xF, (ivs >> 12) & 0xF);
}

u8 is_shiny_gen2_raw(struct gen2_mon_data* src) {
    return is_shiny_gen2_unfiltered(src->ivs);
}

u16 get_mon_index_gen1(int index) {
    if(index > LAST_VALID_GEN_1_MON)
        return 0;
    return gen3_to_1_conv_table_bin[index];
}


u16 get_mon_index_gen2_1(int index) {
    if(index > LAST_VALID_GEN_1_MON)
        return 0;
    return index;
}

u16 get_mon_index_gen2(int index, u8 is_egg) {
    if(index > LAST_VALID_GEN_2_MON)
        return 0;
    if(is_egg)
        return EGG_SPECIES;
    return index;
}

u16 get_mon_index_gen1_to_3(u8 index){
    return gen1_to_3_conv_table_bin[index];
}

u16 get_stat_exp_contribution(u16 stat_exp){
    u16 stat_exp_contribution = Sqrt(stat_exp);
    if((stat_exp_contribution * stat_exp_contribution) < stat_exp)
        stat_exp_contribution += 1;
    if(stat_exp_contribution >= 0x100)
        stat_exp_contribution = 0xFF;
    return stat_exp_contribution & 0xFF;
}

u16 calc_stats_gen1(u16 species, u8 stat_index, u8 level, u8 iv, u16 stat_exp) {
    if(species > LAST_VALID_GEN_1_MON)
        species = 0;
    species =  get_mon_index_gen1(species);
    if(stat_index >= GEN1_STATS_TOTAL)
        stat_index = GEN1_STATS_TOTAL-1;
    u16 base = 5;
    if(stat_index == HP_STAT_INDEX)
        base = level + 10;
    return base + Div((((stats_table_gen1[species].stats[stat_index] + iv)*2) + (get_stat_exp_contribution(stat_exp) >> 2)) * level, 100);
}

u16 calc_stats_gen2(u16 species, u32 pid, u8 stat_index, u8 level, u8 iv, u16 stat_exp) {
    if(species > LAST_VALID_GEN_2_MON)
        species = 0;
    if(stat_index >= GEN2_STATS_TOTAL)
        stat_index = GEN2_STATS_TOTAL-1;
    
    u16 mon_index = get_mon_index(species, pid, 0, 0);
    
    stat_index = stat_index_conversion_gen2[stat_index];
    u16 base = 5;
    if(stat_index == HP_STAT_INDEX)
        base = level + 10;
    return base + Div((((stats_table_gen2[mon_index].stats[stat_index] + iv)*2) + (get_stat_exp_contribution(stat_exp) >> 2)) * level, 100);
}

u8 get_gender_thresholds_gen12(u8 gender_kind) {
    if(gender_kind >= TOTAL_GENDER_KINDS)
        gender_kind = 0;
    return gender_thresholds_gen12[gender_kind];
}

u8 get_gender_useless_atk_ivs_gen12(u8 gender_kind) {
    if(gender_kind >= TOTAL_GENDER_KINDS)
        gender_kind = 0;
    return 4-gender_useful_atk_ivs[gender_kind];
}

u8 get_pokemon_gender_kind_gen2(u8 index, u8 is_egg, u8 curr_gen) {
    if(curr_gen == 1)
        return pokemon_gender_bin[get_mon_index_gen2_1(index)];
    return pokemon_gender_bin[get_mon_index_gen2(index, is_egg)];
}

const u8* get_pokemon_name_gen2_gen3_enc(int index, u8 is_egg, u8 language) {
    u16 mon_index = get_mon_index_gen2(index, is_egg);
    if (mon_index == MR_MIME_SPECIES)
        mon_index = MR_MIME_OLD_NAME_POS;
    if (mon_index == UNOWN_SPECIES)
        mon_index = UNOWN_REAL_NAME_POS;

    return get_pokemon_name_language(mon_index, language);
}

const u8* set_buffer_with_gen2(const u8* src_gen3_enc, u8 language, u8* buffer) {
    u8 string_cap = STRING_GEN2_INT_CAP;
    u8 is_jp = 0;
    if(language == JAPANESE_LANGUAGE) {
        string_cap = STRING_GEN2_JP_CAP;
        is_jp = 1;
    }

    text_gen3_to_gen12(src_gen3_enc, buffer, string_cap, string_cap, is_jp, is_jp);
    return buffer;
}

const u8* get_pokemon_name_gen2(int index, u8 is_egg, u8 language, u8* buffer) {
    return set_buffer_with_gen2(get_pokemon_name_gen2_gen3_enc(index, is_egg, language), language, buffer);
}

const u8* get_default_trainer_name_gen2(u8 language, u8* buffer) {
    return set_buffer_with_gen2(get_default_trainer_name(language), language, buffer);
}

u8 get_pokemon_gender_gen2(u8 index, u8 atk_ivs, u8 is_egg, u8 curr_gen) {
    u8 gender_kind = get_pokemon_gender_kind_gen2(index, is_egg, curr_gen);
    switch(gender_kind){
        case M_GENDER_INDEX:
        case NIDORAN_M_GENDER_INDEX:
            return M_GENDER;
        case F_GENDER_INDEX:
        case NIDORAN_F_GENDER_INDEX:
            return F_GENDER;
        case U_GENDER_INDEX:
            return U_GENDER;
        default:
            if(atk_ivs >= get_gender_thresholds_gen12(gender_kind))
                return M_GENDER;
            return F_GENDER;
    }
}
