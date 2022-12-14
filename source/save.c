#include <gba.h>
#include "save.h"

#define FLASH_WRITE_CMD *(flash_access+0x5555) = 0xAA;*(flash_access+0x2AAA) = 0x55;*(flash_access+0x5555) = 0xA0;
#define FLASH_BANK_CMD *(flash_access+0x5555) = 0xAA;*(flash_access+0x2AAA) = 0x55;*(flash_access+0x5555) = 0xB0;
#define timeout 0x1000
#define BANK_SIZE 0x10000

IWRAM_CODE void sram_write(u8* data, int size) {
    vu8* sram_access = (vu8*)SRAM;
    for(int i = 0; i < size; i++)
        *(sram_access+i) = data[i];
}

IWRAM_CODE int is_sram_correct(u8* data, int size) {
    vu8* sram_access = (vu8*)SRAM;
    for(int i = 0; i < size; i++)
        if (*(sram_access+i) != data[i])
            return 0;
    if(size > BANK_SIZE)
        return 0;
    return 1;
}

void flash_write(u8* data, int size, int has_banks) {
    vu8* flash_access = (vu8*)SRAM;
    int base_difference;
    if(has_banks) {
        FLASH_BANK_CMD
        *(flash_access) = 0;
        base_difference = 0;
    }
    for(int i = 0; i < size; i++) {
        if((i == BANK_SIZE) && has_banks) {
            FLASH_BANK_CMD
            *(flash_access) = 1;
            base_difference = BANK_SIZE;
        }
        FLASH_WRITE_CMD
        *(flash_access+i-base_difference) = data[i];
        for(int j = 0; (j < timeout) && (*(flash_access+i-base_difference) != data[i]); j++);
    }
    if(has_banks) {
        FLASH_BANK_CMD
        *(flash_access) = 0;
        base_difference = 0;
    }
}

int is_flash_correct(u8* data, int size, int has_banks) {
    vu8* flash_access = (vu8*)SRAM;
    int base_difference;
    if(has_banks) {
        FLASH_BANK_CMD
        *(flash_access) = 0;
        base_difference = 0;
    }
    for(int i = 0; i < size; i++) {
        if((i == BANK_SIZE) && has_banks) {
            FLASH_BANK_CMD
            *(flash_access) = 1;
            base_difference = BANK_SIZE;
        }
        if (*(flash_access+i-base_difference) != data[i]) {
            if(has_banks) {
                FLASH_BANK_CMD
                *(flash_access) = 0;
                base_difference = 0;
            }
            return 0;
        }
    }
    if(has_banks) {
        FLASH_BANK_CMD
        *(flash_access) = 0;
        base_difference = 0;
        if(size > (2*BANK_SIZE))
            return 0;
    }
    else if(size > BANK_SIZE)
        return 0;
    return 1;
}

void rom_write(u8* data, int size) {
    vu8* free_section_ptr = (vu8*)free_section;
    for(int i = 0; i < size; i++)
        *(free_section_ptr+i) = data[i];
}

unsigned int get_rom_address() {
    return (unsigned int)free_section;
}

int is_rom_correct(u8* data, int size) {
    vu8* free_section_ptr = (vu8*)free_section;
    for(int i = 0; i < size; i++)
        if (*(free_section_ptr+i) != data[i])
            return 0;
    if(size > (2*BANK_SIZE))
        return 0;
    return 1;
}