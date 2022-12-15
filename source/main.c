// SPDX-License-Identifier: MIT
//
// Copyright (c) 2020 Antonio Niño Díaz (AntonioND)

#include <stdio.h>
#include <string.h>

#include <gba.h>

#include "multiboot_handler.h"
#include "gb_dump_receiver.h"
//#include "save.h"

// --------------------------------------------------------------------

#define PACKED __attribute__((packed))
#define ALWAYS_INLINE __attribute__((always_inline)) static inline
#define MAX_DUMP_SIZE 0x20000
#define REG_WAITCNT		*(vu16*)(REG_BASE + 0x204)  // Wait state Control
#define SAVE_TYPES 4
#define SRAM_SAVE_TYPE 0
#define FLASH_64_SAVE_TYPE 1
#define FLASH_128_SAVE_TYPE 2
#define ROM_SAVE_TYPE 3

// --------------------------------------------------------------------

const char* save_strings[] = {"SRAM 64KiB", "Flash 64 KiB", "Flash 128 KiB", "Inside ROM"};

// void save_to_memory(int size) {
    // int chosen_save_type = 0;
    // u8 copied_properly = 0;
    // int decided = 0, completed = 0;
    // u16 keys;
    // REG_WAITCNT |= 3;
    
    // while(!completed) {
        // while(!decided) {
            // iprintf("\x1b[2J");
            // iprintf("Save type: %s\n\n", save_strings[chosen_save_type]);
            // iprintf("LEFT/RIGHT: Change save type\n\n");
            // iprintf("START: Save\n\n");
        
            // scanKeys();
            // keys = keysDown();
            
            // while ((!(keys & KEY_START)) && (!(keys & KEY_LEFT)) && (!(keys & KEY_RIGHT))) {
                // VBlankIntrWait();
                // scanKeys();
                // keys = keysDown();
            // }
            
            // if(keys & KEY_START)
                // decided = 1;
            // else if(keys & KEY_LEFT)
                // chosen_save_type -= 1;
            // else if(keys & KEY_RIGHT)
                // chosen_save_type += 1;
            
            // if(chosen_save_type < 0)
                // chosen_save_type = SAVE_TYPES - 1;
            // if(chosen_save_type >= SAVE_TYPES)
                // chosen_save_type = 0;
        // }
        
        // iprintf("\x1b[2J");
        
        // switch (chosen_save_type) {
            // case SRAM_SAVE_TYPE:
                // sram_write((u8*)EWRAM, size);
                // copied_properly = is_sram_correct((u8*)EWRAM, size);
                // break;
            // case FLASH_64_SAVE_TYPE:
                // flash_write((u8*)EWRAM, size, 0);
                // copied_properly = is_flash_correct((u8*)EWRAM, size, 0);
                // break;
            // case FLASH_128_SAVE_TYPE:
                // flash_write((u8*)EWRAM, size, 1);
                // copied_properly = is_flash_correct((u8*)EWRAM, size, 1);
                // break;
            // case ROM_SAVE_TYPE:
                // rom_write((u8*)EWRAM, size);
                // copied_properly = is_rom_correct((u8*)EWRAM, size);
            // default:
                // break;
        // }
        
        // if(copied_properly) {
            // iprintf("All went well!\n\n");
            // completed = 1;
        // }
        // else {
            // iprintf("Not everything went right!\n\n");
            // iprintf("START: Try saving again\n\n");
            // iprintf("SELECT: Abort\n\n");
        
            // scanKeys();
            // keys = keysDown();
            
            // while ((!(keys & KEY_START)) && (!(keys & KEY_SELECT))) {
                // VBlankIntrWait();
                // scanKeys();
                // keys = keysDown();
            // }
            
            // if(keys & KEY_SELECT) {
                // completed = 1;
                // iprintf("\x1b[2J");
            // }
            // else
                // decided = 0;
        // }
    // }
    
    // if(chosen_save_type == ROM_SAVE_TYPE)
        // iprintf("Save location: 0x%X\n\n", get_rom_address()-0x8000000);
// }

#include "sio.h"
#define VCOUNT_TIMEOUT 28 * 8

const u8 gen2_start_trade_states[] = {0x01, 0xFE, 0x61, 0xD1, 0xFE};
const u8 gen2_next_trade_states[] = {0xFE, 0x61, 0xD1, 0xFE, 0xFE};
const u8 gen_2_states_max = 4;
void start_gen2(void)
{
    u8 index = 0;
    u8 received;
    init_sio_normal(SIO_MASTER, SIO_8);
    
    while(2) {
        received = timed_sio_normal_master(gen2_start_trade_states[index], SIO_8, VCOUNT_TIMEOUT);
        if(received == gen2_next_trade_states[index] && index < gen_2_states_max)
            index++;
        else
            iprintf("0x%X\n", received);
    }
}
const u8 gen2_start_trade_slave_states[] = {0x02, 0x61, 0xD1, 0x00, 0xFE};
const u8 gen2_next_trade_slave_states[] = {0x01, 0x61, 0xD1, 0x00, 0xFE};
void start_gen2_slave(void)
{
    u8 index = 0;
    u8 received;
    init_sio_normal(SIO_SLAVE, SIO_8);
    
    while(2) {
        received = sio_normal(gen2_start_trade_slave_states[index], SIO_SLAVE, SIO_8);
        iprintf("0x%X\n", received);
        if(received == gen2_next_trade_slave_states[index] && index < gen_2_states_max)
            index++;
    }
}

int main(void)
{
    int val = 0;
    u16 keys;
    enum MULTIBOOT_RESULTS result;
    
    irqInit();
    irqEnable(IRQ_VBLANK);

    consoleDemoInit();
    
    iprintf("\x1b[2J");
    
    start_gen2_slave();
    start_gen2();
    
    // while (1) {
        // iprintf("START: Send the dumper\n\n");
        // iprintf("SELECT: Dump\n\n");
    
        // scanKeys();
        // keys = keysDown();
        
        // while ((!(keys & KEY_START)) && (!(keys & KEY_SELECT))) {
            // VBlankIntrWait();
            // scanKeys();
            // keys = keysDown();
        // }
        
        // if(keys & KEY_START) {        
            // result = multiboot_normal((u16*)payload, (u16*)(payload + PAYLOAD_SIZE));
            
            // iprintf("\x1b[2J");
            
            // if(result == MB_SUCCESS)
                // iprintf("Multiboot successful!\n\n");
            // else if(result == MB_NO_INIT_SYNC)
                // iprintf("Couldn't sync.\nTry again!\n\n");
            // else
                // iprintf("There was an error.\nTry again!\n\n");
        // }
        // else {
            // val = read_dump(MAX_DUMP_SIZE);
            
            // iprintf("\x1b[2J");
            
            // if(val == GENERIC_DUMP_ERROR)
                // iprintf("There was an error!\nTry again!\n\n");
            // else if(val == SIZE_DUMP_ERROR)
                // iprintf("Dump size is too great!\n\n");
            // else {
                // save_to_memory(val);
            // }
        // }
    // }

    return 0;
}
