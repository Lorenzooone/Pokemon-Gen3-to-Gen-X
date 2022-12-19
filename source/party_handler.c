#include <gba.h>
#include "party_handler.h"

u8 get_unown_letter_gen3(u32);

// Order is G A E M, or M E A G reversed. These are their indexes.
u8 positions[] = {0b11100100, 0b10110100, 0b11011000, 0b10011100, 0b01111000, 0b01101100,
                  0b11100001, 0b10110001, 0b11010010, 0b10010011, 0b01110010, 0b01100011,
                  0b11001001, 0b10001101, 0b11000110, 0b10000111, 0b01001110, 0b01001011,
                  0b00111001, 0b00101101, 0b00110110, 0b00100111, 0b00011110, 0b00011011};

u8 gender_thresholds_gen3[] = {127, 0, 31, 63, 191, 225, 254, 255};
u8 gender_thresholds_gen12[] = {8, 0, 2, 4, 12, 14, 16, 17};

u8 sprite_counter;

#define NAME_SIZE 11
#define UNOWN_NUMBER 201
#define UNOWN_B_START 415
const u8 pokemon_names_bin[];
const u8 sprites_cmp_bin[];
const u8 palettes_references_bin[];

const u8* get_pokemon_name(int index, u32 pid){
    if(index != UNOWN_NUMBER)
        return &(pokemon_names_bin[index*NAME_SIZE]);

    u8 letter = get_unown_letter_gen3(pid);
    if(letter == 0)
        return &(pokemon_names_bin[index*NAME_SIZE]);
    else
        return &(pokemon_names_bin[(UNOWN_B_START+letter-1)*NAME_SIZE]);
}

const u8* get_pokemon_sprite_pointer(int index, u32 pid){
    u16* sprites_cmp_bin_16 = (u16*)sprites_cmp_bin;
    u32 position = sprites_cmp_bin_16[0];
    if(index != UNOWN_NUMBER)
        return &(sprites_cmp_bin[(sprites_cmp_bin_16[1+index]<<2)+position]);

    u8 letter = get_unown_letter_gen3(pid);
    if(letter == 0)
        return &(sprites_cmp_bin[(sprites_cmp_bin_16[1+index]<<2)+position]);
    else
        return &(sprites_cmp_bin[(sprites_cmp_bin_16[UNOWN_B_START+letter]<<2)+position]);
}

u8 get_palette_references(int index, u32 pid){
    if(index != UNOWN_NUMBER)
        return palettes_references_bin[index];

    u8 letter = get_unown_letter_gen3(pid);
    if(letter == 0)
        return palettes_references_bin[index];
    else
        return palettes_references_bin[(UNOWN_B_START+letter-1)];
}

void init_sprite_counter(){
    sprite_counter = 0;
}

u8 get_sprite_counter(){
    return sprite_counter;
}

#define OAM 0x7000000

void load_pokemon_sprite(int index, u32 pid){
    u32 address = get_pokemon_sprite_pointer(index, pid);
    u8 palette = get_palette_references(index, pid);
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
    
    iprintf("Species: %s\n", get_pokemon_name(growth->species, src->pid));
    load_pokemon_sprite(growth->species, src->pid);
    
    return 1;
}