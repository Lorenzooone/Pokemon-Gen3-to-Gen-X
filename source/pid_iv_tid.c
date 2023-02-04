#include <gba.h>
#include "pid_iv_tid.h"
#include "rng.h"
#include "gen12_methods.h"
#include "text_handler.h"
#include "fast_pokemon_methods.h"
#include "optimized_swi.h"
#include "print_system.h"
#include "useful_qualifiers.h"
#include <stddef.h>

#include "shiny_unown_banned_tsv_bin.h"
#include "shiny_unown_banned_tsv_letter_table_bin.h"
    
#define TEST_WORST_CASE 0

#define NUM_SEEDS 0x10000
#define NUM_DIFFERENT_PSV (0x10000>>3)
#define NUM_DIFFERENT_UNOWN_PSV (0x10000>>5)

static u8 is_bad_tsv(u16);
static void get_letter_valid_natures(u16, u8, u8*);
static u32 get_prev_seed(u32);
static u32 get_next_seed(u32);
void _generate_egg_info(u8, u16, u16, u8, u8, u32*, u32*, u32);
void _generate_egg_shiny_info(u8, u16, u8, u8, u32*, u32*, u32);
void _generate_static_info(u8, u16, u16, u32*, u32*, u32);
void _generate_static_shiny_info(u8, u16, u32*, u32*, u32);
void _generate_unown_info(u8, u8, u8, u16, u32*, u32*, u32);
u8 search_specific_low_pid(u32, u32*, u32*);
u8 search_unown_pid_masks(u32, u8, u32*, u32*);
void _generate_unown_shiny_info(u8, u16, u8, u32*, u32*, u32);

// Nidoran M is special, it has to have 0x8000 set in the lower PID
u8 gender_useful_atk_ivs[] = {3, 4, 1, 2, 2, 1, 4, 4, 4, 4};
u16 gender_values[] = {0x7F, 0, 0x1F, 0x3F, 0xBF, 0xDF, 0, 0, 0x7FFF, 0};
const u8 wanted_nature_shiny_table[NUM_NATURES] = {0, 1, 2, 3, 4, 5, 6, 7, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x15, 0x16, 0x17, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45};

u8 unown_tsv_possibilities[4][NUM_UNOWN_LETTERS_GEN3][4];
u8 unown_tsv_numbers[4][NUM_UNOWN_LETTERS_GEN3];

void init_unown_tsv() {
    for(int i = 0; i < NUM_UNOWN_LETTERS_GEN3; i++) {
        int pos = i;
        for(int j = 0; j < 4; j++)
            unown_tsv_numbers[j][i] = 0;
        while(pos < 256) {
            u8 flag = ((pos >> 2)&3) ^ ((pos >> 6)&3);
            unown_tsv_possibilities[flag][i][unown_tsv_numbers[flag][i]++] = pos;
            pos += NUM_UNOWN_LETTERS_GEN3;
        }
    }
}

ALWAYS_INLINE MAX_OPTIMIZE u8 is_bad_tsv(u16 tsv) {
    tsv = (tsv >> 3) << 3;
    const u32* shiny_unown_banned_tsv_bin_32 = (const u32*)shiny_unown_banned_tsv_bin;
    for(size_t i = 0; i < (shiny_unown_banned_tsv_bin_size >> 2); i++) {
        if(tsv == (shiny_unown_banned_tsv_bin_32[i] & 0xFFFF))
            return 1;
    }
    return 0;
}

ALWAYS_INLINE MAX_OPTIMIZE void get_letter_valid_natures(u16 tsv, u8 letter, u8* valid_natures) {
    tsv = (tsv >> 3) << 3;
    for(int i = 0; i < NUM_NATURES; i++)
        valid_natures[i] = 1;
    u8 letter_kind = shiny_unown_banned_tsv_letter_table_bin[letter] >> 4;
    u8 letter_dist = shiny_unown_banned_tsv_letter_table_bin[letter] & 0xF;
    if(letter_kind == 0xF)
        return;
    const u32* shiny_unown_banned_tsv_bin_32 = (const u32*)shiny_unown_banned_tsv_bin;
    for(size_t i = 0; i < (shiny_unown_banned_tsv_bin_size >> 2); i++) {
        if(tsv == (shiny_unown_banned_tsv_bin_32[i] & 0xFFFF)) {
            u8 letter_kind_table = (shiny_unown_banned_tsv_bin_32[i]>>0x14)&0xF;
            u8 letter_dist_table = (shiny_unown_banned_tsv_bin_32[i]>>0x10)&0xF;
            if(letter_kind == letter_kind_table) {
                letter_dist += letter_dist_table;
                if(letter_dist >= 5)
                    letter_dist -= 5;
                for(int j = 0; j < 5; j++)
                    valid_natures[letter_dist + (j*5)] = 0;
            }
        }
    }
}

ALWAYS_INLINE MAX_OPTIMIZE u32 get_prev_seed(u32 seed) {
    return (seed-0x6073) * 0xEEB9EB65;
}

ALWAYS_INLINE MAX_OPTIMIZE u32 get_next_seed(u32 seed) {
    return ((seed*0x41C64E6D)+0x6073);
}

IWRAM_CODE MAX_OPTIMIZE u32 generate_ot(u16 tid, u8* name) {
    // Worst case: ANY
    // This should NOT be random...
    // All Pokémon from a trainer should have the same TID/SID!
    
    u32 seed = 0;
    u16 sid = 0;
    seed = get_next_seed(seed);
    
    for(int i = 0; i < OT_NAME_GEN3_SIZE; i++) {
        if(name[i] == GEN3_EOL)
            break;
        seed += name[i];
        seed = get_next_seed(seed);
    }
                            
    seed += tid;
    do {
        // Unown V and I checks here!
        seed = get_next_seed(seed);
        sid = seed >> 0x10;
    }
    while(is_bad_tsv(sid^tid));
    
    return (sid << 0x10) | tid;
}

IWRAM_CODE MAX_OPTIMIZE void _generate_egg_info(u8 wanted_nature, u16 wanted_ivs, u16 tsv, u8 gender, u8 gender_kind, u32* dst_pid, u32* dst_ivs, u32 start_seed) {
    // Worst case: 0, 0x2113, 0, 0, 0, 34
    u8 atk_ivs = ((((wanted_ivs>>4) & 0xF)<<(4-gender_useful_atk_ivs[gender_kind])) & 0xF)<<1;
    u8 def_ivs = ((wanted_ivs) & 0xF)<<1;
    u8 spe_ivs = ((wanted_ivs>>12) & 0xF)<<1;
    u8 spa_ivs = ((wanted_ivs>>8) & 0xF)<<1;
    
    u32 pass_counter = 0;
    
    u8 limit = ((spa_ivs+1) * 2);
    if(limit > 0x20)
        limit = 0x20;
    u8 start = (spa_ivs*2) - 0x1F;
    if((spa_ivs*2) < 0x1F)
        start = 0;
    
    u8 base_spe = (start_seed >> 0) & 1;
    u8 base_spd = (start_seed >> 1) & 1;
    u8 initial_spa = SWI_DivMod((start_seed>>3)&0x1FFF, (limit-start));
    
    for(int i = 0; i < (limit-start); i++) {
        for(int j = 0; j < 2; j++) {
            u8 new_spa_ivs = start + (initial_spa-i);
            u8 new_spd_ivs = (spa_ivs*2)-new_spa_ivs + (j^base_spd);
            if(new_spa_ivs > ((spa_ivs*2) + (j^base_spd)))
                continue;
            if(new_spd_ivs > 0x1F)
                continue;
            for(int u = 0; u < 2; u++) {
                u8 new_spe_ivs = spe_ivs + (u^base_spe);
                for(int k = 0; k < 2; k++) {
                    for(int l = 0; l < NUM_SEEDS; l++) {
                        u32 pid = 0;
                        u32 ivs = 0;
                        u32 seed = (k<<31) | (new_spe_ivs << 16) | (new_spa_ivs << 21) | (new_spd_ivs << 26) | l;
                        
                        u16 generated_ivs = get_prev_seed(seed) >> 16;
                        u8 new_atk_ivs = (((generated_ivs >> 5)&0x1F)>>(1+(4-gender_useful_atk_ivs[gender_kind]))) << (1+(4-gender_useful_atk_ivs[gender_kind]));
                        u8 new_def_ivs = (((generated_ivs >> 10)&0x1F)>>1) << 1;
                        
                        if(new_atk_ivs == atk_ivs && new_def_ivs == def_ivs) {
                            ivs = (generated_ivs & 0x7FFF) | (((seed>>16) & 0x7FFF)<<15);
                            seed = get_prev_seed(seed);
                            seed = get_prev_seed(seed);
                            u16 high_pid = seed >> 16;
                            u32 inner_seed = get_prev_seed(start_seed>>0x10);
                            u16 lower_pid = 0;
                            do {
                                inner_seed = get_prev_seed(inner_seed);
                                lower_pid = inner_seed >> 0x10;
                                if(gender_values[gender_kind] != 0) {
                                    if((gender == M_GENDER) && ((lower_pid & 0xFF) < gender_values[gender_kind]))
                                        lower_pid = lower_pid | (gender_values[gender_kind]+1);
                                    else if((gender == F_GENDER) && ((lower_pid & 0xFF) >= gender_values[gender_kind])){
                                        lower_pid = (lower_pid & 0xFF00) | (lower_pid & gender_values[gender_kind]);
                                        if((lower_pid & 0xFF) == gender_values[gender_kind])
                                            lower_pid -= 1;
                                    }
                                }
                                else if(gender_kind == NIDORAN_F_GENDER_INDEX)
                                    lower_pid &= 0x7FFF;
                                
                                lower_pid &= 0xFFE0;
                                
                            } while((lower_pid == 0) || (lower_pid == 0xFFE0) || (((high_pid) ^ (lower_pid) ^ tsv) < 0x20));
                            
                            pid = (high_pid << 0x10) | lower_pid;
                            u8 nature = get_nature_fast(pid);
                            u8 nature_diff = wanted_nature - nature;
                            if(nature > wanted_nature)
                                nature_diff = wanted_nature - nature + NUM_NATURES;
                            lower_pid += nature_diff;
                            
                            if(nature_diff < (0x20-NUM_NATURES))
                                if((start_seed >> 2) & 1) {
                                    lower_pid += NUM_NATURES;
                                    if((gender == F_GENDER) && (gender_values[gender_kind] == 0x1F) && ((lower_pid & 0xFF) == 0x1F))
                                        lower_pid -= NUM_NATURES;
                                }
                            *dst_ivs |= ivs;
                            *dst_pid = lower_pid | (high_pid << 0x10);
                            return;
                        }
                    }
                    pass_counter++;
                }
            }
        }
        if(initial_spa == i)
            initial_spa += (limit-start);
    }
}

IWRAM_CODE MAX_OPTIMIZE void generate_egg_info(u8 index, u8 wanted_nature, u16 wanted_ivs, u16 tsv, u8 curr_gen, u32* dst_pid, u32* dst_ivs) {
    _generate_egg_info(wanted_nature, wanted_ivs, tsv, get_pokemon_gender_gen2(index, (wanted_ivs>>4)&0xF, 0, curr_gen), get_pokemon_gender_kind_gen2(index, 0, curr_gen), dst_pid, dst_ivs, get_rng());
}

IWRAM_CODE MAX_OPTIMIZE void _generate_egg_shiny_info(u8 wanted_nature, u16 tsv, u8 gender, u8 gender_kind, u32* dst_pid, u32* dst_ivs, u32 start_seed) {
    // Worst case: ANY
    u16 lower_pid = start_seed &0xFFFF;
    
    if(gender_values[gender_kind] != 0) {
        if((gender == M_GENDER) && ((lower_pid & 0xFF) < gender_values[gender_kind]))
            lower_pid = lower_pid | (gender_values[gender_kind]+1);
        else if((gender == F_GENDER) && ((lower_pid & 0xFF) >= gender_values[gender_kind])){
            lower_pid = (lower_pid & 0xFF00) | (lower_pid & gender_values[gender_kind]);
            if((lower_pid & 0xFF) == gender_values[gender_kind])
                lower_pid -= 1;
        }
    }
    else if(gender_kind == NIDORAN_F_GENDER_INDEX)
        lower_pid &= 0x7FFF;
                            
    lower_pid &= 0xFFF8;
    
    // 0 is not allowed
    if (lower_pid == 0)
        lower_pid = 0x200;
    
    // 0xFFFF is not allowed (%0xFFFE + 1)
    if(lower_pid == 0xFFF8)
        lower_pid -= 0x100;
    
    u16 higher_pid = (lower_pid ^ tsv) & 0xFFF8;
    u32 pid = (higher_pid << 0x10) | lower_pid;
    
    u8 nature = get_nature_fast(pid);
    u8 nature_diff = wanted_nature - nature;
    if(nature > wanted_nature)
        nature_diff = wanted_nature - nature + NUM_NATURES;
    
    lower_pid |= (wanted_nature_shiny_table[nature_diff] & 0x7);
    higher_pid |= ((wanted_nature_shiny_table[nature_diff]>>4) & 0x7);
    pid = (higher_pid << 0x10) | lower_pid;
    
    u32 seed = (higher_pid << 16) | (start_seed >> 16);
    seed = get_next_seed(seed);
    u16 generated_ivs = (seed >> 16) & 0x7FFF;
    
    *dst_ivs |= generated_ivs | (((get_next_seed(seed)>>16)&0x7FFF)<<15);
    *dst_pid = pid;
}

IWRAM_CODE MAX_OPTIMIZE void generate_egg_shiny_info(u8 index, u8 wanted_nature, u16 wanted_ivs, u16 tsv, u8 curr_gen, u32* dst_pid, u32* dst_ivs) {
    _generate_egg_shiny_info(wanted_nature, tsv, get_pokemon_gender_gen2(index, (wanted_ivs>>4)&0xF, 0, curr_gen), get_pokemon_gender_kind_gen2(index, 0, curr_gen), dst_pid, dst_ivs, get_rng());
}

IWRAM_CODE MAX_OPTIMIZE void _generate_static_info(u8 wanted_nature, u16 wanted_ivs, u16 tsv, u32* dst_pid, u32* dst_ivs, u32 start_seed) {
    // Worst case: 6, 12706, 0xA30E, 0
    u8 atk_ivs = ((wanted_ivs>>4) & 0xF)<<1;
    u8 def_ivs = ((wanted_ivs) & 0xF)<<1;
    u8 spe_ivs = ((wanted_ivs>>12) & 0xF)<<1;
    u8 spa_ivs = ((wanted_ivs>>8) & 0xF)<<1;
    
    u8 limit = ((spa_ivs+1) * 2);
    if(limit > 0x20)
        limit = 0x20;
    u8 start = (spa_ivs*2) - 0x1F;
    if((spa_ivs*2) < 0x1F)
        start = 0;
    
    u32 pass_counter = 0;
    
    u8 base_spe = (start_seed >> 0) & 1;
    u8 base_spd = (start_seed >> 1) & 1;
    u8 initial_spa = SWI_DivMod(start_seed>>2, (limit-start));
    
    for(int i = 0; i < (limit-start); i++) {
        for(int j = 0; j < 2; j++) {
            u8 new_spa_ivs = start + (initial_spa-i);
            u8 new_spd_ivs = (spa_ivs*2)-new_spa_ivs + (j^base_spd);
            if(new_spa_ivs > ((spa_ivs*2) + (j^base_spd)))
                continue;
            if(new_spd_ivs > 0x1F)
                continue;
            for(int u = 0; u < 2; u++) {
                u8 new_spe_ivs = spe_ivs + (u^base_spe);
                for(int k = 0; k < 2; k++) {
                    for(int l = 0; l < NUM_SEEDS; l++) {
                        u32 pid = 0;
                        u32 ivs = 0;
                        u32 seed = (k<<31) | (new_spe_ivs << 16) | (new_spa_ivs << 21) | (new_spd_ivs << 26) | l;
                        
                        u16 generated_ivs = get_prev_seed(seed) >> 16;
                        u8 new_atk_ivs = (((generated_ivs >> 5)&0x1F)>>1) << 1;
                        u8 new_def_ivs = (((generated_ivs >> 10)&0x1F)>>1) << 1;
                        
                        if(new_atk_ivs == atk_ivs && new_def_ivs == def_ivs) {
                            ivs = (generated_ivs & 0x7FFF) | (((seed>>16) & 0x7FFF)<<15);
                            seed = get_prev_seed(seed);
                            seed = get_prev_seed(seed);
                            pid |= seed & 0xFFFF0000;
                            seed = get_prev_seed(seed);
                            pid |= (seed & 0xFFFF0000)>>16;
                            u16 shiny_pid = (pid>>16) ^ (pid & 0xFFFF) ^ tsv;
                            u8 nature = get_nature_fast(pid);
                            if((nature == wanted_nature) && (shiny_pid >= 8)) {
                                *dst_pid = pid;
                                *dst_ivs |= ivs;
                                return;
                            }
                        }
                    }
                    pass_counter++;
                }
            }
        }
        if(initial_spa == i)
            initial_spa += (limit-start);
    }
}

IWRAM_CODE MAX_OPTIMIZE void generate_static_info(u8 wanted_nature, u16 wanted_ivs, u16 tsv, u32* dst_pid, u32* dst_ivs) {
    _generate_static_info(wanted_nature, wanted_ivs, tsv, dst_pid, dst_ivs, get_rng());
}

IWRAM_CODE MAX_OPTIMIZE void _generate_static_shiny_info(u8 wanted_nature, u16 tsv, u32* dst_pid, u32* dst_ivs, u32 given_seed) {
    // Worst case: 0, 0x2088, (base_increment = 1, initial_pos = 0x6D0, set TEST_WORST_CASE to 1 to test it, with seed 0x36810000)
    s32 base_increment;
    u16 initial_pos;
    u16 high_pid;
    u16 low_pid;
    u8 nature;
    u32 generating_seed = get_next_seed(given_seed);
    #if TEST_WORST_CASE
    base_increment = (1 << ((given_seed >> 16) & 3))*(-1 + (2 * ((given_seed>>18)&1)));
    initial_pos = given_seed >> 19;
    #else
    base_increment = (1 << ((generating_seed >> 16) & 3))*(-1 + (2 * ((generating_seed>>18)&1)));
    initial_pos = generating_seed >> 19;
    #endif
    u16 pos = initial_pos;
    generating_seed = get_next_seed(generating_seed);
    
    for(int i = 0; i < NUM_DIFFERENT_PSV; i++) {

        generating_seed = get_next_seed(generating_seed); 
        low_pid = generating_seed >> 16;
        generating_seed = get_next_seed(generating_seed);
        high_pid = generating_seed >> 16;
        
        u16 shiny_pid = (high_pid ^ low_pid ^ tsv);
        
        nature = get_nature_fast((high_pid<<16) | low_pid);
        
        if((shiny_pid < 8) && (nature == wanted_nature)) {
            generating_seed = get_next_seed(generating_seed); 
            u16 generated_ivs = (generating_seed >> 16)&0x7FFF;
            generating_seed = get_next_seed(generating_seed);
            *dst_pid = low_pid | (high_pid<<16);
            *dst_ivs |= generated_ivs | (((generating_seed>>16)&0x7FFF)<<15);
            return;
        }
        
        high_pid = pos<<3;
        
        low_pid = (high_pid ^ tsv);
        
        nature = get_nature_fast((high_pid<<16) | low_pid);
        
        low_pid = low_pid & 0xFFF8;
        high_pid = high_pid & 0xFFF8;
        nature = get_nature_fast((high_pid<<16) | low_pid);
        u8 nature_diff = wanted_nature-nature;
        if(wanted_nature < nature)
            nature_diff = NUM_NATURES + wanted_nature-nature;

        low_pid |= (wanted_nature_shiny_table[nature_diff] & 0x7);
        high_pid |= ((wanted_nature_shiny_table[nature_diff]>>4) & 0x7);
        
        for(int j = 0; j < NUM_SEEDS; j++) {
            u32 seed = (low_pid<<16) | j;
            
            u16 generated_high_pid = get_next_seed(seed) >> 16;
            
            if(high_pid == generated_high_pid) {
                seed = get_next_seed(seed);
                seed = get_next_seed(seed);
                u16 generated_ivs = (seed>>16)&0x7FFF;
                seed = get_next_seed(seed);
                *dst_pid = low_pid | (high_pid<<16);
                *dst_ivs |= generated_ivs | (((seed>>16)&0x7FFF)<<15);
                return;
            }
        }

        if(pos + base_increment < 0)
            pos += NUM_DIFFERENT_PSV;
        if(pos + base_increment >= NUM_DIFFERENT_PSV)
            pos += base_increment - NUM_DIFFERENT_PSV;
        else
            pos += base_increment;
        if(pos == initial_pos) {
            pos += 1;
            initial_pos += 1;
            if(initial_pos >= NUM_DIFFERENT_PSV)
                initial_pos -= NUM_DIFFERENT_PSV;            
            if(pos >= NUM_DIFFERENT_PSV)
                pos -= NUM_DIFFERENT_PSV;
        }
    }
}

IWRAM_CODE MAX_OPTIMIZE void generate_static_shiny_info(u8 wanted_nature, u16 tsv, u32* dst_pid, u32* dst_ivs) {
    _generate_static_shiny_info(wanted_nature, tsv, dst_pid, dst_ivs, get_rng());
}

IWRAM_CODE MAX_OPTIMIZE void _generate_unown_info(u8 wanted_nature, u8 letter, u8 rest_of_ivs, u16 tsv, u32* dst_pid, u32* dst_ivs, u32 start_seed) {
    // Worst case: 15, 4, 0xCE, 0xA021, 0x1AA
    u8 atk_ivs = (((rest_of_ivs>>0)&1) | (((rest_of_ivs>>0)&2)<<2))<<1;
    u8 def_ivs = (((rest_of_ivs>>2)&1) | (((rest_of_ivs>>2)&2)<<2))<<1;
    u8 spe_ivs = (((rest_of_ivs>>4)&1) | (((rest_of_ivs>>4)&2)<<2))<<1;
    u8 spa_ivs = (((rest_of_ivs>>6)&1) | (((rest_of_ivs>>6)&2)<<2))<<1;
    
    u8 base_hp = start_seed & 0x1F;
    u8 base_atk = (start_seed >> 5) & 7;
    u8 base_def = (start_seed >> 8) & 7;
    
    for(int i = 0; i < 0x20; i++) {
        u8 new_hp_ivs = (base_hp-i);
        u8 inside_base_atk = base_atk;
        for(int j = 0; j < 8; j++) {
            u8 inside_base_def = base_def;
            for(int u = 0; u < 8; u++) {
                u8 new_atk_ivs = atk_ivs | ((inside_base_atk-j) & 1) | ((((inside_base_atk-j)>>1)&3)<<2);
                u8 new_def_ivs = def_ivs | ((inside_base_def-u) & 1) | ((((inside_base_def-u)>>1)&3)<<2);
                for(u32 k = 0; k < 2; k++) {
                    for(u32 l = 0; l < NUM_SEEDS; l++) {
                        u32 pid = 0;
                        u32 ivs = 0;
                        u32 seed = (k<<31) | (new_hp_ivs << 16) | (new_atk_ivs << 21) | (new_def_ivs << 26) | l;
                        
                        u16 generated_ivs = get_next_seed(seed) >> 16;
                        u8 new_spe_ivs = (generated_ivs & 0x12);
                        u8 new_spa_ivs = (((((generated_ivs >> 5) & 0x1F) + ((generated_ivs >> 10) & 0x1F))>>1) & 0x12);
                        
                        if(new_spa_ivs == spa_ivs && new_spe_ivs == spe_ivs) {
                            ivs = ((generated_ivs & 0x7FFF)<<15) | ((seed>>16) & 0x7FFF);
                            seed = get_prev_seed(seed);
                            pid |= seed >> 16;
                            seed = get_prev_seed(seed);
                            pid |= seed & 0xFFFF0000;
                            u16 shiny_pid = (pid>>16) ^ (pid & 0xFFFF) ^ tsv;
                            u8 nature = get_nature_fast(pid);
                            u8 generated_letter = get_unown_letter_gen3_fast(pid);
                            if((nature == wanted_nature) && (letter == generated_letter) && (shiny_pid >= 8)) {
                                *dst_ivs |= ivs;
                                *dst_pid = pid;
                                return;
                            }
                        }
                    }
                }
                if(u == inside_base_def)
                    inside_base_def += 8;
            }
            if(j == inside_base_atk)
                inside_base_atk += 8;
        }
        if(i == base_hp)
            base_hp += 0x20;
    }
}

IWRAM_CODE MAX_OPTIMIZE void generate_unown_info(u8 wanted_nature, u16 wanted_ivs, u16 tsv, u32* dst_pid, u32* dst_ivs) {
    u8 atk_ivs = ((wanted_ivs>>4) & 0xF);
    u8 def_ivs = ((wanted_ivs) & 0xF);
    u8 spe_ivs = ((wanted_ivs>>12) & 0xF);
    u8 spa_ivs = ((wanted_ivs>>8) & 0xF);
    u8 letter = get_unown_letter_gen2_fast(wanted_ivs);
    u8 rest_of_ivs = (((atk_ivs & 1) | ((atk_ivs>>2)&2))<<0) | (((def_ivs & 1) | ((def_ivs>>2)&2))<<2) | (((spe_ivs & 1) | ((spe_ivs>>2)&2))<<4) | (((spa_ivs & 1) | ((spa_ivs>>2)&2))<<6);
    _generate_unown_info(wanted_nature, letter, rest_of_ivs, tsv, dst_pid, dst_ivs, get_rng());
}

IWRAM_CODE MAX_OPTIMIZE u8 search_specific_low_pid(u32 pid, u32* dst_pid, u32* dst_ivs) {
    u16 new_high = pid >> 16;
    u16 new_low = pid & 0xFFFF;
    for(int u = 0; u < NUM_SEEDS; u++) {
        u32 seed = (new_high<<16) | u;
        
        u16 generated_low_pid = get_next_seed(seed) >> 16;
        
        if(generated_low_pid == new_low) {
            seed = get_next_seed(seed);
            seed = get_next_seed(seed);
            u32 ivs = (seed >> 16) & 0x7FFF;
            seed = get_next_seed(seed);
            ivs |= ((seed >> 16) & 0x7FFF)<<15;
            *dst_pid = (new_high<<16) | generated_low_pid;
            *dst_ivs |= ivs;
            return 1;
        }
    }
    return 0;
}

IWRAM_CODE MAX_OPTIMIZE u8 search_unown_pid_masks(u32 pid, u8 wanted_nature, u32* dst_pid, u32* dst_ivs) {
    for(int i = 0; i < 0x20; i++)
        for(int j = 0; j < 0x80; j++) {
            u32 mask = 0;
            if(j&1)
                mask +=4;
            if(j&2)
                mask += 0x40000;
            for(int k = 0; k < 5; k++)
                if((j>>(2+k))&1)
                    mask += 0x10001 << (3+k);
            for(int k = 0; k < 5; k++)
                if((i>>k)&1)
                    mask += 0x01000100 << (3+k);
            u8 xor_mod_change = get_nature_fast(pid ^ mask);
            if(xor_mod_change == wanted_nature)
                if(search_specific_low_pid(pid ^ mask, dst_pid, dst_ivs))
                    return 1;
        }
    return 0;  
}

IWRAM_CODE MAX_OPTIMIZE void _generate_unown_shiny_info(u8 wanted_nature, u16 tsv, u8 letter, u32* dst_pid, u32* dst_ivs, u32 given_seed) {
    // Worst case: 5, 0x4FF8, 5, 0x8FC00000
    u16 high_pid;
    u16 low_pid;
    u8 nature;
    u16 shiny_pid;
    u32 pid = 0;
    
    u8 tsv_flag = ((tsv>>8)&3);
    u8 num_valid_values = unown_tsv_numbers[tsv_flag][letter];
    
    u32 generating_seed = get_next_seed(given_seed);
    #if TEST_WORST_CASE
    s32 base_increment = (((given_seed >> 16) & 0xF)*2);
    if(base_increment == 0)
        base_increment += 1;
    base_increment*=(-1 + (2 * ((given_seed>>20)&1)));
    u16 initial_pos = given_seed >> 21;
    #else
    s32 base_increment = (((generating_seed >> 16) & 0xF)*2);
    if(base_increment == 0)
        base_increment += 1;
    base_increment*=(-1 + (2 * ((generating_seed>>20)&1)));
    u16 initial_pos = generating_seed >> 21;
    #endif
    u16 pos = initial_pos;
    generating_seed = get_next_seed(generating_seed);
    
    for(int iter = 0; iter < NUM_DIFFERENT_UNOWN_PSV; iter++) {
        generating_seed = get_next_seed(generating_seed); 
        low_pid = generating_seed >> 16;
        generating_seed = get_next_seed(generating_seed); 
        high_pid = generating_seed >> 16;
        
        shiny_pid = (high_pid ^ low_pid ^ tsv);
        
        pid = (high_pid<<16) | low_pid;
        nature = get_nature_fast(pid);
        
        // Basically, asking for a miracle
        if((nature == wanted_nature) && (get_unown_letter_gen3_fast(pid) == letter) && (shiny_pid < 8)) {
            generating_seed = get_next_seed(generating_seed); 
            u16 generated_ivs = (generating_seed >> 16)&0x7FFF;
            generating_seed = get_next_seed(generating_seed);
            *dst_pid = pid;
            *dst_ivs |= generated_ivs | (((generating_seed>>16)&0x7FFF)<<15);
            return;
        }
        
        low_pid = ((pos << 3) & 0xF8) | (((pos>>5)<<(8+2)));
        
        for(int j = 0; j < num_valid_values; j++) {
            u16 converted_low_pid_flag = unown_tsv_possibilities[tsv_flag][letter][j];
            u16 converted_high_pid_flag = ((converted_low_pid_flag >> 4)&3) | (((converted_low_pid_flag >> 6)&3)<<8);
            converted_low_pid_flag = ((converted_low_pid_flag >> 0)&3) | (((converted_low_pid_flag >> 2)&3)<<8);
            u16 inside_low_pid = low_pid | converted_low_pid_flag;
            high_pid = converted_high_pid_flag | ((inside_low_pid ^ tsv)&0xFCF8);
            
            for(int k = 0; k < 4; k++) {
                u16 new_high = high_pid;
                u16 new_low = inside_low_pid;
                
                if(k&1) {
                    // NOT
                    new_high = (new_high & 0x0303) | ((~new_high) & 0xFCFC);
                    new_low = (new_low & 0x0303) | ((~new_low) & 0xFCFC);
                }
                
                if(k&2) {
                    // SWAP
                    u16 old_new_high = new_high;
                    new_high = (new_high & 0x0303) | ((new_low) & 0xFCFC);
                    new_low = (new_low & 0x0303) | ((old_new_high) & 0xFCFC);
                }
                
                if(search_unown_pid_masks((new_high<<16) | new_low, wanted_nature, dst_pid, dst_ivs))
                    return;
            }
        }
        if(pos + base_increment < 0)
            pos += (0x10000>>5);
        if(pos + base_increment >= (0x10000>>5))
            pos = pos + base_increment - (0x10000>>5);
        else
            pos += base_increment;
        if(pos == initial_pos) {
            pos += 1;
            initial_pos += 1;
            if(initial_pos >= (0x10000>>5))
                initial_pos -= (0x10000>>5);
            if(pos >= (0x10000>>5))
                pos -= (0x10000>>5);
        }
    }
}

IWRAM_CODE MAX_OPTIMIZE void generate_unown_shiny_info(u8 wanted_nature, u16 wanted_ivs, u16 tsv, u32* dst_pid, u32* dst_ivs) {
    u8 letter = get_unown_letter_gen2_fast(wanted_ivs);
    u8 valid_natures[NUM_NATURES];
    get_letter_valid_natures(tsv, letter, valid_natures);
    // Unown TSV checks
    while(!valid_natures[wanted_nature]){
        wanted_nature++;
        if(wanted_nature >= NUM_NATURES)
            wanted_nature -= NUM_NATURES;
    }
    _generate_unown_shiny_info(wanted_nature, letter, tsv, dst_pid, dst_ivs, get_rng());
}

IWRAM_CODE MAX_OPTIMIZE u8 get_roamer_ivs(u32 pid, u8 hp_ivs, u8 atk_ivs, u32* dst_ivs) {
    atk_ivs &= 7;
    
    for(u32 l = 0; l < NUM_SEEDS; l++) {
        u32 ivs = 0;
        u32 seed = (pid<<16) | l;
        
        seed = get_next_seed(seed);
        
        u32 generated_pid = (seed & 0xFFFF0000)| (pid&0xFFFF);
        
        if(pid == generated_pid) {
            seed = get_next_seed(seed);
            ivs = (seed >> 16) & 0x7FFF;
            if((((ivs >> 0)&0x1F) == hp_ivs) && (((ivs >> 5)&7) == atk_ivs)) {
                seed = get_next_seed(seed);
                ivs |= ((seed & 0x7FFF0000) >> 1);
                *dst_ivs = ivs;
                return 1;
            }
        }
    }
    return 0;
}

#if !(TEST_WORST_CASE)
void worst_case_conversion_tester(u32* UNUSED(counter)) {
#else
void worst_case_conversion_tester(u32* counter) {
    
    u32 curr_counter = *counter;
    u32 max_counter = 0;
    u32 pid, ivs;
    
    curr_counter = *counter;
    max_counter = 0;
    
    _generate_egg_info(0, 0x2113, 0, 0, 0, &pid, &ivs, 34);
    
    max_counter = 0;
    if(max_counter < ((*counter)-curr_counter))
        max_counter = ((*counter)-curr_counter);
    curr_counter = *counter;
    
    PRINT_FUNCTION("Max time 1 alt: 0x\x04\n", max_counter);
    
    curr_counter = *counter;
    max_counter = 0;
    
    _generate_static_info(6, 12706, 0xA30E, &pid, &ivs, 0);
    
    if(max_counter < ((*counter)-curr_counter))
        max_counter = ((*counter)-curr_counter);
    curr_counter = *counter;
    
    PRINT_FUNCTION("Max time 2 alt: 0x\x04\n", max_counter);
    
    curr_counter = *counter;
    max_counter = 0;
    
    _generate_unown_info(15, 4, 0xCE, 0xA021, &pid, &ivs, 0x1AA);
    max_counter = 0;
    if(max_counter < ((*counter)-curr_counter))
        max_counter = ((*counter)-curr_counter);
    curr_counter = *counter;
    
    PRINT_FUNCTION("Max time 4: 0x\x04\n", max_counter);
    
    curr_counter = *counter;
    max_counter = 0;
    
    _generate_egg_shiny_info(0, 0x0, 1, 0, &pid, &ivs, 0);
    
    if(max_counter < ((*counter)-curr_counter))
        max_counter = ((*counter)-curr_counter);
    curr_counter = *counter;
    
    PRINT_FUNCTION("Max time 1 s: 0x\x04\n", max_counter);
    
    curr_counter = *counter;
    max_counter = 0;
    
    _generate_static_shiny_info(0, 0x2088, &pid, &ivs, 0x36810000);
    
    if(max_counter < ((*counter)-curr_counter))
        max_counter = ((*counter)-curr_counter);
    curr_counter = *counter;
    
    PRINT_FUNCTION("Max time 2 s: 0x\x04\n", max_counter);
    
    curr_counter = *counter;
    max_counter = 0;
    
    _generate_unown_shiny_info(5, 0x4FF8, 5, &pid, &ivs, 0x8FC00000);
    
    if(max_counter < ((*counter)-curr_counter))
        max_counter = ((*counter)-curr_counter);
    curr_counter = *counter;
    
    PRINT_FUNCTION("Max time 4 s: 0x\x04\n", max_counter);
    
    #endif
}
