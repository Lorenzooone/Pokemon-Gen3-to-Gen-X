#ifndef INPUT_HANDLER__
#define INPUT_HANDLER__

#include "party_handler.h"
#include "gen3_save.h"

#define CANCEL_INFO 0xFF
#define CANCEL_TRADING 0xFF
#define CANCEL_NATURE 0xFF
#define CONFIRM_NATURE 3
#define INC_NATURE 1
#define DEC_NATURE 2
#define CANCEL_TRADE_START 0xFF
#define CANCEL_TRADE_OPTIONS 0xFF
#define OFFER_INFO_DISPLAY 0x12
#define START_MULTIBOOT 0x49
#define VIEW_OWN_PARTY 0x65

#define PAGES_TOTAL 5
#define FIRST_PAGE 1

u8 handle_input_multiboot_menu(u16);
u8 handle_input_info_menu(struct game_data_t*, u8*, u8, u16, u8*, u8, u8*);
u8 handle_input_offer_info_menu(struct game_data_t*, u8*, const u8**, u16, u8*);
u8 handle_input_trading_menu(u8*, u8*, u16, u8, u8);
u8 handle_input_main_menu(u8*, u16, u8*, u8*, u8*, u8*);
u8 handle_input_trade_options(u16, u8*);
u8 handle_input_nature_menu(u16);
u8 handle_input_offer_options(u16, u8*, u8*);
u8 handle_input_trade_setup(u16, u8);

#endif