#ifndef RNG__
#define RNG__

void init_rng(u32, u32);
void disable_advances(void);
void enable_advances(void);
u32 get_rng(void);
void advance_rng(void);
void increase_rng(u32, u32);

#endif
