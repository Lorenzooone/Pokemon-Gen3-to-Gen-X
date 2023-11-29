#include "base_include.h"
#include "menu_text_handler.h"
#include "input_handler.h"
#include "sprite_handler.h"
#include "print_system.h"
#include "rtc_helpers.h"
#include <agbabi.h>
#include <stddef.h>

#define CLOCK_MENU_X_DISTANCE 2

void print_clock(void) {
    default_reset_screen();

    set_text_y(Y_LIMIT-3);

    REG_IME = 0;
    if(!__agbabi_rtc_init()) {
        __agbabi_datetime_t datetime = __agbabi_rtc_datetime();
        REG_IME = 1;
        u16 year = bcd_decode(datetime[0], 1);
        u8 month = bcd_decode(datetime[0] >> 8, 1);
        u8 day = bcd_decode(datetime[0] >> 16, 1);
        u8 wday = bcd_decode(datetime[0] >> 24, 1);
        u8 hour = bcd_decode(datetime[1] & 0x3F, 1);
        u8 minute = bcd_decode(datetime[1] >> 8, 1);
        u8 second = bcd_decode(datetime[1] >> 16, 1);
        PRINT_FUNCTION("\x0B/\x0B/\x0B \x0B \x0B:\x0B:\x0B\n\n", day, 2, month, 2, year, 2, wday, 2, hour, 2, minute, 2, second, 2);
    }
    else
        REG_IME = 1;
}

void print_new_clock(u32 date, u32 time, u8 update) {
    if(!update)
        return;

    u8 old_screen = get_screen_num();
    set_screen(old_screen + 1);
    reset_screen(BLANK_FILL);

    u16 year = date & 0xFF;
    u8 month = (date >> 8) & 0xFF;
    u8 day = (date >> 16) & 0xFF;
    u8 wday = (date >> 24) & 0xFF;
    u8 hour = time & 0xFF;
    u8 minute = (time >> 8) & 0xFF;
    u8 second = (time >> 16) & 0xFF;
    set_text_x(CLOCK_MENU_X_DISTANCE);
    PRINT_FUNCTION("Year: <\x0B>\n", year, 2);
    set_text_x(CLOCK_MENU_X_DISTANCE);
    PRINT_FUNCTION("Month: <\x0B>\n", month, 2);
    set_text_x(CLOCK_MENU_X_DISTANCE);
    PRINT_FUNCTION("Day: <\x0B>\n", day, 2);
    set_text_x(CLOCK_MENU_X_DISTANCE);
    PRINT_FUNCTION("WDay: <\x0B>\n", wday, 2);
    set_text_x(CLOCK_MENU_X_DISTANCE);
    PRINT_FUNCTION("Hour: <\x0B>\n", hour, 2);
    set_text_x(CLOCK_MENU_X_DISTANCE);
    PRINT_FUNCTION("Minute: <\x0B>\n", minute, 2);
    set_text_x(CLOCK_MENU_X_DISTANCE);
    PRINT_FUNCTION("Second: <\x0B>\n", second, 2);
    set_text_x(CLOCK_MENU_X_DISTANCE);
    PRINT_FUNCTION("Set New Time");
    
    set_text_y(Y_LIMIT-4);
    PRINT_FUNCTION("DD/MM/YY WD hh:mm:ss\n\n\n");
    PRINT_FUNCTION("B: SoftReset");
    set_screen(old_screen);
}
