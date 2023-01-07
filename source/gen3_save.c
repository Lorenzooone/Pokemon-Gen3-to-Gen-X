#include <gba.h>
#include "save.h"
#include "gen3_save.h"
#include "text_handler.h"

#define MAGIC_NUMBER 0x08012025
#define INVALID_SLOT 2
#define SAVE_SLOT_INDEX_POS 0xDFFC
#define SECTION_ID_POS 0xFF4
#define CHECKSUM_POS 0xFF6
#define MAGIC_NUMBER_POS 0xFF8
#define SAVE_NUMBER_POS 0xFFC
#define SECTION_TOTAL 14

#define SECTION_TRAINER_INFO_ID 0
#define SECTION_PARTY_INFO_ID 1
#define SECTION_DEX_SEEN_1_ID 1
#define SECTION_DEX_SEEN_2_ID 4
#define SECTION_MAIL_ID 3
#define SECTION_GIFT_RIBBON_ID 4

#define DEX_POS_0 0x28
#define RSE_PARTY 0x234
#define FRLG_PARTY 0x34

#define VALID_MAPS 36

const u16 summable_bytes[SECTION_TOTAL] = {3884, 3968, 3968, 3968, 3848, 3968, 3968, 3968, 3968, 3968, 3968, 3968, 3968, 2000};
const u16 pokedex_extra_pos_1 [] = {0x938, 0x5F8, 0x988};
const u16 pokedex_extra_pos_2 [] = {0xC0C, 0xB98, 0xCA4};
const u16 mail_pos [] = {0xC4C, 0xDD0, 0xCE0};
const u16 ribbon_pos [] = {0x290, 0x21C, 0x328};
const u16 special_area [] = {0, 9, 9};
const u16 rs_valid_maps[VALID_MAPS] = {0x0202, 0x0203, 0x0301, 0x0302, 0x0405, 0x0406, 0x0503, 0x0504, 0x0603, 0x0604, 0x0700, 0x0701, 0x0804, 0x0805, 0x090a, 0x090b, 0x0a05, 0x0a06, 0x0b05, 0x0b06, 0x0c02, 0x0c03, 0x0d06, 0x0d07, 0x0e03, 0x0e04, 0x0f02, 0x0f03, 0x100c, 0x100d, 0x100a, 0x1918, 0x1919, 0x191a, 0x191b, 0x1a05};

void init_game_data(struct game_data_t* game_data) {
    init_game_identifier(&game_data->game_identifier);
    
    game_data->party_1.total = 0;
    game_data->party_2.total = 0;
    game_data->party_3.total = 0;
    
    for(int i = 0; i < (OT_NAME_GEN3_SIZE+1); i++)
        game_data->trainer_name[i] = GEN3_EOL;
}

u32 read_section_id(int slot, int section_pos) {
    if(slot != 0)
        slot = 1;
    if(section_pos < 0)
        section_pos = 0;
    if(section_pos >= SECTION_TOTAL)
        section_pos = SECTION_TOTAL - 1;
    
    return read_short_save((slot * SAVE_SLOT_SIZE) + (section_pos * SECTION_SIZE) + SECTION_ID_POS);
}

void read_game_data_trainer_info(int slot, struct game_data_t* game_data) {
    if(slot != 0)
        slot = 1;

    for(int i = 0; i < SECTION_TOTAL; i++)
        if(read_section_id(slot, i) == SECTION_TRAINER_INFO_ID) {
            copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE), game_data->trainer_name, OT_NAME_GEN3_SIZE+1);
            game_data->trainer_gender = *(vu8*)((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + 8);
            game_data->trainer_id = read_int_save((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + 10);
            copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + DEX_POS_0, game_data->pokedex_owned, DEX_BYTES);
            copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + DEX_POS_0 + DEX_BYTES, game_data->pokedex_seen, DEX_BYTES);
            determine_game_with_save(&game_data->game_identifier, slot, i, summable_bytes[SECTION_TRAINER_INFO_ID]);
            
            break;
        }
}

void read_party(int slot, struct game_data_t* game_data) {
    if(slot != 0)
        slot = 1;
    
    read_game_data_trainer_info(slot, game_data);
    if(game_data->game_identifier.game_is_jp == UNDETERMINED) {
        if(game_data->trainer_name[OT_NAME_JP_GEN3_SIZE+1] == 0)
            game_data->game_identifier.game_is_jp = 1;
        else
            game_data->game_identifier.game_is_jp = 0;
    }
    u16 add_on = RSE_PARTY;
    if(game_data->game_identifier.game_main_version == FRLG_MAIN_GAME_CODE)
        add_on = FRLG_PARTY;

    for(int i = 0; i < SECTION_TOTAL; i++)
        if(read_section_id(slot, i) == SECTION_PARTY_INFO_ID) {
            copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + add_on, (u8*)(&game_data->party_3), sizeof(struct gen3_party));
            break;
        }
    if(game_data->party_3.total > PARTY_SIZE)
        game_data->party_3.total = PARTY_SIZE;
    u8 curr_slot = 0;
    u8 found = 0;
    for(int i = 0; i < game_data->party_3.total; i++) {
        process_gen3_data(&game_data->party_3.mons[i], &game_data->party_3_undec[i], game_data->game_identifier.game_main_version, game_data->game_identifier.game_sub_version);
        if(game_data->party_3_undec[i].is_valid_gen3)
            found = 1;
    }
    if (!found)
        game_data->party_3.total = 0;
    for(int i = 0; i < game_data->party_3.total; i++)
        if(gen3_to_gen2(&game_data->party_2.mons[curr_slot], &game_data->party_3_undec[i], game_data->trainer_id)) {
            curr_slot++;
            game_data->party_3_undec[i].is_valid_gen2 = 1;
        }
        else
            game_data->party_3_undec[i].is_valid_gen2 = 0;
    game_data->party_2.total = curr_slot;
    curr_slot = 0;
    for(int i = 0; i < game_data->party_3.total; i++)
        if(gen3_to_gen1(&game_data->party_1.mons[curr_slot], &game_data->party_3_undec[i], game_data->trainer_id)) {
            curr_slot++;
            game_data->party_3_undec[i].is_valid_gen1 = 1;
        }
        else
            game_data->party_3_undec[i].is_valid_gen1 = 0;
    game_data->party_1.total = curr_slot;
}

u32 read_slot_index(int slot) {
    if(slot != 0)
        slot = 1;
    
    return read_int_save((slot * SAVE_SLOT_SIZE) + SAVE_SLOT_INDEX_POS);
}

u8 validate_slot(int slot) {
    if(slot != 0)
        slot = 1;
    
    u16 valid_sections = 0;
    u32 buffer[SECTION_SIZE>>2];
    u16* buffer_16 = (u16*)buffer;
    u32 current_save_index = read_slot_index(slot);
    
    for(int i = 0; i < SECTION_TOTAL; i++)
    {
        copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE), (u8*)buffer, SECTION_SIZE);
        if(buffer[MAGIC_NUMBER_POS>>2] != MAGIC_NUMBER)
            return 0;
        if(buffer[SAVE_NUMBER_POS>>2] != current_save_index)
            return 0;
        u16 section_id = buffer_16[SECTION_ID_POS>>1];
        if(section_id >= SECTION_TOTAL)
            return 0;
        u16 prepared_checksum = buffer_16[CHECKSUM_POS>>1];
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

void read_gen_3_data(struct game_data_t* game_data){
    init_bank();
    
    u8 slot = get_slot();
    if(slot == INVALID_SLOT) {
        return;
    }
    
    read_party(slot, game_data);
}