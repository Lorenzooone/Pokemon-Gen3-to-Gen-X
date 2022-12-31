// SPDX-License-Identifier: MIT
//
// Copyright (c) 2020 Antonio Niño Díaz (AntonioND)

#include <stdio.h>
#include <string.h>

#include <gba.h>

#include "multiboot_handler.h"
#include "graphics_handler.h"
//#include "save.h"

// --------------------------------------------------------------------

#define PACKED __attribute__((packed))
#define ALWAYS_INLINE __attribute__((always_inline)) static inline
#define MAX_DUMP_SIZE 0x20000
#define REG_WAITCNT		*(vu16*)(REG_BASE + 0x204)  // Wait state Control

// --------------------------------------------------------------------

const char* person_strings[] = {" - You", " - Other"};
const char* game_strings[] = {"RS", "FRLG", "E"};
const char* unidentified_string = "Unidentified";
const char* subgame_rs_strings[] = {"R", "S"};
const char* subgame_frlg_strings[] = {"FR", "LG"};
const char* actor_strings[] = {"Master", "Slave"};
const char* region_strings[] = {"Int", "Jap"};
const char* slash_string = {"/"};
const char* target_strings[] = {"Gen 1", "Gen 2", "Gen 3"};
const char* stat_strings[] = {"Hp", "Atk", "Def", "SpA", "SpD", "Spe"};
const char* contest_strings[] = {"Coolness", "Beauty", "Cuteness", "Smartness", "Toughness", "Feel"};

enum STATE {MAIN_MENU, MULTIBOOT, TRADING_MENU, INFO_MENU};
enum STATE curr_state;
u32 counter = 0;
u32 input_counter = 0;

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

#define MAGIC_NUMBER 0x08012025
#define INVALID_SLOT 2
#define SAVE_SLOT_SIZE 0xE000
#define SAVE_SLOT_INDEX_POS 0xDFFC
#define SECTION_ID_POS 0xFF4
#define SECTION_SIZE 0x1000
#define SECTION_TOTAL 14

#define SECTION_TRAINER_INFO_ID 0
#define SECTION_PARTY_INFO_ID 1
#define SECTION_DEX_SEEN_1_ID 1
#define SECTION_DEX_SEEN_2_ID 4
#define SECTION_MAIL_ID 3
#define SECTION_GIFT_RIBBON_ID 4

#define SECTION_GAME_CODE 0xAC
#define FRLG_GAME_CODE 0x1
#define RS_GAME_CODE 0
#define RS_END_BATTLE_TOWER 0x890
#define DEX_POS_0 0x28
#define RSE_PARTY 0x234
#define FRLG_PARTY 0x34

#define ROM 0x8000000

#define UNDETERMINED 0xFF
#define NUMBER_OF_GAMES 5
#define GAME_STRING_POS (ROM+0xA0)
#define GAME_STRING_SIZE 0xC
#define JAPANESE_ID_POS (ROM+0xAF)
#define JAPANESE_ID 0x4A
#define RS_MAIN_GAME_CODE 0x0
#define R_SUB_GAME_CODE 0x0
#define S_SUB_GAME_CODE 0x1
#define FRLG_MAIN_GAME_CODE 0x1
#define FR_SUB_GAME_CODE 0x0
#define LG_SUB_GAME_CODE 0x1
#define E_MAIN_GAME_CODE 0x2
#define E_SUB_GAME_CODE 0x0

#define VALID_MAPS 36

#include "party_handler.h"
#include "text_handler.h"
#include "sprite_handler.h"

const char* game_identifiers[NUMBER_OF_GAMES] = {"POKEMON RUBY", "POKEMON SAPP", "POKEMON FIRE", "POKEMON LEAF", "POKEMON EMER"};
const u8 main_identifiers[NUMBER_OF_GAMES] = {RS_MAIN_GAME_CODE,RS_MAIN_GAME_CODE,FRLG_MAIN_GAME_CODE,FRLG_MAIN_GAME_CODE,E_MAIN_GAME_CODE};
const u8 sub_identifiers[NUMBER_OF_GAMES] = {R_SUB_GAME_CODE,S_SUB_GAME_CODE,FR_SUB_GAME_CODE,LG_SUB_GAME_CODE,E_SUB_GAME_CODE};

const u16 summable_bytes[SECTION_TOTAL] = {3884, 3968, 3968, 3968, 3848, 3968, 3968, 3968, 3968, 3968, 3968, 3968, 3968, 2000};
const u16 pokedex_extra_pos_1 [] = {0x938, 0x5F8, 0x988};
const u16 pokedex_extra_pos_2 [] = {0xC0C, 0xB98, 0xCA4};
const u16 mail_pos [] = {0xC4C, 0xDD0, 0xCE0};
const u16 ribbon_pos [] = {0x290, 0x21C, 0x328};
const u16 special_area [] = {0, 9, 9};
const u16 rs_valid_maps[VALID_MAPS] = {0x0202, 0x0203, 0x0301, 0x0302, 0x0405, 0x0406, 0x0503, 0x0504, 0x0603, 0x0604, 0x0700, 0x0701, 0x0804, 0x0805, 0x090a, 0x090b, 0x0a05, 0x0a06, 0x0b05, 0x0b06, 0x0c02, 0x0c03, 0x0d06, 0x0d07, 0x0e03, 0x0e04, 0x0f02, 0x0f03, 0x100c, 0x100d, 0x100a, 0x1918, 0x1919, 0x191a, 0x191b, 0x1a05};

u8 game_is_jp;
u8 game_main_version;
u8 game_sub_version;
u8 pokedex_seen[DEX_BYTES];
u8 pokedex_owned[DEX_BYTES];
u8 giftRibbons[2][GIFT_RIBBONS];
u8 trainer_name[2][OT_NAME_GEN3_SIZE+1];
u8 trainer_gender;
u32 trainer_id;
struct mail_gen3 mails_3[2][PARTY_SIZE];
struct gen3_party parties_3[2];
struct gen3_mon_data_unenc parties_3_undec[2][PARTY_SIZE];
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

u32 read_game_data_trainer_info(int slot, u8 game_main_version) {
    if(slot != 0)
        slot = 1;
    
    u32 game_code = -1;

    for(int i = 0; i < SECTION_TOTAL; i++)
        if(read_section_id(slot, i) == SECTION_TRAINER_INFO_ID) {
            copy_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE), trainer_name[0], OT_NAME_GEN3_SIZE+1);
            trainer_gender = *(vu8*)((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + 8);
            trainer_id = read_int((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + 10);
            copy_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + DEX_POS_0, pokedex_owned, DEX_BYTES);
            copy_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + DEX_POS_0 + DEX_BYTES, pokedex_seen, DEX_BYTES);
            
            if(game_main_version == UNDETERMINED) {
                game_code = read_int((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + SECTION_GAME_CODE);
                if((game_code != FRLG_GAME_CODE) && (game_code != RS_GAME_CODE)) {
                    u8 found = 0;
                    for(int j = RS_END_BATTLE_TOWER; i < summable_bytes[SECTION_TRAINER_INFO_ID]; i++)
                        if((*(vu8*)((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + j))) {
                            found = 1;
                            break;
                        }
                    if(found)
                        game_code = (FRLG_GAME_CODE+1);
                    else
                        game_code = RS_GAME_CODE;
                }
                switch(game_code) {
                    case RS_GAME_CODE:
                        game_main_version = RS_MAIN_GAME_CODE;
                        break;
                    case FRLG_GAME_CODE:
                        game_main_version = FRLG_MAIN_GAME_CODE;
                        break;
                    default:
                        game_main_version = E_MAIN_GAME_CODE;
                        break;
                }
            }
            
            break;
        }
    
    return game_main_version;
}

void read_party(int slot) {
    if(slot != 0)
        slot = 1;
    
    game_main_version = read_game_data_trainer_info(slot, game_main_version);
    if(game_is_jp == UNDETERMINED) {
        if(trainer_name[OT_NAME_JP_GEN3_SIZE+1] == 0)
            game_is_jp = 1;
        else
            game_is_jp = 0;
    }
    u16 add_on = RSE_PARTY;
    if(game_main_version == FRLG_MAIN_GAME_CODE)
        add_on = FRLG_PARTY;

    for(int i = 0; i < SECTION_TOTAL; i++)
        if(read_section_id(slot, i) == SECTION_PARTY_INFO_ID) {
            copy_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + add_on, (u8*)(&parties_3[0]), sizeof(struct gen3_party));
            break;
        }
    if(parties_3[0].total > PARTY_SIZE)
        parties_3[0].total = PARTY_SIZE;
    u8 curr_slot = 0;
    u8 found = 0;
    for(int i = 0; i < parties_3[0].total; i++) {
        process_gen3_data(&parties_3[0].mons[i], &parties_3_undec[0][i]);
        if(parties_3_undec[0][i].is_valid_gen3)
            found = 1;
    }
    if (!found)
        parties_3[0].total = 0;
    for(int i = 0; i < parties_3[0].total; i++)
        if(gen3_to_gen2(&parties_2[0].mons[curr_slot], &parties_3_undec[0][i], trainer_id)) {
            curr_slot++;
            parties_3_undec[0][i].is_valid_gen2 = 1;
        }
        else
            parties_3_undec[0][i].is_valid_gen2 = 0;
    parties_2[0].total = curr_slot;
    curr_slot = 0;
    for(int i = 0; i < parties_3[0].total; i++)
        if(gen3_to_gen1(&parties_1[0].mons[curr_slot], &parties_3_undec[0][i], trainer_id)) {
            curr_slot++;
            parties_3_undec[0][i].is_valid_gen1 = 1;
        }
        else
            parties_3_undec[0][i].is_valid_gen1 = 0;
    parties_1[0].total = curr_slot;
}

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
    parties_1[0].total = 0;
    parties_2[0].total = 0;
    parties_3[0].total = 0;
    parties_1[1].total = 0;
    parties_2[1].total = 0;
    parties_3[1].total = 0;
    
    u8 slot = get_slot();
    if(slot == INVALID_SLOT) {
        return;
    }
    
    read_party(slot);
    
    // Terminate the names
    trainer_name[0][OT_NAME_GEN3_SIZE] = GEN3_EOL;
    for(int i = 0; i < (OT_NAME_GEN3_SIZE+1); i++)
        trainer_name[1][i] = GEN3_EOL;
}

void get_game_id(){
    u8 tmp_buffer[GAME_STRING_SIZE+1];
    text_generic_copy((u8*)GAME_STRING_POS, tmp_buffer, GAME_STRING_SIZE, GAME_STRING_SIZE+1);
    iprintf("%s\n", tmp_buffer);
    for(int i = 0; i < NUMBER_OF_GAMES; i++)
        if(text_generic_is_same(game_identifiers[i], (u8*)GAME_STRING_POS, GAME_STRING_SIZE, GAME_STRING_SIZE)) {
            game_main_version = main_identifiers[i];
            game_sub_version = sub_identifiers[i];
            if((*(u8*)JAPANESE_ID_POS) == JAPANESE_ID)
                game_is_jp = 1;
            else
                game_is_jp = 0;
        }
}

u8 get_valid_options() {
    return (parties_1[0].total > 0 ? 1: 0) | (parties_2[0].total > 0 ? 2: 0) | (parties_3[0].total > 0 ? 4: 0);
}

void fill_options_array(u8* options) {
    u8 curr_slot = 0;
    for(int i = 0; i < 3; i++)
        options[i] = 0;
    if(parties_1[0].total > 0) {
        options[curr_slot++] = 1;
        for(int i = curr_slot; i < 3; i++)
            options[i] = 1;
    }
    if(parties_2[0].total > 0) {
        options[curr_slot++] = 2;
        for(int i = curr_slot; i < 3; i++)
            options[i] = 2;
    }
    if(parties_3[0].total > 0) {
        options[curr_slot++] = 3;
        for(int i = curr_slot; i < 3; i++)
            options[i] = 3;
    }
}

u8 get_number_of_higher_options(u8* options, u8 curr_option) {
    u8 curr_num = 0;
    u8 highest_found = curr_option;
    for(int i = 0; i < 3; i++)
        if(options[i] > highest_found) {
            curr_num++;
            highest_found = options[i];
        }
    return curr_num;
}

u8 get_number_of_lower_options(u8* options, u8 curr_option) {
    u8 curr_num = 0;
    u8 highest_found = curr_option;
    for(int i = 2; i >= 0; i--)
        if(options[i] < highest_found) {
            curr_num++;
            highest_found = options[i];
        }
    return curr_num;
}

void fill_trade_options(u8* options, u8 curr_gen, u8 is_own) {
    u8 index = 1;
    if(is_own)
        index = 0;
    struct gen3_mon_data_unenc* party = parties_3_undec[index];
    u8 real_party_size = parties_3[index].total;
    if(real_party_size > PARTY_SIZE)
        real_party_size = PARTY_SIZE;
    
    u8 curr_slot = 0;
    for(int i = 0; i < real_party_size; i++) {
        u8 is_valid = party[i].is_valid_gen3;
        if(curr_gen == 2)
            is_valid = party[i].is_valid_gen2;
        if(curr_gen == 1)
            is_valid = party[i].is_valid_gen1;

        if(is_valid)
            options[curr_slot++] = i;
    }
    for(int i = curr_slot; i < PARTY_SIZE; i++)
        options[i] = 0xFF;
}

#define X_TILES 30
#define BASE_Y_SPRITE_TRADE_MENU 0
#define BASE_Y_SPRITE_INCREMENT_TRADE_MENU 24
#define BASE_X_SPRITE_TRADE_MENU 8

void print_game_info(){
    iprintf("\n Game: ");
    const char* chosen_str = game_strings[game_main_version];
    switch(game_main_version) {
        case RS_MAIN_GAME_CODE:
            if(game_sub_version != UNDETERMINED)
                chosen_str = subgame_rs_strings[game_sub_version];
            break;
        case FRLG_MAIN_GAME_CODE:
            if(game_sub_version != UNDETERMINED)
                chosen_str = subgame_frlg_strings[game_sub_version];
            break;
        case E_MAIN_GAME_CODE:
            break;
        default:
            chosen_str = unidentified_string;
            break;
    }
    iprintf("%s\n", chosen_str);
}

void print_trade_menu(u8 update, u8 curr_gen, u8 load_sprites) {
    if(!update)
        return;
    
    iprintf("\x1b[2J");
    
    u8 printable_string[X_TILES+1];
    u8 tmp_buffer[X_TILES+1];
    
    text_generic_terminator_fill(printable_string, X_TILES+1);
    for(int i = 0; i < 2; i++) {
        text_gen3_to_generic(trainer_name[i], tmp_buffer, OT_NAME_GEN3_SIZE+1, X_TILES, 0, 0);
        text_generic_concat(tmp_buffer, person_strings[i], printable_string + (i*(X_TILES>>1)), OT_NAME_GEN3_SIZE, (X_TILES >> 1)-OT_NAME_GEN3_SIZE, X_TILES>>1);
    }
    text_generic_replace(printable_string, X_TILES, GENERIC_EOL, GENERIC_SPACE);
    iprintf("%s", printable_string);

    if(load_sprites)
        reset_sprites_to_cursor();

    u8 options[2][PARTY_SIZE];
    fill_trade_options(options[0], curr_gen, 1);
    fill_trade_options(options[1], curr_gen, 0);
    for(int i = 0; i < PARTY_SIZE; i++) {
        text_generic_terminator_fill(printable_string, X_TILES+1);
        for(int j = 0; j < 2; j++) {
            // These two values are for debug only - They should be j and i
            u8 party_index = j;
            u8 mon_index = i;
            if(options[party_index][mon_index] != 0xFF) {
                struct gen3_mon_data_unenc* mon = &parties_3_undec[party_index][options[party_index][mon_index]];
                // I tried just using printf here with left padding, but it's EXTREMELY slow
                text_generic_copy(get_pokemon_name_raw(mon), printable_string + 5 + ((X_TILES>>1)*j), NAME_SIZE, (X_TILES>>1) - 5);
                if(load_sprites)
                    load_pokemon_sprite_raw(mon, BASE_Y_SPRITE_TRADE_MENU + (i*BASE_Y_SPRITE_INCREMENT_TRADE_MENU), (((X_TILES >> 1) << 3)*j) + BASE_X_SPRITE_TRADE_MENU);
            }
        }
        text_generic_replace(printable_string, X_TILES, GENERIC_EOL, GENERIC_SPACE);
        iprintf("\n%s\n", printable_string);
    }
    iprintf("  Cancel");
}

#define BASE_Y_SPRITE_INFO_PAGE 0
#define BASE_X_SPRITE_INFO_PAGE 0
#define TEXT_BORDER_DISTANCE 0
#define PAGES_TOTAL 5
#define FIRST_PAGE 1

void print_pokemon_base_info(u8 load_sprites, struct gen3_mon_data_unenc* mon, u8 is_jp, u8 is_egg, u8 page) {
    u8 printable_string[X_TILES+1];
    
    u8 is_shiny = is_shiny_gen3_raw(mon, 0);
    u8 has_pokerus = has_pokerus_gen3_raw(mon);
    
    iprintf("\x1b[2J");
    
    u8 page_total = PAGES_TOTAL-FIRST_PAGE+1;
    
    if(is_egg)
        page_total = FIRST_PAGE;
    
    if(page == FIRST_PAGE)
        iprintf(" ");
    else
        iprintf("<");
        
    iprintf("%d / %d", page, page_total);
    
    if(page < page_total)
        iprintf(">");

    if(load_sprites) {
        reset_sprites_to_cursor();
        load_pokemon_sprite_raw(mon, BASE_Y_SPRITE_INFO_PAGE, BASE_X_SPRITE_INFO_PAGE);
    }
    
    if(!is_egg) {
        text_gen3_to_generic(mon->src->nickname, printable_string, NICKNAME_GEN3_SIZE, X_TILES, is_jp, 0);
        iprintf("\n\n    %s - %s %c\n", printable_string, get_pokemon_name_raw(mon), get_pokemon_gender_char_raw(mon));
    
        iprintf("    ");
        if(is_shiny)
            iprintf("Shiny");
    
        if(is_shiny && has_pokerus)
            iprintf(" - ");
    
        if(has_pokerus == HAS_POKERUS)
            iprintf("Has Pokerus");
        else if(has_pokerus == HAD_POKERUS)
            iprintf("Had Pokerus");
    }
    else
        iprintf("\n\n    %s\n", get_pokemon_name_raw(mon));
    
}

void print_bottom_info(){
    iprintf("\nB: Go Back");
}

void print_pokemon_page1(struct gen3_mon_data_unenc* mon) {
    u8 printable_string[X_TILES+1];
    u8 is_jp = (mon->src->language == JAPANESE_LANGUAGE);
    u8 is_egg = is_egg_gen3_raw(mon);
    
    if(!is_egg) {
        
        iprintf("\nLevel: %d\n", to_valid_level_gen3(mon->src));
        
        if(is_jp)
            iprintf("\nLanguage: Japanese\n");
        else
            iprintf("\nLanguage: International\n");
        
        text_gen3_to_generic(mon->src->ot_name, printable_string, OT_NAME_GEN3_SIZE, X_TILES, is_jp, 0);
        iprintf("\nOT: %s - %c - %05d\n", printable_string, get_trainer_gender_char_raw(mon), (mon->src->ot_id)&0xFFFF);
        iprintf("\nItem: %s\n", get_item_name_raw(mon));
        
        iprintf("\nMet in: %s\n", get_met_location_name_gen3_raw(mon));
        u8 met_level = get_met_level_gen3_raw(mon);
        if(met_level > 0)
            iprintf("\nCaught at Level %d\n\nCaught in %s Ball\n\n", met_level, get_pokeball_base_name_gen3_raw(mon));
        else
            iprintf("\nHatched in %s Ball\n\n\n\n", get_pokeball_base_name_gen3_raw(mon));
    }
    else
        iprintf("\nHatches in : %d Egg Cycles\n\nRoughly hatches in: %d Steps\n\n\n\n\n\n\n\n\n\n\n\n", mon->growth.friendship, mon->growth.friendship * 0x100);
}

void print_pokemon_page2(struct gen3_mon_data_unenc* mon) {
        
    iprintf("\nSTAT    VALUE    EV    IV\n");
    for(int i = 0; i < GEN2_STATS_TOTAL; i++) {
        iprintf("\n %-3s", stat_strings[i]);
        if(i == HP_STAT_INDEX) {
            u16 hp = calc_stats_gen3_raw(mon, i);
            u16 curr_hp = mon->src->curr_hp;
            if(curr_hp > hp)
                curr_hp = hp;
            iprintf("   % 3d/%-3d  ", curr_hp, hp);
        }
        else
            iprintf("     % 3d    ", calc_stats_gen3_raw(mon,i));
        iprintf("% 3d   % 3d\n", get_evs_gen3(&mon->evs, i), get_ivs_gen3(&mon->misc, i));
    }
    
    iprintf("\n   Hidden Power %s: %d", get_hidden_power_type_name_gen3(&mon->misc), get_hidden_power_power_gen3(&mon->misc));
    //print_game_info();
}

void print_pokemon_page3(struct gen3_mon_data_unenc* mon) {

    iprintf("\nMOVES            PP UP\n");
    for(int i = 0; i < (MOVES_SIZE); i++){
        iprintf("\n %-14s    %d\n", get_move_name_gen3(&mon->attacks, i), (mon->growth.pp_bonuses >> (2*i)) & 3);
    }
    
    iprintf("\nAbility: %s\n", get_ability_name_raw(mon));
    
    iprintf("\nExperience: %d\n", get_proper_exp_raw(mon));
    
    if(to_valid_level_gen3(mon->src) < MAX_LEVEL)
        iprintf("\nNext Lv. in: %d > Lv. %d", get_level_exp_mon_index(get_mon_index_raw(mon), to_valid_level_gen3(mon->src)+1) - get_proper_exp_raw(mon), to_valid_level_gen3(mon->src)+1);
    else
        iprintf("\n");
}

void print_pokemon_page4(struct gen3_mon_data_unenc* mon) {

    iprintf("\nCONTEST STAT     VALUE\n");
    for(int i = 0; i < CONTEST_STATS_TOTAL; i++){
        iprintf("\n %-17s% 3d\n", contest_strings[i], mon->evs.contest[i]);
    }

    iprintf("\n");
}

#define NUM_LINES 10
const u8 ribbon_print_pos[NUM_LINES*2] = {0,1,2,3,4,5,6,7,8,9,14,15,13,16,10,0xFF,11,0xFF,12,0xFF};

void print_pokemon_page5(struct gen3_mon_data_unenc* mon) {
    
    u8 printable_string[X_TILES+1];

    iprintf("\nRIBBONS\n");
    for(int i = 0; i < NUM_LINES; i++){
        // CANNOT USE SNPRINTF OR SPRINTF! THEY ADD 20 KB!
        text_generic_concat(get_ribbon_name(&mon->misc, ribbon_print_pos[(i*2)]), get_ribbon_rank_name(&mon->misc, ribbon_print_pos[(i*2)]), printable_string, X_TILES, X_TILES, X_TILES);
        if(ribbon_print_pos[(i*2)+1] != 0xFF) {
            iprintf("\n %-13s ", printable_string);
            text_generic_concat(get_ribbon_name(&mon->misc, ribbon_print_pos[(i*2)+1]), get_ribbon_rank_name(&mon->misc, ribbon_print_pos[(i*2)+1]), printable_string, X_TILES, X_TILES, X_TILES);
            iprintf(" %-13s", printable_string);
        }
        else
            iprintf("\n %s", printable_string);
    }

    iprintf("\n\n\n");
}

typedef void (*print_functions_t)();

typedef void (*print_info_functions_t)(struct gen3_mon_data_unenc*);

print_info_functions_t print_info_functions[PAGES_TOTAL] = {print_pokemon_page1, print_pokemon_page2, print_pokemon_page3, print_pokemon_page4, print_pokemon_page5};

void print_pokemon_pages(u8 update, u8 load_sprites, struct gen3_mon_data_unenc* mon, u8 page_num) {
    if(!update)
        return;
    
    u8 printable_string[X_TILES+1];
    u8 is_jp = (mon->src->language == JAPANESE_LANGUAGE);
    u8 is_egg = is_egg_gen3_raw(mon);
    
    if(is_egg)
        page_num = FIRST_PAGE;
    if((page_num < FIRST_PAGE) || (page_num > (PAGES_TOTAL-FIRST_PAGE)))
        page_num = FIRST_PAGE;

    print_pokemon_base_info(load_sprites, mon, is_jp, is_egg, page_num);
    
    print_info_functions[page_num-FIRST_PAGE](mon);
    
    print_bottom_info();
}

#define BASE_Y_CURSOR_MAIN_MENU 8
#define BASE_Y_CURSOR_INCREMENT_MAIN_MENU 16

void print_main_menu(u8 update, u8 curr_gen, u8 is_jp, u8 is_master) {
    if(!update)
        return;
    
    u8 options[3];

    iprintf("\x1b[2J");
    
    if(!get_valid_options())
        iprintf("\n  Error reading the data!\n\n\n\n\n\n\n");
    else {
        fill_options_array(options);
        if(curr_gen >= 3)
            curr_gen = 2;
        curr_gen = options[curr_gen];
        if(get_number_of_higher_options(options, curr_gen) > 0 && get_number_of_lower_options(options, curr_gen) > 0)
            iprintf("\n  Target: <%s>\n", target_strings[curr_gen-1]);
        else if(get_number_of_higher_options(options, curr_gen) > 0)
            iprintf("\n  Target:  %s>\n", target_strings[curr_gen-1]);
        else if(get_number_of_lower_options(options, curr_gen) > 0)
            iprintf("\n  Target: <%s\n", target_strings[curr_gen-1]);
        else
            iprintf("\n  Target:  %s\n", target_strings[curr_gen-1]);
        if(curr_gen < 3) {
            if(!is_jp)
                iprintf("\n  Target Region:  %s>\n", region_strings[0]);
            else
            iprintf("\n  Target Region: <%s\n", region_strings[1]);
        }
        else
            iprintf("\n\n");
        if(!is_master)
            iprintf("\n  Act as:  %s>\n", actor_strings[1]);
        else
            iprintf("\n  Act as: <%s\n", actor_strings[0]);
        iprintf("\n  Start Trade\n");
    }
    iprintf("\n  Send Multiboot");
}

void print_multiboot(enum MULTIBOOT_RESULTS result) {

    iprintf("\x1b[2J");
    
    if(result == MB_SUCCESS)
        iprintf("\nMultiboot successful!\n\n\n");
    else if(result == MB_NO_INIT_SYNC)
        iprintf("\nCouldn't sync.\n\nTry again!\n");
    else
        iprintf("\nThere was an error.\n\nTry again!\n");
    iprintf("\nA: To the previous menu");
}

u8 handle_input_multiboot_menu(u16 keys) {
    if(keys & KEY_A)
        return 1;
    return 0;
}

#define START_MULTIBOOT 0x49

u8 init_cursor_y_pos_main_menu(){
    if(!get_valid_options())
        return 4;
    return 0;
}

u8 handle_input_main_menu(u8* cursor_y_pos, u16 keys, u8* update, u8* target, u8* region, u8* master) {

    u8 options[3];
    fill_options_array(options);
    u8 curr_gen = *target;
    if(curr_gen >= 3)
        curr_gen = 2;
    curr_gen = options[curr_gen];
    
    switch(*cursor_y_pos) {
        case 0:
            if((keys & KEY_RIGHT) && (get_number_of_higher_options(options, curr_gen) > 0)) {
                (*target) += 1;
                (*update) = 1;
            }
            else if((keys & KEY_LEFT) && (get_number_of_lower_options(options, curr_gen) > 0)) {
                (*target) -= 1;
                (*update) = 1;
            }
            else if((keys & KEY_A) && (get_number_of_higher_options(options, curr_gen) > 0)) {
                (*target) += 1;
                (*update) = 1;
            }
            else if((keys & KEY_A) && (get_number_of_lower_options(options, curr_gen) > 0)) {
                (*target) -= 1;
                (*update) = 1;
            }
            else if(keys & KEY_DOWN) {
                if(curr_gen == 3)
                    (*cursor_y_pos) += 2;
                else
                    (*cursor_y_pos) += 1;
            }
            else if(keys & KEY_UP) {
                (*cursor_y_pos) = 4;
            }
            break;
        case 1:
            if(curr_gen == 3)
                (*cursor_y_pos) += 1;
            else if((keys & KEY_RIGHT) && (!(*region))) {
                (*region) = 1;
                (*update) = 1;
            }
            else if((keys & KEY_LEFT) && ((*region))) {
                (*region) = 0;
                (*update) = 1;
            }
            else if((keys & KEY_A) && (!(*region))) {
                (*region) = 1;
                (*update) = 1;
            }
            else if((keys & KEY_A) && ((*region))) {
                (*region) = 0;
                (*update) = 1;
            }
            else if(keys & KEY_DOWN) {
                (*cursor_y_pos) += 1;
            }
            else if(keys & KEY_UP) {
                (*cursor_y_pos) -= 1;
            }
            break;
        case 2:
            if((keys & KEY_RIGHT) && (!(*master))) {
                (*master) = 1;
                (*update) = 1;
            }
            else if((keys & KEY_LEFT) && ((*master))) {
                (*master) = 0;
                (*update) = 1;
            }
            else if((keys & KEY_A) && (!(*master))) {
                (*master) = 1;
                (*update) = 1;
            }
            else if((keys & KEY_A) && ((*master))) {
                (*master) = 0;
                (*update) = 1;
            }
            else if(keys & KEY_DOWN)
                (*cursor_y_pos) += 1;
            else if(keys & KEY_UP) {
                if(curr_gen == 3)
                    (*cursor_y_pos) -= 2;
                else
                    (*cursor_y_pos) -= 1;
            }
            break;
        case 3:
            if((keys & KEY_A) && (get_valid_options()))
                return curr_gen;
            else if(keys & KEY_DOWN)
                (*cursor_y_pos) += 1;
            else if(keys & KEY_UP)
                (*cursor_y_pos) -= 1;
            break;
        default:
            if(keys & KEY_A) {
                return START_MULTIBOOT;
            }
            else if((keys & KEY_DOWN) && (get_valid_options())) {
                (*cursor_y_pos) = 0;
            }
            else if((keys & KEY_UP) && (get_valid_options())) {
                (*cursor_y_pos) = 3;
            }
            break;
    }
    return 0;
}

void vblank_update_function() {
	REG_IF |= IRQ_VBLANK;

    move_sprites(counter);
    move_cursor_x(counter);
    counter++;
}

int main(void)
{
    int val = 0;
    counter = 0;
    input_counter = 0;
    u16 keys;
    enum MULTIBOOT_RESULTS result;
    
    game_main_version = UNDETERMINED;
    game_sub_version = UNDETERMINED;
    game_is_jp = UNDETERMINED;
    
    get_game_id();
    
    init_oam_palette();
    init_sprite_counter();
    irqInit();
    irqSet(IRQ_VBLANK, vblank_update_function);
    irqEnable(IRQ_VBLANK);
    irqSet(IRQ_SERIAL, sio_handle_irq_slave);
    irqEnable(IRQ_SERIAL);

    consoleDemoInit();
    init_gender_symbols();
    REG_DISPCNT |= OBJ_ON | OBJ_1D_MAP;
    
    iprintf("\x1b[2J");
    
    read_gen_3_data();
    
    u8 returned_val;
    u8 update = 0;
    u8 target = 1;
    u8 region = 0;
    u8 master = 0;
    u8 cursor_y_pos = init_cursor_y_pos_main_menu();
    curr_state = MAIN_MENU;
    
    print_main_menu(1, target, region, master);
    
    init_item_icon();
    init_cursor(cursor_y_pos);
    
    while(1) {
        scanKeys();
        keys = keysDown();
        
        while ((!(keys & KEY_LEFT)) && (!(keys & KEY_RIGHT)) && (!(keys & KEY_A)) && (!(keys & KEY_UP)) && (!(keys & KEY_DOWN))) {
            VBlankIntrWait();
            scanKeys();
            keys = keysDown();
        }
        input_counter++;
        switch(curr_state) {
            case MAIN_MENU:
                returned_val = handle_input_main_menu(&cursor_y_pos, keys, &update, &target, &region, &master);
                print_main_menu(update, target, region, master);
                //print_pokemon_pages(update, 1, &parties_3_undec[0][0], FIRST_PAGE+1);
                //print_trade_menu(update, 2, 1);
                update_cursor_y(BASE_Y_CURSOR_MAIN_MENU + (BASE_Y_CURSOR_INCREMENT_MAIN_MENU * cursor_y_pos));
                if(returned_val == START_MULTIBOOT) {
                    curr_state = MULTIBOOT;
                    irqDisable(IRQ_SERIAL);
                    disable_cursor();
                    result = multiboot_normal((u16*)EWRAM, (u16*)(EWRAM + 0x3FF40));
                    print_multiboot(result);
                }
                break;
            case MULTIBOOT:
                if(handle_input_multiboot_menu(keys)) {
                    curr_state = MAIN_MENU;
                    print_main_menu(1, target, region, master);
                    cursor_y_pos = init_cursor_y_pos_main_menu();
                    update_cursor_y(BASE_Y_CURSOR_MAIN_MENU + (BASE_Y_CURSOR_INCREMENT_MAIN_MENU * cursor_y_pos));
                }
                break;
            default:
                curr_state = MAIN_MENU;
                print_main_menu(1, target, region, master);
                cursor_y_pos = init_cursor_y_pos_main_menu();
                update_cursor_y(BASE_Y_CURSOR_MAIN_MENU + (BASE_Y_CURSOR_INCREMENT_MAIN_MENU * cursor_y_pos));
                break;
        }
        update = 0;
    }

    //start_gen2_slave();
    //start_gen2();

    return 0;
}
