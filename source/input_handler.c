#include <gba.h>
#include "input_handler.h"
#include "options_handler.h"
#include "communicator.h"

u8 handle_input_multiboot_menu(u16 keys) {
    if(keys & KEY_A)
        return 1;
    return 0;
}

u8 handle_input_info_menu(struct game_data_t* game_data, u8* cursor_y_pos, u8 cursor_x_pos, u16 keys, u8* curr_mon, u8 curr_gen, u8* curr_page) {
    
    u8* options = get_options_trade(cursor_x_pos);
    u8 num_options = get_options_num_trade(cursor_x_pos);
    
    if(num_options == 0)
        return CANCEL_INFO;
    
    if(keys & KEY_B)
        return CANCEL_INFO;
    
    u8 page = *curr_page;
    u8 mon = *curr_mon;
    u8 cursor_y = *cursor_y_pos;
    
    if(*curr_page < FIRST_PAGE) {
        *curr_page = FIRST_PAGE;
        return 1;
    }
    
    if(*curr_page > PAGES_TOTAL) {
        *curr_page = PAGES_TOTAL;
        return 1;
    }
    
    if(game_data[cursor_x_pos].party_3_undec[*curr_mon].is_egg && (page != FIRST_PAGE)) {
        *curr_page = FIRST_PAGE;
        return 1;
    }

    if(keys & KEY_LEFT) {
        if(game_data[cursor_x_pos].party_3_undec[*curr_mon].is_egg) {
            *curr_page = FIRST_PAGE;
            return 0;
        }
        page -= 1;
        if(page >= FIRST_PAGE) {
            *curr_page -= 1;
            return 1;
        }
    }
    else if(keys & KEY_RIGHT) {
        if(game_data[cursor_x_pos].party_3_undec[*curr_mon].is_egg) {
            *curr_page = FIRST_PAGE;
            return 0;
        }
        page += 1;
        if(page <= PAGES_TOTAL) {
            *curr_page += 1;
            return 1;
        }
    }
    else if(keys & KEY_UP) {
        if(num_options <= 1)
            return 0;
        if(*cursor_y_pos == 0)
            *cursor_y_pos = num_options-1;
        else
            *cursor_y_pos -= 1;
        *curr_mon = options[*cursor_y_pos];
        if(game_data[cursor_x_pos].party_3_undec[*curr_mon].is_egg)
            *curr_page = FIRST_PAGE;
        return 1;
    }
    else if(keys & KEY_DOWN) {
        if(num_options <= 1)
            return 0;
        if(*cursor_y_pos >= (num_options-1))
            *cursor_y_pos = 0;
        else
            *cursor_y_pos += 1;
        *curr_mon = options[*cursor_y_pos];
        if(game_data[cursor_x_pos].party_3_undec[*curr_mon].is_egg)
            *curr_page = FIRST_PAGE;
        return 1;
    }
    
    return 0;
}

u8 handle_input_trading_menu(u8* cursor_y_pos, u8* cursor_x_pos, u16 keys, u8 curr_gen, u8 is_own) {
    
    u8* options[2];
    u8 num_options[2];
    for(int i = 0; i < 2; i++) {
        options[i] = get_options_trade(i);
        num_options[i] = get_options_num_trade(i);
    }
    
    if(num_options[0] == 0)
        return CANCEL_TRADING;
    if((!is_own) && (num_options[1] == 0))
        return CANCEL_TRADING;
    
    u8 cursor_x = *cursor_x_pos;
    u8 cursor_y = *cursor_y_pos;
    
    if(keys & KEY_A) {
        if(cursor_y == PARTY_SIZE)
            return CANCEL_TRADING;
        return options[cursor_x][cursor_y]+1;
    }

    if(keys & KEY_LEFT) {
        cursor_x = 0;
        if(cursor_y >= num_options[0])
            cursor_y = num_options[0]-1;
    }
    else if(keys & KEY_RIGHT) {
        cursor_x = 1;
        if((cursor_y >= num_options[1]) && (num_options[1] > 0))
            cursor_y = num_options[1]-1;
        else if(num_options[1] == 0)
            cursor_x = 0;
    }
    else if(keys & KEY_UP) {
        if((cursor_y > 0) && (cursor_y < num_options[cursor_x]))
            cursor_y -= 1;
        else if(cursor_y == PARTY_SIZE) {
            cursor_x = 0;
            cursor_y = num_options[cursor_x]-1;
        }
        else if(cursor_x == 0)
            cursor_y = PARTY_SIZE;
        else if(num_options[1] > 0)
            cursor_y = num_options[1]-1;
        else {
            cursor_x = 0;
            cursor_y = PARTY_SIZE;
        }
    }
    else if(keys & KEY_DOWN) {
        cursor_y += 1;
        if(cursor_y >= (PARTY_SIZE+1)) {
            cursor_y = 0;
            cursor_x = 0;
        }
        else if((cursor_x == 0) && (cursor_y >= num_options[0]))
            cursor_y = PARTY_SIZE;
        else if(cursor_y >= num_options[cursor_x])
            cursor_y = 0;
        if(num_options[cursor_x] == 0) {
            cursor_x = 0;
            cursor_y = PARTY_SIZE;
        }
    }
    
    *cursor_x_pos = cursor_x;
    *cursor_y_pos = cursor_y;
    
    return 0;
}

u8 handle_input_main_menu(u8* cursor_y_pos, u16 keys, u8* update, u8* target, u8* region, u8* master) {

    u8* options = get_options_main();
    
    u8 curr_gen = *target;
    if(curr_gen >= TOTAL_GENS)
        curr_gen = 2;
    
    curr_gen = options[curr_gen];
    
    switch(*cursor_y_pos) {
        case 0:
            if((keys & KEY_RIGHT) && (get_number_of_higher_ordered_options(options, curr_gen, MAIN_OPTIONS_NUM) > 0)) {
                (*target) += 1;
                (*update) = 1;
            }
            else if((keys & KEY_LEFT) && (get_number_of_lower_ordered_options(options, curr_gen, MAIN_OPTIONS_NUM) > 0)) {
                (*target) -= 1;
                (*update) = 1;
            }
            else if((keys & KEY_A) && (get_number_of_higher_ordered_options(options, curr_gen, MAIN_OPTIONS_NUM) > 0)) {
                (*target) += 1;
                (*update) = 1;
            }
            else if((keys & KEY_A) && (get_number_of_lower_ordered_options(options, curr_gen, MAIN_OPTIONS_NUM) > 0)) {
                (*target) -= 1;
                (*update) = 1;
            }
            else if(keys & KEY_DOWN) {
                if(curr_gen == 3)
                    (*cursor_y_pos) += 2;
                else
                    (*cursor_y_pos) += 1;
            }
            else if(keys & KEY_UP) {
                (*cursor_y_pos) = 8;
            }
            break;
        case 1:
            if(curr_gen == 3)
                (*cursor_y_pos) += 1;
            else if((keys & KEY_RIGHT) && (!(*region))) {
                (*region) = 1;
                (*update) = 1;
            }
            else if((keys & KEY_LEFT) && ((*region))) {
                (*region) = 0;
                (*update) = 1;
            }
            else if((keys & KEY_A) && (!(*region))) {
                (*region) = 1;
                (*update) = 1;
            }
            else if((keys & KEY_A) && ((*region))) {
                (*region) = 0;
                (*update) = 1;
            }
            else if(keys & KEY_DOWN) {
                (*cursor_y_pos) += 1;
            }
            else if(keys & KEY_UP) {
                (*cursor_y_pos) -= 1;
            }
            break;
        case 2:
            if((keys & KEY_RIGHT) && (!(*master))) {
                (*master) = 1;
                (*update) = 1;
            }
            else if((keys & KEY_LEFT) && ((*master))) {
                (*master) = 0;
                (*update) = 1;
            }
            else if((keys & KEY_A) && (!(*master))) {
                (*master) = 1;
                (*update) = 1;
            }
            else if((keys & KEY_A) && ((*master))) {
                (*master) = 0;
                (*update) = 1;
            }
            else if(keys & KEY_DOWN)
                (*cursor_y_pos) += 1;
            else if(keys & KEY_UP) {
                if(curr_gen == 3)
                    (*cursor_y_pos) -= 2;
                else
                    (*cursor_y_pos) -= 1;
            }
            break;
        case 3:
            if((keys & KEY_A) && (get_valid_options_main()))
                return curr_gen;
            else if(keys & KEY_DOWN)
                (*cursor_y_pos) += 1;
            else if(keys & KEY_UP)
                (*cursor_y_pos) -= 1;
            break;
        default:
            if(keys & KEY_A) {
                return START_MULTIBOOT;
            }
            else if((keys & KEY_DOWN) && (get_valid_options_main())) {
                (*cursor_y_pos) = 8;
            }
            else if((keys & KEY_UP) && (get_valid_options_main())) {
                (*cursor_y_pos) = 3;
            }
            break;
        case 8:
            if(keys & KEY_A) {
                return VIEW_OWN_PARTY + curr_gen;
            }
            else if((keys & KEY_DOWN) && (get_valid_options_main())) {
                (*cursor_y_pos) = 0;
            }
            else if((keys & KEY_UP) && (get_valid_options_main())) {
                (*cursor_y_pos) = 4;
            }
            break;
    }
    return 0;
}

u8 handle_input_trade_setup(u16 keys, u8 curr_gen) {

    if(keys & KEY_B) {
        if((get_start_state_raw() != START_TRADE_PAR) || (get_transferred(0) == 0)) {
            //if(get_start_state_raw() != START_TRADE_DON)
                return CANCEL_TRADE_START;
        }
    }
    /*if(keys & KEY_B) {
        return CANCEL_TRADE_START;
    }*/

    return 0;
}