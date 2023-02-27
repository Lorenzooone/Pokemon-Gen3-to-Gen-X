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
u8 is_daily_update_safe(struct game_data_t*, u16);
void run_daily_update(struct game_data_t*, u16);
u8 is_daytime(struct game_data_t*);
void change_time_of_day(struct game_data_t* game_data);
u8 is_high_tide(struct game_data_t* game_data);
void change_tide(struct game_data_t* game_data);
void load_time_data(struct game_data_t*, u16, int, int, u8);
void store_time_data(struct game_data_t*, u16, u8*, u8);
u8 can_clock_run(struct clock_events_t*);
void wipe_clock(struct clock_events_t*);

#endif
