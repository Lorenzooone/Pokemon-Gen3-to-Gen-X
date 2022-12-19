#include <gba.h>
#include "party_handler.h"

// Order is G A E M, or M E A G reversed. These are their indexes.
u8 positions[] = {0b11100100, 0b10110100, 0b11011000, 0b10011100, 0b01111000, 0b01101100,
                  0b11100001, 0b10110001, 0b11010010, 0b10010011, 0b01110010, 0b01100011,
                  0b11001001, 0b10001101, 0b11000110, 0b10000111, 0b01001110, 0b01001011,
                  0b00111001, 0b00101101, 0b00110110, 0b00100111, 0b00011110, 0b00011011};

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
    s32 index = DivMod(src->pid, 24);
    if(index < 0)
        index += 24;
    struct gen3_mon_growth* growth = (struct gen3_mon_growth*)&(decryption[3*((positions[index] >> 0)&3)]);
    struct gen3_mon_attacks* attacks = (struct gen3_mon_attacks*)&(decryption[3*((positions[index] >> 2)&3)]);
    struct gen3_mon_evs* evs = (struct gen3_mon_evs*)&(decryption[3*((positions[index] >> 4)&3)]);
    struct gen3_mon_misc* misc = (struct gen3_mon_misc*)&(decryption[3*((positions[index] >> 6)&3)]);
    
    iprintf("Species: %d\n", growth->species);
    
    return 1;
}