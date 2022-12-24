#include <gba.h>
#include "party_handler.h"
#include "text_handler.h"

u8 get_unown_letter_gen3(u32);
u8 get_pokemon_gender_kind_gen3(int, u32, u32);

// Order is G A E M, or M E A G reversed. These are their indexes.
u8 positions[] = {0b11100100, 0b10110100, 0b11011000, 0b10011100, 0b01111000, 0b01101100,
                  0b11100001, 0b10110001, 0b11010010, 0b10010011, 0b01110010, 0b01100011,
                  0b11001001, 0b10001101, 0b11000110, 0b10000111, 0b01001110, 0b01001011,
                  0b00111001, 0b00101101, 0b00110110, 0b00100111, 0b00011110, 0b00011011};

u8 gender_thresholds_gen3[] = {127, 0, 31, 63, 191, 225, 254, 255};
u8 gender_thresholds_gen12[] = {8, 0, 2, 4, 12, 14, 16, 17};

u8 sprite_counter;

#define MF_7_1_INDEX 2
#define M_INDEX 1
#define F_INDEX 6
#define U_INDEX 7
#define M_GENDER 0
#define F_GENDER 1
#define U_GENDER 2
#define EGG_NUMBER 412
#define NAME_SIZE 11
#define UNOWN_NUMBER 201
#define UNOWN_B_START 415
#define UNOWN_EX_LETTER 26
#define UNOWN_I_LETTER 8
#define UNOWN_V_LETTER 21
#define INITIAL_MAIL_GEN3 121
#define LAST_MAIL_GEN3 132
const u8 pokemon_moves_pp_gen1_bin[];
const u8 pokemon_moves_pp_bin[];
const u8 pokemon_gender_bin[];
const u8 pokemon_names_bin[];
const u8 sprites_cmp_bin[];
const u8 palettes_references_bin[];
const u8 item_gen3_to_12_bin[];
const u8 pokemon_exp_groups_bin[];
const u8 exp_table_bin[];
const u8 pokemon_stats_bin[];
const struct exp_level* exp_table = (const struct exp_level*)exp_table_bin;
const struct stats_gen_23* stats_table = (const struct stats_gen_23*)pokemon_stats_bin;

u16 get_mon_index(int index, u32 pid, u32 is_egg){
    if(index > LAST_VALID_GEN_3_MON)
        return 0;
    if(is_egg)
        return EGG_NUMBER;
    if(index != UNOWN_NUMBER)
        return index;
    u8 letter = get_unown_letter_gen3(pid);
    if(letter == 0)
        return UNOWN_NUMBER;
    return UNOWN_B_START+letter-1;
}

const u8* get_pokemon_name(int index, u32 pid, u32 is_egg){
    return &(pokemon_names_bin[get_mon_index(index, pid, is_egg)*NAME_SIZE]);
}

const u8* get_pokemon_sprite_pointer(int index, u32 pid, u32 is_egg){
    u16* sprites_cmp_bin_16 = (u16*)sprites_cmp_bin;
    return &(sprites_cmp_bin[(sprites_cmp_bin_16[1+get_mon_index(index, pid, is_egg)]<<2)+sprites_cmp_bin_16[0]]);
}

u8 get_pokemon_gender_gen3(int index, u32 pid, u32 is_egg){
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

u8 get_pokemon_gender_kind_gen3(int index, u32 pid, u32 is_egg){
    return pokemon_gender_bin[get_mon_index(index, pid, is_egg)];
}

u8 get_palette_references(int index, u32 pid, u32 is_egg){
    return palettes_references_bin[get_mon_index(index, pid, is_egg)];
}

void init_sprite_counter(){
    sprite_counter = 0;
}

u8 get_sprite_counter(){
    return sprite_counter;
}

#define OAM 0x7000000

void load_pokemon_sprite(int index, u32 pid, u32 is_egg){
    u32 address = get_pokemon_sprite_pointer(index, pid, is_egg);
    u8 palette = get_palette_references(index, pid, is_egg);
    LZ77UnCompVram(address, VRAM+0x10000+(sprite_counter*0x400));
    u16 obj_attr_0 = (SCREEN_HEIGHT - 32);
    u16 obj_attr_1 = (32*sprite_counter)|(1<<15);
    u16 obj_attr_2 = (32*sprite_counter)|(palette<<12);
    *((u16*)(OAM + (8*sprite_counter) + 0)) = obj_attr_0;
    *((u16*)(OAM + (8*sprite_counter) + 2)) = obj_attr_1;
    *((u16*)(OAM + (8*sprite_counter) + 4)) = obj_attr_2;
    sprite_counter++;
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

u16 calc_stats_gen2(u16 species, u8 stat_index, u8 level, u8 iv, u16 stat_exp) {
    if(species > LAST_VALID_GEN_2_MON)
        species = 0;
    u16 base = 5;
    if(stat_index == HP_STAT_INDEX)
        base = level + 10;
    return base + Div(((stats_table[species].stats[stat_index] + iv) + (Sqrt(stat_exp) >> 2)) * level, 100);
}

u16 swap_endian_short(u16 shrt) {
    return ((shrt & 0xFF00) >> 8) | ((shrt & 0xFF) << 8);
}

u32 swap_endian_int(u32 integer) {
    return ((integer & 0xFF000000) >> 24) | ((integer & 0xFF) << 24) | ((integer & 0xFF0000) >> 8) | ((integer & 0xFF00) << 8);
}

u8 gen3_to_gen2(struct gen2_mon* dst, struct gen3_mon* src) {
    u32 decryption[ENC_DATA_SIZE>>2];
    
    if(src->is_bad_egg)
        return 0;
    
    // Initial data decryption
    if(!decrypt_data(src, decryption))
        return 0;
    
    // Interpret the decrypted data
    u32 index_key = src->pid;
    // Make use of modulo properties to get this to positives
    while(index_key >= 0x80000000)
        index_key -= 0x7FFFFFF8;
    s32 index = DivMod(index_key, 24);
    
    struct gen3_mon_growth* growth = (struct gen3_mon_growth*)&(decryption[3*((positions[index] >> 0)&3)]);
    struct gen3_mon_attacks* attacks = (struct gen3_mon_attacks*)&(decryption[3*((positions[index] >> 2)&3)]);
    struct gen3_mon_evs* evs = (struct gen3_mon_evs*)&(decryption[3*((positions[index] >> 4)&3)]);
    struct gen3_mon_misc* misc = (struct gen3_mon_misc*)&(decryption[3*((positions[index] >> 6)&3)]);
    
    // Item checks
    if(growth->item >= INITIAL_MAIL_GEN3 && growth->item <= LAST_MAIL_GEN3)
        return 0;
    
    // Validity checks
    if(growth->species > LAST_VALID_GEN_2_MON)
        return 0;
    
    // Get shinyness for checks
    u16 is_shiny = (src->pid & 0xFFFF) ^ (src->pid >> 16) ^ (src->ot_id & 0xFFFF) ^ (src->ot_id >> 16);
    if(is_shiny < 8)
        is_shiny = 1;
    else
        is_shiny = 0;
    
    u8 gender = get_pokemon_gender_gen3(growth->species, src->pid, 0);
    u8 gender_kind = get_pokemon_gender_kind_gen3(growth->species, src->pid, 0);
    u8 letter = 0;
    
    // These Pokemon cannot be both female and shiny in gen 1/2
    if(is_shiny && (gender == F_GENDER) && (gender_kind == MF_7_1_INDEX))
        return 0;
    
    // Unown ! and ? did not exist in gen 2
    // Not only that, but only Unown I and V can be shiny
    if(growth->species == UNOWN_NUMBER) {
        letter = get_unown_letter_gen3(src->pid);
        if(letter >= UNOWN_EX_LETTER)
            return 0;
        if(is_shiny && (letter != UNOWN_I_LETTER) && (letter != UNOWN_V_LETTER))
            return 0;
    }
    
    // Start converting moves
    u8 used_slots = 0;
    for(int i = 0; i < 4; i++)
    {
        dst->moves[i] = 0;
        dst->pps[i] = 0;
    }
    for(int i = 0; i < 4; i++)
        if(attacks->moves[i] > 0 && attacks->moves[i] <= LAST_VALID_GEN_2_MOVE) {
            u8 base_pp = pokemon_moves_pp_bin[attacks->moves[i]];
            u8 bonus_pp = (growth->pp_bonuses >> (2*i)) & 3;
            u8 base_increase_pp = Div(base_pp, 5);
            base_pp += (base_increase_pp * bonus_pp);
            if(base_pp >= 61)
                base_pp = 61;
            base_pp |= (bonus_pp << 6);
            dst->pps[used_slots] = base_pp;
            dst->moves[used_slots++] = attacks->moves[i];
        }
    
    // No valid moves were found
    if(used_slots == 0)
        return 0;
    
    // Item handling
    u16 item = growth->item;
    if(item > LAST_VALID_GEN_3_ITEM)
        item = 0;
    dst->item = item_gen3_to_12_bin[item];
    
    // OT handling
    dst->ot_id = swap_endian_short(src->ot_id & 0xFFFF);
    
    // Level handling
    u8 level = src->level;
    if(level < MIN_LEVEL)
        level = MIN_LEVEL;
    if(level > MAX_LEVEL)
        level = MAX_LEVEL;
    
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
    u32 nature_pid = src->pid;
    while(nature_pid >= 0x80000000)
        nature_pid -= 0x7FFFFFE9;
    u8 nature = DivMod(nature_pid, 25);

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
    dst->level = level;
    for(int i = 0; i < 3; i++)
        dst->exp[2-i] = (exp >> (8*i))&0xFF;
    
    // Zero all EVs, since it's an entirely different system
    for(int i = 0; i < 5; i++)
        dst->evs[i] = 0;
    
    // Assign IVs
    // Keep in mind: Unown letter, gender and shinyness
    // Hidden Power calculations are too restrictive
    u8 atk_ivs = (misc->atk_ivs >> 1);
    u8 def_ivs = (misc->def_ivs >> 1);
    u8 spa_ivs = ((misc->spa_ivs + misc->spd_ivs) >> 2);
    u8 spe_ivs = (misc->spe_ivs >> 1);
    u8 hp_ivs;
    
    // Unown letter
    if(growth->species == UNOWN_NUMBER) {
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
    if(!is_shiny && ((atk_ivs & 2) == 2 && def_ivs == 10 && spa_ivs == 10 && spe_ivs == 10))
        spe_ivs = 11;
    
    // Finally, assign
    dst->ivs = (atk_ivs << 4) | def_ivs | (spa_ivs << 8) | (spe_ivs << 12);
    hp_ivs = ((atk_ivs & 1) << 3) | ((def_ivs & 1) << 2) | ((spe_ivs & 1) << 1) | (spa_ivs & 1);
    
    // Is this really how it works...?
    dst->pokerus = misc->pokerus;
    
    // Defaults
    dst->friendship = BASE_FRIENDSHIP;
    dst->data = 0;
    dst->status = 0;
    dst->unused = 0;
    
    u16 stat = calc_stats_gen2(growth->species, 0, level, hp_ivs, swap_endian_short(dst->evs[0]));
    dst->stats[0] = swap_endian_short(stat);
    dst->stats[1] = swap_endian_short(stat);
    stat = calc_stats_gen2(growth->species, 1, level, atk_ivs, swap_endian_short(dst->evs[1]));
    dst->stats[2] = swap_endian_short(stat);
    stat = calc_stats_gen2(growth->species, 2, level, def_ivs, swap_endian_short(dst->evs[2]));
    dst->stats[3] = swap_endian_short(stat);
    stat = calc_stats_gen2(growth->species, 5, level, spe_ivs, swap_endian_short(dst->evs[3]));
    dst->stats[4] = swap_endian_short(stat);
    stat = calc_stats_gen2(growth->species, 3, level, spa_ivs, swap_endian_short(dst->evs[4]));
    dst->stats[5] = swap_endian_short(stat);
    stat = calc_stats_gen2(growth->species, 4, level, spa_ivs, swap_endian_short(dst->evs[4]));
    dst->stats[6] = swap_endian_short(stat);
    
    dst->is_egg = misc->is_egg;
    
    iprintf("Species: %s\n", get_pokemon_name(growth->species, src->pid, misc->is_egg));
    load_pokemon_sprite(growth->species, src->pid, misc->is_egg);
    
    return 1;
}