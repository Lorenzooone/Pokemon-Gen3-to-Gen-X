#ifndef SAVE__
#define SAVE__

// For SRAM-based carts
IWRAM_CODE int is_sram_correct(u8* data, int size);
IWRAM_CODE void sram_write(u8* data, int size);

// For flash-ROM based carts
void flash_write(u8* data, int size, int has_banks);
int is_flash_correct(u8* data, int size, int has_banks);

u32 read_int_save(u32);
u16 read_short_save(u32);
void copy_save_to_ram(u32, u8*, int);
void init_bank(void);

#endif
