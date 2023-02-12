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

#define UNOWN_B_START 415
#define INITIAL_MAIL_GEN3 121
#define LAST_MAIL_GEN3 132

#define ACT_AS_GEN1_TRADE 1

#define PID_POSITIONS 24

const u8* get_pokemon_name_pure(int, u32, u8);
const u8* get_item_name(int, u8);
u8 get_ability_pokemon(int, u32, u8, u8, u8);
u8 is_ability_valid(u16, u32, u8, u8, u8, u8);
const u8* get_pokemon_sprite_pointer(int, u32, u8, u8);
u8 get_pokemon_sprite_info(int, u32, u8, u8);
u8 get_palette_references(int, u32, u8, u8);
void load_pokemon_sprite(int, u32, u8, u8, u8, u8, u16, u16);
u8 make_moves_legal_gen3(struct gen3_mon_attacks*);
u8 is_egg_gen3(struct gen3_mon*, struct gen3_mon_misc*);
u8 is_shiny_gen3(u32, u32, u8, u32);
u8 get_mail_id(struct gen3_mon*, struct gen3_mon_growth*, u8);
u8 decrypt_data(struct gen3_mon* src, u32* decrypted_dst);
void encrypt_data(struct gen3_mon* dst);
u8 get_hidden_power_type_gen3_pure(u32);
u8 get_hidden_power_type_gen3(struct gen3_mon_misc*);
void make_evs_legal_gen3(struct gen3_mon_evs*);
u8 to_valid_level_gen3(struct gen3_mon*);

// Order is G A E M. Initialized by init_enc_positions
u8 enc_positions[PID_POSITIONS];

u8 gender_thresholds_gen3[TOTAL_GENDER_KINDS] = {127, 0, 31, 63, 191, 225, 254, 255, 0, 254};
u8 stat_index_conversion_gen3[] = {0, 1, 2, 4, 5, 3};

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
    if(index > LAST_VALID_GEN_3_MON)
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

const u8* get_pokemon_name(int index, u32 pid, u8 is_egg, u8 deoxys_form){
    return get_table_pointer(pokemon_names_bin, get_mon_index(index, pid, is_egg, deoxys_form));
}

const u8* get_pokemon_name_pure(int index, u32 pid, u8 is_egg){
    u16 mon_index = get_mon_index(index, pid, is_egg, 0);
    if ((index == UNOWN_SPECIES) && !is_egg)
        mon_index = UNOWN_REAL_NAME_POS;
    if ((index == DEOXYS_SPECIES) && !is_egg)
        mon_index = DEOXYS_SPECIES;
    return get_table_pointer(pokemon_names_bin, mon_index);
}

const u8* get_pokemon_name_raw(struct gen3_mon_data_unenc* data_src){
    if(!data_src->is_valid_gen3)
        return get_pokemon_name(0,0,0,0);

    return get_pokemon_name(data_src->growth.species, data_src->src->pid, data_src->is_egg, data_src->deoxys_form);
}

const u8* get_item_name(int index, u8 is_egg){
    if(is_egg)
        index = 0;
    if(index > LAST_VALID_GEN_3_ITEM)
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
    if((!index) || (index > LAST_VALID_GEN_3_ITEM))
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

void load_pokemon_sprite_raw(struct gen3_mon_data_unenc* data_src, u16 y, u16 x){
    if(!data_src->is_valid_gen3)
        load_pokemon_sprite(0, 0, 0, 0, has_item_raw(data_src), has_mail_raw(data_src), y, x);

    load_pokemon_sprite(data_src->growth.species, data_src->src->pid, data_src->is_egg, data_src->deoxys_form, has_item_raw(data_src), has_mail_raw(data_src), y, x);
}

const u8* get_move_name_gen3(struct gen3_mon_attacks* attacks, u8 slot){
    if(slot >= MOVES_SIZE)
        slot = MOVES_SIZE-1;
    
    u16 move = attacks->moves[slot];
    
    if(move > LAST_VALID_GEN_3_MOVE)
        move = 0;
    
    return get_table_pointer(move_names_bin, move);
}

u8 make_moves_legal_gen3(struct gen3_mon_attacks* attacks){
    u8 previous_moves[MOVES_SIZE];
    u8 curr_slot = 0;
    
    for(size_t i = 0; i < MOVES_SIZE; i++) {
        if((attacks->moves[i] != 0) && (attacks->moves[i] <= LAST_VALID_GEN_3_MOVE)) {
            u8 found = 0;
            for(int j = 0; j < curr_slot; j++)
                if(attacks->moves[i] == previous_moves[j]){
                    found = 1;
                    attacks->moves[i] = 0;
                    break;
                }
            if(!found)
                previous_moves[curr_slot++] = attacks->moves[i];
        }
        else
            attacks->moves[i] = 0;
    }
    
    if(curr_slot)
        return 1;
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

s32 get_proper_exp(struct gen3_mon* src, struct gen3_mon_growth* growth, u8 deoxys_form) {
    u8 level = to_valid_level_gen3(src);
    
    u16 mon_index = get_mon_index(growth->species, src->pid, 0, deoxys_form);
    
    s32 exp = growth->exp;
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

    return get_proper_exp(data_src->src, &data_src->growth, data_src->deoxys_form);
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
    if(species > LAST_VALID_GEN_3_MON)
        species = 0;
    if(stat_index >= GEN2_STATS_TOTAL)
        stat_index = GEN2_STATS_TOTAL-1;
    
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

void process_gen3_data(struct gen3_mon* src, struct gen3_mon_data_unenc* dst, u8 main_version, u8 sub_version) {
    dst->src = src;

    // Default roamer values
    dst->can_roamer_fix = 0;
    dst->fix_has_altered_ot = 0;

    u32 decryption[ENC_DATA_SIZE>>2];
    
    // Initial data decryption
    if(!decrypt_data(src, decryption)) {
        dst->is_valid_gen3 = 0;
        dst->is_valid_gen2 = 0;
        dst->is_valid_gen1 = 0;
        return;
    }
    
    u8 index = get_index_key(src->pid);
    
    // Makes the compiler happy
    struct gen3_mon_growth* growth = (struct gen3_mon_growth*)(((uintptr_t)decryption)+((ENC_DATA_SIZE>>2)*((enc_positions[index] >> 0)&3)));
    struct gen3_mon_attacks* attacks = (struct gen3_mon_attacks*)(((uintptr_t)decryption)+((ENC_DATA_SIZE>>2)*((enc_positions[index] >> 2)&3)));
    struct gen3_mon_evs* evs = (struct gen3_mon_evs*)(((uintptr_t)decryption)+((ENC_DATA_SIZE>>2)*((enc_positions[index] >> 4)&3)));;
    struct gen3_mon_misc* misc = (struct gen3_mon_misc*)(((uintptr_t)decryption)+((ENC_DATA_SIZE>>2)*((enc_positions[index] >> 6)&3)));;
    
    for(size_t i = 0; i < sizeof(struct gen3_mon_growth); i++)
        ((u8*)(&dst->growth))[i] = ((u8*)growth)[i];
    for(size_t i = 0; i < sizeof(struct gen3_mon_attacks); i++)
        ((u8*)(&dst->attacks))[i] = ((u8*)attacks)[i];
    for(size_t i = 0; i < sizeof(struct gen3_mon_evs); i++)
        ((u8*)(&dst->evs))[i] = ((u8*)evs)[i];
    for(size_t i = 0; i < sizeof(struct gen3_mon_misc); i++)
        ((u8*)(&dst->misc))[i] = ((u8*)misc)[i];
    
    // Species checks
    if((growth->species > LAST_VALID_GEN_3_MON) || (growth->species == 0)) {
        dst->is_valid_gen3 = 0;
        dst->is_valid_gen2 = 0;
        dst->is_valid_gen1 = 0;
        return;
    }
    
    // Obedience checks
    if((growth->species == MEW_SPECIES) || (growth->species == DEOXYS_SPECIES))
        misc->obedience = 1;
    
    // Display the different Deoxys forms
    dst->deoxys_form = DEOXYS_NORMAL;
    if(main_version == E_MAIN_GAME_CODE)
        dst->deoxys_form = DEOXYS_SPE;
    if(main_version == FRLG_MAIN_GAME_CODE) {
        if(sub_version == FR_SUB_GAME_CODE)
            dst->deoxys_form = DEOXYS_ATK;
        if(sub_version == LG_SUB_GAME_CODE)
            dst->deoxys_form = DEOXYS_DEF;
    }
    
    make_evs_legal_gen3(evs);
    
    if(!make_moves_legal_gen3(attacks)) {
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
        dst->is_valid_gen3 = 0;
        dst->is_valid_gen2 = 0;
        dst->is_valid_gen1 = 0;
        return;
    }
    
    // We reuse this SOOOO much...
    dst->is_egg = is_egg_gen3(src, misc);
    
    // Eggs should not have items or mails
    if(dst->is_egg) {
        growth->item = NO_ITEM_ID;
        src->mail_id = GEN3_NO_MAIL;
    }
    
    if((growth->item > LAST_VALID_GEN_3_ITEM) || (growth->item == ENIGMA_BERRY_ID))
        growth->item = NO_ITEM_ID;
    
    src->level = to_valid_level_gen3(src);
    
    growth->exp = get_proper_exp_raw(dst);
    
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
    for(size_t i = 0; i < OT_NAME_GEN3_SIZE+1; i++)
        mail->ot_name[i] = GEN3_EOL;
    mail->ot_id = 0;
    mail->species = BULBASAUR_SPECIES;
    mail->item = 0;

    mon->mail_id = GEN3_NO_MAIL;
}

u8 trade_evolve(struct gen3_mon* mon, struct gen3_mon_data_unenc* mon_data, const u16** learnset_ptr, u8 curr_gen) {
    *learnset_ptr = NULL;
    const u16* learnsets = (const u16*)learnset_evos_gen3_bin;
    const u16* trade_evolutions = (const u16*)trade_evolutions_bin;
    u16 max_index = LAST_VALID_GEN_3_MON;
    if(curr_gen == 1) {
        learnsets = (const u16*)learnset_evos_gen1_bin;
        max_index = LAST_VALID_GEN_1_MON;
    }
    if(curr_gen == 2) {
        learnsets = (const u16*)learnset_evos_gen2_bin;
        max_index = LAST_VALID_GEN_2_MON;
    }
    
    struct gen3_mon_growth* growth = &mon_data->growth;
    
    u8 found = 0;
    u16 num_entries = trade_evolutions[0];
    
    for(int i = 0; i < num_entries; i++)
        if(growth->species == trade_evolutions[1+i])
            if((!trade_evolutions[1+(2*i)]) || (growth->item == trade_evolutions[1+(2*i)]))
                if((trade_evolutions[1+(3*i)] <= max_index) && ((ACT_AS_GEN1_TRADE && (curr_gen == 1)) || (growth->item != EVERSTONE_ID))) {
                    found = 1;
                    //Evolve
                    growth->species = trade_evolutions[1+(3*i)];
                    // Consume the evolution item, if needed
                    if(growth->item == trade_evolutions[1+(2*i)])
                        growth->item = NO_ITEM_ID;
                    break;
                }
    
    if(!found)
        return 0;
    
    // Update growth
    place_and_encrypt_gen3_data(mon_data, mon);
    
    // Calculate stats
    recalc_stats_gen3(mon_data, mon);
    
    // Find if the mon should learn new moves
    num_entries = learnsets[0];
    for(int i = 0; i < num_entries; i++) {
        u16 base_pos = learnsets[1+i] >> 1;
        if(learnsets[base_pos++] == growth->species) {
            u16 num_levels = learnsets[base_pos++];
            for(int j = 0; j < num_levels; j++) {
                u16 level = learnsets[base_pos++];
                if(level == mon->level) {
                    *learnset_ptr = &learnsets[base_pos];
                    break;
                }
            }
            break;
        }
    }

    return 1;
}
