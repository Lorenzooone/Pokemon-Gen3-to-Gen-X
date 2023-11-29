#include "base_include.h"
#include "rtc_helpers.h"
#include "useful_qualifiers.h"
#include <stddef.h>
#include <agbabi.h>

u8 is_leap_year(u16);
u8 get_max_days_in_month(u16, u8);
u32 sanitize_rtc_byte(u32, u8, u8, u8);

const u8 num_days_per_month[NUM_MONTHS] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

u32 sanitize_rtc_byte(u32 data, u8 num_byte, u8 upper, u8 lower) {
    u32 mask = ~(0xFF << (8 * num_byte));
    if(((data & (~mask)) >> (8 * num_byte)) == ((lower - 1) & 0xFF))
        return (data & mask) | (upper << (8 * num_byte));
    if((((data & (~mask)) >> (8 * num_byte)) >= ((upper + 1) & 0xFF)) || (((data & (~mask)) >> (8 * num_byte)) < lower))
        return (data & mask) | (lower << (8 * num_byte));
    return data;
}

u32 update_rtc_byte(u32 data, u8 num_byte, u8 is_positive) {
    u32 mask = ~(0xFF << (8 * num_byte));
    if(is_positive)
        return (data & mask) | (((data & (~mask)) + (1 << (8 * num_byte))) & (~mask));
    return (data & mask) | (((data & (~mask)) - (1 << (8 * num_byte))) & (~mask));
}

u32 sanitize_date(u32 date) {
    date = sanitize_rtc_byte(date, 0, 99, 0);
    date = sanitize_rtc_byte(date, 1, 12, 1);
    date = sanitize_rtc_byte(date, 2, get_max_days_in_month(date & 0xFF, (date >> 8) & 0xFF), 1);
    date = sanitize_rtc_byte(date, 3, 6, 0);
    return date;
}

u32 sanitize_time(u32 time) {
    time = sanitize_rtc_byte(time, 0, 23, 0);
    time = sanitize_rtc_byte(time, 1, 59, 0);
    time = sanitize_rtc_byte(time, 2, 59, 0);
    return time;
}

u8 is_leap_year(u16 year) {
    if((((year % 4) == 0) && ((year % 100) != 0)) || ((year % 400) == 0))
        return 1;
    return 0;
}

u8 get_max_days_in_month(u16 year, u8 month) {
    u8 num_days = num_days_per_month[month - 1];
    if(is_leap_year(year) && (month == (LEAP_YEAR_EXTRA_DAY_MONTH)))
        num_days++;
    return num_days;
}

u32 bcd_decode(u32 data, u8 num_bytes) {
    u32 ret_val = 0;
    u32 mult = 1;
    for(int i = 0; i < (num_bytes * 2); i++) {
        if((data & 0xF) > 9)
            return ERROR_OUT_BCD;
        ret_val += (data & 0xF) * mult;
        data >>= 4;
        mult *= 10;
    }
    return ret_val;
}

u32 bcd_encode_bytes(u32 data, u8 num_bytes) {
    u32 ret_val = 0;
    for(int i = 0; i < num_bytes; i++) {
        u32 value = data & 0xFF;
        u32 out_val = 0;
        for(int j = 0; j < 2; j++) {
            out_val |= (value % 10) << (4 * j);
            value /= 10;
        }
        if(value)
            out_val = ERROR_OUT_BCD;
        ret_val |= out_val << (8 * i);
        data >>= 8;
    }
    return ret_val;
}
