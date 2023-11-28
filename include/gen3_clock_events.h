#ifndef GEN3_CLOCK_EVENTS__
#define GEN3_CLOCK_EVENTS__

#include "version_identifier.h"
#include "gen3_save.h"
#include "gen3_clock_events_t.h"

u8 has_rtc_events(struct game_identity*);
void normalize_time(struct saved_time_t*);
void enable_rtc_reset(struct clock_events_t*);
void disable_rtc_reset(struct clock_events_t*);
u8 is_rtc_reset_enabled(struct clock_events_t*);
u8 is_daily_update_safe(struct game_data_t*, struct clock_events_t*, struct saved_time_t*);
void init_rtc_time(void);
void run_daily_update(struct game_data_t*, struct clock_events_t*, struct saved_time_t*, u8);
u8 is_daytime(struct clock_events_t*, struct saved_time_t*);
void change_time_of_day(struct clock_events_t*, struct saved_time_t*);
u8 is_high_tide(struct clock_events_t*, struct saved_time_t*);
void change_tide(struct clock_events_t*, struct saved_time_t*);
void load_time_data(struct clock_events_t*, u16, int, int, u8, u8, u32);
void store_time_data(struct clock_events_t*, u16, u8*, u8, u8, u32);
u8 can_clock_run(struct clock_events_t*);
void wipe_clock(struct clock_events_t*);
void wipe_time(struct saved_time_t*);
void get_clean_time(struct saved_time_t*, struct saved_time_t*);
void get_increased_time(struct saved_time_t*, struct saved_time_t*, struct saved_time_t*);

#if ACTIVE_RTC_FUNCTIONS
u8 get_max_days_in_month(u16, u8);
u32 bcd_decode(u32, u8);
u32 bcd_encode_bytes(u32, u8);
#endif

#endif
