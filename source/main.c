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
    
    sio_normal_prepare_irq_slave(0x02, SIO_8);
    
    while(3) {
        iprintf("0x%X\n", REG_SIODATA8 & 0xFF);
    }
    
    while(2) {
        received = sio_normal(gen2_start_trade_slave_states[index], SIO_SLAVE, SIO_8);
        iprintf("0x%X\n", received);
        if(received == gen2_next_trade_slave_states[index] && index < gen_2_states_max)
            index++;
    }
}

#define FLASH_BANK_CMD *(((vu8*)SRAM)+0x5555) = 0xAA;*(((vu8*)SRAM)+0x2AAA) = 0x55;*(((vu8*)SRAM)+0x5555) = 0xB0;
#define BANK_SIZE 0x10000

#define SAVE_SLOT_SIZE 0xE000
#define SAVE_SLOT_INDEX_POS 0xDFFC
#define SECTION_ID_POS 0xFF4
#define SECTION_SIZE 0x1000
#define SECTION_TOTAL 14
#define SECTION_TRAINER_INFO_ID 0
#define SECTION_PARTY_INFO_ID 1
#define SECTION_GAME_CODE 0xAC
#define FRLG_GAME_CODE 0x1
#define RSE_PARTY 0x234
#define FRLG_PARTY 0x34

#include "party_handler.h"

u8 trainer_name[8];
u8 trainer_gender;
u32 trainer_id;
struct gen3_party parties_3[2];
struct gen2_party parties_2[2];
struct gen1_party parties_1[2];

u8 current_bank;

void init_bank(){
    current_bank = 2;
}

void bank_check(u32 address){
    u8 bank = 0;
    if(address >= BANK_SIZE)
        bank = 1;
    if(bank != current_bank) {
        FLASH_BANK_CMD
        *((vu8*)SRAM) = bank;
        current_bank = bank;
    }
}

u32 read_int(u32 address){
    bank_check(address);
    return (*(vu8*)(SRAM+address)) + ((*(vu8*)(SRAM+address+1)) << 8) + ((*(vu8*)(SRAM+address+2)) << 16) + ((*(vu8*)(SRAM+address+3)) << 24);
}

u16 read_short(u32 address){
    bank_check(address);
    return (*(vu8*)(SRAM+address)) + ((*(vu8*)(SRAM+address+1)) << 8);
}

void copy_to_ram(u32 base_address, u8* new_address, int size){
    bank_check(base_address);
    for(int i = 0; i < size; i++)
        new_address[i] = (*(vu8*)(SRAM+base_address+i));
}

u32 read_slot_index(int slot) {
    if(slot != 0)
        slot = 1;
    
    return read_int((slot * SAVE_SLOT_SIZE) + SAVE_SLOT_INDEX_POS);
}

u32 read_section_id(int slot, int section_pos) {
    if(slot != 0)
        slot = 1;
    if(section_pos < 0)
        section_pos = 0;
    if(section_pos >= SECTION_TOTAL)
        section_pos = SECTION_TOTAL - 1;
    
    return read_short((slot * SAVE_SLOT_SIZE) + (section_pos * SECTION_SIZE) + SECTION_ID_POS);
}

u32 read_game_code(int slot) {
    if(slot != 0)
        slot = 1;
    
    u32 game_code = -1;

    for(int i = 0; i < SECTION_TOTAL; i++)
        if(read_section_id(slot, i) == SECTION_TRAINER_INFO_ID) {
            copy_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE), &trainer_name[0], 8);
            copy_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + 8, &trainer_gender, 1);
            copy_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + 10, (u8*)&trainer_id, 4);
            game_code = read_int((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + SECTION_GAME_CODE);
            break;
        }
    
    return game_code;
}

void read_party(int slot) {
    if(slot != 0)
        slot = 1;
    
    u32 game_code = read_game_code(slot);
    u16 add_on = RSE_PARTY;
    if(game_code == FRLG_GAME_CODE)
        add_on = FRLG_PARTY;

    for(int i = 0; i < SECTION_TOTAL; i++)
        if(read_section_id(slot, i) == SECTION_PARTY_INFO_ID) {
            copy_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + add_on, (u8*)(&parties_3[0]), sizeof(struct gen3_party));
            break;
        }
    iprintf("Party size: %d\n", parties_3[0].total);
    for(int i = 0; i < parties_3[0].total; i++)
        gen3_to_gen2(&parties_2[0].mons[0], &parties_3[0].mons[i]);
}

#define MAGIC_NUMBER 0x08012025
#define INVALID_SLOT 2
u16 summable_bytes[SECTION_TOTAL] = {3884, 3968, 3968, 3968, 3848, 3968, 3968, 3968, 3968, 3968, 3968, 3968, 3968, 2000};

u8 validate_slot(int slot) {
    if(slot != 0)
        slot = 1;
    
    u16 valid_sections = 0;
    u32 buffer[0x400];
    u16* buffer_16 = (u16*)buffer;
    u32 current_save_index = read_slot_index(slot);
    
    for(int i = 0; i < SECTION_TOTAL; i++)
    {
        copy_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE), (u8*)buffer, SECTION_SIZE);
        if(buffer[0x3FE] != MAGIC_NUMBER)
            return 0;
        if(buffer[0x3FF] != current_save_index)
            return 0;
        u16 section_id = buffer_16[0x7FA];
        if(section_id >= SECTION_TOTAL)
            return 0;
        u16 prepared_checksum = buffer_16[0x7FB];
        u32 checksum = 0;
        for(int j = 0; j < (summable_bytes[section_id] >> 2); j++)
            checksum += buffer[j];
        checksum = ((checksum & 0xFFFF) + (checksum >> 16)) & 0xFFFF;
        if(checksum != prepared_checksum)
            return 0;
        valid_sections |= (1 << section_id);
    }
    
    if(valid_sections != (1 << (SECTION_TOTAL))-1)
        return 0;

    return 1;
}

u8 get_slot(){
    u32 last_valid_save_index = 0;
    u8 slot = INVALID_SLOT;

    for(int i = 0; i < 2; i++) {
        u32 current_save_index = read_slot_index(i);
        if((current_save_index >= last_valid_save_index) && validate_slot(i)) {
            slot = i;
            last_valid_save_index = current_save_index;
        }
    }
    return slot;
}

void read_gen_3_data(){
    REG_WAITCNT |= 3;
    init_bank();
    
    u8 slot = get_slot();
    if(slot == INVALID_SLOT) {
        iprintf("Error reading the data!\n");
        return;
    }
    
    read_party(slot);
        
}

const u8 sprite_palettes_bin[];
const u32 sprite_palettes_bin_size;

void init_oam_palette(){
    u16* sprite_palettes_bin_16 = (u16*)sprite_palettes_bin;
    for(int i = 0; i < (sprite_palettes_bin_size>>1); i++)
        SPRITE_PALETTE[i] = sprite_palettes_bin_16[i];
}

#define OAM 0x7000000

int main(void)
{
    int val = 0;
    u16 keys;
    enum MULTIBOOT_RESULTS result;
    
    init_oam_palette();
    init_sprite_counter();
    irqInit();
    irqEnable(IRQ_VBLANK);
    irqSet(IRQ_SERIAL, sio_handle_irq_slave);
    irqEnable(IRQ_SERIAL);

    consoleDemoInit();
    REG_DISPCNT |= OBJ_ON | OBJ_1D_MAP;
    
    iprintf("\x1b[2J");
    
    read_gen_3_data();
    
    u8 curr_frame = 0;
    while(1){
        VBlankIntrWait();
        curr_frame++;
        if(curr_frame == 8) {
            for(int i = 0; i < get_sprite_counter(); i++) {
                u16 obj_data_2 = *((u16*)(OAM+4+(i*8)));
                if(obj_data_2 & 0x10)
                    obj_data_2 &= ~0x10;
                else
                    obj_data_2 |= 0x10;
                *((u16*)(OAM+4+(i*8))) = obj_data_2;
            }
            curr_frame = 0;
        }
    }
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
