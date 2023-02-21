#ifndef MENU_TEXT_HANDLER__
#define MENU_TEXT_HANDLER__

#include "party_handler.h"
#include "gen3_save.h"
#include "multiboot_handler.h"

enum MOVES_PRINTING_TYPE{LEARNT_P, DID_NOT_LEARN_P, LEARNABLE_P};
enum CRASH_REASONS{BAD_SAVE, BAD_TRADE, CARTRIDGE_REMOVED};

void print_learnable_move(struct gen3_mon_data_unenc*, u16, enum MOVES_PRINTING_TYPE);
void print_game_info(struct game_data_t*, int);
void print_crash(enum CRASH_REASONS);
void print_trade_menu(struct game_data_t*, u8, u8, u8, u8);
void print_trade_menu_cancel(u8);
void print_trade_animation_send(struct gen3_mon_data_unenc*);
void print_trade_animation_recv(struct gen3_mon_data_unenc*);
void print_learnable_moves_menu(struct gen3_mon_data_unenc*, u16);
void print_set_nature(u8, struct gen3_mon_data_unenc*);
void print_iv_fix(struct gen3_mon_data_unenc*);
void print_pokemon_pages(u8, u8, struct gen3_mon_data_unenc*, u8);
void print_main_menu(u8, u8, u8, u8);
void print_multiboot_mid_process(u8);
void print_multiboot(enum MULTIBOOT_RESULTS);
void print_start_trade(void);
void print_swap_cartridge_menu(void);
void print_waiting(s8);
void print_saving(void);
void print_loading(void);
void print_evolution_animation(struct gen3_mon_data_unenc*);
void print_invalid(u8);
void print_offer_screen(struct game_data_t*, u8, u8);
void print_offer_options_screen(struct game_data_t*, u8, u8);
void print_trade_options(u8, u8);

#endif
