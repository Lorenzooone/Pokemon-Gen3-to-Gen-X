#include <gba.h>
#include "party_handler.h"
#include "text_handler.h"
#include "sprite_handler.h"
#include "version_identifier.h"
#include "bin_table_handler.h"
#include "gen3_save.h"
#include "rng.h"
#include "pid_iv_tid.h"
#include "fast_pokemon_methods.h"
#include "optimized_swi.h"
#include "useful_qualifiers.h"
#include "gen_converter.h"
#include <stddef.h>

#include "pokemon_gender_bin.h"
#include "pokemon_names_bin.h"
#include "egg_names_bin.h"
#include "language_names_index_bin.h"
#include "gen3_spanish_special_valid_chars_bin.h"
#include "gen3_german_valid_chars_bin.h"
#include "gen3_int_valid_chars_bin.h"
#include "gen3_jap_valid_chars_bin.h"
#include "item_names_bin.h"
#include "location_names_bin.h"
#include "pokeball_names_bin.h"
#include "nature_names_bin.h"
#include "type_names_bin.h"
#include "move_names_bin.h"
#include "ability_names_bin.h"
#include "contest_rank_names_bin.h"
#include "ribbon_names_bin.h"
#include "sprites_cmp_bin.h"
#include "sprites_info_bin.h"
#include "palettes_references_bin.h"
#include "pokemon_exp_groups_bin.h"
#include "exp_table_bin.h"
#include "pokemon_stats_bin.h"
#include "pokemon_stats_gen1_bin.h"
#include "pokemon_natures_bin.h"
#include "pokemon_abilities_bin.h"
#include "trade_evolutions_bin.h"
#include "learnset_evos_gen1_bin.h"
#include "learnset_evos_gen2_bin.h"
#include "learnset_evos_gen3_bin.h"
#include "dex_conversion_bin.h"
#include "pokemon_moves_pp_bin.h"
#include "trainer_names_bin.h"
#include "trainer_names_celebi_bin.h"
#include "special_evolutions_bin.h"
#include "invalid_pokemon_bin.h"
#include "invalid_held_items_bin.h"

#define UNOWN_B_START 415
#define INITIAL_MAIL_GEN3 121
#define LAST_MAIL_GEN3 132

#define ACT_AS_GEN1_TRADE (!get_gen1_everstone())

#define PID_POSITIONS 24

const u8* get_item_name(int, u8);
u8 get_ability_pokemon(int, u32, u8, u8, u8);
u8 is_ability_valid(u16, u32, u8, u8, u8, u8);
const u8* get_pokemon_sprite_pointer(int, u32, u8, u8);
u8 get_pokemon_sprite_info(int, u32, u8, u8);
u8 get_palette_references(int, u32, u8, u8);
void load_pokemon_sprite(int, u32, u8, u8, u8, u8, u16, u16);
void teach_move(struct gen3_mon_attacks*, struct gen3_mon_growth*, u16, u8);
void swap_move(struct gen3_mon_attacks*, struct gen3_mon_growth*, u8, u8);
u8 make_moves_legal_gen3(struct gen3_mon_attacks*, struct gen3_mon_growth*);
u8 is_egg_gen3(struct gen3_mon*, struct gen3_mon_misc*);
u8 is_shiny_gen3(u32, u32, u8, u32);
u8 get_mail_id(struct gen3_mon*, struct gen3_mon_growth*, u8);
u8 decrypt_data(struct gen3_mon* src, u32* decrypted_dst);
void encrypt_data(struct gen3_mon* dst);
u8 get_hidden_power_type_gen3_pure(u32);
u8 get_hidden_power_type_gen3(struct gen3_mon_misc*);
void make_evs_legal_gen3(struct gen3_mon_evs*);
u8 to_valid_level_gen3(struct gen3_mon*);
const u8* get_validity_keyboard(u8, u8);
u8 use_special_keyboard(struct gen3_mon_misc*, u8);
void sanitize_nickname(u8*, u8, u8, u16, u8);
void set_deoxys_form_inner(struct gen3_mon_data_unenc*, u8, u8);
u8 decrypt_to_data_unenc(struct gen3_mon*, struct gen3_mon_data_unenc*);
const struct learnset_data_mon_moves* extract_learnable_moves(const u16*, u16, u8);
const u16* get_special_evolutions(u16);
u16 get_trade_evolution(u16, u16, u8, u8*, u8*);
void evolve_mon(struct gen3_mon*, struct gen3_mon_data_unenc*, u16, u8, const u16*);
u8 is_pokerus_strain_valid(u8);
u8 get_pokerus_strain_max_days(u8);
u8 are_pokerus_days_valid(u8);
void sanitize_pokerus(u8*);

// Order is G A E M. Initialized by init_enc_positions
u8 enc_positions[PID_POSITIONS];

const u8 gender_thresholds_gen3[TOTAL_GENDER_KINDS] = {127, 0, 31, 63, 191, 225, 254, 255, 0, 254};
const u8 stat_index_conversion_gen3[GEN2_STATS_TOTAL] = {0, 1, 2, 4, 5, 3};
const u8 language_keyboard_kind[NUM_LANGUAGES] = {1,0,1,1,1,2,1,1};

const u16* pokemon_abilities_bin_16 = (const u16*)pokemon_abilities_bin;
const struct exp_level* exp_table = (const struct exp_level*)exp_table_bin;
const struct stats_gen_23* stats_table_gen3 = (const struct stats_gen_23*)pokemon_stats_bin;

void init_enc_positions() {
    u8 pos = 0;
    for(int i = 0; i < 4; i++)
        for(int j = 0; j < 4; j++)
            if(j != i)
                for(int k = 0; k < 4; k++)
                    if((k != i) && (k != j))
                        for(int l = 0; l < 4; l++)
                            if((l != i) && (l != j) && (l != k))
                                enc_positions[pos++] = (0<<(i*2)) | (1<<(j*2)) | (2<<(k*2)) | (3<<(l*2));
}

u8 get_valid_language(u8 language) {
    if(language >= NUM_LANGUAGES)
        language = DEFAULT_NAME_BAD_LANGUAGE;
    return language;
}

u8 get_index_key(u32 pid){
    // Make use of modulo properties to get this to positives
    while(pid >= 0x80000000)
        pid -= 0x7FFFFFF8;
    return SWI_DivMod(pid, PID_POSITIONS);
}

u8 get_nature(u32 pid){
    return get_nature_fast(pid);
}

u8 get_unown_letter_gen3(u32 pid){
    return get_unown_letter_gen3_fast(pid);
}

u16 get_mon_index(int index, u32 pid, u8 is_egg, u8 deoxys_form){
    if(!is_species_valid(index))
        return 0;
    if(is_egg)
        return EGG_SPECIES;
    if(index == UNOWN_SPECIES) {
        u8 letter = get_unown_letter_gen3(pid);
        if(!letter)
            return UNOWN_SPECIES;
        return UNOWN_B_START+letter-1;
    }
    else if(index == DEOXYS_SPECIES) {
        if(!deoxys_form)
            return DEOXYS_SPECIES;
        return DEOXYS_FORMS_POS+deoxys_form-1;
    }
    return index;
}

u16 get_mon_index_raw(struct gen3_mon_data_unenc* data_src){
    if(!data_src->is_valid_gen3)
        return get_mon_index(0,0,0,0);

    return get_mon_index(data_src->growth.species, data_src->src->pid, data_src->is_egg, data_src->deoxys_form);
}

const u8* get_pokemon_name(int index, u32 pid, u8 is_egg, u8 deoxys_form, u8 language){
    return get_pokemon_name_language(get_mon_index(index, pid, is_egg, deoxys_form), language);
}

const u8* get_pokemon_name_pure(int index, u8 is_egg, u8 language){
    u16 mon_index = get_mon_index(index, 0, is_egg, 0);
    if ((index == UNOWN_SPECIES) && !is_egg)
        mon_index = UNOWN_REAL_NAME_POS;
    if ((index == DEOXYS_SPECIES) && !is_egg)
        mon_index = DEOXYS_SPECIES;
    return get_pokemon_name_language(mon_index, language);
}

const u8* get_pokemon_name_raw(struct gen3_mon_data_unenc* data_src){
    if(!data_src->is_valid_gen3)
        return get_pokemon_name(0,0,0,0, SYS_LANGUAGE);

    return get_pokemon_name(data_src->growth.species, data_src->src->pid, data_src->is_egg, data_src->deoxys_form, SYS_LANGUAGE);
}

size_t get_pokemon_name_raw_language_limit(struct gen3_mon_data_unenc* data_src){
    if(!data_src->is_valid_gen3)
        return SYS_LANGUAGE_LIMIT;

    if(IS_SYS_LANGUAGE_JAPANESE && (data_src->growth.species == UNOWN_SPECIES) && (!data_src->is_egg))
        return SYS_LANGUAGE_LIMIT + 2; // Account for the space and the extra letter
        
    if(IS_SYS_LANGUAGE_JAPANESE && (data_src->growth.species == DEOXYS_SPECIES) && (!data_src->is_egg) && (data_src->deoxys_form))
        return SYS_LANGUAGE_LIMIT + 2; // Account for the space and the extra letter

    return SYS_LANGUAGE_LIMIT;
}

const u16* get_learnset_for_species(const u16* learnsets, u16 species){
    u16 num_entries = learnsets[0];
    for(int i = 0; i < num_entries; i++) {
        u16 base_pos = learnsets[1+i] >> 1;
        if(learnsets[base_pos++] == species)
            return &learnsets[base_pos];
    }
    return NULL;
}

const u8* get_pokemon_name_language(u16 index, u8 language) {
    language = get_valid_language(language);
    if(index == EGG_SPECIES)
        return get_table_pointer(egg_names_bin, language);
    return get_table_pointer(pokemon_names_bin, (index * NUM_POKEMON_NAME_LANGUAGES) + language_names_index_bin[language]);
}

const u8* get_item_name(int index, u8 is_egg){
    if(is_egg)
        index = 0;
    if(!is_item_valid(index))
        index = 0;
    return get_table_pointer(item_names_bin, index);
}

u8 has_item_raw(struct gen3_mon_data_unenc* data_src){
    if(!data_src->is_valid_gen3)
        return 0;

    u16 index = data_src->growth.item;
    u8 is_egg = data_src->is_egg;
    
    if(is_egg)
        return 0;
    if((!index) || (!is_item_valid(index)))
        return 0;
    
    return 1;
}

const u8* get_item_name_raw(struct gen3_mon_data_unenc* data_src){
    if(!data_src->is_valid_gen3)
        return get_item_name(0,0);

    return get_item_name(data_src->growth.item, data_src->is_egg);
}

u16 get_possible_abilities_pokemon(int index, u32 pid, u8 is_egg, u8 deoxys_form){
    u16 mon_index = get_mon_index(index, pid, is_egg, deoxys_form);
    return pokemon_abilities_bin_16[mon_index];
}

u8 get_ability_pokemon(int index, u32 pid, u8 is_egg, u8 ability_bit, u8 deoxys_form){
    return (get_possible_abilities_pokemon(index, pid, is_egg, deoxys_form) >> (8*ability_bit))&0xFF;
}

const u8* get_ability_name_raw(struct gen3_mon_data_unenc* data_src){
    if(!data_src->is_valid_gen3)
        return get_table_pointer(ability_names_bin, get_ability_pokemon(0,0,0,0,0));

    return get_table_pointer(ability_names_bin, get_ability_pokemon(data_src->growth.species, data_src->src->pid, data_src->is_egg, data_src->misc.ability, data_src->deoxys_form));
}

u8 is_ability_valid(u16 index, u32 pid, u8 ability_bit, u8 met_location, u8 origin_game, u8 deoxys_form){
    u16 abilities = get_possible_abilities_pokemon(index, pid, 0, deoxys_form);
    u8 abilities_same = (abilities&0xFF) == ((abilities>>8)&0xFF);
    
    if(abilities_same && ability_bit)
        return 0;
    
    if(!((pid&1) ^ ability_bit))
        return 1;
    
    if(met_location == TRADE_MET)
        return 1;
    
    if(origin_game == COLOSSEUM_CODE)
        return 1;
    
    if(abilities_same)
        return 1;
    
    return 0;
}

const u8* get_ribbon_name(struct gen3_mon_misc* misc, u8 ribbon_num){
    if(ribbon_num > LAST_RIBBON)
        return get_table_pointer(ribbon_names_bin, NO_RIBBON_ID);
    
    if(ribbon_num <= LAST_RIBBON_CONTEST) {
        if ((misc->ribbons >> (3*ribbon_num))&7)
            return get_table_pointer(ribbon_names_bin, ribbon_num);
        else
            return get_table_pointer(ribbon_names_bin, NO_RIBBON_ID);
    }
    
    if((misc->ribbons >> (((LAST_RIBBON_CONTEST+1)*3)+(ribbon_num-(LAST_RIBBON_CONTEST+1))))&1)
            return get_table_pointer(ribbon_names_bin, ribbon_num);
    return get_table_pointer(ribbon_names_bin, NO_RIBBON_ID);
}

const u8* get_ribbon_rank_name(struct gen3_mon_misc* misc, u8 ribbon_num){
    if((ribbon_num <= LAST_RIBBON_CONTEST) && ((misc->ribbons >> (3*ribbon_num))&7))
        return get_table_pointer(contest_rank_names_bin, ((misc->ribbons >> (3*ribbon_num))&7));
    else
        return get_table_pointer(contest_rank_names_bin, NO_RANK_ID);
}

const u8* get_met_location_name_gen3_raw(struct gen3_mon_data_unenc* data_src){
    if(!data_src->is_valid_gen3)
        return get_table_pointer(location_names_bin, EMPTY_LOCATION);

    u16 location = data_src->misc.met_location;
    u8 origin_game = (data_src->misc.origins_info>>7)&0xF;
    
    if(origin_game == COLOSSEUM_CODE)
        location = COLOSSEUM_ALT;
    else if((location == BATTLE_FACILITY) && (origin_game == EMERALD_CODE))
        location = BATTLE_FACILITY_ALT;
    else if((location == HIDEOUT) && (origin_game == RUBY_CODE))
        location = HIDEOUT_ALT;
    else if((location == DEPT_STORE) && (origin_game == EMERALD_CODE))
        location = DEPT_STORE_ALT;
    
    return get_table_pointer(location_names_bin, location);
}

u8 get_met_level_gen3_raw(struct gen3_mon_data_unenc* data_src){
    if(!data_src->is_valid_gen3)
        return 0;

    u8 level = data_src->misc.origins_info&0x7F;
    if(level > 100)
        level = 100;
    return level;
}

const u8* get_pokeball_base_name_gen3_raw(struct gen3_mon_data_unenc* data_src){
    if(!data_src->is_valid_gen3)
        return get_table_pointer(pokeball_names_bin, 0);

    return get_table_pointer(pokeball_names_bin, (data_src->misc.origins_info>>11)&0xF);
}

u8 get_trainer_gender_char_raw(struct gen3_mon_data_unenc* data_src){
    if(!data_src->is_valid_gen3)
        return GENERIC_U_GENDER;

    if(data_src->misc.origins_info>>15)
        return GENERIC_F_GENDER;
    return GENERIC_M_GENDER;
}

const u8* get_pokemon_sprite_pointer(int index, u32 pid, u8 is_egg, u8 deoxys_form){
    return get_table_pointer(sprites_cmp_bin, get_mon_index(index, pid, is_egg, deoxys_form));
}

u8 get_pokemon_sprite_info(int index, u32 pid, u8 is_egg, u8 deoxys_form){
    u16 mon_index = get_mon_index(index, pid, is_egg, deoxys_form);
    return (sprites_info_bin[mon_index>>2]>>(2*(mon_index&3)))&3;
}

u8 get_palette_references(int index, u32 pid, u8 is_egg, u8 deoxys_form){
    return palettes_references_bin[get_mon_index(index, pid, is_egg, deoxys_form)];
}

void load_pokemon_sprite(int index, u32 pid, u8 is_egg, u8 deoxys_form, u8 has_item, u8 has_mail, u16 y, u16 x){
    set_pokemon_sprite(get_pokemon_sprite_pointer(index, pid, is_egg, deoxys_form), get_palette_references(index, pid, is_egg, deoxys_form), get_pokemon_sprite_info(index, pid, is_egg, deoxys_form), has_item, has_mail, y, x);
}

void load_pokemon_sprite_raw(struct gen3_mon_data_unenc* data_src, u8 show_item, u16 y, u16 x){
    if(!data_src->is_valid_gen3)
        load_pokemon_sprite(0, 0, 0, 0, has_item_raw(data_src) & show_item, has_mail_raw(data_src) & show_item, y, x);

    load_pokemon_sprite(data_src->growth.species, data_src->src->pid, data_src->is_egg, data_src->deoxys_form, has_item_raw(data_src) & show_item, has_mail_raw(data_src) & show_item, y, x);
}

const u8* get_move_name_raw(u16 move){
    
    if(!is_move_valid(move, LAST_VALID_GEN_3_MOVE))
        move = 0;
    
    return get_table_pointer(move_names_bin, move);
}

const u8* get_move_name_gen3(struct gen3_mon_attacks* attacks, u8 slot){
    if(slot >= MOVES_SIZE)
        slot = MOVES_SIZE-1;
    
    return get_move_name_raw(attacks->moves[slot]);
}

void swap_move(struct gen3_mon_attacks* attacks, struct gen3_mon_growth* growth, u8 base_index, u8 other_index) {
    if((base_index >= MOVES_SIZE) || (other_index >= MOVES_SIZE))
        return;
    
    u16 other_move = attacks->moves[other_index];
    u8 other_pps = attacks->pp[other_index];
    u8 other_pp_bonuses = (growth->pp_bonuses>>(2*other_index))&3;

    if(!is_move_valid(attacks->moves[other_index], LAST_VALID_GEN_3_MOVE)) {
        other_move = 0;
        other_pps = 0;
        other_pp_bonuses = 0;
    }
    
    attacks->moves[other_index] = attacks->moves[base_index];
    attacks->pp[other_index] = attacks->pp[base_index];
    growth->pp_bonuses &= ~(3<<(2*other_index));
    growth->pp_bonuses |= ((growth->pp_bonuses>>(2*base_index))&3)<<(2*other_index);

    if(!is_move_valid(attacks->moves[other_index], LAST_VALID_GEN_3_MOVE)) {
        attacks->moves[other_index] = 0;
        attacks->pp[other_index] = 0;
        growth->pp_bonuses &= ~(3<<(2*other_index));
    }
    
    attacks->moves[base_index] = other_move;
    attacks->pp[base_index] = other_pps;
    growth->pp_bonuses &= ~(3<<(2*base_index));
    growth->pp_bonuses |= other_pp_bonuses<<(2*base_index);
}

u8 is_move_valid(u16 move, u16 last_valid_move) {
    return move && (move <= last_valid_move) && (move != STRUGGLE_MOVE_ID);
}

u8 get_pp_of_move(u16 move, u8 bonus, u16 last_valid_move) {
    if(!is_move_valid(move, last_valid_move))
        return 0;
    u8 base_val = pokemon_moves_pp_bin[move];
    u8 increment = SWI_Div(base_val, 5);
    return base_val + ((bonus & 3) * increment);
}

void teach_move(struct gen3_mon_attacks* attacks, struct gen3_mon_growth* growth, u16 new_move, u8 base_index) {
    if(base_index >= MOVES_SIZE)
        return;
    
    growth->pp_bonuses &= ~(3<<(2*base_index));

    if(!is_move_valid(new_move, LAST_VALID_GEN_3_MOVE))
        new_move = 0;

    attacks->moves[base_index] = new_move;
    attacks->pp[base_index] = get_pp_of_move(new_move, (growth->pp_bonuses>>(2*base_index))&3, LAST_VALID_GEN_3_MOVE);
}

u8 make_moves_legal_gen3(struct gen3_mon_attacks* attacks, struct gen3_mon_growth* growth){
    u16 previous_moves[MOVES_SIZE];
    u8 curr_slot = 0;
    
    for(size_t i = 0; i < MOVES_SIZE; i++) {
        if(is_move_valid(attacks->moves[i], LAST_VALID_GEN_3_MOVE)) {
            u8 found = 0;
            for(int j = 0; j < curr_slot; j++)
                if(attacks->moves[i] == previous_moves[j]){
                    found = 1;
                    teach_move(attacks, growth, 0, i);
                    break;
                }
            if(!found)
                previous_moves[curr_slot++] = attacks->moves[i];
        }
        else
            teach_move(attacks, growth, 0, i);
    }
    
    if(curr_slot) {
        for(size_t i = 0; i < MOVES_SIZE; i++) {
            if(!attacks->moves[i])
                for(size_t j = i + 1; j < MOVES_SIZE; j++)
                    if(attacks->moves[j]) {
                        swap_move(attacks, growth, i, j);
                        break;
                    }
            attacks->pp[i] = get_pp_of_move(attacks->moves[i], (growth->pp_bonuses>>(2*i))&3, LAST_VALID_GEN_3_MOVE);
            if(attacks->pp[i] == 1)
                growth->pp_bonuses &= ~(3<<(2*i));
        }
        return 1;
    }
    return 0;
}

u8 get_pokemon_gender_gen3(int index, u32 pid, u8 is_egg, u8 deoxys_form){
    u8 gender_kind = get_pokemon_gender_kind_gen3(index, pid, is_egg, deoxys_form);
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
            if((pid & 0xFF) >= gender_thresholds_gen3[gender_kind])
                return M_GENDER;
            return F_GENDER;
    }
}

u8 get_pokemon_gender_raw(struct gen3_mon_data_unenc* data_src){
    if(!data_src->is_valid_gen3)
        return GENERIC_U_GENDER;

    return get_pokemon_gender_gen3(data_src->growth.species, data_src->src->pid, data_src->is_egg, data_src->deoxys_form);
}

char get_pokemon_gender_char_raw(struct gen3_mon_data_unenc* data_src){
    if(!data_src->is_valid_gen3)
        return get_pokemon_gender_gen3(0,0,0,0);

    u16 mon_index = get_mon_index_raw(data_src);

    u8 gender = get_pokemon_gender_raw(data_src);
    char gender_char = GENERIC_M_GENDER;
    if(gender == F_GENDER)
        gender_char = GENERIC_F_GENDER;
    if((gender == U_GENDER)||(mon_index == NIDORAN_M_SPECIES)||(mon_index == NIDORAN_F_SPECIES))
        gender_char = GENERIC_U_GENDER;

    return gender_char;
}

u8 get_pokemon_gender_kind_gen3(int index, u32 pid, u8 is_egg, u8 deoxys_form){
    return pokemon_gender_bin[get_mon_index(index, pid, is_egg, deoxys_form)];
}

u8 get_pokemon_gender_kind_gen3_raw(struct gen3_mon_data_unenc* data_src){
    if(!data_src->is_valid_gen3)
        return get_pokemon_gender_kind_gen3(0,0,0,0);

    return get_pokemon_gender_kind_gen3(data_src->growth.species, data_src->src->pid, data_src->is_egg, data_src->deoxys_form);
}

u8 is_egg_gen3(struct gen3_mon* UNUSED(src), struct gen3_mon_misc* misc){
    // In case it ends up being more complex, for some reason
    return misc->is_egg;
}

u8 is_egg_gen3_raw(struct gen3_mon_data_unenc* data_src){
    if(!data_src->is_valid_gen3)
        return 1;

    return data_src->is_egg;
}

u8 has_pokerus_gen3_raw(struct gen3_mon_data_unenc* data_src){
    if(!data_src->is_valid_gen3)
        return NO_POKERUS;

    if(!data_src->misc.pokerus)
        return NO_POKERUS;
    
    if(data_src->misc.pokerus & 0xF)
        return HAS_POKERUS;
    return HAD_POKERUS;
}

void update_pokerus_gen3(struct gen3_mon_data_unenc* data_src, u16 days_increase){
    if(data_src->successfully_decrypted)
        if(data_src->misc.pokerus & 0xF) {
            if(((data_src->misc.pokerus & 0xF) < days_increase) || (days_increase > 4))
                data_src->misc.pokerus &= 0xF0;
            else
                data_src->misc.pokerus -= days_increase;

            if(!data_src->misc.pokerus)
                data_src->misc.pokerus = 0x10;
            place_and_encrypt_gen3_data(data_src, data_src->src);
        }
}

u8 give_pokerus_gen3(struct gen3_mon_data_unenc* data_src){
    if(data_src->successfully_decrypted)
        if(!data_src->misc.pokerus) {
            // This is for resolving the issue in FRLG, not for cheating,
            // so a non-spreadable Pokérus is given
            data_src->misc.pokerus = 0x10;
            place_and_encrypt_gen3_data(data_src, data_src->src);
            return 1;
        }
    return 0;
}

u8 would_update_end_pokerus_gen3(struct gen3_mon_data_unenc* data_src, u16 days_increase){
    if(data_src->successfully_decrypted)
        if(data_src->misc.pokerus & 0xF)
            if(((data_src->misc.pokerus & 0xF) <= days_increase) || (days_increase > 4))
                return 1;
    return 0;
}

u8 is_shiny_gen3(u32 pid, u32 ot_id, u8 is_egg, u32 trainer_id){
    // The one who hatches the egg influences the shinyness
    if(is_egg)
        ot_id = trainer_id;
    if(((pid & 0xFFFF) ^ (pid >> 16) ^ (ot_id & 0xFFFF) ^ (ot_id >> 16)) < 8)
        return 1;
    return 0;
}

u8 is_shiny_gen3_raw(struct gen3_mon_data_unenc* data_src, u32 trainer_id){
    if(!data_src->is_valid_gen3)
        return 0;

    return is_shiny_gen3(data_src->src->pid, data_src->src->ot_id, data_src->is_egg, trainer_id);
}

u8 has_mail(struct gen3_mon* src, struct gen3_mon_growth* growth, u8 is_egg) {
    if(is_egg)
        return 0;
    if(growth->item < INITIAL_MAIL_GEN3 || growth->item > LAST_MAIL_GEN3)
        return 0;
    if(src->mail_id == GEN3_NO_MAIL)
        return 0;
    return 1;
}

u8 has_mail_raw(struct gen3_mon_data_unenc* data_src) {
    if(!data_src->is_valid_gen3){
        data_src->src->mail_id = GEN3_NO_MAIL;
        return 0;
    }

    return has_mail(data_src->src, &data_src->growth, data_src->is_egg);
}

u8 get_mail_id(struct gen3_mon* src, struct gen3_mon_growth* growth, u8 is_egg) {
    if(!has_mail(src, growth, is_egg))
        return GEN3_NO_MAIL;
    return src->mail_id;
}

u8 get_mail_id_raw(struct gen3_mon_data_unenc* data_src) {
    if(!data_src->is_valid_gen3) {
        data_src->src->mail_id = GEN3_NO_MAIL;
        return GEN3_NO_MAIL;
    }

    return get_mail_id(data_src->src, &data_src->growth, data_src->is_egg);
}

u16 get_dex_index_raw(struct gen3_mon_data_unenc* data_src){
    if(!data_src->is_valid_gen3)
        return NO_DEX_INDEX;
    if(data_src->is_egg)
        return NO_DEX_INDEX;

    const u16* dex_conversion_bin_16 = (const u16*)dex_conversion_bin;
    u16 mon_index = get_mon_index_raw(data_src);
    return dex_conversion_bin_16[mon_index];
}

u8 decrypt_data(struct gen3_mon* src, u32* decrypted_dst) {
    // Decrypt the data
    u32 key = src->pid ^ src->ot_id;
    for(size_t i = 0; i < (ENC_DATA_SIZE>>2); i++)
        decrypted_dst[i] = src->enc_data[i] ^ key;
    
    // Validate the data
    u16* decrypted_dst_16 = (u16*)decrypted_dst;
    u16 checksum = 0;
    for(size_t i = 0; i < (ENC_DATA_SIZE>>1); i++)
        checksum += decrypted_dst_16[i];
    if(checksum != src->checksum)
        return 0;
    
    return 1;
}

void encrypt_data(struct gen3_mon* dst) {
    // Prepare the checksum
    u16 checksum = 0;
    for(size_t i = 0; i < (ENC_DATA_SIZE>>1); i++)
        checksum += ((u16*)dst->enc_data)[i];
    dst->checksum = checksum;
    
    // Encrypt the data
    u32 key = dst->pid ^ dst->ot_id;
    for(size_t i = 0; i < (ENC_DATA_SIZE>>2); i++)
        dst->enc_data[i] = dst->enc_data[i] ^ key;
}

u8 get_ivs_gen3_pure(u32 ivs, u8 stat_index) {
    if(stat_index >= GEN2_STATS_TOTAL)
        stat_index = GEN2_STATS_TOTAL-1;
    
    u8 real_stat_index = stat_index_conversion_gen3[stat_index];
    
    return (ivs >> (5*real_stat_index))&0x1F;
}

u8 get_ivs_gen3(struct gen3_mon_misc* misc, u8 stat_index) {
    if(stat_index >= GEN2_STATS_TOTAL)
        stat_index = GEN2_STATS_TOTAL-1;
    
    u32 ivs = (*(((u32*)misc)+1));
    return get_ivs_gen3_pure(ivs, stat_index);
}

u8 get_hidden_power_type_gen3_pure(u32 ivs) {
    u32 type = 0;
    for(int i = 0; i < GEN2_STATS_TOTAL; i++)
        type += (((ivs >> (5*i))&0x1F)&1)<<i;
    return Div(type*15, (1<<GEN2_STATS_TOTAL)-1);
}

u8 get_hidden_power_power_gen3_pure(u32 ivs) {
    u32 power = 0;
    for(int i = 0; i < GEN2_STATS_TOTAL; i++)
        power += ((((ivs >> (5*i))&0x1F)>>1)&1)<<i;
    return Div(power*40, (1<<GEN2_STATS_TOTAL)-1) + 30;
}

u8 get_hidden_power_type_gen3(struct gen3_mon_misc* misc) {
    u32 ivs = (*(((u32*)misc)+1));
    return get_hidden_power_type_gen3_pure(ivs);
}

u8 get_hidden_power_power_gen3(struct gen3_mon_misc* misc) {
    u32 ivs = (*(((u32*)misc)+1));
    return get_hidden_power_power_gen3_pure(ivs);
}

const u8* get_hidden_power_type_name_gen3_pure(u32 ivs) {
    return get_table_pointer(type_names_bin, get_hidden_power_type_gen3_pure(ivs));
}

const u8* get_hidden_power_type_name_gen3(struct gen3_mon_misc* misc) {
    u32 ivs = (*(((u32*)misc)+1));
    return get_hidden_power_type_name_gen3_pure(ivs);
}

const u8* get_nature_name(u32 pid) {
    return get_table_pointer(nature_names_bin, get_nature(pid));
}

char get_nature_symbol(u32 pid, u8 stat_index) {
    u8 nature = get_nature(pid);
    u8 boosted_stat = pokemon_natures_bin[(2*nature)];
    u8 nerfed_stat = pokemon_natures_bin[(2*nature)+1];
    if(boosted_stat == nerfed_stat)
        return ' ';
    if(boosted_stat == stat_index)
        return '+';
    if(nerfed_stat == stat_index)
        return '-';
    return ' ';
}

s32 get_level_exp_mon_index(u16 mon_index, u8 level) {
    return exp_table[level].exp_kind[pokemon_exp_groups_bin[mon_index]];
}

s32 get_proper_exp(struct gen3_mon* src, struct gen3_mon_growth* growth, u8 is_egg, u8 deoxys_form) {
    u8 level = to_valid_level_gen3(src);
    s32 exp = growth->exp;
    u16 mon_index = get_mon_index(growth->species, src->pid, 0, deoxys_form);

    if(is_egg) {
        level = EGG_LEVEL_GEN3;
        exp = get_level_exp_mon_index(mon_index, level);
    }

    s32 min_exp = get_level_exp_mon_index(mon_index, level);
    s32 max_exp = min_exp;
    if(level == MAX_LEVEL)
        exp = min_exp;
    else
        max_exp = get_level_exp_mon_index(mon_index, level+1)-1;
    if(exp < min_exp)
        exp = min_exp;
    if(exp > max_exp)
        exp = max_exp;
    if(exp < 0)
        exp = 0;

    return exp;
}

s32 get_proper_exp_raw(struct gen3_mon_data_unenc* data_src) {
    if(!data_src->is_valid_gen3)
        return 0;

    return get_proper_exp(data_src->src, &data_src->growth, data_src->is_egg, data_src->deoxys_form);
}

void make_evs_legal_gen3(struct gen3_mon_evs* evs) {
    u8 sum = 0;

    // Are they within the cap?
    for(int i = 0; i < EVS_TOTAL_GEN3; i++) {
        if(sum + evs->evs[i] > MAX_EVS)
            evs->evs[i] -= sum + evs->evs[i] - MAX_EVS;
        sum += evs->evs[i];
    }
}

u8 get_evs_gen3(struct gen3_mon_evs* evs, u8 stat_index) {
    if(stat_index >= EVS_TOTAL_GEN3)
        stat_index = EVS_TOTAL_GEN3-1;

    u8 own_evs[] = {0,0,0,0,0,0};
    u8 sum = 0;
    
    // Are they within the cap?
    for(int i = 0; i < EVS_TOTAL_GEN3; i++) {
        if(sum + evs->evs[i] > MAX_EVS)
            own_evs[i] = MAX_EVS-sum;
        else
            own_evs[i] = evs->evs[i];
        sum += own_evs[i];
    }
    
    u8 real_stat_index = stat_index_conversion_gen3[stat_index];
    
    return own_evs[real_stat_index];
}

u16 calc_stats_gen3(u16 species, u32 pid, u8 stat_index, u8 level, u8 iv, u8 ev, u8 deoxys_form) {
    if(!is_species_valid(species))
        species = 0;
    if(stat_index >= GEN2_STATS_TOTAL)
        stat_index = GEN2_STATS_TOTAL-1;

    if((species == SHEDINJA_SPECIES) && (stat_index == HP_STAT_INDEX))
        return 1;

    u8 nature = get_nature(pid);
    u16 mon_index = get_mon_index(species, pid, 0, deoxys_form);

    u16 base = 5;
    if(stat_index == HP_STAT_INDEX)
        base = level + 10;
    u8 boosted_stat = pokemon_natures_bin[(2*nature)];
    u8 nerfed_stat = pokemon_natures_bin[(2*nature)+1];
    u16 stat = base + Div(((2*stats_table_gen3[mon_index].stats[stat_index]) + iv + (ev >> 2)) * level, 100);
    if(boosted_stat == nerfed_stat)
        return stat;
    if(boosted_stat == stat_index)
        return stat + Div(stat, 10);
    if(nerfed_stat == stat_index)
        return stat - Div(stat, 10);
    // Makes the compiler happy
    return stat;
}

u16 calc_stats_gen3_raw(struct gen3_mon_data_unenc* data_src, u8 stat_index) {    
    if(!data_src->is_valid_gen3)
        return 0;

    return calc_stats_gen3(data_src->growth.species, data_src->src->pid, stat_index, to_valid_level_gen3(data_src->src), get_ivs_gen3(&data_src->misc, stat_index), get_evs_gen3(&data_src->evs, stat_index), data_src->deoxys_form);
}

u16 calc_stats_gen3_raw_alternative(struct gen3_mon_data_unenc* data_src, struct alternative_data_gen3* data_alt, u8 stat_index) {
    if(!data_src->is_valid_gen3)
        return 0;

    return calc_stats_gen3(data_src->growth.species, data_alt->pid, stat_index, to_valid_level_gen3(data_src->src), get_ivs_gen3_pure(data_alt->ivs, stat_index), get_evs_gen3(&data_src->evs, stat_index), data_src->deoxys_form);
}

u8 to_valid_level(u8 level) {
    if(level < MIN_LEVEL)
        return MIN_LEVEL;
    if(level > MAX_LEVEL)
        return MAX_LEVEL;
    return level;
}

u8 to_valid_level_gen3(struct gen3_mon* src) {
    return to_valid_level(src->level);
}

void recalc_stats_gen3(struct gen3_mon_data_unenc* data_dst, struct gen3_mon* dst) {
    // Calculate stats
    dst->curr_hp = calc_stats_gen3_raw(data_dst, HP_STAT_INDEX);
    for(int i = 0; i < GEN2_STATS_TOTAL; i++)
        dst->stats[stat_index_conversion_gen3[i]] = calc_stats_gen3_raw(data_dst, i);
    
    // Reset the status
    dst->status = 0;
}

u8 is_pokerus_strain_valid(u8 pokerus_byte) {
    if(!pokerus_byte)
        return 1;
    if(!((pokerus_byte >> 4) & 7))
        return 0;
    return 1;
}

u8 get_pokerus_strain_max_days(u8 pokerus_byte) {
    if(!((pokerus_byte >> 4) & 7))
        return 0;
    return ((pokerus_byte >> 4) & 3) + 1;
}

u8 are_pokerus_days_valid(u8 pokerus_byte) {
    if(!pokerus_byte)
        return 1;
    if(!is_pokerus_strain_valid(pokerus_byte))
        return 0;
    if((pokerus_byte & 0xF) > get_pokerus_strain_max_days(pokerus_byte))
        return 0;
    return 1;
}

void sanitize_pokerus(u8* pokerus_byte) {
    if(!is_pokerus_strain_valid(*pokerus_byte))
        *pokerus_byte |= 0x10;
    if(!are_pokerus_days_valid(*pokerus_byte))
        *pokerus_byte = ((*pokerus_byte) & 0xF0) | get_pokerus_strain_max_days(*pokerus_byte);
}

void place_and_encrypt_gen3_data(struct gen3_mon_data_unenc* src, struct gen3_mon* dst) {
    u8 index = get_index_key(dst->pid);
    
    size_t pos_data = (ENC_DATA_SIZE>>2)*((enc_positions[index] >> 0)&3);
    for(size_t i = 0; i < sizeof(struct gen3_mon_growth); i++)
        ((u8*)dst->enc_data)[pos_data+i] = ((u8*)(&src->growth))[i];
    pos_data = (ENC_DATA_SIZE>>2)*((enc_positions[index] >> 2)&3);
    for(size_t i = 0; i < sizeof(struct gen3_mon_attacks); i++)
        ((u8*)dst->enc_data)[pos_data+i] = ((u8*)(&src->attacks))[i];
    pos_data = (ENC_DATA_SIZE>>2)*((enc_positions[index] >> 4)&3);
    for(size_t i = 0; i < sizeof(struct gen3_mon_evs); i++)
        ((u8*)dst->enc_data)[pos_data+i] = ((u8*)(&src->evs))[i];
    pos_data = (ENC_DATA_SIZE>>2)*((enc_positions[index] >> 6)&3);
    for(size_t i = 0; i < sizeof(struct gen3_mon_misc); i++)
        ((u8*)dst->enc_data)[pos_data+i] = ((u8*)(&src->misc))[i];
    
    encrypt_data(dst);
}

const u8* get_default_trainer_name(u8 language) {
    return get_table_pointer(trainer_names_bin, get_valid_language(language));
}

const u8* get_celebi_trainer_name(u8 language) {
    return get_table_pointer(trainer_names_celebi_bin, get_valid_language(language));
}

const u8* get_validity_keyboard(u8 language, u8 load_special_keyboard) {
    language = get_valid_language(language);
    u8 kind = language_keyboard_kind[language];
    const u8* validity_keyboard = gen3_int_valid_chars_bin;
    if(kind == 0)
        validity_keyboard = gen3_jap_valid_chars_bin;
    else if(kind == 1)
        validity_keyboard = gen3_int_valid_chars_bin;
    else if(kind == 2)
        validity_keyboard = gen3_german_valid_chars_bin;
    if(load_special_keyboard)
        validity_keyboard = gen3_spanish_special_valid_chars_bin;
    return validity_keyboard;
}

u8 use_special_keyboard(struct gen3_mon_misc* misc, u8 language) {
    if(language != SPANISH_LANGUAGE)
        return 0;
    // I could make stricter checks,
    // but these should be fine for now...
    if((misc->met_location == TRADE_MET) || (misc->met_location == EVENT_MET))
        return 1;
    if((misc->met_location == DUKING_TRADE_LOCATION) && (((misc->origins_info>>7)&0xF) == COLOSSEUM_CODE))
        return 1;
    return 0;
}

void sanitize_nickname(u8* nickname, u8 language, u8 is_egg, u16 species, u8 load_special_keyboard) {
    language = get_valid_language(language);
    const u8* validity_keyboard = get_validity_keyboard(language, load_special_keyboard);
    size_t name_limit = NICKNAME_GEN3_SIZE;
    if(GET_LANGUAGE_IS_JAPANESE(language))
        name_limit = NICKNAME_JP_GEN3_SIZE;

    sanitize_name_gen3(nickname, get_pokemon_name_pure(species, 0, language), validity_keyboard, NICKNAME_GEN3_MAX_SIZE, name_limit);

    if(is_egg)
        text_gen3_copy(get_pokemon_name_pure(species, 1, JAPANESE_LANGUAGE), nickname, NICKNAME_JP_GEN3_SIZE, name_limit);

    limit_name_gen3(nickname, NICKNAME_GEN3_MAX_SIZE, name_limit);
}

void sanitize_ot_name(u8* ot_name, u8 max_size, u8 language, u8 load_special_keyboard) {
    language = get_valid_language(language);
    const u8* validity_keyboard = get_validity_keyboard(language, load_special_keyboard);
    size_t name_limit = OT_NAME_GEN3_SIZE;
    if(GET_LANGUAGE_IS_JAPANESE(language))
        name_limit = OT_NAME_JP_GEN3_SIZE;

    sanitize_name_gen3(ot_name, get_default_trainer_name(language), validity_keyboard, max_size, name_limit);
    limit_name_gen3(ot_name, max_size, name_limit);
}

void set_deoxys_form_inner(struct gen3_mon_data_unenc* dst, u8 main_version, u8 sub_version) {
    // Display the different Deoxys forms
    dst->deoxys_form = DEOXYS_NORMAL;
    if(main_version == E_MAIN_GAME_CODE)
        dst->deoxys_form = DEOXYS_SPE;
    if(main_version == FRLG_MAIN_GAME_CODE) {
        dst->deoxys_form = DEOXYS_ATK;
        if(sub_version == LG_SUB_GAME_CODE)
            dst->deoxys_form = DEOXYS_DEF;
    }
}

void set_deoxys_form(struct gen3_mon_data_unenc* dst, u8 main_version, u8 sub_version) {
    if(!dst->is_valid_gen3)
        return;

    set_deoxys_form_inner(dst, main_version, sub_version);

    // Reclac the stats if needed
    if(dst->growth.species == DEOXYS_SPECIES)
        recalc_stats_gen3(dst, dst->src);
}

u8 decrypt_to_data_unenc(struct gen3_mon* src, struct gen3_mon_data_unenc* dst) {
    u32 decryption[ENC_DATA_SIZE>>2];

    // Initial data decryption
    if(!decrypt_data(src, decryption))
        return 0;

    u8 index = get_index_key(src->pid);

    struct gen3_mon_growth* tmp_growth = (struct gen3_mon_growth*)&decryption[((ENC_DATA_SIZE>>2)*((enc_positions[index] >> 0)&3))>>2];
    struct gen3_mon_attacks* tmp_attacks = (struct gen3_mon_attacks*)&decryption[((ENC_DATA_SIZE>>2)*((enc_positions[index] >> 2)&3))>>2];
    struct gen3_mon_evs* tmp_evs = (struct gen3_mon_evs*)&decryption[((ENC_DATA_SIZE>>2)*((enc_positions[index] >> 4)&3))>>2];
    struct gen3_mon_misc* tmp_misc = (struct gen3_mon_misc*)&decryption[((ENC_DATA_SIZE>>2)*((enc_positions[index] >> 6)&3))>>2];

    for(size_t i = 0; i < sizeof(struct gen3_mon_growth); i++)
        ((u8*)&dst->growth)[i] = ((u8*)tmp_growth)[i];
    for(size_t i = 0; i < sizeof(struct gen3_mon_attacks); i++)
        ((u8*)&dst->attacks)[i] = ((u8*)tmp_attacks)[i];
    for(size_t i = 0; i < sizeof(struct gen3_mon_evs); i++)
        ((u8*)&dst->evs)[i] = ((u8*)tmp_evs)[i];
    for(size_t i = 0; i < sizeof(struct gen3_mon_misc); i++)
        ((u8*)&dst->misc)[i] = ((u8*)tmp_misc)[i];

    return 1;
}

u8 is_item_valid(u16 item) {
    if(item > LAST_VALID_GEN_3_ITEM)
        return 0;
    if(invalid_held_items_bin[item>>3] & (1<<(item&7)))
        return 0;
    return 1;
}

u8 is_species_valid(u16 species) {
    if(species > LAST_VALID_GEN_3_MON)
        return 0;
    if(invalid_pokemon_bin[species>>3] & (1<<(species&7)))
        return 0;
    return 1;
}

void process_gen3_data(struct gen3_mon* src, struct gen3_mon_data_unenc* dst, u8 main_version, u8 sub_version) {
    dst->src = src;
    
    // Default roamer values
    dst->can_roamer_fix = 0;
    dst->fix_has_altered_ot = 0;
    
    // Default learnable moves values
    dst->learnable_moves = NULL;
    dst->successfully_decrypted = 0;
    
    // Initial data decryption
    if(!decrypt_to_data_unenc(src, dst)) {
        dst->is_valid_gen3 = 0;
        dst->is_valid_gen2 = 0;
        dst->is_valid_gen1 = 0;
        return;
    }
    
    dst->successfully_decrypted = 1;
    
    struct gen3_mon_growth* growth = &dst->growth;
    struct gen3_mon_attacks* attacks = &dst->attacks;
    struct gen3_mon_evs* evs = &dst->evs;
    struct gen3_mon_misc* misc = &dst->misc;
    
    // Evolution testing
    //if((growth->species == ESPEON_SPECIES) || (growth->species == UMBREON_SPECIES))
    //    growth->species = EEVEE_SPECIES;
    //if(growth->species == ALAKAZAM_SPECIES)
    //    growth->species = KADABRA_SPECIES;
    // Pokerus testing
    //misc->pokerus = 0;
    
    // Species checks
    if(!is_species_valid(growth->species)) {
        dst->is_valid_gen3 = 0;
        dst->is_valid_gen2 = 0;
        dst->is_valid_gen1 = 0;
        return;
    }
    
    // Obedience checks
    if((growth->species == MEW_SPECIES) || (growth->species == DEOXYS_SPECIES))
        misc->obedience = 1;
    
    // Display the different Deoxys forms
    set_deoxys_form_inner(dst, main_version, sub_version);
    
    make_evs_legal_gen3(evs);
    
    if(!make_moves_legal_gen3(attacks, growth)) {
        dst->is_valid_gen3 = 0;
        dst->is_valid_gen2 = 0;
        dst->is_valid_gen1 = 0;
        return;
    }
    
    if(!is_ability_valid(growth->species, src->pid, misc->ability, misc->met_location, (misc->origins_info>>7) & 0xF, dst->deoxys_form)) {
        dst->is_valid_gen3 = 0;
        dst->is_valid_gen2 = 0;
        dst->is_valid_gen1 = 0;
        return;
    }

    // Bad egg checks
    if(src->is_bad_egg) {
        dst->successfully_decrypted = 0;
        dst->is_valid_gen3 = 0;
        dst->is_valid_gen2 = 0;
        dst->is_valid_gen1 = 0;
        return;
    }
    
    // Sanitize language data too...
    if((src->language < FIRST_VALID_LANGUAGE) || (src->language >= NUM_LANGUAGES) || (src->language == KOREAN_LANGUAGE))
        src->language = DEFAULT_NAME_BAD_LANGUAGE;
    
    // Sanitize pokerus data...
    sanitize_pokerus(&misc->pokerus);
    
    // We reuse this SOOOO much...
    dst->is_egg = is_egg_gen3(src, misc);
    if(dst->is_egg) {
        src->use_egg_name = 1;
        src->language = JAPANESE_LANGUAGE;    
        // Eggs should not have items or mails
        growth->item = NO_ITEM_ID;
        src->mail_id = GEN3_NO_MAIL;
        // Eggs cannot have pokerus
        misc->pokerus = 0;
        // Eggs cannot have PP UPs
        growth->pp_bonuses = 0;
        src->level = EGG_LEVEL_GEN3;
        if(get_fast_hatch_eggs())
            growth->friendship = 0;
    }
    
    if((!is_item_valid(growth->item)) || (growth->item == ENIGMA_BERRY_ID))
        growth->item = NO_ITEM_ID;
    
    src->level = to_valid_level_gen3(src);
    
    growth->exp = get_proper_exp(src, growth, dst->is_egg, dst->deoxys_form);
    
    // Fix Nincada evolving to Shedinja in a Japanese game
    if((!dst->is_egg) && (growth->species == SHEDINJA_SPECIES))
        if((src->language != JAPANESE_LANGUAGE) && text_gen3_is_same(src->nickname, get_pokemon_name_pure(SHEDINJA_SPECIES, 0, JAPANESE_LANGUAGE), NICKNAME_GEN3_MAX_SIZE, NICKNAME_GEN3_MAX_SIZE))
            text_gen3_copy(get_pokemon_name_pure(SHEDINJA_SPECIES, 0, src->language), src->nickname, NICKNAME_GEN3_SIZE, NICKNAME_GEN3_SIZE);

    u8 special_keyboard = use_special_keyboard(misc, src->language);
    // Sanitize text to avoid crashes in-game
    sanitize_nickname(src->nickname, src->language, dst->is_egg, growth->species, special_keyboard);
    sanitize_ot_name(src->ot_name, OT_NAME_GEN3_MAX_SIZE, src->language, special_keyboard);

    // Set the new "cleaned" data
    place_and_encrypt_gen3_data(dst, src);
    
    dst->is_valid_gen3 = 1;
    
    // Recalc the stats, always
    recalc_stats_gen3(dst, src);

    // Handle roamers
    preload_if_fixable(dst);
}

void clean_mail_gen3(struct mail_gen3* mail, struct gen3_mon* mon){
    for(size_t i = 0; i < MAIL_WORDS_SIZE; i++)
        mail->words[i] = 0;
    for(size_t i = 0; i < (OT_NAME_GEN3_MAX_SIZE+1); i++)
        mail->ot_name[i] = GEN3_EOL;
    mail->ot_id = 0;
    mail->species = BULBASAUR_SPECIES;
    mail->item = 0;

    mon->mail_id = GEN3_NO_MAIL;
}

void evolve_mon(struct gen3_mon* mon, struct gen3_mon_data_unenc* mon_data, u16 evolved_species, u8 consume_item, const u16* learnsets) {
    // Load the pre-evo name
    mon_data->pre_evo_string = get_pokemon_name_raw(mon_data);
    mon_data->pre_evo_string_length = get_pokemon_name_raw_language_limit(mon_data);
    // Determine if we're going to change the name
    u8 replace_name = 0;
    if(text_gen3_is_same(get_pokemon_name_pure(mon_data->growth.species, mon_data->is_egg, mon->language), mon->nickname, GET_LANGUAGE_NICKNAME_LIMIT(mon->language), GET_LANGUAGE_NICKNAME_LIMIT(mon->language)))
        replace_name = 1;
    // Evolve
    mon_data->growth.species = evolved_species;
    // Consume the evolution item, if needed
    if(consume_item)
        mon_data->growth.item = NO_ITEM_ID;
    // Update the name, if needed
    if(replace_name)
        text_gen3_copy(get_pokemon_name_pure(mon_data->growth.species, mon_data->is_egg, mon->language), mon->nickname, GET_LANGUAGE_NICKNAME_LIMIT(mon->language), GET_LANGUAGE_NICKNAME_LIMIT(mon->language));
    
    // Update growth
    place_and_encrypt_gen3_data(mon_data, mon);
    
    // Calculate stats
    recalc_stats_gen3(mon_data, mon);
    
    // Find if the mon should learn new moves
    const struct learnset_data_mon_moves* found_learnset = extract_learnable_moves(learnsets, mon_data->growth.species, mon->level);
    if(found_learnset != NULL)
        mon_data->learnable_moves = found_learnset;
}

const u16* get_special_evolutions(u16 species) {
    const u16* special_evolutions = (const u16*)special_evolutions_bin;
    u16 num_entries = special_evolutions[0];
    for(size_t i = 0; i < num_entries; i++) {
        size_t pos = special_evolutions[1+i]>>1;
        if(special_evolutions[pos] == species)
            return &special_evolutions[pos+1];
    }
    return NULL;
}

u8 own_menu_evolve(struct gen3_mon_data_unenc* mon_data, u8 index) {
    const u16* learnsets = (const u16*)learnset_evos_gen3_bin;

    if((!mon_data->is_valid_gen3) || mon_data->is_egg)
        return 0;

    const u16* special_evolutions = get_special_evolutions(mon_data->growth.species);

    mon_data->growth.friendship = BASE_FRIENDSHIP;

    if(!special_evolutions) {
        if(get_evolve_without_trade())
            return trade_evolve(mon_data->src, mon_data, 3);
        else
            return 0;
    }

    u16 num_evos = special_evolutions[0];

    if(index >= num_evos)
        return 0;

    u16 evolved_species = special_evolutions[1+index];

    if((!evolved_species) || (!is_species_valid(evolved_species)))
        return 0;

    mon_data->src->level = to_valid_level_gen3(mon_data->src);
    mon_data->src->level += 1;
    if(mon_data->src->level >= MAX_LEVEL)
        mon_data->src->level = MAX_LEVEL;
    mon_data->growth.exp = get_proper_exp_raw(mon_data);
    // The actual evolution logic
    evolve_mon(mon_data->src, mon_data, evolved_species, 0, learnsets);

    return 1;
}

u16 get_own_menu_evolution_species(struct gen3_mon_data_unenc* mon_data, u8 index, u8* is_special) {
    u8 useless = 0;
    *is_special = 0;

    if((!mon_data->is_valid_gen3) || mon_data->is_egg)
        return 0;

    const u16* special_evolutions = get_special_evolutions(mon_data->growth.species);

    if(!special_evolutions) {
        if(get_evolve_without_trade())
            return get_trade_evolution(mon_data->growth.species, mon_data->growth.item, 3, &useless, &useless);
        else
            return 0;
    }

    u16 num_evos = special_evolutions[0];

    if(index >= num_evos)
        return 0;

    u16 evolved_species = special_evolutions[1+index];

    if((!evolved_species) || (!is_species_valid(evolved_species)))
        evolved_species = 0;

    *is_special = 1;
    return evolved_species;
}

u16 can_own_menu_evolve(struct gen3_mon_data_unenc* mon_data) {
    u8 useless = 0;

    if((!mon_data->is_valid_gen3) || mon_data->is_egg)
        return 0;

    const u16* special_evolutions = get_special_evolutions(mon_data->growth.species);
    if(special_evolutions)
        return special_evolutions[0];

    if(get_evolve_without_trade() && get_trade_evolution(mon_data->growth.species, mon_data->growth.item, 3, &useless, &useless))
        return 1;

    return 0;
}

u16 get_trade_evolution(u16 species, u16 item, u8 curr_gen, u8* consume_item, u8* cross_gen_evo) {
    const u16* trade_evolutions = (const u16*)trade_evolutions_bin;
    u16 num_entries = trade_evolutions[0];
    const u16* trade_evolutions_base_ids = trade_evolutions+1;
    const u16* trade_evolutions_item_ids = trade_evolutions+1+num_entries;
    const u16* trade_evolutions_evo_ids = trade_evolutions+1+(2*num_entries);
    u16 max_index = LAST_VALID_GEN_3_MON;
    if(curr_gen == 1)
        max_index = LAST_VALID_GEN_1_MON;
    if(curr_gen == 2)
        max_index = LAST_VALID_GEN_2_MON;
    *consume_item = 0;
    *cross_gen_evo = 0;
    for(int i = 0; i < num_entries; i++)
        if(species == trade_evolutions_base_ids[i])
            if((!trade_evolutions_item_ids[i]) || (item == trade_evolutions_item_ids[i]))
                if(((ACT_AS_GEN1_TRADE && (curr_gen == 1)) || (item != EVERSTONE_ID)))
                    if((trade_evolutions_evo_ids[i] <= max_index) || get_allow_cross_gen_evos()) {
                        if(trade_evolutions_item_ids[i])
                            *consume_item = 1;
                        if(trade_evolutions_evo_ids[i] > max_index)
                            *cross_gen_evo = 1;
                        return trade_evolutions_evo_ids[i];
                    }
    return 0;
}

u8 trade_evolve(struct gen3_mon* mon, struct gen3_mon_data_unenc* mon_data, u8 curr_gen) {
    const u16* learnsets = (const u16*)learnset_evos_gen3_bin;

    if(curr_gen == 1)
        learnsets = (const u16*)learnset_evos_gen1_bin;
    if(curr_gen == 2)
        learnsets = (const u16*)learnset_evos_gen2_bin;
    
    if((!mon_data->is_valid_gen3) || mon_data->is_egg)
        return 0;
    
    u8 consume_item = 0;
    u8 cross_gen_evo = 0;
    
    u16 evolved_species = get_trade_evolution(mon_data->growth.species, mon_data->growth.item, curr_gen, &consume_item, &cross_gen_evo);
    
    if((!evolved_species) || (!is_species_valid(evolved_species)))
        return 0;
    
    if(cross_gen_evo)
        learnsets = (const u16*)learnset_evos_gen3_bin;
    
    // The actual evolution logic
    evolve_mon(mon, mon_data, evolved_species, consume_item, learnsets);

    return 1;
}

const struct learnset_data_mon_moves* extract_learnable_moves(const u16* gen_learnsets, u16 species, u8 level) {
    // Find if the mon should learn new moves
    const u16* found_learnset = get_learnset_for_species(gen_learnsets, species);
    if(found_learnset == NULL) 
        return NULL;
    u16 num_levels = found_learnset[0];
    const u16* level_learnsets = found_learnset+1;
    for(int j = 0; j < num_levels; j++) {
        u16 moves_level = level_learnsets[0];
        if(moves_level == level)
            return (const struct learnset_data_mon_moves*)(level_learnsets+1);
        u16 num_moves = level_learnsets[1];
        level_learnsets += num_moves + 2;
    }
    return NULL;
}

enum LEARNABLE_MOVES_RETVAL learn_if_possible(struct gen3_mon_data_unenc* mon, u32 index) {
    if(mon->learnable_moves == NULL) {
        place_and_encrypt_gen3_data(mon, mon->src);
        return COMPLETED;
    }
    
    u16 num_moves = mon->learnable_moves->num_moves;
    if(index >= num_moves) {
        place_and_encrypt_gen3_data(mon, mon->src);
        return COMPLETED;
    }
    
    u16 move = mon->learnable_moves->moves[index];
    
    if((!is_move_valid(move, LAST_VALID_GEN_3_MOVE)) || (!move))
        return SKIPPED;
    
    struct gen3_mon_attacks* attacks = &mon->attacks;
    struct gen3_mon_growth* growth = &mon->growth;
    
    for(size_t i = 0; i < MOVES_SIZE; i++)
        if(attacks->moves[i] == move)
            return SKIPPED;
    
    for(size_t i = 0; i < MOVES_SIZE; i++)
        if(!is_move_valid(attacks->moves[i], LAST_VALID_GEN_3_MOVE)) {
            teach_move(attacks, growth, move, i);
            return LEARNT;
        }
    
    if(mon->is_egg) {
        for(size_t i = 0; i < MOVES_SIZE-1; i++)
            swap_move(attacks, growth, i, i+1);
        teach_move(attacks, growth, move, MOVES_SIZE-1);
        return LEARNT;
    }

    return LEARNABLE;
}

u8 forget_and_learn_move(struct gen3_mon_data_unenc* mon, u32 index, u32 forget_index) {
    if(mon->learnable_moves == NULL)
        return 0;
    
    u16 num_moves = mon->learnable_moves->num_moves;
    if(index >= num_moves)
        return 0;
    
    if(forget_index >= MOVES_SIZE)
        return 0;
    
    u16 move = mon->learnable_moves->moves[index];

    if((!is_move_valid(move, LAST_VALID_GEN_3_MOVE)) || (!move))
        return 0;

    struct gen3_mon_attacks* attacks = &mon->attacks;
    struct gen3_mon_growth* growth = &mon->growth;
    
    // Do not limit HMs' replaceability for better user experience,
    // even though normal games do it
    teach_move(attacks, growth, move, forget_index);
    
    return 1;
}
