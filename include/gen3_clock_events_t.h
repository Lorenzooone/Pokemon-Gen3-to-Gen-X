#ifndef GEN3_CLOCK_EVENTS_T__
#define GEN3_CLOCK_EVENTS_T__

#include "party_handler.h"
#include "useful_qualifiers.h"

#define ACTUALLY_RUN_EVENTS 1

#define DAILY_FLAGS_TOTAL 0x40
#define TOTAL_DEWFORD_TRENDS 5
#define TOTAL_TV_SHOWS 25
#define TOTAL_NEWS 16
#define TOTAL_DAILY_SHOW_VARS 7
#define TOTAL_BERRY_TREES 0x80

#define NUM_BERRIES 0x2B
#define ENIGMA_BERRY_BERRY_ID 0x2A
#define BERRY_NAME_SIZE 6
#define BERRY_STAGE_DURATION 60
#define BERRY_ITEM_EFFECT_SIZE 18
#define ENIGMA_BERRY_VALID 0xFFFF
#define NUM_WATER_STAGES 4
#define WATERED_BIT_SIZE NUM_WATER_STAGES
#define BERRY_STAGE_NO_BERRY    0  // there is no tree planted and the soil is completely flat.
#define BERRY_STAGE_PLANTED     1
#define BERRY_STAGE_SPROUTED    2
#define BERRY_STAGE_TALLER      3
#define BERRY_STAGE_FLOWERING   4
#define BERRY_STAGE_BERRIES     5
#define BERRY_STAGE_SPARKLING   255

struct berry_growth_data_t {
    u8 max_yield;
    u8 min_yield;
    u8 growth_time;
} PACKED ALIGNED(1);

struct enigma_berry_data_t {
    u8 name[BERRY_NAME_SIZE+1];
    u8 firmness;
    u16 size;
    u8 max_yield;
    u8 min_yield;
    u8* desc_1;
    u8* desc_2;
    u8 growth_time;
    u8 spicy;
    u8 dry;
    u8 sweet;
    u8 bitter;
    u8 sour;
    u8 smooth;
    u8 padding;
    u8 item_effect[BERRY_ITEM_EFFECT_SIZE];
    u8 hold_effect;
    u8 hold_effect_param;
    u32 checksum;
} PACKED ALIGNED(4);

struct berry_tree_t {
    u8 berry;
    u8 stage : 7;
    u8 no_growth : 1;
    u16 next_update_minutes;
    u8 berry_yield;
    u8 num_regrowths : 4;
    u8 watered_bitfield : WATERED_BIT_SIZE;
    u16 padding;
} PACKED ALIGNED(4);

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
    struct enigma_berry_data_t enigma_berry_data;
    struct berry_tree_t berry_trees[TOTAL_BERRY_TREES];
    #endif
};

#endif
