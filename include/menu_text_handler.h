#ifndef MENU_TEXT_HANDLER__
#define MENU_TEXT_HANDLER__

#include "party_handler.h"
#include "gen3_save.h"
#include "multiboot_handler.h"
#include "config_settings.h"

enum MOVES_PRINTING_TYPE{LEARNED_P, DID_NOT_LEARN_P, LEARNABLE_P};
enum CRASH_REASONS{BAD_SAVE, BAD_TRADE, CARTRIDGE_REMOVED};

void print_game_info(struct game_data_t*, int);
void print_learnable_move(struct gen3_mon_data_unenc*, u16, enum MOVES_PRINTING_TYPE);
void print_crash(enum CRASH_REASONS);
void print_trade_menu(struct game_data_t*, u8, u8, u8, u8);
void print_trade_menu_cancel(u8);
void print_trade_animation_send(struct gen3_mon_data_unenc*);
void print_trade_animation_recv(struct gen3_mon_data_unenc*);
void print_learnable_moves_menu(struct gen3_mon_data_unenc*, u16);
void print_set_nature(u8, struct gen3_mon_data_unenc*);
void print_iv_fix(struct gen3_mon_data_unenc*);
void print_pokemon_pages(u8, u8, struct gen3_mon_data_unenc*, u8);
void print_main_menu(u8, u8, u8, u8, struct game_data_t*, struct game_data_priv_t*);
void print_multiboot_mid_process(u8);
void print_multiboot(enum MULTIBOOT_RESULTS);
void print_start_trade(void);
void print_swap_cartridge_menu(void);
void print_waiting(s8);
void print_saving(void);
void print_loading(void);
void print_evolution_animation(struct gen3_mon_data_unenc*);
void print_invalid(u8);
void print_rejected(u8);
void print_offer_screen(struct game_data_t*, u8, u8);
void print_gen12_settings_menu(u8);
void print_base_settings_menu(struct game_identity*, u8, u8);
void print_offer_options_screen(struct game_data_t*, u8, u8);
void print_trade_options(u8, u8, u8);
void print_colour_settings_menu(u8);
void print_evolution_menu(struct gen3_mon_data_unenc*, u16, u8, u8);
void print_evolution_window(struct gen3_mon_data_unenc*);
void print_cheats_menu(u8);
void print_clock_menu(struct clock_events_t*, struct saved_time_t*, u8);
void print_warning_when_clock_changed(void);
void print_load_warnings(struct game_data_t*, struct game_data_priv_t*);

#endif
