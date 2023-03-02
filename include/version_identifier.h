#ifndef VERSION_IDENTIFIER__
#define VERSION_IDENTIFIER__

#include "useful_qualifiers.h"

#define UNDETERMINED 0xFF
#define NUMBER_OF_GAMES 5
#define RS_MAIN_GAME_CODE 0x0
#define R_SUB_GAME_CODE 0x0
#define S_SUB_GAME_CODE 0x1
#define FRLG_MAIN_GAME_CODE 0x1
#define FR_SUB_GAME_CODE 0x0
#define LG_SUB_GAME_CODE 0x1
#define E_MAIN_GAME_CODE 0x2
#define E_SUB_GAME_CODE 0x0

#define NUM_MAIN_GAME_ID 3

#define S_VERSION_ID 1
#define R_VERSION_ID (S_VERSION_ID+1)
#define E_VERSION_ID 3
#define FR_VERSION_ID 4
#define LG_VERSION_ID (FR_VERSION_ID+1)
#define FIRST_VERSION_ID S_VERSION_ID

struct game_identity {
    u8 language;
    u8 game_main_version;
    u8 game_sub_version;
    u8 language_is_sys : 1;
    u8 game_sub_version_undetermined : 1;
    u8 padding : 6;
} PACKED ALIGNED(4);

void init_game_identifier(struct game_identity*);
u8 is_trainer_name_japanese(u8*);
void get_game_id(struct game_identity*);
void change_sub_version(struct game_identity*);
void determine_game_with_save(struct game_identity*, u8, u8, u16);
u8 determine_possible_main_game_for_slot(u8, u8, u16);
u8 id_to_version(struct game_identity*);

#endif
