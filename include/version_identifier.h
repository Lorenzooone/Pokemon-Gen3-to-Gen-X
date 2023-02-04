#ifndef VERSION_IDENTIFIER__
#define VERSION_IDENTIFIER__

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

#define S_VERSION_ID 1
#define R_VERSION_ID (S_VERSION_ID+1)
#define E_VERSION_ID 3
#define FR_VERSION_ID 4
#define LG_VERSION_ID (FR_VERSION_ID+1)

struct game_identity {
    u8 game_is_jp;
    u8 game_main_version;
    u8 game_sub_version;
};

void init_game_identifier(struct game_identity*);
void get_game_id(struct game_identity*);
void determine_game_with_save(struct game_identity*, u8, u8, u16);
u8 id_to_version(struct game_identity*);

#endif