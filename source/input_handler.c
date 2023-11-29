#include "base_include.h"
#include "input_handler.h"
#include "sprite_handler.h"
#include "useful_qualifiers.h"
#include "rtc_helpers.h"

u8 handle_input_new_clock_menu(u16 keys, u32* date, u32* time, u8* cursor_y_pos) {
    if(keys & KEY_B)
        return 0;

    if((keys & KEY_UP) || (keys & KEY_DOWN)) {
        if(((*cursor_y_pos) == 0) && (keys & KEY_UP))
            *cursor_y_pos = 7;
        else if(keys & KEY_UP)
            *cursor_y_pos -= 1;
        else if(keys & KEY_DOWN)
            *cursor_y_pos += 1;
        if((*cursor_y_pos) == 8)
            *cursor_y_pos = 0;
        return 0;
     }

    if(((*cursor_y_pos) == 7) && (keys & KEY_A))
        return SET_NEW_CLOCK;

    u32 new_date = *date;
    u32 new_time = *time;

    switch(*cursor_y_pos) {
        case 0:
        case 1:
        case 2:
        case 3:
            if((keys & KEY_A) || (keys & KEY_RIGHT))
                new_date = update_rtc_byte(new_date, *cursor_y_pos, 1);
            else if(keys & KEY_LEFT)
                new_date = update_rtc_byte(new_date, *cursor_y_pos, 0);
            break;
        case 4:
        case 5:
        case 6:
            if((keys & KEY_A) || (keys & KEY_RIGHT))
                new_time = update_rtc_byte(new_time, (*cursor_y_pos) - 4, 1);
            else if(keys & KEY_LEFT)
                new_time = update_rtc_byte(new_time, (*cursor_y_pos) - 4, 0);
            break;
        default:
            break;
    }
    
    new_date = sanitize_date(new_date);
    new_time = sanitize_time(new_time);
    
    if(new_date != (*date)) {
        *date = new_date;
        return 1;
    }
    
    if(new_time != (*time)) {
        *time = new_time;
        return 1;
    }

    return 0;
}
