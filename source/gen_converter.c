#include <gba.h>
#include "party_handler.h"
#include "gen12_methods.h"
#include "gen_converter.h"
#include "text_handler.h"
#include "version_identifier.h"
#include "bin_table_handler.h"
#include "gen3_save.h"
#include "rng.h"
#include "pid_iv_tid.h"
#include "fast_pokemon_methods.h"
#include "optimized_swi.h"
#include "useful_qualifiers.h"
#include <stddef.h>

#include "pokemon_moves_pp_bin.h"
#include "item_gen3_to_12_bin.h"
#include "item_gen12_to_3_bin.h"
#include "pokemon_types_gen1_bin.h"
#include "encounter_types_bin.h"
#include "egg_locations_bin.h"
#include "encounters_static_bin.h"
#include "encounters_roamers_bin.h"
#include "encounters_unown_bin.h"
#include "encounter_types_gen2_bin.h"
#include "special_encounters_gen2_bin.h"

#define MF_7_1_INDEX 2
#define UNOWN_EX_LETTER 26
#define UNOWN_I_LETTER 8
#define UNOWN_V_LETTER 21

#define ROAMER_ROUTES_START_RSE 16
#define ROAMER_ROUTES_END_RSE 49
#define ROAMER_ROUTES_START_FRLG 101
#define ROAMER_ROUTES_END_FRLG 125
#define ROAMER_LEVEL_RSE 40
#define ROAMER_LEVEL_FRLG 50

u16 swap_endian_short(u16);
u32 swap_endian_int(u32);
u8 validate_converting_mon_of_gen1(u8, struct gen1_mon_data*);
u8 validate_converting_mon_of_gen2(u8, struct gen2_mon_data*, u8*);
u8 validate_converting_mon_of_gen3(struct gen3_mon*, struct gen3_mon_growth*, u8, u8, u8, u8, u8);
u8 convert_moves_of_gen3(struct gen3_mon_attacks*, u8, u8*, u8*, u8);
u8 convert_moves_to_gen3(struct gen3_mon_attacks*, struct gen3_mon_growth*, u8*, u8*, u8);
u8 convert_item_of_gen3(u16);
u16 convert_item_to_gen3(u16);
void convert_exp_nature_of_gen3(struct gen3_mon*, struct gen3_mon_growth*, u8*, u8*, u8);
u8 get_exp_nature(struct gen3_mon*, struct gen3_mon_growth*, u8, u8*);
void convert_evs_of_gen3(struct gen3_mon_evs*, u16*);
void convert_evs_to_gen3(struct gen3_mon_evs*, u16*);
u8 get_encounter_type_gen3(u16);
u8 are_roamer_ivs(struct gen3_mon_misc*);
u8 is_roamer_frlg(u16, u8, u8, u8);
u8 is_roamer_rse(u16, u8, u8, u8);
u8 is_roamer_gen3(struct gen3_mon_misc*, u16);
u16 convert_ivs_of_gen3(struct gen3_mon_misc*, u16, u32, u8, u8, u8, u8, u8);
void set_ivs(struct gen3_mon_misc*, u32);
void preset_alter_data(struct gen3_mon_data_unenc*, struct alternative_data_gen3*);
void set_origin_pid_iv(struct gen3_mon*, struct gen3_mon_misc*, u16, u16, u8, u8, u8, u8);
u8 are_trainers_same(struct gen3_mon*, u8);
void fix_name_change_from_gen3(struct gen3_mon*, u16,  u8*, u8, u8);
void fix_name_change_to_gen3(struct gen3_mon*, u8);
void convert_strings_of_gen3(struct gen3_mon*, u16, u8*, u8*, u8*, u8*, u8, u8);
void convert_strings_of_gen12(struct gen3_mon*, u8, u8*, u8*, u8);

u16 swap_endian_short(u16 shrt) {
    return ((shrt & 0xFF00) >> 8) | ((shrt & 0xFF) << 8);
}

u32 swap_endian_int(u32 integer) {
    return ((integer & 0xFF000000) >> 24) | ((integer & 0xFF) << 24) | ((integer & 0xFF0000) >> 8) | ((integer & 0xFF00) << 8);
}

u8 validate_converting_mon_of_gen1(u8 index, struct gen1_mon_data* src) {
    
    // Check for matching index to species
    if(index != src->species)
        return 0;
    
    u8 conv_species = get_mon_index_gen1_to_3(src->species);
    
    // Is this a valid mon
    if((conv_species > LAST_VALID_GEN_1_MON) || (conv_species == 0))
        return 0;
    
    // Does it have a valid movepool
    if(!has_legal_moves_gen12(src->moves, 0))
        return 0;
    
    // Check for valid types
    u8 matched[2] = {0,0};
    for(int i = 0; i < 2; i++) {
        if((src->type[i] == pokemon_types_gen1_bin[(2*src->species)]) && (!matched[0]))
            matched[0] = 1;
        else if((src->type[i] == pokemon_types_gen1_bin[(2*src->species)+1]) && (!matched[1]))
            matched[1] = 1;
    }
    for(int i = 0; i < 2; i++)
        if(!matched[i])
            return 0;
    
    return 1;
}

u8 validate_converting_mon_of_gen2(u8 index, struct gen2_mon_data* src, u8* is_egg) {
    if(index == GEN2_EGG)
        *is_egg = 1;
    else
        *is_egg = 0;
    
    // Check for matching index to species
    if((!(*is_egg)) && (index != src->species))
        return 0;
    
    // Is this a valid mon
    if((src->species > LAST_VALID_GEN_2_MON) || (src->species == 0))
        return 0;
    
    // Does it have a valid movepool
    if(!has_legal_moves_gen12(src->moves, 1))
        return 0;
    
    return 1;
}

u8 validate_converting_mon_of_gen3(struct gen3_mon* src, struct gen3_mon_growth* growth, u8 is_shiny, u8 gender, u8 gender_kind, u8 is_egg, u8 is_gen2) {
    u8 last_valid_mon = LAST_VALID_GEN_1_MON;
    if(is_gen2)
        last_valid_mon = LAST_VALID_GEN_2_MON;

    // Bad egg checks
    if(src->is_bad_egg)
        return 0;
    
    // Item checks
    if(has_mail(src, growth, is_egg))
        return 0;
    
    // Validity checks
    if((growth->species > last_valid_mon) || (growth->species == 0))
        return 0;
    
    // These Pokemon cannot be both female and shiny in gen 1/2
    if(is_shiny && (gender == F_GENDER) && (gender_kind == MF_7_1_INDEX))
        return 0;
    
    // Unown ! and ? did not exist in gen 2
    // Not only that, but only Unown I and V can be shiny
    if(is_gen2 && (growth->species == UNOWN_SPECIES)) {
        u8 letter = get_unown_letter_gen3(src->pid);
        if(letter >= UNOWN_EX_LETTER)
            return 0;
        if(is_shiny && (letter != UNOWN_I_LETTER) && (letter != UNOWN_V_LETTER))
            return 0;
    }
    
    // Eggs cannot be traded to gen 1
    if((!is_gen2) && is_egg)
        return 0;
    
    return 1;
}

u8 convert_moves_of_gen3(struct gen3_mon_attacks* attacks, u8 pp_bonuses, u8* moves, u8* pps, u8 is_gen2) {
    // Start converting moves
    u16 last_valid_move = LAST_VALID_GEN_1_MOVE;
    if(is_gen2)
        last_valid_move = LAST_VALID_GEN_2_MOVE;
    
    u8 used_slots = 0;
    
    for(size_t i = 0; i < MOVES_SIZE; i++)
    {
        moves[i] = 0;
        pps[i] = 0;
    }
    for(size_t i = 0; i < MOVES_SIZE; i++) {
        u16 move = attacks->moves[i];
        if((move > 0) && (move <= last_valid_move)) {
            u8 base_pp = pokemon_moves_pp_bin[move];
            u8 bonus_pp = (pp_bonuses >> (2*i)) & 3;
            u8 base_increase_pp = Div(base_pp, 5);
            base_pp += (base_increase_pp * bonus_pp);
            // Limit the PP to its maximum of 61
            if(base_pp > 61)
                base_pp = 61;
            base_pp |= (bonus_pp << 6);
            pps[used_slots] = base_pp;
            moves[used_slots++] = move;
        }
    }
    
    return used_slots;
}

u8 convert_moves_to_gen3(struct gen3_mon_attacks* attacks, struct gen3_mon_growth* growth, u8* moves, u8* pps, u8 is_gen2) {
    // Start converting moves
    u8 last_valid_move = LAST_VALID_GEN_1_MOVE;
    if(is_gen2)
        last_valid_move = LAST_VALID_GEN_2_MOVE;
    
    u8 used_slots = 0;

    for(size_t i = 0; i < MOVES_SIZE; i++) {
        u16 move = moves[i];
        if((move > 0) && (move <= last_valid_move)) {
            u8 base_pp = pokemon_moves_pp_bin[move];
            u8 bonus_pp = (pps[i] >> 6) & 3;
            u8 base_increase_pp = Div(base_pp, 5);
            base_pp += (base_increase_pp * bonus_pp);
            
            growth->pp_bonuses |= (bonus_pp)<<(2*i);
            attacks->pp[used_slots] = base_pp;
            attacks->moves[used_slots++] = move;
        }
    }

    return used_slots;
}

u8 convert_item_of_gen3(u16 item) {
    if(item > LAST_VALID_GEN_3_ITEM)
        item = 0;
    item = item_gen3_to_12_bin[item];
    if((item == GEN2_NO_ITEM) || (item == GEN2_MAIL))
        item = 0;
    return item;
}

u16 convert_item_to_gen3(u16 item) {
    item = item_gen12_to_3_bin[item*2] + (item_gen12_to_3_bin[(item*2)+1]<<8);
    if(item == GEN3_NO_ITEM)
        item = 0;
    return item;
}

void convert_exp_nature_of_gen3(struct gen3_mon* src, struct gen3_mon_growth* growth, u8* level_ptr, u8* exp_ptr, u8 is_gen2) {
    if(((is_gen2) && (growth->species > LAST_VALID_GEN_2_MON)) || ((!is_gen2) && (growth->species > LAST_VALID_GEN_1_MON)))
        return;
    
    // Level handling
    u8 level = to_valid_level_gen3(src);
    
    u16 mon_index = get_mon_index(growth->species, src->pid, 0, 0);
    
    // Experience handling
    s32 exp = get_proper_exp(src, growth, 0);
    
    s32 max_exp = get_level_exp_mon_index(mon_index, level);
    if(level < MAX_LEVEL)
        max_exp = get_level_exp_mon_index(mon_index, level+1)-1;
    
    // Save nature in experience, like the Gen I-VII conversion
    u8 nature = get_nature(src->pid);

    // Nature handling
    u8 exp_nature = DivMod(exp, NUM_NATURES);
    if(exp_nature > nature)
        nature += NUM_NATURES;
    exp += nature - exp_nature;
    if(level == MAX_LEVEL)
        exp = max_exp;
    if(level < MAX_LEVEL)
        while(exp > max_exp) {
            level++;
            if(level == MAX_LEVEL) {
                exp = max_exp+1;
                break;
            }
            max_exp = get_level_exp_mon_index(mon_index, level+1)-1;
        }
    /*
    if ((level == MAX_LEVEL) && (exp != get_level_exp_mon_index(mon_index, MAX_LEVEL))){
        level--;
        exp -= NUM_NATURES;
    }
    */

    // Store exp and level
    *level_ptr = level;
    for(int i = 0; i < 3; i++)
        exp_ptr[2-i] = (exp >> (8*i))&0xFF;
}

u8 get_exp_nature(struct gen3_mon* dst, struct gen3_mon_growth* growth, u8 level, u8* given_exp) {    
    // Level handling
    level = to_valid_level(level);
    
    u16 mon_index = get_mon_index_gen2(growth->species, 0);
    
    // Experience handling
    s32 exp = get_proper_exp_gen2(mon_index, level, given_exp);
    
    // Save nature in experience, like the Gen I-VII conversion
    u8 nature = SWI_DivMod(exp, NUM_NATURES);

    // Store exp and level
    dst->level = level;
    growth->exp = exp;
    return nature;
}

void convert_evs_of_gen3(struct gen3_mon_evs* evs, u16* old_evs) {
    // Convert to Gen1/2 EVs
    u16 evs_total = 0;
    for(int i = 0; i < EVS_TOTAL_GEN3; i++)
        evs_total += evs->evs[i];
    if(evs_total >= MAX_USABLE_EVS)
        evs_total = MAX_EVS;
    u16 new_evs = ((evs_total+1)>>1) * ((evs_total+1)>>1);
    for(int i = 0; i < EVS_TOTAL_GEN12; i++)
        old_evs[i] = swap_endian_short(new_evs & 0xFFFF);
}

void convert_evs_to_gen3(struct gen3_mon_evs* evs, u16* UNUSED(old_evs)) {
    // A direct conversion is possible, but it would be
    // atrocious for competitive/high-level battling...
    for(int i = 0; i < EVS_TOTAL_GEN3; i++)
        evs->evs[i] = 0;
}

u8 get_encounter_type_gen3(u16 pure_species) {
    if(pure_species > LAST_VALID_GEN_3_MON)
        pure_species = 0;
    return (encounter_types_bin[pure_species>>2]>>(2*(pure_species&3)))&3;
}

u8 are_roamer_ivs(struct gen3_mon_misc* misc) {
    return (misc->atk_ivs <= 7) && (misc->def_ivs == 0) && (misc->spa_ivs == 0) && (misc->spd_ivs == 0) && (misc->spe_ivs == 0);
}

u8 is_roamer_frlg(u16 species, u8 origin_game, u8 met_location, u8 met_level) {
    return ((species >= RAIKOU_SPECIES) && (species <= SUICUNE_SPECIES)) && ((origin_game == FR_VERSION_ID) || (origin_game == LG_VERSION_ID)) && ((met_location >= ROAMER_ROUTES_START_FRLG) && (met_location <= ROAMER_ROUTES_END_FRLG)) && (met_level == ROAMER_LEVEL_FRLG);
}

u8 is_roamer_rse(u16 species, u8 origin_game, u8 met_location, u8 met_level) {
    if((species == LATIAS_SPECIES) && ((origin_game == S_VERSION_ID) || (origin_game == E_VERSION_ID)) && ((met_location >= ROAMER_ROUTES_START_RSE) && (met_location <= ROAMER_ROUTES_END_RSE)) && (met_level == ROAMER_LEVEL_RSE))
        return 1;
    if((species == LATIOS_SPECIES) && ((origin_game == R_VERSION_ID) || (origin_game == E_VERSION_ID)) && ((met_location >= ROAMER_ROUTES_START_RSE) && (met_location <= ROAMER_ROUTES_END_RSE)) && (met_level == ROAMER_LEVEL_RSE))
        return 1;
    return 0;
}

u8 is_roamer_gen3(struct gen3_mon_misc* misc, u16 species) {
    u8 origin_game = (misc->origins_info>>7)&0xF;
    u8 met_level = (misc->origins_info)&0x7F;
    return (get_encounter_type_gen3(species) == ROAMER_ENCOUNTER) && are_roamer_ivs(misc) && (is_roamer_frlg(species, origin_game, misc->met_location, met_level) || is_roamer_rse(species, origin_game, misc->met_location, met_level));
}

u16 convert_ivs_of_gen3(struct gen3_mon_misc* misc, u16 species, u32 pid, u8 is_shiny, u8 gender, u8 gender_kind, u8 is_gen2, u8 skip_checks) {
    if(!skip_checks)
        if(((is_gen2) && (species > LAST_VALID_GEN_2_MON)) || ((!is_gen2) && (species > LAST_VALID_GEN_1_MON)))
            return 0;
    
    // Prepare gender related data
    u8 gender_threshold = get_gender_thresholds_gen12(gender_kind);
    u8 gender_useless_ivs = get_gender_useless_atk_ivs_gen12(gender_kind);
        
    // Assign IVs
    // Keep in mind: Unown letter, gender and shinyness
    // Hidden Power calculations are too restrictive
    u8 atk_ivs = ((((misc->atk_ivs >> 1)>>gender_useless_ivs) & ((1<<(4-gender_useless_ivs))-1)) | ((misc->atk_ivs >> 1)<<(4-gender_useless_ivs))) & 0xF;
    u8 def_ivs = misc->def_ivs >> 1;
    u8 spa_ivs = (misc->spa_ivs + misc->spd_ivs) >> 2;
    u8 spe_ivs = misc->spe_ivs >> 1;
    
    // Handle roamers losing IVs when caught
    if(is_roamer_gen3(misc, species)) {
        u32 real_ivs = 0;
        if(get_roamer_ivs(pid, misc->hp_ivs, misc->atk_ivs, &real_ivs)){
            atk_ivs = ((real_ivs >> 5) & 0x1F) >> 1;
            def_ivs = ((real_ivs >> 10) & 0x1F) >> 1;
            spa_ivs = (((real_ivs >> 20) & 0x1F) + ((real_ivs >> 25) & 0x1F)) >> 2;
            spe_ivs = ((real_ivs >> 15) & 0x1F) >> 1;
        }
    }

    // Unown letter
    if((is_gen2) && (species == UNOWN_SPECIES)) {
        u8 letter = get_unown_letter_gen3(pid);
        u8 min_iv_sum = letter * 10;
        u16 max_iv_sum = ((letter+1) * 10)-1;
        if(max_iv_sum >= 0x100)
            max_iv_sum = 0xFF;
        u8 iv_sum = ((spa_ivs & 0x6) >> 1) | ((spe_ivs & 0x6) << 1) | ((def_ivs & 0x6) << 3) | ((atk_ivs & 0x6) << 5);
        if(iv_sum < min_iv_sum)
            iv_sum = min_iv_sum;
        if(iv_sum > max_iv_sum)
            iv_sum = max_iv_sum;
        atk_ivs = (atk_ivs & 0x9) | ((iv_sum >> 5) & 0x6);
        def_ivs = (def_ivs & 0x9) | ((iv_sum >> 3) & 0x6);
        spe_ivs = (spe_ivs & 0x9) | ((iv_sum >> 1) & 0x6);
        spa_ivs = (spa_ivs & 0x9) | ((iv_sum << 1) & 0x6);
    }
    
    // Gender
    if((gender_threshold > 0) && (gender_threshold < 16)) {
        if(gender == F_GENDER) {
            if(atk_ivs >= gender_threshold) {
                u8 is_power_of_2 = 0;
                for(int i = 0; i < 4; i++)
                    if(gender_threshold == (1 << i)) {
                        is_power_of_2 = 1;
                        break;
                    }
                if(is_power_of_2)
                   atk_ivs &= (gender_threshold - 1);
                else {
                    if(is_shiny && (gender_useless_ivs > (4-2)))
                        gender_useless_ivs = 4-2;
                    atk_ivs &= ~(1<<(4-gender_useless_ivs));
                }
            }
        }
        else
            atk_ivs |= gender_threshold;
    }
    
    // Shinyness
    if(is_shiny) {
        atk_ivs = atk_ivs | 2;
        def_ivs = 10;
        spa_ivs = 10;
        spe_ivs = 10;
    }

    if(!is_shiny && is_shiny_gen2(atk_ivs, def_ivs, spa_ivs, spe_ivs))
        spe_ivs = 11;
    
    return (atk_ivs << 4) | def_ivs | (spa_ivs << 8) | (spe_ivs << 12);
}

void set_ivs(struct gen3_mon_misc* misc, u32 ivs) {
    misc->hp_ivs = (ivs >> 0) & 0x1F;
    misc->atk_ivs = (ivs >> 5) & 0x1F;
    misc->def_ivs = (ivs >> 10) & 0x1F;
    misc->spe_ivs = (ivs >> 15) & 0x1F;
    misc->spa_ivs = (ivs >> 20) & 0x1F;
    misc->spd_ivs = (ivs >> 25) & 0x1F;
}

void preset_alter_data(struct gen3_mon_data_unenc* data_src, struct alternative_data_gen3* alternative_data) {
    alternative_data->met_location = data_src->misc.met_location;
    alternative_data->origins_info = data_src->misc.origins_info;
    alternative_data->pid = data_src->src->pid;
    alternative_data->ivs = (data_src->misc.hp_ivs) | (data_src->misc.atk_ivs << 5) | (data_src->misc.def_ivs << 10) | (data_src->misc.spe_ivs << 15) | (data_src->misc.spa_ivs << 20) | (data_src->misc.spd_ivs << 25);
    alternative_data->ability = data_src->misc.ability;
    alternative_data->ot_id = data_src->src->ot_id;
}

void set_alter_data(struct gen3_mon_data_unenc* data_src, struct alternative_data_gen3* alternative_data) {
    data_src->misc.met_location = alternative_data->met_location;
    data_src->misc.origins_info = alternative_data->origins_info;
    data_src->src->pid = alternative_data->pid;
    set_ivs(&data_src->misc, alternative_data->ivs);
    data_src->misc.ability = alternative_data->ability;
    data_src->src->ot_id = alternative_data->ot_id;
    
    // Place all the substructures' data
    place_and_encrypt_gen3_data(data_src, data_src->src);
    
    // Calculate stats
    recalc_stats_gen3(data_src, data_src->src);
}

void alter_nature(struct gen3_mon_data_unenc* data_src, u8 wanted_nature) {
    preset_alter_data(data_src, &data_src->alter_nature);
    
    // Don't edit eggs or invalid
    if((!data_src->is_valid_gen3) || (data_src->is_egg))
        return;
    
    // Normalize nature
    SWI_DivMod(wanted_nature, NUM_NATURES);
    
    if(wanted_nature == get_nature(data_src->src->pid))
        return;
    
    // Prepare generic data
    u16 species = data_src->growth.species;
    u32 ot_id = data_src->alter_nature.ot_id;
    u8 is_shiny = is_shiny_gen3_raw(data_src, ot_id);
    u8 gender = get_pokemon_gender_raw(data_src);
    u8 gender_kind = get_pokemon_gender_kind_gen3_raw(data_src);

    // Prepare extra needed data
    u16 wanted_ivs = convert_ivs_of_gen3(&data_src->misc, species, data_src->alter_nature.pid, is_shiny, gender, gender_kind, 1, 1);
    u8 is_ability_set = 0;
    
    // Get encounter type
    u8 encounter_type = get_encounter_type_gen3(species);
    u8 origin_game = (data_src->misc.origins_info>>7)&0xF;

    // Prepare the TSV
    u16 tsv = (ot_id & 0xFFFF) ^ (ot_id >> 16);
    
    // Prepare pointers
    u32* pid_ptr = &data_src->alter_nature.pid;
    u32* ivs_ptr = &data_src->alter_nature.ivs;
    u8* ability_ptr = &data_src->alter_nature.ability;

    // Get PID and IVs
    switch(encounter_type) {
        case STATIC_ENCOUNTER:
            if(!is_shiny)
                generate_static_info(wanted_nature, wanted_ivs, tsv, pid_ptr, ivs_ptr);
            else
                generate_static_shiny_info(wanted_nature, tsv, pid_ptr, ivs_ptr);
            break;
        case ROAMER_ENCOUNTER:
            if(origin_game == COLOSSEUM_CODE) {
                if(!is_shiny)
                    generate_generic_genderless_shadow_info_colo(wanted_nature, wanted_ivs, tsv, pid_ptr, ivs_ptr, ability_ptr);
                else
                    generate_generic_genderless_shadow_shiny_info_colo(wanted_nature, tsv, pid_ptr, ivs_ptr, ability_ptr);
                is_ability_set = 1;
            }
            else {
                if(!is_shiny)
                    generate_static_info(wanted_nature, wanted_ivs, tsv, pid_ptr, ivs_ptr);
                else
                    generate_static_shiny_info(wanted_nature, tsv, pid_ptr, ivs_ptr);
                // Roamers only get the first byte of their IVs
                if(is_roamer_gen3(&data_src->misc, species))
                    data_src->alter_nature.ivs &= 0xFF;
            }
            break;
        case UNOWN_ENCOUNTER:
            if(!is_shiny)
                generate_unown_info_letter_preloaded(wanted_nature, wanted_ivs, get_unown_letter_gen3(data_src->alter_nature.pid), tsv, pid_ptr, ivs_ptr);
            else
                generate_unown_shiny_info_letter_preloaded(wanted_nature, get_unown_letter_gen3(data_src->alter_nature.pid), tsv, pid_ptr, ivs_ptr);
            break;
        default:
            if(!is_shiny)
                generate_egg_info(species, wanted_nature, wanted_ivs, tsv, 2, pid_ptr, ivs_ptr);
            else
                generate_egg_shiny_info(species, wanted_nature, wanted_ivs, tsv, 2, pid_ptr, ivs_ptr);
            break;
    }
    
    // Set ability
    if((is_ability_set && (*ability_ptr)) || ((!is_ability_set) && ((*pid_ptr) & 1))) {
        u16 abilities = get_possible_abilities_pokemon(species, *pid_ptr, data_src->is_egg, data_src->deoxys_form);
        u8 abilities_same = (abilities&0xFF) == ((abilities>>8)&0xFF);
        if(!abilities_same)
            *ability_ptr = 1;
        else
            *ability_ptr = 0;
    }
    else
        *ability_ptr = 0;
}

void set_origin_pid_iv(struct gen3_mon* dst, struct gen3_mon_misc* misc, u16 species, u16 wanted_ivs, u8 wanted_nature, u8 ot_gender, u8 is_egg, u8 no_restrictions) {
    struct game_data_t* trainer_data = get_own_game_data();
    u8 trainer_game_version = id_to_version(&trainer_data->game_identifier);
    u8 trainer_gender = trainer_data->trainer_gender;
    
    // Handle eggs separately
    u32 ot_id = dst->ot_id;
    if(is_egg)
        ot_id = trainer_data->trainer_id;
    
    // Prepare the TSV
    u16 tsv = (ot_id & 0xFFFF) ^ (ot_id >> 16);

    u8 chosen_version = FR_VERSION_ID;
    if(!no_restrictions) {
        chosen_version = trainer_game_version;
        ot_gender = trainer_gender;
    }

    u8 encounter_type = get_encounter_type_gen3(species);
    u8 is_shiny = is_shiny_gen2_unfiltered(wanted_ivs);
    u32 ivs = 0;
    u8 ability = 0;
    u8 is_ability_set = 0;
    const u8* searchable_table = egg_locations_bin;
    u8 find_in_table = 0;
    
    // Get PID and IVs
    switch(encounter_type) {
        case STATIC_ENCOUNTER:
            if(!is_shiny)
                generate_static_info(wanted_nature, wanted_ivs, tsv, &dst->pid, &ivs);
            else
                generate_static_shiny_info(wanted_nature, tsv, &dst->pid, &ivs);
            searchable_table = encounters_static_bin;
            find_in_table = 1;
            break;
        case ROAMER_ENCOUNTER:
            // Prefer Colosseum/XD encounter, if possible
            if(are_colo_valid_tid_sid(ot_id & 0xFFFF, ot_id >> 0x10)) {
                chosen_version = COLOSSEUM_CODE;
                if(!is_shiny)
                    generate_generic_genderless_shadow_info_colo(wanted_nature, wanted_ivs, tsv, &dst->pid, &ivs, &ability);
                else
                    generate_generic_genderless_shadow_shiny_info_colo(wanted_nature, tsv, &dst->pid, &ivs, &ability);
                is_ability_set = 1;
            }
            else {
                if(!is_shiny)
                    generate_static_info(wanted_nature, wanted_ivs, tsv, &dst->pid, &ivs);
                else
                    generate_static_shiny_info(wanted_nature, tsv, &dst->pid, &ivs);
                // Roamers only get the first byte of their IVs
                ivs &= 0xFF;
            }
            searchable_table = encounters_roamers_bin;
            find_in_table = 1;
            break;
        case UNOWN_ENCOUNTER:
            if(!is_shiny)
                generate_unown_info(wanted_nature, wanted_ivs, tsv, &dst->pid, &ivs);
            else
                generate_unown_shiny_info(wanted_nature, wanted_ivs, tsv, &dst->pid, &ivs);
            searchable_table = encounters_unown_bin;
            find_in_table = 1;
            break;
        default:
            if(!is_shiny)
                generate_egg_info(species, wanted_nature, wanted_ivs, tsv, 2, &dst->pid, &ivs);
            else
                generate_egg_shiny_info(species, wanted_nature, wanted_ivs, tsv, 2, &dst->pid, &ivs);
            break;
    }
    
    // Set met location and origins info
    u8 met_location = egg_locations_bin[chosen_version];
    u8 met_level = 0;
    
    if(find_in_table) {
        u16 mon_index = get_mon_index(species, dst->pid, 0, 0);
        const u8* possible_met_data = search_table_for_index(searchable_table, mon_index);
        if(possible_met_data != NULL) {
            u8 num_elems = possible_met_data[0];
            u8 chosen_entry = 0;
            for(int i = 0; i < num_elems; i++)
                if(possible_met_data[1+(3*i)] == chosen_version)
                    chosen_entry = i;
            chosen_version = possible_met_data[1+(3*chosen_entry)];
            met_location = possible_met_data[2+(3*chosen_entry)];
            met_level = possible_met_data[3+(3*chosen_entry)];
        }
    }
    
    misc->met_location = met_location;
    misc->origins_info = ((ot_gender&1)<<15) | ((POKEBALL_ID&0xF)<<11) | ((chosen_version&0xF)<<7) | ((met_level&0x7F)<<0);
    
    // Set IVs
    set_ivs(misc, ivs);
    
    // Set ability
    if((is_ability_set && ability) || ((!is_ability_set) && (dst->pid & 1))) {
        u16 abilities = get_possible_abilities_pokemon(species, dst->pid, 0, 0);
        u8 abilities_same = (abilities&0xFF) == ((abilities>>8)&0xFF);
        if(!abilities_same)
            misc->ability = 1;
        else
            misc->ability = 0;
    }
    else
        misc->ability = 0;
}

u8 are_trainers_same(struct gen3_mon* dst, u8 is_jp) {
    struct game_data_t* trainer_data = get_own_game_data();
    u8 trainer_is_jp = trainer_data->game_identifier.game_is_jp;
    u32 trainer_id = trainer_data->trainer_id;
    u8* trainer_name = trainer_data->trainer_name;
    
    // Languages do not match
    if(is_jp ^ trainer_is_jp)
        return 0;
    
    // IDs do not match
    if((trainer_id & 0xFFFF) != (dst->ot_id & 0xFFFF))
        return 0;
    
    // Return whether the OT names are the same
    return text_gen3_is_same(dst->ot_name, trainer_name, OT_NAME_GEN3_SIZE, OT_NAME_GEN3_SIZE);
    
}

void fix_name_change_from_gen3(struct gen3_mon* src, u16 species,  u8* nickname, u8 is_egg, u8 is_gen2) {
    u8 tmp_text_buffer[NAME_SIZE];
    
    // Get the string to compare to
    text_generic_to_gen3(get_pokemon_name(species, src->pid, is_egg, 0), tmp_text_buffer, NAME_SIZE, NICKNAME_GEN3_SIZE, 0, 0);
    
    // If it's the same, update the nickname with the new one
    if(text_gen3_is_same(src->nickname, tmp_text_buffer, NICKNAME_GEN3_SIZE, NICKNAME_GEN3_SIZE)) {
        text_gen2_copy(get_pokemon_name_gen2(species, is_egg, 0, tmp_text_buffer), nickname, STRING_GEN2_INT_CAP, STRING_GEN2_INT_CAP);
        // Gen 1 used the wrong dot symbol
        if(!is_gen2)
            text_gen2_replace(nickname, STRING_GEN2_INT_CAP, GEN2_DOT, GEN1_DOT);
    }
}

void fix_name_change_to_gen3(struct gen3_mon* dst, u8 species) {
    u8 tmp_text_buffer[STRING_GEN2_INT_CAP];
    u8 tmp_text_buffer2[NICKNAME_GEN3_SIZE];
    
    // Get the string to compare to
    text_gen12_to_gen3(get_pokemon_name_gen2(species, 0, 0, tmp_text_buffer), tmp_text_buffer2, STRING_GEN2_INT_CAP, NICKNAME_GEN3_SIZE, 0, 0);
    
    // If it's the same, update the nickname with the new one
    if(text_gen3_is_same(dst->nickname, tmp_text_buffer2, NICKNAME_GEN3_SIZE, NICKNAME_GEN3_SIZE))
        text_generic_to_gen3(get_pokemon_name(species, 0, 0, 0), dst->nickname, NAME_SIZE, NICKNAME_GEN3_SIZE, 0, 0);
}

void convert_strings_of_gen3(struct gen3_mon* src, u16 species, u8* ot_name, u8* ot_name_jp, u8* nickname, u8* nickname_jp, u8 is_egg, u8 is_gen2) {
    u8 gen2_buffer[STRING_GEN2_INT_SIZE];
    u8 is_jp = (src->language == JAPANESE_LANGUAGE);
    
    // Text conversions
    text_gen2_terminator_fill(ot_name, STRING_GEN2_INT_SIZE);
    text_gen2_terminator_fill(ot_name_jp, STRING_GEN2_JP_SIZE);
    text_gen2_terminator_fill(nickname, STRING_GEN2_INT_SIZE);
    text_gen2_terminator_fill(nickname_jp, STRING_GEN2_JP_SIZE);
    
    text_gen3_to_gen12(src->ot_name, ot_name, OT_NAME_GEN3_SIZE, STRING_GEN2_INT_CAP, is_jp, 0);
    text_gen3_to_gen12(src->ot_name, ot_name_jp, OT_NAME_GEN3_SIZE, STRING_GEN2_JP_CAP, is_jp, 1);
    text_gen3_to_gen12(src->nickname, nickname, NICKNAME_GEN3_SIZE, STRING_GEN2_INT_CAP, is_jp, 0);
    text_gen3_to_gen12(src->nickname, nickname_jp, NICKNAME_GEN3_SIZE, STRING_GEN2_JP_CAP, is_jp, 1);
    
    // Fix text up
    // "MR.MIME" gen 2 == "MR. MIME" gen 3
    // Idk if something similar happens in Jap...
    // Maybe there are some French things with accents...
    if((species == MR_MIME_SPECIES) && !is_egg)
        fix_name_change_from_gen3(src, species,  nickname, is_egg, is_gen2);
    
    // Put the "EGG" name
    if(is_gen2 && is_egg) {
        text_gen2_copy(get_pokemon_name_gen2(species, is_egg, 0, gen2_buffer), nickname, STRING_GEN2_INT_CAP, STRING_GEN2_INT_CAP);
        text_gen2_copy(get_pokemon_name_gen2(species, is_egg, 1, gen2_buffer), nickname_jp, STRING_GEN2_JP_CAP, STRING_GEN2_JP_CAP);
    }
    
    // Handle bad naming conversions (? >= half the name) and empty names
    size_t question_marks_count = text_gen2_count_question(nickname, STRING_GEN2_INT_CAP) - text_gen3_count_question(src->nickname, NICKNAME_GEN3_SIZE);
    if((question_marks_count >= (text_gen2_size(nickname, STRING_GEN2_INT_CAP) >> 1)) || (text_gen2_size(nickname, STRING_GEN2_INT_CAP) == 0))
        text_gen2_copy(get_pokemon_name_gen2(species, is_egg, 0, gen2_buffer), nickname, STRING_GEN2_INT_CAP, STRING_GEN2_INT_CAP);
    // For the japanese nickname too
    question_marks_count = text_gen2_count_question(nickname_jp, STRING_GEN2_JP_CAP) - text_gen3_count_question(src->nickname, NICKNAME_GEN3_SIZE);
    if((question_marks_count >= (text_gen2_size(nickname_jp, STRING_GEN2_JP_CAP) >> 1)) || (text_gen2_size(nickname_jp, STRING_GEN2_JP_CAP) == 0))
        text_gen2_copy(get_pokemon_name_gen2(species, is_egg, 1, gen2_buffer), nickname_jp, STRING_GEN2_JP_CAP, STRING_GEN2_JP_CAP);

}

void convert_strings_of_gen12(struct gen3_mon* dst, u8 species, u8* ot_name, u8* nickname, u8 is_egg) {
    u8 gen2_buffer[STRING_GEN2_INT_SIZE];
    u8 is_jp = (dst->language == JAPANESE_LANGUAGE);
    
    u8 name_cap = STRING_GEN2_INT_CAP;
    if(is_jp)
        name_cap = STRING_GEN2_JP_CAP;
    
    // Text conversions
    text_gen3_terminator_fill(dst->nickname, NICKNAME_GEN3_SIZE);
    text_gen3_terminator_fill(dst->ot_name, OT_NAME_GEN3_SIZE);
    
    text_gen12_to_gen3(nickname, dst->nickname, name_cap, NICKNAME_GEN3_SIZE, is_jp, is_jp);
    text_gen12_to_gen3(ot_name, dst->ot_name, name_cap, OT_NAME_GEN3_SIZE, is_jp, is_jp);
    
    // Handle Mew's special Japanese-only nature
    if(species == MEW_SPECIES) {
        dst->language = JAPANESE_LANGUAGE;
        text_gen12_to_gen3(nickname, dst->nickname, name_cap, NICKNAME_JP_GEN3_SIZE, is_jp, 1);
        text_gen12_to_gen3(ot_name, dst->ot_name, name_cap, OT_NAME_JP_GEN3_SIZE, is_jp, 1);
        name_cap = STRING_GEN2_JP_CAP;
        is_jp = 1;
    }
    
    // Fix text up
    // "MR.MIME" gen 2 == "MR. MIME" gen 3
    // Idk if something similar happens in Jap...
    // Maybe there are some French things with accents...
    if((species == MR_MIME_SPECIES) && !is_egg && !is_jp)
        fix_name_change_to_gen3(dst, species);
    
    // Put the "EGG" name
    if(is_egg) {
        dst->language = JAPANESE_LANGUAGE;
        text_gen12_to_gen3(get_pokemon_name_gen2(species, 1, 1, gen2_buffer), dst->nickname, STRING_GEN2_JP_CAP, NICKNAME_GEN3_SIZE, 1, 1);
    }
    else {
        // Handle bad naming conversions (? >= half the name) and empty names
        size_t question_marks_count = text_gen3_count_question(dst->nickname, NICKNAME_GEN3_SIZE) - text_gen2_count_question(nickname, name_cap);    
        if((question_marks_count >= (text_gen3_size(dst->nickname, NICKNAME_GEN3_SIZE) >> 1)) || (text_gen3_size(dst->nickname, NICKNAME_GEN3_SIZE) == 0))
            text_gen12_to_gen3(get_pokemon_name_gen2(species, 0, is_jp, gen2_buffer), dst->nickname, name_cap, NICKNAME_GEN3_SIZE, is_jp, is_jp);
    }
}

u8 gen3_to_gen2(struct gen2_mon* dst_data, struct gen3_mon_data_unenc* data_src, u32 trainer_id) {
    
    struct gen3_mon* src = data_src->src;
    struct gen2_mon_data* dst = &dst_data->data;
    
    if(!data_src->is_valid_gen3)
        return 0;
    
    struct gen3_mon_growth* growth = &data_src->growth;
    struct gen3_mon_attacks* attacks = &data_src->attacks;
    struct gen3_mon_evs* evs = &data_src->evs;
    struct gen3_mon_misc* misc = &data_src->misc;
    
    // Get shinyness and gender for checks
    u8 is_shiny = is_shiny_gen3_raw(data_src, trainer_id);
    u8 gender = get_pokemon_gender_gen3(growth->species, src->pid, 0, data_src->deoxys_form);
    u8 gender_kind = get_pokemon_gender_kind_gen3(growth->species, src->pid, 0, data_src->deoxys_form);
    
    // Check that the mon can be traded
    if(!validate_converting_mon_of_gen3(src, growth, is_shiny, gender, gender_kind, data_src->is_egg, 1))
        return 0;
    
    // Reset data structure
    for(size_t i = 0; i < sizeof(struct gen2_mon); i++)
        ((u8*)dst_data)[i] = 0;
    
    // Start setting data
    dst->species = growth->species;
    
    // Convert moves, and check if no valid moves were found
    if(convert_moves_of_gen3(attacks, growth->pp_bonuses, dst->moves, dst->pps, 1)  == 0)
        return 0;
    
    // Item handling
    dst->item = convert_item_of_gen3(growth->item);
    
    // OT handling
    dst->ot_id = swap_endian_short(src->ot_id & 0xFFFF);
    
    // Assign level and experience
    convert_exp_nature_of_gen3(src, growth, &dst->level, dst->exp, 1);
    
    // Convert EVs
    u16 evs_container[EVS_TOTAL_GEN12];
    convert_evs_of_gen3(evs, evs_container);
    for(int i = 0; i < EVS_TOTAL_GEN12; i++)
        dst->evs[i] = evs_container[i];
    
    // Assign IVs
    dst->ivs = convert_ivs_of_gen3(misc, growth->species, src->pid, is_shiny, gender, gender_kind, 1, 0);
    
    // Is this really how it works...?
    dst->pokerus = misc->pokerus;
    
    // Defaults
    dst->friendship = BASE_FRIENDSHIP;
    dst->status = 0;
    dst->unused = 0;
    
    // Handle met location data
    dst->time = 1;
    dst->ot_gender = (misc->origins_info>>15)&1;
    
    // Handle special mons which cannot breed
    const struct special_met_data_gen2* met_data = NULL;
    if(encounter_types_gen2_bin[dst->species>>3]&(1<<(dst->species&7))) {
        met_data = (const struct special_met_data_gen2*)search_table_for_index(special_encounters_gen2_bin, dst->species);
        if(met_data != NULL) {
            dst->location = met_data->location;
            dst->met_level = met_data->level;
            if((dst->location == 0) && (dst->location == dst->met_level)) {
                dst->time = 0;
                dst->ot_gender = 0;
            }
        }
    }
    if(met_data == NULL) {
        dst->met_level = 1;
        dst->location = 1;
    }
    
    // Extra byte for egg data
    dst_data->is_egg = data_src->is_egg;
    
    // Stats calculations
    // Curr HP should be 0 for eggs, otherwise they count as party members
    if(!dst_data->is_egg)
        dst->curr_hp = swap_endian_short(calc_stats_gen2(growth->species, src->pid, HP_STAT_INDEX, dst->level, get_ivs_gen2(dst->ivs, HP_STAT_INDEX), swap_endian_short(dst->evs[HP_STAT_INDEX])));
    else
        dst->curr_hp = 0;
    for(int i = 0; i < GEN2_STATS_TOTAL; i++)
        dst->stats[i] = swap_endian_short(calc_stats_gen2(growth->species, src->pid, i, dst->level, get_ivs_gen2(dst->ivs, i), swap_endian_short(dst->evs[i >= EVS_TOTAL_GEN12 ? EVS_TOTAL_GEN12-1 : i])));
    
    // Store egg cycles
    if(dst_data->is_egg)
        dst->friendship = growth->friendship;

    // Text conversions
    convert_strings_of_gen3(src, growth->species, dst_data->ot_name, dst_data->ot_name_jp, dst_data->nickname, dst_data->nickname_jp, dst_data->is_egg, 1);

    return 1;
}

u8 gen3_to_gen1(struct gen1_mon* dst_data, struct gen3_mon_data_unenc* data_src, u32 trainer_id) {
    
    struct gen3_mon* src = data_src->src;
    struct gen1_mon_data* dst = &dst_data->data;
    
    if(!data_src->is_valid_gen3)
        return 0;
    
    struct gen3_mon_growth* growth = &data_src->growth;
    struct gen3_mon_attacks* attacks = &data_src->attacks;
    struct gen3_mon_evs* evs = &data_src->evs;
    struct gen3_mon_misc* misc = &data_src->misc;
    
    // Get shinyness and gender for checks
    u8 is_shiny = is_shiny_gen3_raw(data_src, trainer_id);
    u8 gender = get_pokemon_gender_gen3(growth->species, src->pid, 0, data_src->deoxys_form);
    u8 gender_kind = get_pokemon_gender_kind_gen3(growth->species, src->pid, 0, data_src->deoxys_form);
    
    // Check that the mon can be traded
    if(!validate_converting_mon_of_gen3(src, growth, is_shiny, gender, gender_kind, data_src->is_egg, 0))
        return 0;
    
    // Reset data structure
    for(size_t i = 0; i < sizeof(struct gen1_mon); i++)
        ((u8*)dst_data)[i] = 0;
    
    // Start setting data
    dst->species = get_mon_index_gen1(growth->species);
    
    // Convert moves, and check if no valid moves were found
    if(convert_moves_of_gen3(attacks, growth->pp_bonuses, dst->moves, dst->pps, 0)  == 0)
        return 0;
    
    // Item handling
    dst->item = convert_item_of_gen3(growth->item);
    
    // Types handling
    dst->type[0] = pokemon_types_gen1_bin[(2*dst->species)];
    dst->type[1] = pokemon_types_gen1_bin[(2*dst->species) + 1];
    
    // OT handling
    dst->ot_id = swap_endian_short(src->ot_id & 0xFFFF);
    
    // Assign level and experience
    convert_exp_nature_of_gen3(src, growth, &dst->level, dst->exp, 0);
    
    // Convert EVs
    u16 evs_container[EVS_TOTAL_GEN12];
    convert_evs_of_gen3(evs, evs_container);
    for(int i = 0; i < EVS_TOTAL_GEN12; i++)
        dst->evs[i] = evs_container[i];
    
    // Assign IVs
    dst->ivs = convert_ivs_of_gen3(misc, growth->species, src->pid, is_shiny, gender, gender_kind, 0, 0);
    
    // Defaults
    dst->bad_level = 0;
    dst->status = 0;
    
    // Stats calculations
    dst->curr_hp = swap_endian_short(calc_stats_gen1(growth->species, HP_STAT_INDEX, dst->level, get_ivs_gen2(dst->ivs, HP_STAT_INDEX), swap_endian_short(dst->evs[HP_STAT_INDEX])));
    for(int i = 0; i < GEN1_STATS_TOTAL; i++)
        dst->stats[i] = swap_endian_short(calc_stats_gen1(growth->species, i, dst->level, get_ivs_gen2(dst->ivs, i), swap_endian_short(dst->evs[i])));

    // Text conversions
    convert_strings_of_gen3(src, growth->species, dst_data->ot_name, dst_data->ot_name_jp, dst_data->nickname, dst_data->nickname_jp, 0, 0);

    return 1;
}

u8 gen2_to_gen3(struct gen2_mon_data* src, struct gen3_mon_data_unenc* data_dst, u8 index, u8* ot_name, u8* nickname, u8 is_jp) {
    struct gen3_mon* dst = data_dst->src;
    u8 no_restrictions = 1;
    u8 is_egg = 0;
    
    // Reset everything
    for(size_t i = 0; i < sizeof(struct gen3_mon); i++)
        ((u8*)(dst))[i] = 0;
    
    data_dst->is_valid_gen3 = 0;
    data_dst->is_valid_gen2 = 0;
    
    // Check if valid
    if(!validate_converting_mon_of_gen2(index, src, &is_egg))
        return 0;
    
    data_dst->is_valid_gen3 = 1;
    data_dst->is_valid_gen2 = 1;
    
    // Set base data
    dst->has_species = 1;
    dst->mail_id = GEN3_NO_MAIL;
    data_dst->is_egg = is_egg;
    
    if(is_jp)
        dst->language = JAPANESE_LANGUAGE;
    else
        dst->language = ENGLISH_LANGUAGE;
    
    // Handle Nickname + OT conversion
    convert_strings_of_gen12(dst, src->species, ot_name, nickname, is_egg);
    
    // Handle OT ID, if same as the game owner, set it to the game owner's
    dst->ot_id = swap_endian_short(src->ot_id);
    
    if(are_trainers_same(dst, is_jp)) {
        no_restrictions = 0;
        dst->ot_id = get_own_game_data()->trainer_id;
    }
    else
        dst->ot_id = generate_ot(dst->ot_id, dst->ot_name);
    
    // Reset everything
    for(size_t i = 0; i < sizeof(struct gen3_mon_growth); i++)
        ((u8*)(&data_dst->growth))[i] = 0;
    for(size_t i = 0; i < sizeof(struct gen3_mon_attacks); i++)
        ((u8*)(&data_dst->attacks))[i] = 0;
    for(size_t i = 0; i < sizeof(struct gen3_mon_evs); i++)
        ((u8*)(&data_dst->evs))[i] = 0;
    for(size_t i = 0; i < sizeof(struct gen3_mon_misc); i++)
        ((u8*)(&data_dst->misc))[i] = 0;
    
    // Set species, exp, level and item
    data_dst->growth.species = src->species;
    u8 wanted_nature = get_exp_nature(dst, &data_dst->growth, src->level, src->exp);
    data_dst->growth.item = convert_item_to_gen3(src->item);
    
    // Convert EVs
    u16 evs_container[EVS_TOTAL_GEN12];
    for(int i = 0; i < EVS_TOTAL_GEN12; i++)
        evs_container[i] = src->evs[i];
    convert_evs_to_gen3(&data_dst->evs, evs_container);
    
    // Handle cases in which the nature would be forced
    if((dst->level == MAX_LEVEL) || (is_egg))
        wanted_nature = SWI_DivMod(get_rng(), NUM_NATURES);
    
    // Store egg cycles
    if(is_egg) {
        data_dst->growth.friendship = src->friendship;
        data_dst->misc.is_egg = 1;
        dst->use_egg_name = 1;
    }
    else
        data_dst->growth.friendship = BASE_FRIENDSHIP;
    
    // Set the moves
    convert_moves_to_gen3(&data_dst->attacks, &data_dst->growth, src->moves, src->pps, 1);
    
    data_dst->misc.pokerus = src->pokerus;
    
    // Special Mew handling
    if(data_dst->growth.species == MEW_SPECIES)
        data_dst->misc.obedience = 1;
    
    // Set the PID-Origin-IVs data, they're all connected
    set_origin_pid_iv(dst, &data_dst->misc, data_dst->growth.species, src->ivs, wanted_nature, src->ot_gender, is_egg, no_restrictions);
    
    // Place all the substructures' data
    place_and_encrypt_gen3_data(data_dst, dst);
    
    // Calculate stats
    recalc_stats_gen3(data_dst, dst);

    return 1;
}

u8 gen1_to_gen3(struct gen1_mon_data* src, struct gen3_mon_data_unenc* data_dst, u8 index, u8* ot_name, u8* nickname, u8 is_jp) {
    struct gen3_mon* dst = data_dst->src;
    u8 no_restrictions = 1;
    
    // Reset everything
    for(size_t i = 0; i < sizeof(struct gen3_mon); i++)
        ((u8*)(dst))[i] = 0;
    
    data_dst->is_valid_gen3 = 0;
    data_dst->is_valid_gen1 = 0;
    
    // Check if valid
    if(!validate_converting_mon_of_gen1(index, src))
        return 0;
    
    data_dst->is_valid_gen3 = 1;
    data_dst->is_valid_gen1 = 1;
    
    // Set base data
    dst->has_species = 1;
    dst->mail_id = GEN3_NO_MAIL;
    data_dst->is_egg = 0;
    
    if(is_jp)
        dst->language = JAPANESE_LANGUAGE;
    else
        dst->language = ENGLISH_LANGUAGE;
    
    // Handle Nickname + OT conversion
    convert_strings_of_gen12(dst, get_mon_index_gen1_to_3(src->species), ot_name, nickname, 0);
    
    // Handle OT ID, if same as the game owner, set it to the game owner's
    dst->ot_id = swap_endian_short(src->ot_id);
    
    if(are_trainers_same(dst, is_jp)) {
        no_restrictions = 0;
        dst->ot_id = get_own_game_data()->trainer_id;
    }
    else
        dst->ot_id = generate_ot(dst->ot_id, dst->ot_name);
    
    // Reset everything
    for(size_t i = 0; i < sizeof(struct gen3_mon_growth); i++)
        ((u8*)(&data_dst->growth))[i] = 0;
    for(size_t i = 0; i < sizeof(struct gen3_mon_attacks); i++)
        ((u8*)(&data_dst->attacks))[i] = 0;
    for(size_t i = 0; i < sizeof(struct gen3_mon_evs); i++)
        ((u8*)(&data_dst->evs))[i] = 0;
    for(size_t i = 0; i < sizeof(struct gen3_mon_misc); i++)
        ((u8*)(&data_dst->misc))[i] = 0;
    
    // Set species, exp, level and item
    data_dst->growth.species = get_mon_index_gen1_to_3(src->species);
    u8 wanted_nature = get_exp_nature(dst, &data_dst->growth, src->level, src->exp);
    data_dst->growth.item = convert_item_to_gen3(src->item);
    
    // Convert EVs
    u16 evs_container[EVS_TOTAL_GEN12];
    for(int i = 0; i < EVS_TOTAL_GEN12; i++)
        evs_container[i] = src->evs[i];
    convert_evs_to_gen3(&data_dst->evs, evs_container);
    
    // Handle cases in which the nature would be forced
    if(dst->level == MAX_LEVEL)
        wanted_nature = SWI_DivMod(get_rng(), NUM_NATURES);
    
    // Set base friendship
    data_dst->growth.friendship = BASE_FRIENDSHIP;
    
    // Set the moves
    convert_moves_to_gen3(&data_dst->attacks, &data_dst->growth, src->moves, src->pps, 1);
    
    data_dst->misc.pokerus = 0;
    
    // Special Mew handling
    if(data_dst->growth.species == MEW_SPECIES)
        data_dst->misc.obedience = 1;
    
    // Set the PID-Origin-IVs data, they're all connected
    set_origin_pid_iv(dst, &data_dst->misc, data_dst->growth.species, src->ivs, wanted_nature, 0, 0, no_restrictions);
    
    // Place all the substructures' data
    place_and_encrypt_gen3_data(data_dst, dst);
    
    // Calculate stats
    recalc_stats_gen3(data_dst, dst);

    return 1;
}
