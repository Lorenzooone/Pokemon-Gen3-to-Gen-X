#ifndef OPTIONS_HANDLER__
#define OPTIONS_HANDLER__

#include "gen3_save.h"
#include "party_handler.h"

#define TRADE_OPTIONS_NO_OPTION 0xFF

#define TRADE_OPTIONS_NUM PARTY_SIZE

#define MAIN_OPTIONS_NO_OPTION 0
#define MAIN_OPTIONS_NUM TOTAL_GENS

u8 get_number_of_higher_ordered_options(u8*, u8, u8);
u8 get_number_of_lower_ordered_options(u8*, u8, u8);

u8 is_valid_for_gen(struct game_data_t*, u8);

u8* get_options_main();
u8 get_valid_options_main();
void prepare_main_options(struct game_data_t*);

u8* get_options_trade(int);
u8 get_options_num_trade(int);
void prepare_options_trade(struct game_data_t*, u8, u8);

#endif
