#include "base_include.h"
#include "input_handler.h"
#include "sprite_handler.h"
#include "options_handler.h"
#include "communicator.h"
#include "gen3_clock_events.h"
#include "useful_qualifiers.h"
#include "config_settings.h"

u8 handle_input_multiboot_menu(u16 keys) {
    if(keys & KEY_A)
        return 1;
    return 0;
}

u8 handle_input_info_menu(struct game_data_t* game_data, u8* cursor_y_pos, u8 cursor_x_pos, u16 keys, u8* curr_mon, u8 UNUSED(curr_gen), u8* curr_page) {
    
    u8* options = get_options_trade(cursor_x_pos);
    u8 num_options = get_options_num_trade(cursor_x_pos);
    
    if(num_options == 0)
        return CANCEL_INFO;
    
    if(keys & KEY_B)
        return CANCEL_INFO;
    
    u8 page = *curr_page;
    
    if(*curr_page < FIRST_PAGE) {
        *curr_page = FIRST_PAGE;
        return 1;
    }
    
    if(game_data[cursor_x_pos].party_3_undec[*curr_mon].is_egg && (page != FIRST_PAGE)) {
        *curr_page = FIRST_PAGE;
        return 1;
    }
    
    if(*curr_page > PAGES_TOTAL) {
        *curr_page = PAGES_TOTAL;
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

u8 handle_input_offer_info_menu(struct game_data_t* game_data, u8* cursor_y_pos, const u8** party_selected_mons, u16 keys, u8* curr_page) {
    
    if(keys & KEY_B)
        return CANCEL_INFO;
    
    u8 page = *curr_page;
    
    if(*curr_page < FIRST_PAGE) {
        *curr_page = FIRST_PAGE;
        return 1;
    }
    
    if(game_data[*cursor_y_pos].party_3_undec[*party_selected_mons[*cursor_y_pos]].is_egg && (page != FIRST_PAGE)) {
        *curr_page = FIRST_PAGE;
        return 1;
    }
    
    if(*curr_page > PAGES_TOTAL) {
        *curr_page = PAGES_TOTAL;
        return 1;
    }

    if(keys & KEY_LEFT) {
        if(game_data[*cursor_y_pos].party_3_undec[*party_selected_mons[*cursor_y_pos]].is_egg) {
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
        if(game_data[*cursor_y_pos].party_3_undec[*party_selected_mons[*cursor_y_pos]].is_egg) {
            *curr_page = FIRST_PAGE;
            return 0;
        }
        page += 1;
        if(page <= PAGES_TOTAL) {
            *curr_page += 1;
            return 1;
        }
    }
    else if((keys & KEY_UP) || (keys & KEY_DOWN)) {
        *cursor_y_pos ^= 1;
        if(game_data[*cursor_y_pos].party_3_undec[*party_selected_mons[*cursor_y_pos]].is_egg)
            *curr_page = FIRST_PAGE;
        return 1;
    }
    
    return 0;
}

u8 handle_input_trading_menu(u8* cursor_y_pos, u8* cursor_x_pos, u16 keys, u8 UNUSED(curr_gen), u8 is_own) {
    
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

    if(keys & KEY_B) {
        cursor_x = 0;
        cursor_y = PARTY_SIZE;
    }
    else if(keys & KEY_LEFT) {
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

u8 handle_input_main_menu(u8* cursor_y_pos, u16 keys, u8* update, u32* scanlines_value, u32* scanlines_start_value, u8* enabled_scanlines_editing) {
    u32 value = *scanlines_value;

    switch(*cursor_y_pos) {
        case 0:
            if(keys & KEY_UP)
                *cursor_y_pos = 4;
            else if(keys & KEY_DOWN)
                *cursor_y_pos += 1;
            else if((keys & KEY_RIGHT) || (keys & KEY_A) || (keys & KEY_LEFT)) {
                *enabled_scanlines_editing = !(*enabled_scanlines_editing);
                *update = 1;
            }
            break;
        case 1:
            if(keys & KEY_UP)
                *cursor_y_pos -= 1;
            else if(keys & KEY_DOWN)
                *cursor_y_pos += 1;
            else if((keys & KEY_RIGHT) || (keys & KEY_A)) {
                value += 1;
                if(value >= SCANLINES)
                    value = 0;
                *scanlines_value = value;
                *update = 1;
            }
            else if(keys & KEY_LEFT) {
                if(value == 0)
                    value = SCANLINES;
                value -= 1;
                *scanlines_value = value;
                *update = 1;
            }
            break;
        case 2:
            if(keys & KEY_UP)
                *cursor_y_pos -= 1;
            else if(keys & KEY_DOWN)
                *cursor_y_pos += 1;
            else if((keys & KEY_RIGHT) || (keys & KEY_A)) {
                for(int i = 0; i < 10; i++) {
                    value += 1;
                    if(value >= SCANLINES)
                        value = 0;
                }
                *scanlines_value = value;
                *update = 1;
            }
            else if(keys & KEY_LEFT) {
                for(int i = 0; i < 10; i++) {
                    if(value == 0)
                        value = SCANLINES;
                    value -= 1;
                }
                *scanlines_value = value;
                *update = 1;
            }
            break;
        case 3:
            value = *scanlines_start_value;
            if(keys & KEY_UP)
                *cursor_y_pos -= 1;
            else if(keys & KEY_DOWN)
                *cursor_y_pos += 1;
            else if((keys & KEY_RIGHT) || (keys & KEY_A)) {
                value += 1;
                if(value >= SCANLINES)
                    value = 0;
                *scanlines_start_value = value;
                *update = 1;
            }
            else if(keys & KEY_LEFT) {
                if(value == 0)
                    value = SCANLINES;
                value -= 1;
                *scanlines_start_value = value;
                *update = 1;
            }
            break;
        case 4:
            value = *scanlines_start_value;
            if(keys & KEY_UP)
                *cursor_y_pos -= 1;
            else if(keys & KEY_DOWN)
                *cursor_y_pos = 0;
            else if((keys & KEY_RIGHT) || (keys & KEY_A)) {
                for(int i = 0; i < 10; i++) {
                    value += 1;
                    if(value >= SCANLINES)
                        value = 0;
                }
                *scanlines_start_value = value;
                *update = 1;
            }
            else if(keys & KEY_LEFT) {
                for(int i = 0; i < 10; i++) {
                    if(value == 0)
                        value = SCANLINES;
                    value -= 1;
                }
                *scanlines_start_value = value;
                *update = 1;
            }
            break;
        default:
            *cursor_y_pos = 0;
            break;
    }
    return 0;
}

u8 handle_input_print_read_info(u16 keys) {
    
    if(keys & KEY_A)
        return 1;
    
    return 0;
}

u8 handle_input_nature_menu(u16 keys) {
    
    if(keys & KEY_B)
        return CANCEL_NATURE;
    
    if(keys & KEY_A)
        return CONFIRM_NATURE;
    
    if(keys & KEY_RIGHT)
        return INC_NATURE;
    else if(keys & KEY_LEFT)
        return DEC_NATURE;
    
    return 0;
}

u8 handle_input_iv_fix_menu(u16 keys) {
    
    if(keys & KEY_B)
        return CANCEL_IV_FIX;
    
    if(keys & KEY_A)
        return CONFIRM_IV_FIX;
    
    return 0;
}

u8 handle_input_learnable_message_moves_menu(u16 keys, u8* cursor_x_pos) {
    
    if(keys & KEY_A) {
        if(*cursor_x_pos)
            return DENIED_LEARNING;
        return ENTER_LEARN_MENU;
    }
    
    if(keys & KEY_B)
        *cursor_x_pos = 1;
    else if((keys & KEY_LEFT) || (keys & KEY_RIGHT))
        *cursor_x_pos ^= 1;
    
    return 0;
}

u8 handle_input_learnable_moves_menu(u16 keys, u8* cursor_y_pos) {
    
    if(keys & KEY_A) {
        if((*cursor_y_pos) >= MOVES_SIZE)
            return DO_NOT_FORGET_MOVE;
        return 1+(*cursor_y_pos);
    }
    
    if(keys & KEY_B)
        *cursor_y_pos = MOVES_SIZE;
    else if(keys & KEY_UP) {
        if(!(*cursor_y_pos))
            *cursor_y_pos = MOVES_SIZE;
        else
            *cursor_y_pos -= 1;
    }
    else if(keys & KEY_DOWN) {
        if((*cursor_y_pos) >= MOVES_SIZE)
            *cursor_y_pos = 0;
        else
            *cursor_y_pos += 1;
    }
    
    return 0;
}

u8 handle_input_evolution_menu(u16 keys, u8* cursor_y_pos, u8* update, u16 num_evolutions) {
    if(keys & KEY_B)
        return EXIT_EVOLUTION;

    if(keys & KEY_A)
        return 1;

    u8 new_cursor_y_pos = *cursor_y_pos;
    if(keys & KEY_UP) {
        if(new_cursor_y_pos)
            new_cursor_y_pos -= 1;
        else
            new_cursor_y_pos = num_evolutions-1;
    }
    else if(keys & KEY_DOWN) {
        new_cursor_y_pos += 1;
        if(new_cursor_y_pos >= num_evolutions)
            new_cursor_y_pos = 0;
    }

    if((*cursor_y_pos) != new_cursor_y_pos)
        *update = 1;
    *cursor_y_pos = new_cursor_y_pos;

    return 0;
}

u8 handle_input_gen12_settings_menu(u16 keys, u8* cursor_y_pos, u8* update) {
    if(keys & KEY_B)
        return EXIT_GEN12_SETTINGS;
    
    switch(*cursor_y_pos) {
        case 0:
            if(keys & KEY_UP)
                *cursor_y_pos = 6;
            else if(keys & KEY_DOWN)
                *cursor_y_pos += 1;
            else if((keys & KEY_RIGHT) || (keys & KEY_A)) {
                set_target_int_language(get_target_int_language() + 1);
                *update = 1;
            }
            else if(keys & KEY_LEFT) {
                set_target_int_language(get_target_int_language() - 1);
                *update = 1;
            }
            break;
        case 1:
            if(keys & KEY_UP)
                *cursor_y_pos -= 1;
            else if(keys & KEY_DOWN)
                *cursor_y_pos += 1;
            else if((keys & KEY_RIGHT) || (keys & KEY_A)) {
                set_default_conversion_game(get_default_conversion_game() + 1);
                *update = 1;
            }
            else if(keys & KEY_LEFT) {
                set_default_conversion_game(get_default_conversion_game() - 1);
                *update = 1;
            }
            break;
        case 2:
            if(keys & KEY_UP)
                *cursor_y_pos -= 1;
            else if(keys & KEY_DOWN)
                *cursor_y_pos += 1;
            else if((keys & KEY_RIGHT) || (keys & KEY_A) || (keys & KEY_LEFT)) {
                set_conversion_colo_xd(!get_conversion_colo_xd());
                *update = 1;
            }
            break;
        case 3:
            if(keys & KEY_UP)
                *cursor_y_pos -= 1;
            else if(keys & KEY_DOWN)
                *cursor_y_pos += 1;
            else if((keys & KEY_RIGHT) || (keys & KEY_A) || (keys & KEY_LEFT)) {
                set_gen1_everstone(!get_gen1_everstone());
                *update = 1;
            }
            break;
        case 4:
            if(keys & KEY_UP)
                *cursor_y_pos -= 1;
            else if(keys & KEY_DOWN)
                *cursor_y_pos += 1;
            else if((keys & KEY_RIGHT) || (keys & KEY_A)) {
                for(size_t i = 0; i < MAX_BALL_RETRIES; i++)
                    if(set_applied_ball(get_applied_ball() + (i + 1)))
                        break;
                *update = 1;
            }
            else if(keys & KEY_LEFT) {
                for(size_t i = 0; i < MAX_BALL_RETRIES; i++)
                    if(set_applied_ball(get_applied_ball() - (i + 1)))
                        break;
                *update = 1;
            }
            break;
        case 5:
            if(keys & KEY_UP)
                *cursor_y_pos -= 1;
            else if(keys & KEY_DOWN)
                *cursor_y_pos += 1;
            else if((keys & KEY_RIGHT) || (keys & KEY_A)) {
                increase_egg_met_location();
                *update = 1;
            }
            else if(keys & KEY_LEFT) {
                decrease_egg_met_location();
                *update = 1;
            }
            break;
        case 6:
            if(keys & KEY_UP)
                *cursor_y_pos -= 1;
            else if(keys & KEY_DOWN)
                *cursor_y_pos = 0;
            else if((keys & KEY_RIGHT) || (keys & KEY_A)) {
                for(size_t i = 0; i < 10; i++)
                    increase_egg_met_location();
                *update = 1;
            }
            else if(keys & KEY_LEFT) {
                for(size_t i = 0; i < 10; i++)
                    decrease_egg_met_location();
                *update = 1;
            }
            break;
        default:
            *cursor_y_pos = 0;
            break;
    }

    return 0;
}

u8 handle_input_base_settings_menu(u16 keys, u8* cursor_y_pos, u8* update, struct game_identity* game_identifier, u8 is_loaded) {
    if(keys & KEY_B)
        return EXIT_BASE_SETTINGS;
    
    switch(*cursor_y_pos) {
        case 0:
            if(keys & KEY_UP)
                *cursor_y_pos = BOTTOM_Y_CURSOR_BASE_SETTINGS_MENU_VALUE;
            else if(keys & KEY_DOWN)
                *cursor_y_pos += 1;
            else if((keys & KEY_RIGHT) || (keys & KEY_A)) {
                set_sys_language(get_sys_language() + 1);
                *update = 1;
            }
            else if(keys & KEY_LEFT) {
                set_sys_language(get_sys_language() - 1);
                *update = 1;
            }
            break;
        case 1:
            if(keys & KEY_A)
                return ENTER_GEN12_MENU;
            else if(keys & KEY_UP)
                *cursor_y_pos -= 1;
            else if(keys & KEY_DOWN) {
                if(is_loaded && has_rtc_events(game_identifier))
                    *cursor_y_pos += 1;
                else if(is_loaded && game_identifier->game_sub_version_undetermined)
                    *cursor_y_pos += 2;
                else
                    *cursor_y_pos += 3;
            }
            break;
        case 2:
            if(keys & KEY_A)
                return ENTER_CLOCK_MENU;
            else if(keys & KEY_UP)
                *cursor_y_pos -= 1;
            else if(keys & KEY_DOWN) {
                if(is_loaded && game_identifier->game_sub_version_undetermined)
                    *cursor_y_pos += 1;
                else
                    *cursor_y_pos += 2;
            }
            break;
        case 3:
            if(keys & KEY_UP) {
                if(is_loaded && has_rtc_events(game_identifier))
                    *cursor_y_pos -= 1;
                else
                    *cursor_y_pos -= 2;
            }
            else if(keys & KEY_DOWN)
                *cursor_y_pos += 1;
            else if((keys & KEY_RIGHT) || (keys & KEY_A) || (keys & KEY_LEFT)) {
                change_sub_version(game_identifier);
                *update = 1;
            }
            break;
        case 4:
            if(keys & KEY_A)
                return ENTER_COLOUR_MENU;
            else if(keys & KEY_UP) {
                if(is_loaded && game_identifier->game_sub_version_undetermined)
                    *cursor_y_pos -= 1;
                else if(is_loaded && has_rtc_events(game_identifier))
                    *cursor_y_pos -= 2;
                else
                    *cursor_y_pos -= 3;
            }
            else if(keys & KEY_DOWN)
                *cursor_y_pos = BOTTOM_Y_CURSOR_BASE_SETTINGS_MENU_VALUE;
            break;
        case BOTTOM_Y_CURSOR_BASE_SETTINGS_MENU_VALUE:
            if(keys & KEY_A)
                return ENTER_CHEATS_MENU;
            else if(keys & KEY_UP)
                *cursor_y_pos = 4;
            else if(keys & KEY_DOWN)
                *cursor_y_pos = 0;
            break;
        default:
            *cursor_y_pos = 0;
            break;
    }

    return 0;
}

u8 handle_input_cheats_menu(u16 keys, u8* cursor_y_pos, u8* update) {
    if(keys & KEY_B)
        return EXIT_CHEAT_SETTINGS;
    
    switch(*cursor_y_pos) {
        case 0:
            if(keys & KEY_UP)
                *cursor_y_pos = 4;
            else if(keys & KEY_DOWN)
                *cursor_y_pos += 1;
            else if((keys & KEY_RIGHT) || (keys & KEY_A) || (keys & KEY_LEFT)) {
                set_allow_cross_gen_evos(!get_allow_cross_gen_evos());
                *update = 1;
            }
            break;
        case 1:
            if(keys & KEY_UP)
                *cursor_y_pos -= 1;
            else if(keys & KEY_DOWN)
                *cursor_y_pos += 1;
            else if((keys & KEY_RIGHT) || (keys & KEY_A) || (keys & KEY_LEFT)) {
                set_evolve_without_trade(!get_evolve_without_trade());
                *update = 1;
            }
            break;
        case 2:
            if(keys & KEY_UP)
                *cursor_y_pos -= 1;
            else if(keys & KEY_DOWN)
                *cursor_y_pos += 1;
            else if((keys & KEY_RIGHT) || (keys & KEY_A) || (keys & KEY_LEFT)) {
                set_allow_undistributed_events(!get_allow_undistributed_events());
                *update = 1;
            }
            break;
        case 3:
            if(keys & KEY_UP)
                *cursor_y_pos -= 1;
            else if(keys & KEY_DOWN)
                *cursor_y_pos += 1;
            else if((keys & KEY_RIGHT) || (keys & KEY_A) || (keys & KEY_LEFT)) {
                set_fast_hatch_eggs(!get_fast_hatch_eggs());
                *update = 1;
            }
            break;
        case 4:
            if(keys & KEY_A)
                return 1;
            else if(keys & KEY_UP)
                *cursor_y_pos -= 1;
            else if(keys & KEY_DOWN)
                *cursor_y_pos = 0;
            break;
        default:
            *cursor_y_pos = 0;
            break;
    }

    return 0;
}

u8 handle_input_clock_menu(u16 keys, struct clock_events_t* clock_events, struct saved_time_t* time_change, u8* cursor_y_pos, u8* update) {
    if(keys & KEY_B)
        return EXIT_CLOCK_SETTINGS;
    
    switch(*cursor_y_pos) {
        case 0:
            if(keys & KEY_UP)
                *cursor_y_pos = BOTTOM_Y_CURSOR_CLOCK_SETTINGS_MENU_VALUE;
            else if(keys & KEY_DOWN)
                *cursor_y_pos += 2;
            else if((keys & KEY_RIGHT) || (keys & KEY_A) || (keys & KEY_LEFT)) {
                change_time_of_day(clock_events, time_change);
                *update = 1;
            }
            break;
        case 2:
            if(keys & KEY_UP)
                *cursor_y_pos -= 2;
            else if(keys & KEY_DOWN)
                *cursor_y_pos += 2;
            else if((keys & KEY_RIGHT) || (keys & KEY_A) || (keys & KEY_LEFT)) {
                change_tide(clock_events, time_change);
                *update = 1;
            }
            break;
        case 4:
            if(keys & KEY_UP)
                *cursor_y_pos -= 2;
            else if(keys & KEY_DOWN)
                *cursor_y_pos += 2;
            else if((keys & KEY_RIGHT) || (keys & KEY_A) || (keys & KEY_LEFT)) {
                if(is_rtc_reset_enabled(clock_events))
                    disable_rtc_reset(clock_events);
                else
                    enable_rtc_reset(clock_events);
                *update = 1;
            }
            break;
        case 6:
            if(keys & KEY_UP)
                *cursor_y_pos -= 2;
            else if(keys & KEY_DOWN)
                *cursor_y_pos += 1;
            else if((keys & KEY_RIGHT) || (keys & KEY_A)) {
                time_change->d += 1;
                *update = 1;
            }
            else if(keys & KEY_LEFT) {
                time_change->d -= 1;
                *update = 1;
            }
            break;
        case 7:
            if(keys & KEY_UP)
                *cursor_y_pos -= 1;
            else if(keys & KEY_DOWN)
                *cursor_y_pos += 1;
            else if((keys & KEY_RIGHT) || (keys & KEY_A)) {
                if(time_change->h < MAX_HOURS-1)
                    time_change->h += 1;
                else
                    time_change->h = 0;
                *update = 1;
            }
            else if(keys & KEY_LEFT) {
                if(time_change->h)
                    time_change->h -= 1;
                else
                    time_change->h = MAX_HOURS-1;
                *update = 1;
            }
            break;
        case 8:
            if(keys & KEY_UP)
                *cursor_y_pos -= 1;
            else if(keys & KEY_DOWN)
                *cursor_y_pos += 1;
            else if((keys & KEY_RIGHT) || (keys & KEY_A)) {
                if(time_change->m < MAX_MINUTES-1)
                    time_change->m += 1;
                else
                    time_change->m = 0;
                *update = 1;
            }
            else if(keys & KEY_LEFT) {
                if(time_change->m)
                    time_change->m -= 1;
                else
                    time_change->m = MAX_MINUTES-1;
                *update = 1;
            }
            break;
        case 9:
            if(keys & KEY_UP)
                *cursor_y_pos -= 1;
            else if(keys & KEY_DOWN)
                *cursor_y_pos = BOTTOM_Y_CURSOR_CLOCK_SETTINGS_MENU_VALUE;
            else if((keys & KEY_RIGHT) || (keys & KEY_A)) {
                if(time_change->s < MAX_SECONDS-1)
                    time_change->s += 1;
                else
                    time_change->s = 0;
                *update = 1;
            }
            else if(keys & KEY_LEFT) {
                if(time_change->s)
                    time_change->s -= 1;
                else
                    time_change->s = MAX_SECONDS-1;
                *update = 1;
            }
            break;
        case BOTTOM_Y_CURSOR_CLOCK_SETTINGS_MENU_VALUE:
            if(keys & KEY_A)
                return 1;
            else if(keys & KEY_UP)
                *cursor_y_pos = 9;
            else if(keys & KEY_DOWN)
                *cursor_y_pos = 0;
            break;
        default:
            *cursor_y_pos = 0;
            break;
    }

    return 0;
}

u8 handle_input_clock_warning_menu(u16 keys, u8* cursor_x_pos) {
    if((keys & KEY_A) && (*cursor_x_pos))
        return EXIT_CLOCK_WARNING_SETTINGS;
    if(keys & KEY_A)
        return 1;

    if(keys & KEY_B)
        *cursor_x_pos = 1;
    else if((keys & KEY_LEFT) || (keys & KEY_RIGHT))
        *cursor_x_pos ^= 1;

    return 0;
}

u8 handle_input_colours_menu(u16 keys, u8* cursor_y_pos, u8* cursor_x_pos, u8* update) {
    
    if((keys & KEY_B) && (!(*cursor_x_pos)))
        return EXIT_COLOURS_SETTINGS;

    if(keys & KEY_B)
        *cursor_x_pos = 0;
    else if(keys & KEY_A) {
        if(!(*cursor_x_pos))
            *cursor_x_pos += 1;
        else {
            set_single_colour(*cursor_y_pos, (*cursor_x_pos)-1, get_single_colour(*cursor_y_pos, (*cursor_x_pos)-1)+1);
            *update = 1;
        }
    }
    else if((keys & KEY_LEFT) && (*cursor_x_pos))
        *cursor_x_pos -= 1;
    else if((keys & KEY_RIGHT) && ((*cursor_x_pos) < NUM_SUB_COLOURS))
        *cursor_x_pos += 1;
    else if((keys & KEY_UP) && (*cursor_x_pos)) {
        set_single_colour(*cursor_y_pos, (*cursor_x_pos)-1, get_single_colour(*cursor_y_pos, (*cursor_x_pos)-1) + 1);
        *update = 1;
    }
    else if((keys & KEY_DOWN) && (*cursor_x_pos)) {
        set_single_colour(*cursor_y_pos, (*cursor_x_pos)-1, get_single_colour(*cursor_y_pos, (*cursor_x_pos)-1) - 1);
        *update = 1;
    }
    else if(keys & KEY_UP) {
        if(*cursor_y_pos)
            *cursor_y_pos -= 1;
        else
            *cursor_y_pos = NUM_COLOURS-1;
    }
    else if(keys & KEY_DOWN) {
        if((*cursor_y_pos) >= (NUM_COLOURS-1))
            *cursor_y_pos = 0;
        else
            *cursor_y_pos += 1;
    }
    
    return 0;
}

u8 handle_input_offer_options(u16 keys, u8* cursor_y_pos, u8* cursor_x_pos) {
    
    if(keys & KEY_B)
        return 1 + 1;

    if(keys & KEY_A) {
        if(!(*cursor_x_pos))
            return 1 + (*cursor_y_pos);
        else
            return OFFER_INFO_DISPLAY + (*cursor_y_pos);
    }
    
    if((keys & KEY_LEFT) || (keys & KEY_RIGHT))
        *cursor_x_pos ^= 1;
        
    if((keys & KEY_UP) || (keys & KEY_DOWN))
        *cursor_y_pos ^= 1;
    
    return 0;
}

u8 handle_input_trade_options(u16 keys, u8* cursor_x_pos) {

    if(keys & KEY_B)
        return CANCEL_TRADE_OPTIONS;

    if(keys & KEY_A)
        return 1 + (*cursor_x_pos);

    if((keys & KEY_LEFT) || (keys & KEY_RIGHT))
        *cursor_x_pos ^= 1;

    //if((keys & KEY_LEFT) && ((*cursor_x_pos) == 1))
    //    cursor_x_pos = 0;
    //if((keys & KEY_RIGHT) && ((*cursor_x_pos) == 0))
    //    cursor_x_pos = 1;

    return 0;
}

u8 handle_input_swap_cartridge_menu(u16 keys) {

    if(keys & KEY_A)
        return 1;

    return 0;
}

u8 handle_input_trade_setup(u16 keys, u8 UNUSED(curr_gen)) {

    if(keys & KEY_B) {
        if((get_start_state_raw() != START_TRADE_PAR) || (get_transferred(0) == 0)) {
            if(get_start_state_raw() != START_TRADE_DON)
                return CANCEL_TRADE_START;
        }
    }
    /*if(keys & KEY_B) {
        return CANCEL_TRADE_START;
    }*/

    return 0;
}
