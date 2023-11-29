#ifndef RTC_HELPERS__
#define RTC_HELPERS__

#define NUM_MONTHS 12
#define LEAP_YEAR_EXTRA_DAY_MONTH 2
#define ERROR_OUT_BCD 0xFF

u32 update_rtc_byte(u32, u8, u8);
u32 sanitize_date(u32);
u32 sanitize_time(u32);
u32 bcd_decode(u32, u8);
u32 bcd_encode_bytes(u32, u8);

#endif
