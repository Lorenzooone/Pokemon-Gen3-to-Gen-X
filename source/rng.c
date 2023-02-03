#include <gba.h>
#include "rng.h"
#include "useful_qualifiers.h"

#define FACTOR_0 0x4C957F2D
#define FACTOR_1 0x5851F42D
#define INCREMENT_0 1
#define INCREMENT_1 0

static void u64_add(u32*, u32*, u32, u32);
void u64_mul(u32*, u32*, u32, u32);

static u32 curr_seed_0;
static u32 curr_seed_1;
static u8 advances_enabled;

ALWAYS_INLINE MAX_OPTIMIZE void u64_add(u32* src0_0_p, u32* src0_1_p, u32 src1_0, u32 src1_1)
{
    u32 src0_0 = *src0_0_p;
    u32 src0_1 = *src0_1_p;
    
    *src0_0_p = src0_0 + src1_0;
    *src0_1_p = src0_1 + src1_1;
    
    if ((*src0_0_p) < src0_0 || (*src0_0_p) < src1_0) {
        (*src0_1_p)++;
    }
}

IWRAM_CODE ARM_TARGET MAX_OPTIMIZE void u64_mul(u32* src0_0_p, u32* src0_1_p, u32 src1_0, u32 src1_1)
{
    register uint32_t src0_0_ asm("r0") = (uint32_t)(*src0_0_p);
    register uint32_t src0_1_ asm("r1") = (uint32_t)(*src0_1_p);
    register uint32_t src1_0_ asm("r2") = (uint32_t)(src1_0);
    register uint32_t src1_1_ asm("r3") = (uint32_t)(src1_1);
    
    asm volatile(
    "mul r3,r0,r3;"
    "mla r3,r1,r2,r3;"
    "umull r0,r1,r2,r0;"
    "add r1,r1,r3;" :
    "=r"(src0_0_), "=r"(src0_1_) :
    "r"(src0_0_), "r"(src0_1_), "r"(src1_0_), "r"(src1_1_) :
    );
    
    *src0_0_p = src0_0_;
    *src0_1_p = src0_1_;
}

void init_rng(u32 val_0, u32 val_1) {
    curr_seed_0 = val_0;
    curr_seed_1 = val_1;
    enable_advances();
}

void disable_advances() {
    advances_enabled = 0;
}

void enable_advances() {
    advances_enabled = 1;
}

void increase_rng(u32 increaser_0, u32 increaser_1) {
    disable_advances();
        u64_add(&curr_seed_0, &curr_seed_1, increaser_0, increaser_1);
    enable_advances();
}

u32 get_rng() {
    // Link cable timings SHOULD give enough Randomness...
    disable_advances();
        u64_mul(&curr_seed_0, &curr_seed_1, FACTOR_0, FACTOR_1);
        if((INCREMENT_0 == 1) && (INCREMENT_1 == 0)) {
            curr_seed_0 += 1;
            if(!curr_seed_0)
                curr_seed_1 += 1;
        }
        else
            u64_add(&curr_seed_0, &curr_seed_1, INCREMENT_0, INCREMENT_1);
    enable_advances();
    return curr_seed_1;
}

void advance_rng() {
    if(advances_enabled)
        get_rng();
}
