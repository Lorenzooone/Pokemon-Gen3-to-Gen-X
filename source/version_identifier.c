#include <gba.h>
#include "version_identifier.h"
#include "save.h"
#include "text_handler.h"
#include "party_handler.h"
#include "gen3_save.h"

#define REG_WAITCNT		*(vu16*)(REG_BASE + 0x204)  // Wait state Control

#define ROM 0x8000000

#define GAME_STRING_POS (ROM+0xA0)
#define GAME_STRING_SIZE 0xC
#define LANGUAGE_ID_POS (ROM+0xAF)
#define JAPANESE_ID 0x4A
#define ENGLISH_ID_0 0x45
#define ENGLISH_ID_1 0x50
#define GERMAN_ID 0x44
#define FRENCH_ID 0x46
#define ITALIAN_ID 0x49
#define SPANISH_ID 0x53

#define SECTION_GAME_CODE 0xAC
#define FRLG_GAME_CODE 0x1
#define RS_GAME_CODE 0
#define RS_END_BATTLE_TOWER 0x890

const char* game_identifier_strings[NUMBER_OF_GAMES] = {"POKEMON RUBY", "POKEMON SAPP", "POKEMON FIRE", "POKEMON LEAF", "POKEMON EMER"};
const u8 main_identifiers[NUMBER_OF_GAMES] = {RS_MAIN_GAME_CODE,RS_MAIN_GAME_CODE,FRLG_MAIN_GAME_CODE,FRLG_MAIN_GAME_CODE,E_MAIN_GAME_CODE};
const u8 sub_identifiers[NUMBER_OF_GAMES] = {R_SUB_GAME_CODE,S_SUB_GAME_CODE,FR_SUB_GAME_CODE,LG_SUB_GAME_CODE,E_SUB_GAME_CODE};
const u8 different_version_sub_identifiers[NUMBER_OF_GAMES] = {R_SUB_GAME_CODE,S_SUB_GAME_CODE,E_SUB_GAME_CODE,LG_SUB_GAME_CODE,FR_SUB_GAME_CODE};

void init_game_identifier(struct game_identity* identifier) {
    identifier->game_main_version = UNDETERMINED;
    identifier->game_sub_version = UNDETERMINED;
    identifier->language = UNDETERMINED;
    identifier->language_is_sys = 0;
    identifier->game_sub_version_undetermined = 0;
    identifier->padding = 0;
}

u8 is_trainer_name_japanese(u8* buffer) {
    for(size_t i = OT_NAME_JP_GEN3_SIZE+2; i < OT_NAME_GEN3_MAX_SIZE+1; i++)
        if(buffer[i])
            return 0;
    return 1;
}

void get_game_id(struct game_identity* identifier) {
    u8 tmp_buffer[GAME_STRING_SIZE+1];
    REG_WAITCNT = 0x4314;
    text_generic_copy((u8*)GAME_STRING_POS, tmp_buffer, GAME_STRING_SIZE, GAME_STRING_SIZE+1);
    u8 language_char = *((u8*)LANGUAGE_ID_POS);
    REG_WAITCNT = 0;
    for(int i = 0; i < NUMBER_OF_GAMES; i++)
        if(text_generic_is_same((const u8*)game_identifier_strings[i], tmp_buffer, GAME_STRING_SIZE, GAME_STRING_SIZE)) {
            identifier->game_main_version = main_identifiers[i];
            identifier->game_sub_version = sub_identifiers[i];
            switch(language_char) {
                case JAPANESE_ID:
                    identifier->language = JAPANESE_LANGUAGE;
                    break;
                case ENGLISH_ID_0:
                case ENGLISH_ID_1:
                    identifier->language = ENGLISH_LANGUAGE;
                    break;
                case FRENCH_ID:
                    identifier->language = FRENCH_LANGUAGE;
                    break;
                case ITALIAN_ID:
                    identifier->language = ITALIAN_LANGUAGE;
                    break;
                case GERMAN_ID:
                    identifier->language = GERMAN_LANGUAGE;
                    break;
                case SPANISH_ID:
                    identifier->language = SPANISH_LANGUAGE;
                    break;
                default:
                    identifier->language = SYS_LANGUAGE;
                    identifier->language_is_sys = 1;
                    break;
            }
        }
}

u8 id_to_version(struct game_identity* identifier) {
    u8 base_game = S_VERSION_ID;
    if(identifier->game_main_version == FRLG_GAME_CODE)
        base_game = FR_VERSION_ID;
    if(identifier->game_main_version == E_MAIN_GAME_CODE)
        base_game = E_VERSION_ID;
    
    if((base_game == S_VERSION_ID) && (identifier->game_sub_version == R_SUB_GAME_CODE))
        base_game = R_VERSION_ID;
    if((base_game == FR_VERSION_ID) && (identifier->game_sub_version == LG_SUB_GAME_CODE))
        base_game = LG_VERSION_ID;
    
    return base_game;
}

void change_sub_version(struct game_identity* identifier) {
    if(!identifier->game_sub_version_undetermined)
        return;

    u8 game_id = id_to_version(identifier);
    if((game_id > NUMBER_OF_GAMES) || (!game_id))
        return;

    identifier->game_sub_version = different_version_sub_identifiers[game_id-1];
}

u8 determine_possible_main_game_for_slot(u8 slot, u8 section_pos, u16 total_bytes) {
    u32 game_code = -1;
    game_code = read_int_save((slot * SAVE_SLOT_SIZE) + (section_pos * SECTION_SIZE) + SECTION_GAME_CODE);
    if((game_code != FRLG_GAME_CODE) && (game_code != RS_GAME_CODE)) {
        u8 found = 0;
        for(int i = RS_END_BATTLE_TOWER; i < total_bytes; i++)
            if(read_byte_save((slot * SAVE_SLOT_SIZE) + (section_pos * SECTION_SIZE) + i)) {
                found = 1;
                break;
            }
        if(found)
            return 1 << E_MAIN_GAME_CODE;
        else
            return (1 << E_MAIN_GAME_CODE) | (1 << RS_MAIN_GAME_CODE);
    }
    switch(game_code) {
        case RS_GAME_CODE:
            return 1 << RS_MAIN_GAME_CODE;
        case FRLG_GAME_CODE:
            return 1 << FRLG_MAIN_GAME_CODE;
        default:
            return 1 << E_MAIN_GAME_CODE;
    }
}

void determine_game_with_save(struct game_identity* identifier, u8 slot, u8 section_pos, u16 total_bytes) {
    u32 game_code = -1;
    if(identifier->game_main_version == UNDETERMINED) {
        game_code = read_int_save((slot * SAVE_SLOT_SIZE) + (section_pos * SECTION_SIZE) + SECTION_GAME_CODE);
        if((game_code != FRLG_GAME_CODE) && (game_code != RS_GAME_CODE)) {
            u8 found = 0;
            for(int i = RS_END_BATTLE_TOWER; i < total_bytes; i++)
                if(read_byte_save((slot * SAVE_SLOT_SIZE) + (section_pos * SECTION_SIZE) + i)) {
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
                identifier->game_main_version = RS_MAIN_GAME_CODE;
                identifier->game_sub_version = R_SUB_GAME_CODE;
                identifier->game_sub_version_undetermined = 1;
                break;
            case FRLG_GAME_CODE:
                identifier->game_main_version = FRLG_MAIN_GAME_CODE;
                identifier->game_sub_version = FR_SUB_GAME_CODE;
                identifier->game_sub_version_undetermined = 1;
                break;
            default:
                identifier->game_main_version = E_MAIN_GAME_CODE;
                break;
        }
    }
}
