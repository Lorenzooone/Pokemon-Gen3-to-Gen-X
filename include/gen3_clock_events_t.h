#ifndef GEN3_CLOCK_EVENTS_T__
#define GEN3_CLOCK_EVENTS_T__

#include "party_handler.h"
#include "useful_qualifiers.h"

#define ACTUALLY_RUN_EVENTS 0

#define DAILY_FLAGS_TOTAL 0x40
#define TOTAL_DEWFORD_TRENDS 5
#define TOTAL_TV_SHOWS 25
#define TOTAL_NEWS 16
#define TOTAL_DAILY_SHOW_VARS 7

struct saved_time_t {
    u16 d;
    u8 h;
    u8 m;
    u8 s;
} PACKED ALIGNED(4);

struct dewford_trend_t {
    u16 trendiness : 7;
    u16 max_trendiness : 7;
    u16 gaining_trendiness : 1;
    u16 padding : 1;
    u16 rand_val;
    u16 words[2];
} PACKED ALIGNED(4);

struct tv_show_t {
    u8 kind;
    u8 is_active;
    u8 data[26];
    u16 src_tid[3];
    u16 tid;
} PACKED ALIGNED(4);

struct tv_show_wom_t {
    u8 kind;
    u8 is_active;
    u16 num_caught;
    u16 species_caught;
    u16 steps;
    u16 species_used;
    u8 location;
    u8 language;
    u8 filler[7];
    u8 name[OT_NAME_GEN3_SIZE+1];
    u8 filler_1;
    u16 src_tid[3];
    u16 tid;
} PACKED ALIGNED(4);

struct tv_show_numo_t {
    u8 kind;
    u8 is_active;
    u16 value;
    u8 index;
    u8 language;
    u8 filler[13];
    u8 name[OT_NAME_GEN3_SIZE+1];
    u8 filler_1;
    u16 src_tid[3];
    u16 tid;
} PACKED ALIGNED(4);

struct news_t {
    u8 kind;
    u8 state;
    u16 days;
} PACKED ALIGNED(4);

struct outbreak_t {
    u16 species;
    u8 location_map_num;
    u8 location_map_group;
    u8 level;
    u8 unused[3];
    u16 moves[MOVES_SIZE];
    u8 unused_1;
    u8 chance;
    u16 days;
} PACKED ALIGNED(4);

struct clock_events_t {
    struct saved_time_t saved_time;
    struct saved_time_t saved_berry_time;
    u8 enable_rtc_reset_flag : 1;
    u16 enable_rtc_reset_var;
    u16 days_var;
    #if ACTUALLY_RUN_EVENTS
    u8 shoal_cave_items_reset_flag : 1;
    u16 lottery_low_var;
    u16 lottery_high_var;
    u16 mirage_low_var;
    u16 mirage_high_var;
    u16 birch_state_var;
    u16 frontier_maniac_var;
    u16 frontier_gambler_var;
    u16 lottery_consumed_var;
    u8 daily_flags[DAILY_FLAGS_TOTAL>>3];
    u16 daily_show_vars[TOTAL_DAILY_SHOW_VARS];
    u32 steps_stat;
    u8 weather_stage;
    struct dewford_trend_t dewford_trends[TOTAL_DEWFORD_TRENDS];
    struct tv_show_t tv_shows[TOTAL_TV_SHOWS];
    struct news_t news[TOTAL_NEWS];
    struct outbreak_t outbreak;
    #endif
};

#endif
