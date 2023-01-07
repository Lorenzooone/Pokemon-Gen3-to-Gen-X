#ifndef RNG__
#define RNG__

#include <gba.h>

void init_rng(u32, u32);
void disable_advances();
void enable_advances();
u32 get_rng();
void advance_rng();
void increase_rng(u32, u32);

#endif