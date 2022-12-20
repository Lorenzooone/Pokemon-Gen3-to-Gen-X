#include <gba.h>
#include "party_handler.h"

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
#define EGG_FLAG 0x40000000
#define EGG_NUMBER 412
#define NAME_SIZE 11
#define UNOWN_NUMBER 201
#define UNOWN_B_START 415
#define UNOWN_EX_LETTER 26
#define UNOWN_I_LETTER 8
#define UNOWN_V_LETTER 21
const u8 pokemon_moves_pp_gen1_bin[];
const u8 pokemon_moves_pp_gen2_bin[];
const u8 pokemon_gender_bin[];
const u8 pokemon_names_bin[];
const u8 sprites_cmp_bin[];
const u8 palettes_references_bin[];

u16 get_mon_index(int index, u32 pid, u32 is_egg){
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

u8 gen3_to_gen2(struct gen2_mon* dst, struct gen3_mon* src) {
    u32 decryption[ENC_DATA_SIZE>>2];
    
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
    
    // These Pokemon cannot be both female and shiny in gen 1/2
    if(is_shiny && (gender == F_GENDER) && (gender_kind == MF_7_1_INDEX))
        return 0;
    
    // Unown ! and ? did not exist in gen 2
    // Not only that, but only Unown I and V can be shiny
    if(growth->species == UNOWN_NUMBER) {
        u8 letter = get_unown_letter_gen3(src->pid);
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
            u8 base_pp = pokemon_moves_pp_gen2_bin[attacks->moves[i]];
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
    
    iprintf("Species: %s\n", get_pokemon_name(growth->species, src->pid, misc->ivs & EGG_FLAG));
    load_pokemon_sprite(growth->species, src->pid, misc->ivs & EGG_FLAG);
    
    return 1;
}