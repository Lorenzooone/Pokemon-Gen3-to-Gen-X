#include <gba.h>
#include "version_identifier.h"
#include "save.h"
#include "text_handler.h"
#include "gen3_save.h"

#define ROM 0x8000000

#define GAME_STRING_POS (ROM+0xA0)
#define GAME_STRING_SIZE 0xC
#define JAPANESE_ID_POS (ROM+0xAF)
#define JAPANESE_ID 0x4A

#define SECTION_GAME_CODE 0xAC
#define FRLG_GAME_CODE 0x1
#define RS_GAME_CODE 0
#define RS_END_BATTLE_TOWER 0x890

const char* game_identifier_strings[NUMBER_OF_GAMES] = {"POKEMON RUBY", "POKEMON SAPP", "POKEMON FIRE", "POKEMON LEAF", "POKEMON EMER"};
const u8 main_identifiers[NUMBER_OF_GAMES] = {RS_MAIN_GAME_CODE,RS_MAIN_GAME_CODE,FRLG_MAIN_GAME_CODE,FRLG_MAIN_GAME_CODE,E_MAIN_GAME_CODE};
const u8 sub_identifiers[NUMBER_OF_GAMES] = {R_SUB_GAME_CODE,S_SUB_GAME_CODE,FR_SUB_GAME_CODE,LG_SUB_GAME_CODE,E_SUB_GAME_CODE};

void init_game_identifier(struct game_identity* identifier) {
    identifier->game_main_version = UNDETERMINED;
    identifier->game_sub_version = UNDETERMINED;
    identifier->game_is_jp = UNDETERMINED;
}

void get_game_id(struct game_identity* identifier){
    u8 tmp_buffer[GAME_STRING_SIZE+1];
    text_generic_copy((u8*)GAME_STRING_POS, tmp_buffer, GAME_STRING_SIZE, GAME_STRING_SIZE+1);
    for(int i = 0; i < NUMBER_OF_GAMES; i++)
        if(text_generic_is_same(game_identifier_strings[i], (u8*)GAME_STRING_POS, GAME_STRING_SIZE, GAME_STRING_SIZE)) {
            identifier->game_main_version = main_identifiers[i];
            identifier->game_sub_version = sub_identifiers[i];
            if((*(u8*)JAPANESE_ID_POS) == JAPANESE_ID)
                identifier->game_is_jp = 1;
            else
                identifier->game_is_jp = 0;
        }
}

u8 id_to_version(struct game_identity* identifier){
    
    u8 base_game = S_VERSION_ID;
    if(identifier->game_main_version == FRLG_GAME_CODE)
        base_game = FR_VERSION_ID;
    if(identifier->game_main_version == E_MAIN_GAME_CODE)
        base_game = E_VERSION_ID;
    
    if((base_game == S_VERSION_ID) && (identifier->game_sub_version == R_SUB_GAME_CODE))
        base_game += 1;
    if((base_game == FR_VERSION_ID) && (identifier->game_sub_version == LG_SUB_GAME_CODE))
        base_game += 1;
    
    return base_game;
}

void determine_game_with_save(struct game_identity* identifier, u8 slot, u8 section_pos, u16 total_bytes) {
    u32 game_code = -1;
    if(identifier->game_main_version == UNDETERMINED) {
        game_code = read_int_save((slot * SAVE_SLOT_SIZE) + (section_pos * SECTION_SIZE) + SECTION_GAME_CODE);
        if((game_code != FRLG_GAME_CODE) && (game_code != RS_GAME_CODE)) {
            u8 found = 0;
            for(int i = RS_END_BATTLE_TOWER; i < total_bytes; i++)
                if((*(vu8*)((slot * SAVE_SLOT_SIZE) + (section_pos * SECTION_SIZE) + i))) {
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
                break;
            case FRLG_GAME_CODE:
                identifier->game_main_version = FRLG_MAIN_GAME_CODE;
                break;
            default:
                identifier->game_main_version = E_MAIN_GAME_CODE;
                break;
        }
    }
}