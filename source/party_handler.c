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

#include "pokemon_moves_pp_bin.h"
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
#include "gen2_names_jap_bin.h"
#include "sprites_cmp_bin.h"
#include "sprites_info_bin.h"
#include "palettes_references_bin.h"
#include "item_gen3_to_12_bin.h"
#include "item_gen12_to_3_bin.h"
#include "gen3_to_1_conv_table_bin.h"
#include "gen1_to_3_conv_table_bin.h"
#include "pokemon_exp_groups_bin.h"
#include "exp_table_bin.h"
#include "pokemon_stats_bin.h"
#include "pokemon_stats_gen1_bin.h"
#include "pokemon_types_gen1_bin.h"
#include "pokemon_natures_bin.h"
#include "pokemon_abilities_bin.h"
#include "encounter_types_bin.h"
#include "egg_locations_bin.h"
#include "encounters_static_bin.h"
#include "encounters_roamers_bin.h"
#include "encounters_unown_bin.h"
#include "encounter_types_gen2_bin.h"
#include "special_encounters_gen2_bin.h"
#include "trade_evolutions_bin.h"
#include "learnset_evos_gen1_bin.h"
#include "learnset_evos_gen2_bin.h"
#include "learnset_evos_gen3_bin.h"
#include "dex_conversion_bin.h"

#define MF_7_1_INDEX 2
#define M_INDEX 1
#define F_INDEX 6
#define U_INDEX 7
#define EGG_NUMBER 412
#define EGG_NUMBER_GEN2 253
#define UNOWN_B_START 415
#define UNOWN_EX_LETTER 26
#define UNOWN_I_LETTER 8
#define UNOWN_V_LETTER 21
#define INITIAL_MAIL_GEN3 121
#define LAST_MAIL_GEN3 132

#define ACT_AS_GEN1_TRADE 1

u8 decrypt_data(struct gen3_mon*, u32*);
u8 is_egg_gen3(struct gen3_mon*, struct gen3_mon_misc*);

// Order is G A E M, or M E A G reversed. These are their indexes.
u8 positions[] = {0b11100100, 0b10110100, 0b11011000, 0b10011100, 0b01111000, 0b01101100,
                  0b11100001, 0b10110001, 0b11010010, 0b10010011, 0b01110010, 0b01100011,
                  0b11001001, 0b10001101, 0b11000110, 0b10000111, 0b01001110, 0b01001011,
                  0b00111001, 0b00101101, 0b00110110, 0b00100111, 0b00011110, 0b00011011};

u8 gender_thresholds_gen3[] = {127, 0, 31, 63, 191, 225, 254, 255, 0, 254};
u8 gender_thresholds_gen12[] = {8, 0, 2, 4, 12, 14, 16, 17, 0, 16};

const u16* pokemon_abilities_bin_16 = (const u16*)pokemon_abilities_bin;
const struct exp_level* exp_table = (const struct exp_level*)exp_table_bin;
const struct stats_gen_23* stats_table = (const struct stats_gen_23*)pokemon_stats_bin;
const struct stats_gen_1* stats_table_gen1 = (const struct stats_gen_1*)pokemon_stats_gen1_bin;

u8 get_index_key(u32 pid){
    // Make use of modulo properties to get this to positives
    while(pid >= 0x80000000)
        pid -= 0x7FFFFFF8;
    return SWI_DivMod(pid, 24);
}

u8 get_nature(u32 pid){
    return get_nature_fast(pid);
}

u8 get_unown_letter_gen3(u32 pid){
    return get_unown_letter_gen3_fast(pid);
}

u8 get_unown_letter_gen2(u16 ivs){
    return get_unown_letter_gen2_fast(ivs);
}

u16 get_mon_index(int index, u32 pid, u8 is_egg, u8 deoxys_form){
    if(index > LAST_VALID_GEN_3_MON)
        return 0;
    if(is_egg)
        return EGG_NUMBER;
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

u16 get_mon_index_gen2(int index, u8 is_egg){
    if(index > LAST_VALID_GEN_2_MON)
        return 0;
    if(is_egg)
        return EGG_NUMBER_GEN2;
    return index;
}

u16 get_mon_index_gen2_1(int index){
    if(index > LAST_VALID_GEN_1_MON)
        return 0;
    return index;
}

u16 get_mon_index_gen1(int index){
    if(index > LAST_VALID_GEN_1_MON)
        return 0;
    return gen3_to_1_conv_table_bin[index];
}

u16 get_mon_index_gen1_to_3(u8 index){
    return gen1_to_3_conv_table_bin[index];
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

const u8* get_pokemon_name_gen2(int index, u8 is_egg, u8 is_jp, u8* buffer){
    if(is_jp)
        return &(gen2_names_jap_bin[get_mon_index_gen2(index, is_egg)*STRING_GEN2_JP_CAP]);
    u8* buffer_two[STRING_GEN2_INT_SIZE];
    u16 mon_index = get_mon_index_gen2(index, is_egg);
    if (mon_index == MR_MIME_SPECIES)
        mon_index = MR_MIME_OLD_NAME_POS;
    if (mon_index == UNOWN_SPECIES)
        mon_index = UNOWN_REAL_NAME_POS;
    text_generic_to_gen3(get_table_pointer(pokemon_names_bin, mon_index), buffer_two, NAME_SIZE, STRING_GEN2_INT_CAP, 0, 0);
    text_gen3_to_gen12(buffer_two, buffer, STRING_GEN2_INT_CAP, STRING_GEN2_INT_CAP, 0, 0);
    return buffer;
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
    
    u8 index = data_src->growth.item;
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

void load_pokemon_sprite(int index, u32 pid, u8 is_egg, u8 deoxys_form, u8 has_item, u16 y, u16 x){
    set_pokemon_sprite(get_pokemon_sprite_pointer(index, pid, is_egg, deoxys_form), get_palette_references(index, pid, is_egg, deoxys_form), get_pokemon_sprite_info(index, pid, is_egg, deoxys_form), !is_egg && has_item, y, x);
}

void load_pokemon_sprite_raw(struct gen3_mon_data_unenc* data_src, u16 y, u16 x){
    if(!data_src->is_valid_gen3)
        return;

    return load_pokemon_sprite(data_src->growth.species, data_src->src->pid, data_src->is_egg, data_src->deoxys_form, has_item_raw(data_src), y, x);
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
    
    for(int i = 0; i < MOVES_SIZE; i++) {
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

u8 has_legal_moves_gen12(u8* moves, u8 is_gen2){
    u8 last_valid_move = LAST_VALID_GEN_1_MOVE;
    if(is_gen2)
        last_valid_move = LAST_VALID_GEN_2_MOVE;
    
    u8 previous_moves[MOVES_SIZE];
    u8 curr_slot = 0;
    for(int i = 0; i < MOVES_SIZE; i++) {
        if((moves[i] != 0) && (moves[i] <= last_valid_move)) {
            for(int j = 0; j < curr_slot; j++)
                if(moves[i] == previous_moves[j])
                    return 0;
            previous_moves[curr_slot++] = moves[i];
        }
    }
    
    if(curr_slot)
        return 1;
    return 0;
}

u8 get_pokemon_gender_gen2(u8 index, u8 atk_ivs, u8 is_egg, u8 curr_gen){
    u8 gender_kind = get_pokemon_gender_kind_gen2(index, is_egg, curr_gen);
    switch(gender_kind){
        case M_INDEX:
        case NIDORAN_M_GENDER_INDEX:
            return M_GENDER;
        case F_INDEX:
        case NIDORAN_F_GENDER_INDEX:
            return F_GENDER;
        case U_INDEX:
            return U_GENDER;
        default:
            if(atk_ivs >= gender_thresholds_gen12[gender_kind])
                return M_GENDER;
            return F_GENDER;
    }
}

u8 get_pokemon_gender_gen3(int index, u32 pid, u8 is_egg, u8 deoxys_form){
    u8 gender_kind = get_pokemon_gender_kind_gen3(index, pid, is_egg, deoxys_form);
    switch(gender_kind){
        case M_INDEX:
        case NIDORAN_M_GENDER_INDEX:
            return M_GENDER;
        case F_INDEX:
        case NIDORAN_F_GENDER_INDEX:
            return F_GENDER;
        case U_INDEX:
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

u8 get_pokemon_gender_kind_gen2(u8 index, u8 is_egg, u8 curr_gen){
    if(curr_gen == 1)
        return pokemon_gender_bin[get_mon_index_gen2_1(index)];
    return pokemon_gender_bin[get_mon_index_gen2(index, is_egg)];
}

u8 is_egg_gen3(struct gen3_mon* src, struct gen3_mon_misc* misc){
    // In case it ends up being more complex, for some reason
    return misc->is_egg;
}

u8 is_egg_gen3_raw(struct gen3_mon_data_unenc* data_src){
    if(!data_src->is_valid_gen3)
        return 1;
    
    return is_egg_gen3(data_src->src, &data_src->misc);
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
    return is_shiny_gen3(data_src->src->pid, data_src->src->ot_id, data_src->is_egg, trainer_id);
}

u8 is_shiny_gen2(u8 atk_ivs, u8 def_ivs, u8 spa_ivs, u8 spe_ivs){
    if((atk_ivs & 2) == 2 && def_ivs == 10 && spa_ivs == 10 && spe_ivs == 10)
        return 1;
    return 0;
}

u8 is_shiny_gen2_unfiltered(u16 ivs){
    return is_shiny_gen2((ivs >> 4) & 0xF, ivs & 0xF, (ivs >> 8) & 0xF, (ivs >> 12) & 0xF);
}

u8 is_shiny_gen2_raw(struct gen2_mon_data* src){
    return is_shiny_gen2_unfiltered(src->ivs);
}

u8 has_mail(struct gen3_mon* src, struct gen3_mon_growth* growth) {
    if(growth->item < INITIAL_MAIL_GEN3 || growth->item > LAST_MAIL_GEN3)
        return 0;
    if(src->mail_id == GEN3_NO_MAIL)
        return 0;
    return 1;
}

u8 get_mail_id(struct gen3_mon* src, struct gen3_mon_growth* growth) {
    if(!has_mail(src, growth))
        return GEN3_NO_MAIL;
    return src->mail_id;
}

u8 get_mail_id_raw(struct gen3_mon_data_unenc* data_src) {
    return get_mail_id(data_src->src, &data_src->growth);
}

u16 get_dex_index_raw(struct gen3_mon_data_unenc* data_src){
    u16* dex_conversion_bin_16 = (u16*)dex_conversion_bin;
    u16 mon_index = get_mon_index_raw(data_src);
    return dex_conversion_bin_16[mon_index];
}

u8 decrypt_data(struct gen3_mon* src, u32* decrypted_dst) {
    // Decrypt the data
    u32 key = src->pid ^ src->ot_id;
    for(int i = 0; i < (ENC_DATA_SIZE>>2); i++)
        decrypted_dst[i] = src->enc_data[i] ^ key;
    
    // Validate the data
    u16* decrypted_dst_16 = (u16*)decrypted_dst;
    u16 checksum = 0;
    for(int i = 0; i < (ENC_DATA_SIZE>>1); i++)
        checksum += decrypted_dst_16[i];
    if(checksum != src->checksum)
        return 0;
    
    return 1;
}

void encrypt_data(struct gen3_mon* dst) {
    // Prepare the checksum
    u16 checksum = 0;
    for(int i = 0; i < (ENC_DATA_SIZE>>1); i++)
        checksum += ((u16*)dst->enc_data)[i];
    dst->checksum = checksum;
    
    // Encrypt the data
    u32 key = dst->pid ^ dst->ot_id;
    for(int i = 0; i < (ENC_DATA_SIZE>>2); i++)
        dst->enc_data[i] = dst->enc_data[i] ^ key;
}

#define ATK_STAT_INDEX 1
#define DEF_STAT_INDEX 2
#define SPE_STAT_INDEX 3
#define EVS_TOTAL 5

u8 index_conversion_gen2[] = {0, 1, 2, 5, 3, 4};
u8 index_conversion_gen3[] = {0, 1, 2, 4, 5, 3};

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

u8 get_ivs_gen3(struct gen3_mon_misc* misc, u8 stat_index) {
    if(stat_index >= GEN2_STATS_TOTAL)
        stat_index = GEN2_STATS_TOTAL-1;
    
    u32 ivs = (*(((u32*)misc)+1));
    u8 real_stat_index = index_conversion_gen3[stat_index];
    
    return (ivs >> (5*real_stat_index))&0x1F;
}

u8 get_hidden_power_type_gen3(struct gen3_mon_misc* misc) {
    u32 type = 0;
    u32 ivs = (*(((u32*)misc)+1));
    for(int i = 0; i < GEN2_STATS_TOTAL; i++)
        type += (((ivs >> (5*i))&0x1F)&1)<<i;
    return Div(type*15, (1<<GEN2_STATS_TOTAL)-1);
}

u8 get_hidden_power_power_gen3(struct gen3_mon_misc* misc) {
    u32 power = 0;
    u32 ivs = (*(((u32*)misc)+1));
    for(int i = 0; i < GEN2_STATS_TOTAL; i++)
        power += ((((ivs >> (5*i))&0x1F)>>1)&1)<<i;
    return Div(power*40, (1<<GEN2_STATS_TOTAL)-1) + 30;
}

const u8* get_hidden_power_type_name_gen3(struct gen3_mon_misc* misc) {
    return get_table_pointer(type_names_bin, get_hidden_power_type_gen3(misc));
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

u32 get_level_exp_mon_index(u16 mon_index, u8 level) {
    return exp_table[level].exp_kind[pokemon_exp_groups_bin[mon_index]];
}

u32 get_proper_exp_gen2(u16 mon_index, u8 level, u8* given_exp) {
    
    s32 exp = (given_exp[0]<<0x10) + (given_exp[1]<<0x8) + (given_exp[2]<<0);
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

u32 get_proper_exp(struct gen3_mon* src, struct gen3_mon_growth* growth, u8 deoxys_form) {
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

u32 get_proper_exp_raw(struct gen3_mon_data_unenc* data_src) {
    return get_proper_exp(data_src->src, &data_src->growth, data_src->deoxys_form);
}

u8 make_evs_legal_gen3(struct gen3_mon_evs* evs) {
    u8 sum = 0;

    // Are they within the cap?
    for(int i = 0; i < GEN2_STATS_TOTAL; i++) {
        if(sum + evs->evs[i] > MAX_EVS)
            evs->evs[i] -= sum + evs->evs[i] - MAX_EVS;
        sum += evs->evs[i];
    }
}

u8 get_evs_gen3(struct gen3_mon_evs* evs, u8 stat_index) {
    if(stat_index >= GEN2_STATS_TOTAL)
        stat_index = GEN2_STATS_TOTAL-1;

    u8 own_evs[] = {0,0,0,0,0,0};
    u8 sum = 0;
    
    // Are they within the cap?
    for(int i = 0; i < GEN2_STATS_TOTAL; i++) {
        if(sum + evs->evs[i] > MAX_EVS)
            own_evs[i] = MAX_EVS-sum;
        else
            own_evs[i] = evs->evs[i];
        sum += own_evs[i];
    }
    
    u8 real_stat_index = index_conversion_gen3[stat_index];
    
    return own_evs[real_stat_index];
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
    return base + Div((((stats_table_gen1[species].stats[stat_index] + iv)*2) + (Sqrt(stat_exp) >> 2)) * level, 100);
}

u16 calc_stats_gen2(u16 species, u32 pid, u8 stat_index, u8 level, u8 iv, u16 stat_exp) {
    if(species > LAST_VALID_GEN_2_MON)
        species = 0;
    if(stat_index >= GEN2_STATS_TOTAL)
        stat_index = GEN2_STATS_TOTAL-1;
    
    u16 mon_index = get_mon_index(species, pid, 0, 0);
    
    stat_index = index_conversion_gen2[stat_index];
    u16 base = 5;
    if(stat_index == HP_STAT_INDEX)
        base = level + 10;
    return base + Div((((stats_table[mon_index].stats[stat_index] + iv)*2) + (Sqrt(stat_exp) >> 2)) * level, 100);
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
    u16 stat = base + Div(((2*stats_table[mon_index].stats[stat_index]) + iv + (ev >> 2)) * level, 100);
    if((boosted_stat == nerfed_stat) || ((boosted_stat != stat_index) && (nerfed_stat != stat_index)))
        return stat;
    if(boosted_stat == stat_index)
        return stat + Div(stat, 10);
    if(nerfed_stat == stat_index)
        return stat - Div(stat, 10);
}

u16 calc_stats_gen3_raw(struct gen3_mon_data_unenc* data_src, u8 stat_index) {    
    return calc_stats_gen3(data_src->growth.species, data_src->src->pid, stat_index, to_valid_level_gen3(data_src->src), get_ivs_gen3(&data_src->misc, stat_index), get_evs_gen3(&data_src->evs, stat_index), data_src->deoxys_form);
}

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
    if(has_mail(src, growth))
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
    
    for(int i = 0; i < MOVES_SIZE; i++)
    {
        moves[i] = 0;
        pps[i] = 0;
    }
    for(int i = 0; i < MOVES_SIZE; i++) {
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

    for(int i = 0; i < MOVES_SIZE; i++) {
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

void convert_exp_nature_of_gen3(struct gen3_mon* src, struct gen3_mon_growth* growth, u8* level_ptr, u8* exp_ptr, u8 is_gen2) {
    if(((is_gen2) && (growth->species > LAST_VALID_GEN_2_MON)) || ((!is_gen2) && (growth->species > LAST_VALID_GEN_1_MON)))
        return;
    
    // Level handling
    u8 level = to_valid_level_gen3(src);
    
    u16 mon_index = get_mon_index(growth->species, src->pid, 0, 0);
    
    // Experience handling
    s32 exp = get_proper_exp(src, growth, 0);
    
    s32 max_exp = exp;
    if(level < MAX_LEVEL)
        max_exp = get_level_exp_mon_index(mon_index, level+1)-1;
    
    // Save nature in experience, like the Gen I-VII conversion
    u8 nature = get_nature(src->pid);

    // Nature handling
    u8 exp_nature = DivMod(exp, NUM_NATURES);
    if(exp_nature > nature)
        nature += NUM_NATURES;
    exp += nature - exp_nature;
    if (level < MAX_LEVEL)
        while (exp > max_exp) {
            level++;
            if(level == MAX_LEVEL) {
                exp = max_exp+1;
                break;
            }
            max_exp = get_level_exp_mon_index(mon_index, level+1)-1;
        }
    if (level == MAX_LEVEL && exp != get_level_exp_mon_index(mon_index, MAX_LEVEL)){
        level--;
        exp -= NUM_NATURES;
    }

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

u16 convert_ivs_of_gen3(struct gen3_mon_misc* misc, u16 species, u32 pid, u8 is_shiny, u8 gender, u8 gender_kind, u8 is_gen2) {
    if(((is_gen2) && (species > LAST_VALID_GEN_2_MON)) || ((!is_gen2) && (species > LAST_VALID_GEN_1_MON)))
        return 0;
        
    // Assign IVs
    // Keep in mind: Unown letter, gender and shinyness
    // Hidden Power calculations are too restrictive
    u8 atk_ivs = misc->atk_ivs >> 1;
    u8 def_ivs = misc->def_ivs >> 1;
    u8 spa_ivs = (misc->spa_ivs + misc->spd_ivs) >> 2;
    u8 spe_ivs = misc->spe_ivs >> 1;
    
    // Handle roamers losing IVs when caught
    u8 origin_game = (misc->origins_info>>7)&0xF;
    if(((species >= RAIKOU_SPECIES) && (species <= SUICUNE_SPECIES)) && ((origin_game == FR_VERSION_ID) || (origin_game == LG_VERSION_ID))) {
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
    if(gender != U_GENDER && gender_kind != M_INDEX && gender_kind != F_INDEX) {
        if(gender == F_GENDER && atk_ivs >= gender_thresholds_gen12[gender_kind])
            atk_ivs = gender_thresholds_gen12[gender_kind] - 1;
        else if(gender == M_GENDER && atk_ivs < gender_thresholds_gen12[gender_kind])
            atk_ivs = gender_thresholds_gen12[gender_kind];
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

    u8 encounter_type = (encounter_types_bin[species>>2]>>(2*(species&3)))&3;
    u8 is_shiny = is_shiny_gen2_unfiltered(wanted_ivs);
    u32 ivs = 0;
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
            if(!is_shiny)
                generate_static_info(wanted_nature, wanted_ivs, tsv, &dst->pid, &ivs);
            else
                generate_static_shiny_info(wanted_nature, tsv, &dst->pid, &ivs);
            // Roamers only get the first byte of their IVs
            ivs &= 0xFF;
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
        u8* possible_met_data = search_table_for_index(searchable_table, mon_index);
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
    if(dst->pid & 1) {
        u16 abilities = get_possible_abilities_pokemon(species, dst->pid, 0, 0);
        u8 abilities_same = (abilities&0xFF) == ((abilities>>8)&0xFF);
        if(!abilities_same)
            misc->ability = 1;
    }
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
    s32 question_marks_count = text_gen2_count_question(nickname, STRING_GEN2_INT_CAP) - text_gen3_count_question(src->nickname, NICKNAME_GEN3_SIZE);
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
        s32 question_marks_count = text_gen3_count_question(dst->nickname, NICKNAME_GEN3_SIZE) - text_gen2_count_question(nickname, name_cap);    
        if((question_marks_count >= (text_gen3_size(dst->nickname, NICKNAME_GEN3_SIZE) >> 1)) || (text_gen3_size(dst->nickname, NICKNAME_GEN3_SIZE) == 0))
            text_gen12_to_gen3(get_pokemon_name_gen2(species, 0, is_jp, gen2_buffer), dst->nickname, name_cap, NICKNAME_GEN3_SIZE, is_jp, is_jp);
    }
}

void recalc_stats_gen3(struct gen3_mon_data_unenc* data_dst, struct gen3_mon* dst) {
    // Calculate stats
    dst->curr_hp = calc_stats_gen3_raw(data_dst, HP_STAT_INDEX);
    for(int i = 0; i < GEN2_STATS_TOTAL; i++)
        dst->stats[index_conversion_gen3[i]] = calc_stats_gen3_raw(data_dst, i);
    
    // Reset the status
    dst->status = 0;
}

void place_and_encrypt_gen3_data(struct gen3_mon_data_unenc* src, struct gen3_mon* dst) {
    u8 index = get_index_key(dst->pid);
    
    u8 pos_data = 12*((positions[index] >> 0)&3);
    for(int i = 0; i < sizeof(struct gen3_mon_growth); i++)
        ((u8*)dst->enc_data)[pos_data+i] = ((u8*)(&src->growth))[i];
    pos_data = 12*((positions[index] >> 2)&3);
    for(int i = 0; i < sizeof(struct gen3_mon_attacks); i++)
        ((u8*)dst->enc_data)[pos_data+i] = ((u8*)(&src->attacks))[i];
    pos_data = 12*((positions[index] >> 4)&3);
    for(int i = 0; i < sizeof(struct gen3_mon_evs); i++)
        ((u8*)dst->enc_data)[pos_data+i] = ((u8*)(&src->evs))[i];
    pos_data = 12*((positions[index] >> 6)&3);
    for(int i = 0; i < sizeof(struct gen3_mon_misc); i++)
        ((u8*)dst->enc_data)[pos_data+i] = ((u8*)(&src->misc))[i];
    
    encrypt_data(dst);
}

void process_gen3_data(struct gen3_mon* src, struct gen3_mon_data_unenc* dst, u8 main_version, u8 sub_version) {
    dst->src = src;
    
    u32 decryption[ENC_DATA_SIZE>>2];
    
    // Initial data decryption
    if(!decrypt_data(src, decryption)) {
        dst->is_valid_gen3 = 0;
        dst->is_valid_gen2 = 0;
        dst->is_valid_gen1 = 0;
        return;
    }
    
    u8 index = get_index_key(src->pid);
    
    struct gen3_mon_growth* growth = (struct gen3_mon_growth*)&(decryption[3*((positions[index] >> 0)&3)]);
    struct gen3_mon_attacks* attacks = (struct gen3_mon_attacks*)&(decryption[3*((positions[index] >> 2)&3)]);
    struct gen3_mon_evs* evs = (struct gen3_mon_evs*)&(decryption[3*((positions[index] >> 4)&3)]);
    struct gen3_mon_misc* misc = (struct gen3_mon_misc*)&(decryption[3*((positions[index] >> 6)&3)]);
    
    for(int i = 0; i < sizeof(struct gen3_mon_growth); i++)
        ((u8*)(&dst->growth))[i] = ((u8*)growth)[i];
    for(int i = 0; i < sizeof(struct gen3_mon_attacks); i++)
        ((u8*)(&dst->attacks))[i] = ((u8*)attacks)[i];
    for(int i = 0; i < sizeof(struct gen3_mon_evs); i++)
        ((u8*)(&dst->evs))[i] = ((u8*)evs)[i];
    for(int i = 0; i < sizeof(struct gen3_mon_misc); i++)
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
    
    if((growth->item >= LAST_VALID_GEN_3_ITEM) || (growth->item == ENIGMA_BERRY_ID))
        growth->item = NO_ITEM_ID;
    
    src->level = to_valid_level_gen3(src);
    
    growth->exp = get_proper_exp_raw(dst);
    
    // Set the new "cleaned" data
    place_and_encrypt_gen3_data(dst, src);
    
    // We reuse this SOOOO much...
    dst->is_egg = is_egg_gen3(src, misc);
    
    // Recalc the stats, always
    recalc_stats_gen3(dst, src);
    
    dst->is_valid_gen3 = 1;
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
    for(int i = 0; i < sizeof(struct gen2_mon); i++)
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
    
    // Zero all EVs, since it's an entirely different system
    for(int i = 0; i < EVS_TOTAL; i++)
        dst->evs[i] = 0;
    
    // Assign IVs
    dst->ivs = convert_ivs_of_gen3(misc, growth->species, src->pid, is_shiny, gender, gender_kind, 1);
    
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
    struct special_met_data_gen2* met_data = NULL;
    if(encounter_types_gen2_bin[dst->species>>3]&(1<<(dst->species&7))) {
        met_data = (struct special_met_data_gen2*)search_table_for_index(special_encounters_gen2_bin, dst->species);
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
        dst->stats[i] = swap_endian_short(calc_stats_gen2(growth->species, src->pid, i, dst->level, get_ivs_gen2(dst->ivs, i), swap_endian_short(dst->evs[i >= EVS_TOTAL ? EVS_TOTAL-1 : i])));
    
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
    for(int i = 0; i < sizeof(struct gen1_mon); i++)
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
    
    // Zero all EVs, since it's an entirely different system
    for(int i = 0; i < EVS_TOTAL; i++)
        dst->evs[i] = 0;
    
    // Assign IVs
    dst->ivs = convert_ivs_of_gen3(misc, growth->species, src->pid, is_shiny, gender, gender_kind, 0);
    
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
    for(int i = 0; i < sizeof(struct gen3_mon); i++)
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
    for(int i = 0; i < sizeof(struct gen3_mon_growth); i++)
        ((u8*)(&data_dst->growth))[i] = 0;
    for(int i = 0; i < sizeof(struct gen3_mon_attacks); i++)
        ((u8*)(&data_dst->attacks))[i] = 0;
    for(int i = 0; i < sizeof(struct gen3_mon_evs); i++)
        ((u8*)(&data_dst->evs))[i] = 0;
    for(int i = 0; i < sizeof(struct gen3_mon_misc); i++)
        ((u8*)(&data_dst->misc))[i] = 0;
    
    // Set species, exp, level and item
    data_dst->growth.species = src->species;
    u8 wanted_nature = get_exp_nature(dst, &data_dst->growth, src->level, src->exp);
    data_dst->growth.item = convert_item_to_gen3(src->item);
    
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
    for(int i = 0; i < sizeof(struct gen3_mon); i++)
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
    for(int i = 0; i < sizeof(struct gen3_mon_growth); i++)
        ((u8*)(&data_dst->growth))[i] = 0;
    for(int i = 0; i < sizeof(struct gen3_mon_attacks); i++)
        ((u8*)(&data_dst->attacks))[i] = 0;
    for(int i = 0; i < sizeof(struct gen3_mon_evs); i++)
        ((u8*)(&data_dst->evs))[i] = 0;
    for(int i = 0; i < sizeof(struct gen3_mon_misc); i++)
        ((u8*)(&data_dst->misc))[i] = 0;
    
    // Set species, exp, level and item
    data_dst->growth.species = get_mon_index_gen1_to_3(src->species);
    u8 wanted_nature = get_exp_nature(dst, &data_dst->growth, src->level, src->exp);
    data_dst->growth.item = convert_item_to_gen3(src->item);
    
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

void clean_mail_gen3(struct mail_gen3* mail, struct gen3_mon* mon){
    for(int i = 0; i < MAIL_WORDS_SIZE; i++)
        mail->words[i] = 0;
    for(int i = 0; i < OT_NAME_GEN3_SIZE+1; i++)
        mail->ot_name[i] = GEN3_EOL;
    mail->ot_id = 0;
    mail->species = BULBASAUR_SPECIES;
    mail->item = 0;

    mon->mail_id = GEN3_NO_MAIL;
}

u8 trade_evolve(struct gen3_mon* mon, struct gen3_mon_data_unenc* mon_data, u16** learnset_ptr, u8 curr_gen) {
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
