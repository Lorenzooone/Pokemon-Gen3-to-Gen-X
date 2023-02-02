#ifndef FAST_POKEMON_METHODS__
#define FAST_POKEMON_METHODS__

#include "optimized_swi.h"
#include "party_handler.h"

static __attribute__((optimize(3), always_inline)) inline u8 get_nature_fast(u32 pid){
    // Make use of modulo properties to get this to positives
    while(pid >= 0x80000000)
        pid -= 0x7FFFFFE9;
    return SWI_DivMod(pid, NUM_NATURES);
}

static __attribute__((optimize(3), always_inline)) inline u8 get_unown_letter_gen3_fast(u32 pid){
    return SWI_DivMod((pid & 3) + (((pid >> 8) & 3) << 2) + (((pid >> 16) & 3) << 4) + (((pid >> 24) & 3) << 6), NUM_UNOWN_LETTERS_GEN3);
}

static __attribute__((optimize(3), always_inline)) inline u8 get_unown_letter_gen2_fast(u16 ivs){
    u8 atk_ivs = ((ivs>>4) & 0xF);
    u8 def_ivs = ((ivs) & 0xF);
    u8 spe_ivs = ((ivs>>12) & 0xF);
    u8 spa_ivs = ((ivs>>8) & 0xF);
    return SWI_Div(((((atk_ivs>>1)&3)<<6) | (((def_ivs>>1)&3)<<4) | (((spe_ivs>>1)&3)<<2) | (((spa_ivs>>1)&3)<<0)), 10);
}

#endif
