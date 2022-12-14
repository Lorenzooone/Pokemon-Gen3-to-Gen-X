#ifndef SAVE__
#define SAVE__

// For SRAM-based carts
IWRAM_CODE int is_sram_correct(u8* data, int size);
IWRAM_CODE void sram_write(u8* data, int size);

// For flash-ROM based carts
void flash_write(u8* data, int size, int has_banks);
int is_flash_correct(u8* data, int size, int has_banks);

// For the repro carts with a writable ROM
int is_rom_correct(u8* data, int size);
void rom_write(u8* data, int size);
unsigned int get_rom_address(void);

#endif