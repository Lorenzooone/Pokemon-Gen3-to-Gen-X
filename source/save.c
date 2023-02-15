#include <gba.h>
#include "save.h"
#include <stddef.h>
#include "useful_qualifiers.h"

#define REG_WAITCNT		*(vu16*)(REG_BASE + 0x204)  // Wait state Control

#define IS_FLASH 1
#define SAVE_POS SRAM

#define BASE_FLASH_CMD *((vu8*)(SAVE_POS+0x5555)) = 0xAA; *((vu8*)(SAVE_POS+0x2AAA)) = 0x55;
#define FLASH_WRITE_CMD BASE_FLASH_CMD *((vu8*)(SAVE_POS+0x5555)) = 0xA0;
#define FLASH_BANK_CMD BASE_FLASH_CMD *((vu8*)(SAVE_POS+0x5555)) = 0xB0;
#define FLASH_ENTER_MAN_CMD BASE_FLASH_CMD *((vu8*)(SAVE_POS+0x5555)) = 0x90;
#define FLASH_EXIT_MAN_CMD BASE_FLASH_CMD *((vu8*)(SAVE_POS+0x5555)) = 0xF0;
#define FLASH_ERASE_SECTOR_BASE_CMD BASE_FLASH_CMD *((vu8*)(SAVE_POS+0x5555)) = 0x80; BASE_FLASH_CMD
#define FLASH_TERM_CMD *((vu8*)(SAVE_POS+0x5555)) = 0xF0;
#define TIMEOUT 20000
#define ERASE_TIMEOUT (TIMEOUT)
#define ERASED_BYTE 0xFF
#define BANK_SIZE 0x10000
#define NUM_BANKS 2

#define MACRONIX_MAN_ID 0xC2
#define ERASE_SECTOR_FINAL_CMD 0x30

#if IS_FLASH
#define BANK_LIMIT BANK_SIZE
#else
#define BANK_LIMIT (NUM_BANKS * BANK_SIZE)
#endif

uintptr_t bank_check(uintptr_t);
static u8 read_direct_single_byte_save(uintptr_t);
static void write_direct_single_byte_save(uintptr_t, u8);

u8 current_bank;
u8 is_macronix;

void init_bank(){
    REG_WAITCNT |= 3;
    current_bank = NUM_BANKS;
    is_macronix = 0;
    #if IS_FLASH
    FLASH_ENTER_MAN_CMD
    for(vu32 j = 0; j < TIMEOUT; j++);
    u8 man_id = *((vu8*)SAVE_POS);
    if(man_id == MACRONIX_MAN_ID)
        is_macronix = 1;
    FLASH_EXIT_MAN_CMD
    for(vu32 j = 0; j < TIMEOUT; j++);
    #endif
}

uintptr_t bank_check(uintptr_t address){
    address %= (NUM_BANKS * BANK_SIZE);
    #if IS_FLASH
    u8 bank = address / BANK_SIZE;
    address %= BANK_SIZE;

    if(bank != current_bank) {
        FLASH_BANK_CMD
        *((vu8*)SAVE_POS) = bank;
        current_bank = bank;
    }
    #endif
    return address;
}

void erase_sector(uintptr_t address) {
    address = bank_check(address);
    address >>= SECTOR_SIZE_BITS;
    address <<= SECTOR_SIZE_BITS;
    u8 failed = 1;
    vu8* save_data = (vu8*)SAVE_POS;
    for(int i = 0; failed && (i < 3); i++) {
        #if IS_FLASH
        FLASH_ERASE_SECTOR_BASE_CMD
        save_data[address] = ERASE_SECTOR_FINAL_CMD;
        #else
        for(size_t j = 0; j < SECTOR_SIZE; j++)
            save_data[address+(SECTOR_SIZE-1)-j] = ERASED_BYTE;
        #endif
        
        for(vu32 j = 0; j < ERASE_TIMEOUT; j++);

        failed = 0;
        for(size_t j = 0; j < SECTOR_SIZE; j++)
            if(read_direct_single_byte_save(address+j) != ERASED_BYTE) {
                failed = 1;
                break;
            }
       if(is_macronix && failed)
            FLASH_TERM_CMD
    }
}

ALWAYS_INLINE u8 read_direct_single_byte_save(uintptr_t address) {
    return *(vu8*)(SAVE_POS+address);
}

ALWAYS_INLINE void write_direct_single_byte_save(uintptr_t address, u8 data) {
    vu8* save_data = (vu8*)SAVE_POS;
    for(int i = 0; (i < 3) && save_data[address] != data; i++) {
        #if IS_FLASH
        FLASH_WRITE_CMD
        #endif
        save_data[address] = data;
        for(vu32 j = 0; (j < TIMEOUT) && (save_data[address] != data); j++);
        if(is_macronix && (save_data[address] != data))
            FLASH_TERM_CMD
    }
}

u8 read_byte_save(uintptr_t address){
    address = bank_check(address);
    return read_direct_single_byte_save(address);
}

void write_byte_save(uintptr_t address, u8 data){
    address = bank_check(address);
    write_direct_single_byte_save(address, data);
}

u32 read_int_save(uintptr_t address){
    address = bank_check(address);
    u32 data_out = 0;
    if((address + sizeof(u32)) > BANK_LIMIT) {
        for(size_t i = 0; i < sizeof(u32); i++)
            data_out += read_byte_save(address + i) << (i*8);
    }
    else {
        for(size_t i = 0; i < sizeof(u32); i++)
            data_out += read_direct_single_byte_save(address + i) << (i*8);
    }
    return data_out;
}

u16 read_short_save(uintptr_t address){
    address = bank_check(address);
    u16 data_out = 0;
    if((address + sizeof(u16)) > BANK_LIMIT) {
        for(size_t i = 0; i < sizeof(u16); i++)
            data_out += read_byte_save(address + i) << (i*8);
    }
    else {
        for(size_t i = 0; i < sizeof(u16); i++)
            data_out += read_direct_single_byte_save(address + i) << (i*8);
    }
    return data_out;
}

void write_int_save(uintptr_t address, u32 data){
    address = bank_check(address);
    if((address + sizeof(u32)) > BANK_LIMIT) {
        for(size_t i = 0; i < sizeof(u32); i++)
            write_byte_save(address + i, (data >> (i*8)) & 0xFF);
    }
    else {
        for(size_t i = 0; i < sizeof(u32); i++)
            write_direct_single_byte_save(address + i, (data >> (i*8)) & 0xFF);
    }
}

void write_short_save(uintptr_t address, u16 data){
    address = bank_check(address);
    if((address + sizeof(u16)) > BANK_LIMIT) {
        for(size_t i = 0; i < sizeof(u16); i++)
            write_byte_save(address + i, (data >> (i*8)) & 0xFF);
    }
    else {
        for(size_t i = 0; i < sizeof(u16); i++)
            write_direct_single_byte_save(address + i, (data >> (i*8)) & 0xFF);
    }
}

void copy_save_to_ram(uintptr_t base_address, u8* new_address, size_t size){
    base_address = bank_check(base_address);
    if((base_address + size) > BANK_LIMIT) {
        for(size_t i = 0; i < size; i++)
            new_address[i] = read_byte_save(base_address + i);
    }
    else {
        for(size_t i = 0; i < size; i++)
            new_address[i] = read_direct_single_byte_save(base_address + i);
    }
}

void copy_ram_to_save(u8* base_address, uintptr_t save_address, size_t size){
    save_address = bank_check(save_address);
    if((save_address + size) > BANK_LIMIT) {
        for(size_t i = 0; i < size; i++)
            write_byte_save(save_address + i, base_address[i]);
    }
    else {
        for(size_t i = 0; i < size; i++)
            write_direct_single_byte_save(save_address + i, base_address[i]);
    }
}

u8 is_save_correct(u8* base_address, uintptr_t save_address, size_t size) {
    save_address = bank_check(save_address);
    if((save_address + size) > BANK_LIMIT) {
        for(size_t i = 0; i < size; i++)
            if(read_byte_save(save_address + i) != base_address[i])
                return 0;
    }
    else {
        for(size_t i = 0; i < size; i++)
            if(read_direct_single_byte_save(save_address + i) != base_address[i])
                return 0;
    }
    return 1;
}
