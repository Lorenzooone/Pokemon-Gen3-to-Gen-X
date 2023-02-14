#ifndef SAVE__
#define SAVE__

#include <stddef.h>

u32 read_int_save(uintptr_t);
u16 read_short_save(uintptr_t);
u8 read_byte_save(uintptr_t);
void copy_save_to_ram(uintptr_t, u8*, size_t);
void write_int_save(uintptr_t, u32);
void write_short_save(uintptr_t, u16);
void write_byte_save(uintptr_t, u8);
void copy_ram_to_save(u8*, uintptr_t, size_t);
u8 is_save_correct(u8*, uintptr_t, size_t);
void init_bank(void);

#endif
