#include <gba.h>
#include "save.h"
#include <stddef.h>

#define REG_WAITCNT		*(vu16*)(REG_BASE + 0x204)  // Wait state Control

#define FLASH_WRITE_CMD *(flash_access+0x5555) = 0xAA;*(flash_access+0x2AAA) = 0x55;*(flash_access+0x5555) = 0xA0;
#define FLASH_BANK_CMD *(flash_access+0x5555) = 0xAA;*(flash_access+0x2AAA) = 0x55;*(flash_access+0x5555) = 0xB0;
#define timeout 0x1000
#define BANK_SIZE 0x10000

#define IS_FLASH 1
#define SAVE_POS SRAM

uintptr_t bank_check(uintptr_t);
IWRAM_CODE int is_sram_correct(u8*, size_t);
IWRAM_CODE void sram_write(u8*, size_t);
void flash_write(u8*, size_t, int);
int is_flash_correct(u8*, size_t, int);

u8 current_bank;

void init_bank(){
    REG_WAITCNT |= 3;
    current_bank = 2;
}

uintptr_t bank_check(uintptr_t address){
    #if IS_FLASH
    vu8* flash_access = (vu8*)SAVE_POS;
    u8 bank = 0;
    if(address >= BANK_SIZE) {
        bank = 1;
        address %= BANK_SIZE;
    }
    if(bank != current_bank) {
        FLASH_BANK_CMD
        *((vu8*)SAVE_POS) = bank;
        current_bank = bank;
    }
    #endif
    return address;
}

u32 read_int_save(uintptr_t address){
    address = bank_check(address);
    return (*(vu8*)(SAVE_POS+address)) + ((*(vu8*)(SAVE_POS+address+1)) << 8) + ((*(vu8*)(SAVE_POS+address+2)) << 16) + ((*(vu8*)(SAVE_POS+address+3)) << 24);
}

u16 read_short_save(uintptr_t address){
    address = bank_check(address);
    return (*(vu8*)(SAVE_POS+address)) + ((*(vu8*)(SAVE_POS+address+1)) << 8);
}

u8 read_byte_save(uintptr_t address){
    address = bank_check(address);
    return *(vu8*)(SAVE_POS+address);
}

void copy_save_to_ram(uintptr_t base_address, u8* new_address, size_t size){
    base_address = bank_check(base_address);
    for(size_t i = 0; i < size; i++)
        new_address[i] = (*(vu8*)(SAVE_POS+base_address+i));
}

IWRAM_CODE void sram_write(u8* data, size_t size) {
    vu8* sram_access = (vu8*)SAVE_POS;
    for(size_t i = 0; i < size; i++)
        *(sram_access+i) = data[i];
}

IWRAM_CODE int is_sram_correct(u8* data, size_t size) {
    vu8* sram_access = (vu8*)SAVE_POS;
    for(size_t i = 0; i < size; i++)
        if (*(sram_access+i) != data[i])
            return 0;
    if(size > BANK_SIZE)
        return 0;
    return 1;
}

void flash_write(u8* data, size_t size, int has_banks) {
    vu8* flash_access = (vu8*)SAVE_POS;
    int base_difference;
    if(has_banks) {
        FLASH_BANK_CMD
        *(flash_access) = 0;
        base_difference = 0;
    }
    for(size_t i = 0; i < size; i++) {
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

int is_flash_correct(u8* data, size_t size, int has_banks) {
    vu8* flash_access = (vu8*)SAVE_POS;
    int base_difference;
    if(has_banks) {
        FLASH_BANK_CMD
        *(flash_access) = 0;
        base_difference = 0;
    }
    for(size_t i = 0; i < size; i++) {
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
