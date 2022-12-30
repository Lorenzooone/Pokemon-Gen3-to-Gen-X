#include <gba.h>
#include "party_handler.h"
#include "text_handler.h"
#include "sprite_handler.h"
#include "graphics_handler.h"

#include "pokemon_moves_pp_gen1_bin.h"
#include "pokemon_moves_pp_bin.h"
#include "pokemon_gender_bin.h"
#include "pokemon_names_bin.h"
#include "item_names_bin.h"
#include "location_names_bin.h"
#include "pokeball_names_bin.h"
#include "gen2_names_jap_bin.h"
#include "sprites_cmp_bin.h"
#include "sprites_info_bin.h"
#include "palettes_references_bin.h"
#include "item_gen3_to_12_bin.h"
#include "gen3_to_1_conv_table_bin.h"
#include "gen1_to_3_conv_table_bin.h"
#include "pokemon_exp_groups_bin.h"
#include "exp_table_bin.h"
#include "pokemon_stats_bin.h"
#include "pokemon_stats_gen1_bin.h"
#include "pokemon_types_gen1_bin.h"
#include "pokemon_natures_bin.h"

u8 decrypt_data(struct gen3_mon*, u32*);
u8 is_egg_gen3(struct gen3_mon*, struct gen3_mon_misc*);
u8 get_unown_letter_gen3(u32);
u8 get_pokemon_gender_kind_gen3(int, u32, u8);

// Order is G A E M, or M E A G reversed. These are their indexes.
u8 positions[] = {0b11100100, 0b10110100, 0b11011000, 0b10011100, 0b01111000, 0b01101100,
                  0b11100001, 0b10110001, 0b11010010, 0b10010011, 0b01110010, 0b01100011,
                  0b11001001, 0b10001101, 0b11000110, 0b10000111, 0b01001110, 0b01001011,
                  0b00111001, 0b00101101, 0b00110110, 0b00100111, 0b00011110, 0b00011011};

u8 gender_thresholds_gen3[] = {127, 0, 31, 63, 191, 225, 254, 255};
u8 gender_thresholds_gen12[] = {8, 0, 2, 4, 12, 14, 16, 17};

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

const struct exp_level* exp_table = (const struct exp_level*)exp_table_bin;
const struct stats_gen_23* stats_table = (const struct stats_gen_23*)pokemon_stats_bin;
const struct stats_gen_1* stats_table_gen1 = (const struct stats_gen_1*)pokemon_stats_gen1_bin;

u8 get_index_key(u32 pid){
    // Make use of modulo properties to get this to positives
    while(pid >= 0x80000000)
        pid -= 0x7FFFFFF8;
    return DivMod(pid, 24);
}

u8 get_nature(u32 pid){
    // Make use of modulo properties to get this to positives
    while(pid >= 0x80000000)
        pid -= 0x7FFFFFE9;
    return DivMod(pid, 25);
}

u16 get_mon_index(int index, u32 pid, u8 is_egg){
    if(index > LAST_VALID_GEN_3_MON)
        return 0;
    if(is_egg)
        return EGG_NUMBER;
    if(index != UNOWN_SPECIES)
        return index;
    u8 letter = get_unown_letter_gen3(pid);
    if(letter == 0)
        return UNOWN_SPECIES;
    return UNOWN_B_START+letter-1;
}

u16 get_mon_index_raw(struct gen3_mon_data_undec* data_src){
    if(!data_src->is_valid_gen3)
        return get_mon_index(0,0,0);
    
    return get_mon_index(data_src->growth.species, data_src->src->pid, is_egg_gen3_raw(data_src));
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

const u8* get_pokemon_name(int index, u32 pid, u8 is_egg){
    return get_table_pointer(pokemon_names_bin, get_mon_index(index, pid, is_egg));
}

const u8* get_pokemon_name_pure(int index, u32 pid, u8 is_egg){
    u16 mon_index = get_mon_index(index, pid, is_egg);
    if ((index == UNOWN_SPECIES) && !is_egg)
        mon_index = UNOWN_REAL_NAME_POS;
    if ((index == DEOXYS_SPECIES) && !is_egg)
        mon_index = DEOXYS_SPECIES;
    return get_table_pointer(pokemon_names_bin, mon_index);
}

const u8* get_pokemon_name_raw(struct gen3_mon_data_undec* data_src){
    if(!data_src->is_valid_gen3)
        return get_pokemon_name(0,0,0);
    
    return get_pokemon_name(data_src->growth.species, data_src->src->pid, is_egg_gen3_raw(data_src));
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

const u8* get_item_name_raw(struct gen3_mon_data_undec* data_src){
    if(!data_src->is_valid_gen3)
        return get_item_name(0,0);
    
    return get_item_name(data_src->growth.item, is_egg_gen3_raw(data_src));
}

const u8* get_met_location_name_gen3_raw(struct gen3_mon_data_undec* data_src){
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

u8 get_met_level_gen3_raw(struct gen3_mon_data_undec* data_src){
    if(!data_src->is_valid_gen3)
        return 0;
    u8 level = data_src->misc.origins_info&0x7F;
    if(level > 100)
        level = 100;
    return level;
}

const u8* get_pokeball_base_name_gen3_raw(struct gen3_mon_data_undec* data_src){
    if(!data_src->is_valid_gen3)
        return get_table_pointer(pokeball_names_bin, 0);
    
    return get_table_pointer(pokeball_names_bin, (data_src->misc.origins_info>>11)&0xF);
}

u8 get_trainer_gender_char_raw(struct gen3_mon_data_undec* data_src){
    if(!data_src->is_valid_gen3)
        return GENERIC_U_GENDER;
    
    if(data_src->misc.origins_info>>15)
        return GENERIC_F_GENDER;
    return GENERIC_M_GENDER;
}

const u8* get_pokemon_sprite_pointer(int index, u32 pid, u8 is_egg){
    return get_table_pointer(sprites_cmp_bin, get_mon_index(index, pid, is_egg));
}

u8 get_pokemon_sprite_info(int index, u32 pid, u8 is_egg){
    u16 mon_index = get_mon_index(index, pid, is_egg);
    return (sprites_info_bin[mon_index>>2]>>(2*(mon_index&3)))&3;
}

u8 get_palette_references(int index, u32 pid, u8 is_egg){
    return palettes_references_bin[get_mon_index(index, pid, is_egg)];
}

void load_pokemon_sprite(int index, u32 pid, u8 is_egg, u8 has_item, u16 y, u16 x){
    u8 sprite_counter = get_sprite_counter();
    u32 address = get_pokemon_sprite_pointer(index, pid, is_egg);
    u8 palette = get_palette_references(index, pid, is_egg);
    load_pokemon_sprite_gfx(address, get_vram_pos(), get_pokemon_sprite_info(index, pid, is_egg));
    set_attributes(y, x |(1<<15), (32*sprite_counter)|(palette<<12));
    inc_sprite_counter();
    if(!is_egg && has_item)
        set_item_icon(y, x);
}

void load_pokemon_sprite_raw(struct gen3_mon_data_undec* data_src, u16 y, u16 x){
    if(!data_src->is_valid_gen3)
        return;

    return load_pokemon_sprite(data_src->growth.species, data_src->src->pid, is_egg_gen3_raw(data_src), (data_src->growth.item > 0) && (data_src->growth.item <= LAST_VALID_GEN_3_ITEM), y, x);
}

u8 get_pokemon_gender_gen3(int index, u32 pid, u8 is_egg){
    u8 gender_kind = get_pokemon_gender_kind_gen3(index, pid, is_egg);
    switch(gender_kind){
        case M_INDEX:
            return M_GENDER;
        case F_INDEX:
            return F_GENDER;
        case U_INDEX:
            return U_GENDER;
        default:
            if((pid & 0xFF) >= gender_thresholds_gen3[gender_kind])
                return M_GENDER;
            return F_GENDER;
    }
}

u8 get_pokemon_gender_raw(struct gen3_mon_data_undec* data_src){
    if(!data_src->is_valid_gen3)
        return GENERIC_U_GENDER;
    
    return get_pokemon_gender_gen3(data_src->growth.species, data_src->src->pid, is_egg_gen3_raw(data_src));
}

char get_pokemon_gender_char_raw(struct gen3_mon_data_undec* data_src){
    if(!data_src->is_valid_gen3)
        return get_pokemon_gender_gen3(0,0,0);

    u16 mon_index = get_mon_index_raw(data_src);

    u8 gender = get_pokemon_gender_raw(data_src);
    char gender_char = GENERIC_M_GENDER;
    if(gender == F_GENDER)
        gender_char = GENERIC_F_GENDER;
    if((gender == U_GENDER)||(mon_index == NIDORAN_M_SPECIES)||(mon_index == NIDORAN_F_SPECIES))
        gender_char = GENERIC_U_GENDER;

    return gender_char;
}

u8 get_pokemon_gender_kind_gen3(int index, u32 pid, u8 is_egg){
    return pokemon_gender_bin[get_mon_index(index, pid, is_egg)];
}

u8 is_egg_gen3(struct gen3_mon* src, struct gen3_mon_misc* misc){
    // In case it ends up being more complex, for some reason
    return misc->is_egg;
}

u8 is_egg_gen3_raw(struct gen3_mon_data_undec* data_src){
    if(!data_src->is_valid_gen3)
        return 1;
    
    return is_egg_gen3(data_src->src, &data_src->misc);
}

u8 has_pokerus_gen3_raw(struct gen3_mon_data_undec* data_src){
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

u8 is_shiny_gen3_raw(struct gen3_mon_data_undec* data_src, u32 trainer_id){
    return is_shiny_gen3(data_src->src->pid, data_src->src->ot_id, is_egg_gen3_raw(data_src), trainer_id);
}

u8 is_shiny_gen2(u8 atk_ivs, u8 def_ivs, u8 spa_ivs, u8 spe_ivs){
    if((atk_ivs & 2) == 2 && def_ivs == 10 && spa_ivs == 10 && spe_ivs == 10)
        return 1;
    return 0;
}

u8 is_shiny_gen2_raw(struct gen2_mon* src){
    return is_shiny_gen2((src->ivs >> 4) & 0xF, src->ivs & 0xF, (src->ivs >> 8) & 0xF, (src->ivs >> 12) & 0xF);
}

u8 get_unown_letter_gen3(u32 pid){
    return DivMod((pid & 3) + (((pid >> 8) & 3) << 2) + (((pid >> 16) & 3) << 4) + (((pid >> 24) & 3) << 6), 28);
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

#define HP_STAT_INDEX 0
#define ATK_STAT_INDEX 1
#define DEF_STAT_INDEX 2
#define SPE_STAT_INDEX 3
#define GEN2_STATS_TOTAL 6
#define GEN1_STATS_TOTAL 5
#define EVS_TOTAL 5

u8 index_conversion_gen2[] = {0, 1, 2, 5, 3, 4};

u8 get_iv_gen2(u16 ivs, u8 stat_index) {
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

u16 calc_stats_gen1(u16 species, u8 stat_index, u8 level, u8 iv, u16 stat_exp) {
    if(species > LAST_VALID_GEN_1_MON)
        species = 0;
    species =  get_mon_index_gen1(species);
    if(stat_index >= GEN1_STATS_TOTAL)
        stat_index = GEN1_STATS_TOTAL-1;
    u16 base = 5;
    if(stat_index == HP_STAT_INDEX)
        base = level + 10;
    return base + Div(((stats_table_gen1[species].stats[stat_index] + iv) + (Sqrt(stat_exp) >> 2)) * level, 100);
}

u16 calc_stats_gen2(u16 species, u8 stat_index, u8 level, u8 iv, u16 stat_exp) {
    if(species > LAST_VALID_GEN_2_MON)
        species = 0;
    if(stat_index >= GEN2_STATS_TOTAL)
        stat_index = GEN2_STATS_TOTAL-1;
    stat_index = index_conversion_gen2[stat_index];
    u16 base = 5;
    if(stat_index == HP_STAT_INDEX)
        base = level + 10;
    return base + Div(((stats_table[species].stats[stat_index] + iv) + (Sqrt(stat_exp) >> 2)) * level, 100);
}

u16 calc_stats_gen3(u16 species, u8 stat_index, u8 level, u8 iv, u8 ev, u8 nature) {
    if(species > LAST_VALID_GEN_3_MON)
        species = 0;
    if(stat_index >= GEN2_STATS_TOTAL)
        stat_index = GEN2_STATS_TOTAL-1;
    u16 base = 5;
    if(stat_index == HP_STAT_INDEX)
        base = level + 10;
    u8 boosted_stat = pokemon_natures_bin[(2*nature)];
    u8 nerfed_stat = pokemon_natures_bin[(2*nature)+1];
    u16 stat = base + Div(((stats_table[species].stats[stat_index] + iv) + (ev >> 2)) * level, 100);
    if((boosted_stat == nerfed_stat) || ((boosted_stat != stat_index) && (nerfed_stat != stat_index)))
        return stat;
    if(boosted_stat == stat_index)
        return stat + Div(stat, 10);
    if(nerfed_stat == stat_index)
        return stat - Div(stat, 10);
}

u16 swap_endian_short(u16 shrt) {
    return ((shrt & 0xFF00) >> 8) | ((shrt & 0xFF) << 8);
}

u32 swap_endian_int(u32 integer) {
    return ((integer & 0xFF000000) >> 24) | ((integer & 0xFF) << 24) | ((integer & 0xFF0000) >> 8) | ((integer & 0xFF00) << 8);
}

u8 validate_converting_mon_of_gen3(struct gen3_mon* src, struct gen3_mon_growth* growth, u8 is_shiny, u8 gender, u8 gender_kind, u8 is_egg, u8 is_gen2) {
    u8 last_valid_mon = LAST_VALID_GEN_1_MON;
    if(is_gen2)
        last_valid_mon = LAST_VALID_GEN_2_MON;

    // Bad egg checks
    if(src->is_bad_egg)
        return 0;
    
    // Item checks
    if(growth->item >= INITIAL_MAIL_GEN3 && growth->item <= LAST_MAIL_GEN3)
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

u8 convert_item_of_gen3(u16 item) {
    if(item > LAST_VALID_GEN_3_ITEM)
        item = 0;
    return item_gen3_to_12_bin[item];
}

u8 to_valid_level_gen3(struct gen3_mon* src) {
    u8 level = src->level;
    if(level < MIN_LEVEL)
        return MIN_LEVEL;
    if(level > MAX_LEVEL)
        return MAX_LEVEL;
    return level;
}

void convert_exp_nature_of_gen3(struct gen3_mon* src, struct gen3_mon_growth* growth, u8* level_ptr, u8* exp_ptr, u8 is_gen2) {
    if(((is_gen2) && (growth->species > LAST_VALID_GEN_2_MON)) || (growth->species > LAST_VALID_GEN_1_MON))
        return;
    
    // Level handling
    u8 level = to_valid_level_gen3(src);
    
    // Experience handling
    s32 exp = growth->exp;
    s32 min_exp = exp_table[level].exp_kind[pokemon_exp_groups_bin[growth->species]];
    s32 max_exp = min_exp;
    if(level == MAX_LEVEL)
        exp = exp_table[MAX_LEVEL].exp_kind[pokemon_exp_groups_bin[growth->species]];
    else
        max_exp = exp_table[level+1].exp_kind[pokemon_exp_groups_bin[growth->species]]-1;
    if(exp < min_exp)
        exp = min_exp;
    if(exp > max_exp)
        exp = max_exp;
    if(exp < 0)
        exp = 0;
    
    // Save nature in experience, like the Gen I-VII conversion
    u8 nature = get_nature(src->pid);

    // Nature handling
    u8 exp_nature = DivMod(exp, 25);
    if(exp_nature > nature)
        nature += 25;
    exp += nature - exp_nature;
    if (level < MAX_LEVEL)
        while (exp > max_exp) {
            level++;
            if(level == MAX_LEVEL)
                break;
            max_exp = exp_table[level+1].exp_kind[pokemon_exp_groups_bin[growth->species]]-1;
        }
    if (level == MAX_LEVEL && exp != exp_table[MAX_LEVEL].exp_kind[pokemon_exp_groups_bin[growth->species]]){
        level--;
        exp -= 25;
    }

    // Store exp and level
    *level_ptr = level;
    for(int i = 0; i < 3; i++)
        exp_ptr[2-i] = (exp >> (8*i))&0xFF;
}

u16 convert_ivs_of_gen3(struct gen3_mon_misc* misc, u16 species, u32 pid, u8 is_shiny, u8 gender, u8 gender_kind, u8 is_gen2) {
    if(((is_gen2) && (species > LAST_VALID_GEN_2_MON)) || (species > LAST_VALID_GEN_1_MON))
        return 0;
        
    // Assign IVs
    // Keep in mind: Unown letter, gender and shinyness
    // Hidden Power calculations are too restrictive
    u8 atk_ivs = (misc->atk_ivs >> 1);
    u8 def_ivs = (misc->def_ivs >> 1);
    u8 spa_ivs = ((misc->spa_ivs + misc->spd_ivs) >> 2);
    u8 spe_ivs = (misc->spe_ivs >> 1);
    
    // Unown letter
    if((is_gen2) && (species == UNOWN_SPECIES)) {
        u8 letter = get_unown_letter_gen3(pid);
        u8 min_iv_sum = letter * 10;
        u8 max_iv_sum = ((letter+1) * 10)-1;
        if(letter == 25)
            max_iv_sum = 255;
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

void fix_name_change_of_gen3(struct gen3_mon* src, u16 species,  u8* nickname, u8 is_egg, u8 is_gen2) {
    u8 tmp_text_buffer[NAME_SIZE];
    
    // Get the string to compare to
    text_generic_to_gen3(get_pokemon_name(species, src->pid, is_egg), tmp_text_buffer, NAME_SIZE, NICKNAME_GEN3_SIZE, 0, 0);
    
    // If it's the same, update the nickname with the new one
    if(text_gen3_is_same(src->nickname, tmp_text_buffer, NICKNAME_GEN3_SIZE, NICKNAME_GEN3_SIZE)) {
        text_gen2_copy(get_pokemon_name_gen2(species, is_egg, 0, tmp_text_buffer), nickname, STRING_GEN2_INT_CAP, STRING_GEN2_INT_CAP);
        // Gen 1 used the wrong dot symbol
        if(!is_gen2)
            text_gen2_replace(nickname, STRING_GEN2_INT_CAP, GEN2_DOT, GEN1_DOT);
    }
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
        fix_name_change_of_gen3(src, species,  nickname, is_egg, is_gen2);
    
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

void process_gen3_data(struct gen3_mon* src, struct gen3_mon_data_undec* dst) {
    dst->src = src;
    
    u32 decryption[ENC_DATA_SIZE>>2];
    
    // Initial data decryption
    if(!decrypt_data(src, decryption)) {
        dst->is_valid_gen3 = 0;
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
        return;
    }
    
    // Obedience checks
    if(((growth->species == MEW_SPECIES) || (growth->species == DEOXYS_SPECIES)) && (!misc->obedience)) {
        dst->is_valid_gen3 = 0;
        return;
    }

    // Bad egg checks
    if(src->is_bad_egg) {
        dst->is_valid_gen3 = 0;
        return;
    }
    
    dst->is_valid_gen3 = 1;
}

u8 gen3_to_gen2(struct gen2_mon* dst, struct gen3_mon_data_undec* data_src, u32 trainer_id) {
    
    struct gen3_mon* src = data_src->src;
    
    if(!data_src->is_valid_gen3)
        return 0;
    
    struct gen3_mon_growth* growth = &data_src->growth;
    struct gen3_mon_attacks* attacks = &data_src->attacks;
    struct gen3_mon_evs* evs = &data_src->evs;
    struct gen3_mon_misc* misc = &data_src->misc;
    
    // Get shinyness and gender for checks
    u8 is_shiny = is_shiny_gen3_raw(data_src, trainer_id);
    u8 gender = get_pokemon_gender_gen3(growth->species, src->pid, 0);
    u8 gender_kind = get_pokemon_gender_kind_gen3(growth->species, src->pid, 0);
    
    // Check that the mon can be traded
    if(!validate_converting_mon_of_gen3(src, growth, is_shiny, gender, gender_kind, is_egg_gen3(src, misc), 1))
        return 0;
    
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
    dst->data = 0;
    dst->status = 0;
    dst->unused = 0;
    
    // Stats calculations
    dst->curr_hp = swap_endian_short(calc_stats_gen2(growth->species, HP_STAT_INDEX, dst->level, get_iv_gen2(dst->ivs, HP_STAT_INDEX), swap_endian_short(dst->evs[HP_STAT_INDEX])));
    for(int i = 0; i < GEN2_STATS_TOTAL; i++)
        dst->stats[i] = swap_endian_short(calc_stats_gen2(growth->species, i, dst->level, get_iv_gen2(dst->ivs, i), swap_endian_short(dst->evs[i >= EVS_TOTAL ? EVS_TOTAL-1 : i])));
    
    // Extra byte for egg data
    dst->is_egg = is_egg_gen3(src, misc);
    
    // Store egg cycles
    if(dst->is_egg)
        dst->friendship = growth->friendship;

    // Text conversions
    convert_strings_of_gen3(src, growth->species, dst->ot_name, dst->ot_name_jp, dst->nickname, dst->nickname_jp, dst->is_egg, 1);

    return 1;
}

u8 gen3_to_gen1(struct gen1_mon* dst, struct gen3_mon_data_undec* data_src, u32 trainer_id) {
    
    struct gen3_mon* src = data_src->src;
    
    if(!data_src->is_valid_gen3)
        return 0;
    
    struct gen3_mon_growth* growth = &data_src->growth;
    struct gen3_mon_attacks* attacks = &data_src->attacks;
    struct gen3_mon_evs* evs = &data_src->evs;
    struct gen3_mon_misc* misc = &data_src->misc;
    
    // Get shinyness and gender for checks
    u8 is_shiny = is_shiny_gen3_raw(data_src, trainer_id);
    u8 gender = get_pokemon_gender_gen3(growth->species, src->pid, 0);
    u8 gender_kind = get_pokemon_gender_kind_gen3(growth->species, src->pid, 0);
    
    // Check that the mon can be traded
    if(!validate_converting_mon_of_gen3(src, growth, is_shiny, gender, gender_kind, is_egg_gen3(src, misc), 0))
        return 0;
    
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
    dst->curr_hp = swap_endian_short(calc_stats_gen1(growth->species, HP_STAT_INDEX, dst->level, get_iv_gen2(dst->ivs, HP_STAT_INDEX), swap_endian_short(dst->evs[HP_STAT_INDEX])));
    for(int i = 0; i < GEN1_STATS_TOTAL; i++)
        dst->stats[i] = swap_endian_short(calc_stats_gen1(growth->species, i, dst->level, get_iv_gen2(dst->ivs, i), swap_endian_short(dst->evs[i])));

    // Text conversions
    convert_strings_of_gen3(src, growth->species, dst->ot_name, dst->ot_name_jp, dst->nickname, dst->nickname_jp, 0, 0);

    return 1;
}