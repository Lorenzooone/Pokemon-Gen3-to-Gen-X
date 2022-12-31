#ifndef MENU_TEXT_HANDLER__
#define MENU_TEXT_HANDLER__

#include "party_handler.h"
#include "gen3_save.h"
#include "multiboot_handler.h"

#define X_TILES 30
#define BASE_Y_SPRITE_TRADE_MENU 0
#define BASE_Y_SPRITE_INCREMENT_TRADE_MENU 24
#define BASE_X_SPRITE_TRADE_MENU 8

#define BASE_Y_SPRITE_INFO_PAGE 0
#define BASE_X_SPRITE_INFO_PAGE 0

void print_game_info(struct game_data_t*, int);
void print_trade_menu(struct game_data_t*, u8, u8, u8, u8);
void print_pokemon_pages(u8, u8, struct gen3_mon_data_unenc*, u8);
void print_main_menu(u8, u8, u8, u8);
void print_multiboot(enum MULTIBOOT_RESULTS);

#endif