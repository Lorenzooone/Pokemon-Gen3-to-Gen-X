#ifndef SAVE__
#define SAVE__

#include <stddef.h>

u32 read_int_save(uintptr_t);
u16 read_short_save(uintptr_t);
u8 read_byte_save(uintptr_t);
void copy_save_to_ram(uintptr_t, u8*, size_t);
void init_bank(void);

#endif
