#ifndef MENU_TEXT_HANDLER__
#define MENU_TEXT_HANDLER__

#include "party_handler.h"
#include "gen3_save.h"
#include "multiboot_handler.h"

void print_game_info(struct game_data_t*, int);
void print_trade_menu(struct game_data_t*, u8, u8, u8, u8);
void print_trade_menu_cancel(u8);
void print_set_nature(u8, struct gen3_mon_data_unenc*);
void print_iv_fix(struct gen3_mon_data_unenc*);
void print_pokemon_pages(u8, u8, struct gen3_mon_data_unenc*, u8);
void print_main_menu(u8, u8, u8, u8);
void print_multiboot_mid_process(u8);
void print_multiboot(enum MULTIBOOT_RESULTS);
void print_start_trade(void);
void print_waiting(void);
void print_saving(void);
void print_invalid(u8);
void print_offer_screen(struct game_data_t*, u8, u8);
void print_offer_options_screen(struct game_data_t*, u8, u8);
void print_trade_options(u8, u8);

#endif
