#include <gba.h>
#include "options_handler.h"

u8 options_main[3];
u8 valid_options_main;
u8 options_trade[2][PARTY_SIZE];
u8 num_options_trade[2];

void set_valid_options_main(struct game_data_t* game_data) {
    valid_options_main = (game_data->party_1.total > 0 ? 1: 0) | (game_data->party_2.total > 0 ? 2: 0) | (game_data->party_3.total > 0 ? 4: 0);
}

void fill_options_main_array(struct game_data_t* game_data) {
    u8 curr_slot = MAIN_OPTIONS_NO_OPTION;
    for(int i = 0; i < MAIN_OPTIONS_NUM; i++)
        options_main[i] = 0;
    if(game_data->party_1.total > 0) {
        options_main[curr_slot++] = 1;
        for(int i = curr_slot; i < MAIN_OPTIONS_NUM; i++)
            options_main[i] = 1;
    }
    if(game_data->party_2.total > 0) {
        options_main[curr_slot++] = 2;
        for(int i = curr_slot; i < MAIN_OPTIONS_NUM; i++)
            options_main[i] = 2;
    }
    if(game_data->party_3.total > 0) {
        options_main[curr_slot++] = 3;
        for(int i = curr_slot; i < MAIN_OPTIONS_NUM; i++)
            options_main[i] = 3;
    }
}

void prepare_main_options(struct game_data_t* game_data) {
    fill_options_main_array(game_data);
    set_valid_options_main(game_data);
}

u8 get_valid_options_main() {
    return valid_options_main;
}

u8* get_options_main() {
    return options_main;
}

u8 get_number_of_higher_ordered_options(u8* options, u8 curr_option, u8 limit) {
    u8 curr_num = 0;
    u8 highest_found = curr_option;
    for(int i = 0; i < limit; i++)
        if(options[i] > highest_found) {
            curr_num++;
            highest_found = options[i];
        }
    return curr_num;
}

u8 get_number_of_lower_ordered_options(u8* options, u8 curr_option, u8 limit) {
    u8 curr_num = 0;
    u8 lowest_found = curr_option;
    for(int i = limit-1; i >= 0; i--)
        if(options[i] < lowest_found) {
            curr_num++;
            lowest_found = options[i];
        }
    return curr_num;
}

u8* get_options_trade(int index) {
    return options_trade[index];
}

u8 get_options_num_trade(int index) {
    return num_options_trade[index];
}

int prepare_trade_options_num(u8* options) {
    for(int i = 0; i < PARTY_SIZE; i++)
        if(options[i] == TRADE_OPTIONS_NO_OPTION)
            return i;
    return PARTY_SIZE;
}

void fill_trade_options(u8* options, struct game_data_t* game_data, u8 curr_gen) {
    
    struct gen3_mon_data_unenc* party = game_data->party_3_undec;
    u8 real_party_size = game_data->party_3.total;
    if(real_party_size > PARTY_SIZE)
        real_party_size = PARTY_SIZE;
    
    u8 curr_slot = 0;
    for(int i = 0; i < real_party_size; i++) {
        u8 is_valid = party[i].is_valid_gen3;
        if(curr_gen == 2)
            is_valid = party[i].is_valid_gen2;
        if(curr_gen == 1)
            is_valid = party[i].is_valid_gen1;

        if(is_valid)
            options[curr_slot++] = i;
    }
    for(int i = curr_slot; i < PARTY_SIZE; i++)
        options[i] = TRADE_OPTIONS_NO_OPTION;
}

void prepare_options_trade(struct game_data_t* game_data, u8 curr_gen, u8 is_own) {
    fill_trade_options(options_trade[0], &game_data[0], curr_gen);
    if(!is_own)
        fill_trade_options(options_trade[1], &game_data[1], curr_gen);
    else
        options_trade[1][0] = TRADE_OPTIONS_NO_OPTION;
    for(int i = 0; i < 2; i++)
        num_options_trade[i] = prepare_trade_options_num(options_trade[i]);
}