#include <gba.h>
#include "gen3_clock_events_t.h"
#include "gen3_save.h"
#include "party_handler.h"
#include "gen3_clock_events.h"
#include "rng.h"
#include "optimized_swi.h"
#include "text_handler.h"
#include <stddef.h>

#define MAX_DAYS 0xFFFF
#define MAX_HOURS 24
#define MAX_MINUTES 60
#define MAX_SECONDS 60
#define TIDE_OFFSET 3
#define DAY_START 12

#define SECTION_SAVED_TIME_ID 0

#define SAVED_TIME_POS 0x98
#define SAVED_BERRY_TIME_POS 0xA0

#define RTC_ENABLE_VAR_VALUE 0x920

#if ACTUALLY_RUN_EVENTS
#define SECTION_WEATHER_ID 1
#define SECTION_TV_ID 3
#define SECTION_NEWS_ID 3
#define SECTION_OUTBREAK_ID 3

#define WEATHER_POS 0x2F
#define DEWFORD_E_POS 0xF68
#define DEWFORD_CHUNK_1_SIZE (MAX_SECTION_SIZE - DEWFORD_E_POS)

#define TV_SHOW_OUTBREAK_DAYS_DATA_POS 0x14
#define TV_SHOW_OUTBREAK_ID 41
#define TV_SHOW_WOM_ID 25
#define TV_SHOW_NUMO_ID 37
#define TV_SHOW_WOM_POS (TOTAL_TV_SHOWS-1)
#define MIN_CAUGHT_WOM 20
#define TV_SHOW_NORMAL_TOTAL 5

#define NUM_WEATHER_STAGES 4
#define NUM_BIRCH_STATES 7
#define NUM_FRONTIER_MANIAC_STATES 10
#define NUM_FRONTIER_GAMBLER_STATES 12

#define TIME_RNG_MULT 1103515245
#define TIME_RNG_ADD 12345

#define STEPS_STAT_NUM 5
#endif

void increase_clock(struct saved_time_t*, u16, u8, u8, u8, u8);
void swap_time(struct saved_time_t*);
void subtract_time(struct saved_time_t*, struct saved_time_t*, struct saved_time_t*);
#if ACTUALLY_RUN_EVENTS
u32 get_next_seed_time(u32);
void reset_daily_flags(struct clock_events_t*);
void reset_shoal_cave_items(struct clock_events_t*);
void reset_lottery(struct clock_events_t*);
void copy_dewford_trend(struct dewford_trend_t*, struct dewford_trend_t*);
void clear_single_news(struct news_t*);
void clear_single_tv_show(struct tv_show_t*);
void copy_tv_show(struct tv_show_t*, struct tv_show_t*);
void copy_news(struct news_t*, struct news_t*);
void copy_dewford_trend(struct dewford_trend_t*, struct dewford_trend_t*);
void store_tid_tv_show(struct tv_show_t*, u16);
int find_first_empty_record_mix_tv_slot(struct tv_show_t*);
void no_gap_tv_shows(struct tv_show_t*);
void no_gap_news(struct news_t*);
u8 exists_record_mix_tv_slot(struct tv_show_t*, u8, u8, u16);
void try_airing_world_of_masters_show(struct tv_show_t*, u32, u16, u8*, u8);
void try_airing_number_one_show(struct tv_show_t*, u8, u16, u16, u8*, u8);
void schedule_world_of_masters_show(struct tv_show_t*, u32, u16, u8*, u8);
void schedule_number_one_show(struct tv_show_t*, u16*, u16, u8*, u8);
void update_news(struct news_t*, u8, u16);
void update_outbreak_tv(struct tv_show_t*, struct outbreak_t*, u16);
void update_outbreak(struct outbreak_t*, u16);
void update_tv_shows(struct game_data_t*, u16);
void update_dewford_trends(struct dewford_trend_t*, u16);
void update_weather(struct clock_events_t*, u16);
void update_pokerus_daily(struct game_data_t*, u16);
void update_mirage_island(struct clock_events_t*, u16);
void update_birch_state(struct clock_events_t*, u16);
void update_frontier_maniac(struct clock_events_t*, u16);
void update_frontier_gambler(struct clock_events_t*, u16);
#endif

struct saved_time_t base_rtc_time = {.d = 1, .h = 0, .m = 0, .s = 0};
const u16 enable_rtc_flag_num[NUM_MAIN_GAME_ID] = {0x62, 0x37, 0x62};
const u16 enable_rtc_var_num[NUM_MAIN_GAME_ID] = {0x2C, 0x32, 0x2C};
const u16 days_var_num[NUM_MAIN_GAME_ID] = {0x40, 0, 0x40};
#if ACTUALLY_RUN_EVENTS
//const u16 shoal_tide_flag_num[NUM_MAIN_GAME_ID] = {0x3A, 0, 0x3A};
const u16 shoal_items_flag_num[NUM_MAIN_GAME_ID] = {0x5F, 0, 0x5F};
const u16 daily_flags_start_num[NUM_MAIN_GAME_ID] = {0xC0, 0, 0xC0};
const u16 lottery_consumed_var_num[NUM_MAIN_GAME_ID] = {0x45, 0, 0x45};
const u16 birch_state_var_num[NUM_MAIN_GAME_ID] = {0x49, 0, 0x49};
const u16 lottery_low_var_num[NUM_MAIN_GAME_ID] = {0x4B, 0, 0x4B};
const u16 mirage_high_var_num[NUM_MAIN_GAME_ID] = {0x24, 0, 0x24};
const u16 frontier_maniac_var_num[NUM_MAIN_GAME_ID] = {0, 0, 0x2F};
const u16 frontier_gambler_var_num[NUM_MAIN_GAME_ID] = {0, 0, 0x30};
const u16 daily_vars_num[TOTAL_DAILY_SHOW_VARS] = {0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xF1};
const u8 section_dewford_chunk_1[NUM_MAIN_GAME_ID] = {3, 4, 3};
const u8 section_dewford_chunk_2[NUM_MAIN_GAME_ID] = {3, 4, 4};
const u16 dewford_chunk_1_pos[NUM_MAIN_GAME_ID] = {0xED4, 0xD8, DEWFORD_E_POS};
const u16 dewford_chunk_2_pos[NUM_MAIN_GAME_ID] = {dewford_chunk_1_pos[0]+DEWFORD_CHUNK_1_SIZE, dewford_chunk_1_pos[1]+DEWFORD_CHUNK_1_SIZE, 0};
const u16 tv_shows_pos[NUM_MAIN_GAME_ID] = {0x838, 0, 0x8CC};
const u16 news_pos[NUM_MAIN_GAME_ID] = {0xBBC, 0, 0xC50};
const u16 outbreak_pos[NUM_MAIN_GAME_ID] = {0xBFC, 0, 0xC90};

const u16 daily_show_thresholds[TOTAL_DAILY_SHOW_VARS] = {100, 50, 100, 20, 20, 20, 30};
#endif

u8 has_rtc_events(struct game_identity* game_id) {
    if(game_id->game_main_version == RS_MAIN_GAME_CODE)
        return 1;
    if(game_id->game_main_version == E_MAIN_GAME_CODE)
        return 1;
    return 0;
}

u8 is_daytime(struct game_data_t* game_data) {
    swap_time(&game_data->clock_events.saved_time);
    u8 retval = game_data->clock_events.saved_time.h >= DAY_START;
    swap_time(&game_data->clock_events.saved_time);
    return retval;
}

void change_time_of_day(struct game_data_t* game_data) {
    u16 curr_d = game_data->clock_events.saved_time.d;
    increase_clock(&game_data->clock_events.saved_time, 0, MAX_HOURS>>1, 0, 0, 1);
    if(game_data->clock_events.saved_time.d != curr_d)
        increase_clock(&game_data->clock_events.saved_time, 0xFFFF, 0, 0, 0, 1);
}

u8 is_high_tide(struct game_data_t* game_data) {
    swap_time(&game_data->clock_events.saved_time);
    u8 tidal_hours = game_data->clock_events.saved_time.h - TIDE_OFFSET;
    if(game_data->clock_events.saved_time.h < TIDE_OFFSET)
        tidal_hours = game_data->clock_events.saved_time.h + MAX_HOURS - TIDE_OFFSET;
    swap_time(&game_data->clock_events.saved_time);
    if(tidal_hours >= (MAX_HOURS>>1))
        tidal_hours -= MAX_HOURS>>1;
    if(tidal_hours >= (MAX_HOURS>>2))
        return 1;
    return 0;
}

void change_tide(struct game_data_t* game_data) {
    u16 curr_d = game_data->clock_events.saved_time.d;
    u8 curr_daytime = is_daytime(game_data);
    increase_clock(&game_data->clock_events.saved_time, 0, MAX_HOURS>>2, 0, 0, 1);
    if(is_daytime(game_data) != curr_daytime)
        change_time_of_day(game_data);
    if(game_data->clock_events.saved_time.d != curr_d)
        increase_clock(&game_data->clock_events.saved_time, 0xFFFF, 0, 0, 0, 1);
}

void subtract_time(struct saved_time_t* base, struct saved_time_t* sub, struct saved_time_t* target) {
    int s = base->s;
    int m = base->m;
    int h = base->h;
    u16 d = base->d;
    while(sub->s > s) {
        s += MAX_SECONDS;
        m--;
    }
    s -= sub->s;
    while(sub->m > m) {
        m += MAX_MINUTES;
        h--;
    }
    m -= sub->m;
    while(sub->h > h) {
        h += MAX_HOURS;
        d--;
    }
    h -= sub->h;
    d -= sub->d;
    target->s = (u8)s;
    target->m = (u8)m;
    target->h = (u8)h;
    target->d = d;
    normalize_time(target);
}

void swap_time(struct saved_time_t* saved_time) {
    // 0 0 0 0 should result in 1 0 0 0
    // That is because the base time with an expired battery is 1 0 0 0, Jan 1 2000
    subtract_time(&base_rtc_time, saved_time, saved_time);
}

void increase_clock(struct saved_time_t* saved_time, u16 d, u8 h, u8 m, u8 s, u8 is_offset) {
    // This value is subtracted, so we need to get it to the positives...
    if(is_offset)
        swap_time(saved_time);
    u32 res = saved_time->s + s;
    while(res >= MAX_SECONDS) {
        res -= MAX_SECONDS;
        m++;
        if(!m) {
            m = 256 - MAX_MINUTES;
            h++;
            if(!h) {
                h = 256 - MAX_HOURS;
                d++;
            }
        }
    }
    saved_time->s = res;
    res = saved_time->m + m;
    while(res >= MAX_MINUTES) {
        res -= MAX_MINUTES;
        h++;
        if(!h) {
            h = 256 - MAX_HOURS;
            d++;
        }
    }
    saved_time->m = res;
    res = saved_time->h + h;
    while(res >= MAX_HOURS) {
        res -= MAX_HOURS;
        d++;
    }
    saved_time->h = res;
    res = saved_time->d + d;
    saved_time->d = res & 0xFFFF;
    
    if(is_offset)
        swap_time(saved_time);

    normalize_time(saved_time);
}

void normalize_time(struct saved_time_t* saved_time) {
    saved_time->h = SWI_DivMod(saved_time->h, MAX_HOURS);
    saved_time->m = SWI_DivMod(saved_time->m, MAX_MINUTES);
    saved_time->s = SWI_DivMod(saved_time->s, MAX_SECONDS);
}

void run_daily_update(struct game_data_t* game_data, u16 days_increase) {
    increase_clock(&game_data->clock_events.saved_time, days_increase, 0, 0, 0, 1);

    #if ACTUALLY_RUN_EVENTS
    // To be 100% faithful, events should be stopped inside of a Pokémon Center...
    // But doing that would defeat the purpose of this
    if(has_rtc_events(&game_data->game_identifier)) {
        reset_daily_flags(&game_data->clock_events);
        update_dewford_trends(game_data->clock_events.dewford_trends, days_increase);
        update_tv_shows(game_data, days_increase);
        update_weather(&game_data->clock_events, days_increase);
        update_pokerus_daily(game_data, days_increase);
        update_mirage_island(&game_data->clock_events, days_increase);
        update_birch_state(&game_data->clock_events, days_increase);
        if(game_data->game_identifier.game_main_version == E_MAIN_GAME_CODE) {
            update_frontier_maniac(&game_data->clock_events, days_increase);
            update_frontier_gambler(&game_data->clock_events, days_increase);
        }
        reset_shoal_cave_items(&game_data->clock_events);
        reset_lottery(&game_data->clock_events);
        game_data->clock_events.days_var += days_increase;
    }
    #endif
}

void enable_rtc_reset(struct clock_events_t* clock_events) {
    clock_events->enable_rtc_reset_flag = 1;
    clock_events->enable_rtc_reset_var = RTC_ENABLE_VAR_VALUE;
}

void disable_rtc_reset(struct clock_events_t* clock_events) {
    clock_events->enable_rtc_reset_flag = 0;
    clock_events->enable_rtc_reset_var = 0;
}

u8 is_rtc_reset_enabled(struct clock_events_t* clock_events) {
    return (clock_events->enable_rtc_reset_flag == 1) && (clock_events->enable_rtc_reset_var == RTC_ENABLE_VAR_VALUE);
}

u8 can_clock_run(struct clock_events_t* clock_events) {
    return clock_events->days_var < 0x7FFF;
}

void load_time_data(struct game_data_t* game_data, u16 section_id, int slot, int index, u8 game_id) {
    if(section_id == SECTION_SAVED_TIME_ID) {
        copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (index * SECTION_SIZE) + SAVED_TIME_POS, (u8*)&game_data->clock_events.saved_time, sizeof(struct saved_time_t));
        normalize_time(&game_data->clock_events.saved_time);
        copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (index * SECTION_SIZE) + SAVED_BERRY_TIME_POS, (u8*)&game_data->clock_events.saved_berry_time, sizeof(struct saved_time_t));
        normalize_time(&game_data->clock_events.saved_berry_time);
    }

    if(section_id == SECTION_SYS_FLAGS_ID) {
        game_data->clock_events.enable_rtc_reset_flag = get_sys_flag_save(slot, index, game_id, enable_rtc_flag_num[game_id]);
        #if ACTUALLY_RUN_EVENTS
        if(has_rtc_events(&game_data->game_identifier)) {
            game_data->clock_events.shoal_cave_items_reset_flag = get_sys_flag_save(slot, index, game_id, shoal_items_flag_num[game_id]);
            for(size_t j = 0; j < (DAILY_FLAGS_TOTAL>>3); j++)
                game_data->clock_events.daily_flags[j] = get_sys_flag_byte_save(slot, index, game_id, daily_flags_start_num[game_id] + (j<<3));
        }
        #endif
    }

    if(section_id == SECTION_VARS_ID) {
        game_data->clock_events.enable_rtc_reset_var = get_var_save(slot, index, game_id, enable_rtc_var_num[game_id]);
        if(has_rtc_events(&game_data->game_identifier)) {
            game_data->clock_events.days_var = get_var_save(slot, index, game_id, days_var_num[game_id]);
            #if ACTUALLY_RUN_EVENTS
            game_data->clock_events.lottery_low_var = get_var_save(slot, index, game_id, lottery_low_var_num[game_id]);
            game_data->clock_events.lottery_high_var = get_var_save(slot, index, game_id, lottery_low_var_num[game_id]+1);
            game_data->clock_events.mirage_high_var = get_var_save(slot, index, game_id, mirage_high_var_num[game_id]);
            game_data->clock_events.mirage_low_var = get_var_save(slot, index, game_id, mirage_high_var_num[game_id]+1);
            game_data->clock_events.birch_state_var = get_var_save(slot, index, game_id, birch_state_var_num[game_id]);
            game_data->clock_events.lottery_consumed_var = get_var_save(slot, index, game_id, lottery_consumed_var_num[game_id]);
            if(game_id == E_MAIN_GAME_CODE) {
                game_data->clock_events.frontier_maniac_var = get_var_save(slot, index, game_id, frontier_maniac_var_num[game_id]);
                game_data->clock_events.frontier_gambler_var = get_var_save(slot, index, game_id, frontier_gambler_var_num[game_id]);
                for(int j = 0; j < TOTAL_DAILY_SHOW_VARS; j++)
                    game_data->clock_events.daily_show_vars[j] = get_var_save(slot, index, game_id, daily_vars_num[j]);
            }
            #endif
        }
    }

    #if ACTUALLY_RUN_EVENTS
    if(section_id == SECTION_GAME_STATS_ID)
        game_data->clock_events.steps_stat = get_stat_save(slot, index, game_id, STEPS_STAT_NUM) ^ game_data->stat_enc_key;

    if(section_id == SECTION_WEATHER_ID)
        game_data->clock_events.weather_stage = read_byte_save((slot * SAVE_SLOT_SIZE) + (index * SECTION_SIZE) + WEATHER_POS);
    
    if(has_rtc_events(&game_data->game_identifier)) {
        if(section_id == section_dewford_chunk_1[game_id])
            copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (index * SECTION_SIZE) + dewford_chunk_1_pos[game_id], (u8*)game_data->clock_events.dewford_trends, DEWFORD_CHUNK_1_SIZE);
        if(section_id == section_dewford_chunk_2[game_id])
            copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (index * SECTION_SIZE) + dewford_chunk_2_pos[game_id], ((u8*)game_data->clock_events.dewford_trends) + DEWFORD_CHUNK_1_SIZE, sizeof(game_data->clock_events.dewford_trends)-DEWFORD_CHUNK_1_SIZE);
        if(section_id == SECTION_TV_ID)
            copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (index * SECTION_SIZE) + tv_shows_pos[game_id], (u8*)game_data->clock_events.tv_shows, sizeof(game_data->clock_events.tv_shows));
        if(section_id == SECTION_NEWS_ID)
            copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (index * SECTION_SIZE) + news_pos[game_id], (u8*)game_data->clock_events.news, sizeof(game_data->clock_events.news));
        if(section_id == SECTION_OUTBREAK_ID)
            copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (index * SECTION_SIZE) + outbreak_pos[game_id], (u8*)&game_data->clock_events.outbreak, sizeof(game_data->clock_events.outbreak));
    }
    #endif
}

void store_time_data(struct game_data_t* game_data, u16 section_id, u8* buffer_8, u8 game_id) {
    if(section_id == SECTION_SAVED_TIME_ID) {
        for(size_t j = 0; j < sizeof(struct saved_time_t); j++)
            buffer_8[SAVED_TIME_POS+j] = ((u8*)&game_data->clock_events.saved_time)[j];
        for(size_t j = 0; j < sizeof(struct saved_time_t); j++)
            buffer_8[SAVED_BERRY_TIME_POS+j] = ((u8*)&game_data->clock_events.saved_berry_time)[j];
    }

    if(section_id == SECTION_SYS_FLAGS_ID) {
        set_sys_flag_save(buffer_8, game_id, enable_rtc_flag_num[game_id], game_data->clock_events.enable_rtc_reset_flag);
        #if ACTUALLY_RUN_EVENTS
        if(has_rtc_events(&game_data->game_identifier)) {
            set_sys_flag_save(buffer_8, game_id, shoal_items_flag_num[game_id], game_data->clock_events.shoal_cave_items_reset_flag);
            for(size_t j = 0; j < (DAILY_FLAGS_TOTAL>>3); j++)
                set_sys_flag_byte_save(buffer_8, game_id, daily_flags_start_num[game_id] + (j<<3), game_data->clock_events.daily_flags[j]);
        }
        #endif
    }

    if(section_id == SECTION_VARS_ID) {
        set_var_save(buffer_8, game_id, enable_rtc_var_num[game_id], game_data->clock_events.enable_rtc_reset_var);
        if(has_rtc_events(&game_data->game_identifier)) {
            set_var_save(buffer_8, game_id, days_var_num[game_id], game_data->clock_events.days_var);
            #if ACTUALLY_RUN_EVENTS
            set_var_save(buffer_8, game_id, lottery_low_var_num[game_id], game_data->clock_events.lottery_low_var);
            set_var_save(buffer_8, game_id, lottery_low_var_num[game_id]+1, game_data->clock_events.lottery_high_var);
            set_var_save(buffer_8, game_id, mirage_high_var_num[game_id], game_data->clock_events.mirage_high_var);
            set_var_save(buffer_8, game_id, mirage_high_var_num[game_id]+1, game_data->clock_events.mirage_low_var);
            set_var_save(buffer_8, game_id, birch_state_var_num[game_id], game_data->clock_events.birch_state_var);
            set_var_save(buffer_8, game_id, lottery_consumed_var_num[game_id], game_data->clock_events.lottery_consumed_var);
            if(game_id == E_MAIN_GAME_CODE) {
                set_var_save(buffer_8, game_id, frontier_maniac_var_num[game_id], game_data->clock_events.frontier_maniac_var);
                set_var_save(buffer_8, game_id, frontier_gambler_var_num[game_id], game_data->clock_events.frontier_gambler_var);
                for(int j = 0; j < TOTAL_DAILY_SHOW_VARS; j++)
                    set_var_save(buffer_8, game_id, daily_vars_num[j], game_data->clock_events.daily_show_vars[j]);
            }
            #endif
        }
    }

    #if ACTUALLY_RUN_EVENTS
    if(section_id == SECTION_WEATHER_ID)
        buffer_8[WEATHER_POS] = game_data->clock_events.weather_stage;

    if(has_rtc_events(&game_data->game_identifier)) {
        if(section_id == section_dewford_chunk_1[game_id])
            for(size_t j = 0; j < DEWFORD_CHUNK_1_SIZE; j++)
                buffer_8[dewford_chunk_1_pos[game_id]+j] = ((u8*)game_data->clock_events.dewford_trends)[j];
        if(section_id == section_dewford_chunk_2[game_id])
            for(size_t j = 0; j < (sizeof(game_data->clock_events.dewford_trends)-DEWFORD_CHUNK_1_SIZE); j++)
                buffer_8[dewford_chunk_2_pos[game_id]+j] = ((u8*)game_data->clock_events.dewford_trends)[DEWFORD_CHUNK_1_SIZE+j];
        if(section_id == SECTION_TV_ID)
            for(size_t j = 0; j < sizeof(game_data->clock_events.tv_shows); j++)
                buffer_8[tv_shows_pos[game_id]+j] = ((u8*)game_data->clock_events.tv_shows)[j];
        if(section_id == SECTION_NEWS_ID)
            for(size_t j = 0; j < sizeof(game_data->clock_events.news); j++)
                buffer_8[news_pos[game_id]+j] = ((u8*)game_data->clock_events.news)[j];
        if(section_id == SECTION_OUTBREAK_ID)
            for(size_t j = 0; j < sizeof(game_data->clock_events.outbreak); j++)
                buffer_8[outbreak_pos[game_id]+j] = ((u8*)&game_data->clock_events.outbreak)[j];
    }
    #endif
}

#if ACTUALLY_RUN_EVENTS
u32 get_next_seed_time(u32 seed) {
    return (seed * TIME_RNG_MULT) + TIME_RNG_ADD;
}

void reset_daily_flags(struct clock_events_t* clock_events) {
    for(size_t i = 0; i < (DAILY_FLAGS_TOTAL>>3); i++)
        clock_events->daily_flags[i] = 0;
}

void clear_single_news(struct news_t* news) {
    for(size_t j = 0; j < sizeof(struct news_t*); j++)
        ((u8*)news)[j] = 0;
}

void clear_single_tv_show(struct tv_show_t* tv_show) {
    for(size_t j = 0; j < sizeof(struct tv_show_t*); j++)
        ((u8*)tv_show)[j] = 0;
}

void copy_tv_show(struct tv_show_t* src_show, struct tv_show_t* dst_show) {
    for(size_t i = 0; i < sizeof(struct tv_show_t); i++)
        ((u8*)dst_show)[i] = ((u8*)src_show)[i];
}

void copy_news(struct news_t* src_news, struct news_t* dst_news) {
    for(size_t i = 0; i < sizeof(struct news_t); i++)
        ((u8*)dst_news)[i] = ((u8*)src_news)[i];
}

void copy_dewford_trend(struct dewford_trend_t* src_trend, struct dewford_trend_t* dst_trend) {
    for(size_t i = 0; i < sizeof(struct dewford_trend_t); i++)
        ((u8*)dst_trend)[i] = ((u8*)src_trend)[i];
}

void store_tid_tv_show(struct tv_show_t* tv_show, u16 tid) {
    tv_show->src_tid[1] = tid;
    tv_show->src_tid[2] = tid;
    tv_show->tid = tid;
}

int find_first_empty_record_mix_tv_slot(struct tv_show_t* tv_shows) {
    for(int i = TV_SHOW_NORMAL_TOTAL; i < (TOTAL_TV_SHOWS-1); i++)
        if(!tv_shows[i].kind)
            return i;
    return -1;
}

void no_gap_tv_shows(struct tv_show_t* tv_shows) {
    // Make all the active normal TV shows near each other
    for(int i = 0; i < (TV_SHOW_NORMAL_TOTAL-1); i++)
        if(!tv_shows[i].kind)
            for (int j = i + 1; j < TV_SHOW_NORMAL_TOTAL; j++)
                if (tv_shows[j].kind) {
                    copy_tv_show(&tv_shows[j], &tv_shows[i]);
                    clear_single_tv_show(&tv_shows[j]);
                    break;
                }

    // Make all the active Record Mix TV shows near each other
    for(int i = TV_SHOW_NORMAL_TOTAL; i < (TOTAL_TV_SHOWS-1-1); i++)
        if(!tv_shows[i].kind)
            for (int j = i + 1; j < (TOTAL_TV_SHOWS-1); j++)
                if (tv_shows[j].kind) {
                    copy_tv_show(&tv_shows[j], &tv_shows[i]);
                    clear_single_tv_show(&tv_shows[j]);
                    break;
                }
}

void no_gap_news(struct news_t* news) {
    // Make all the active ones near each other
    for(int i = 0; i < (TOTAL_NEWS-1); i++)
        if(!news[i].kind)
            for (int j = i + 1; j < TOTAL_NEWS; j++)
                if (news[j].kind) {
                    copy_news(&news[j], &news[i]);
                    clear_single_news(&news[j]);
                    break;
                }
}

u8 exists_record_mix_tv_slot(struct tv_show_t* tv_shows, u8 kind, u8 clear, u16 tid) {
    for (int i = TV_SHOW_NORMAL_TOTAL; i < (TOTAL_TV_SHOWS-1); i++)
        if((tv_shows[i].kind == kind) && (tid == tv_shows[i].tid)) {
            if (clear) {
                clear_single_tv_show(&tv_shows[i]);
                no_gap_tv_shows(tv_shows);
            }
            return 1;
        }
    return 0;
}

void try_airing_world_of_masters_show(struct tv_show_t* tv_shows, u32 steps, u16 tid, u8* name, u8 is_jp) {
    int curr_slot = find_first_empty_record_mix_tv_slot(tv_shows);
    if((curr_slot != -1) && (!exists_record_mix_tv_slot(tv_shows, TV_SHOW_WOM_ID, 0, tid))) {
        copy_tv_show(&tv_shows[TV_SHOW_WOM_POS], &tv_shows[curr_slot]);
        struct tv_show_wom_t* wom_show = (struct tv_show_wom_t*)&tv_shows[curr_slot];
        wom_show->is_active = 0; // Show is not active until passed via Record Mix
        wom_show->steps = steps - wom_show->steps;
        text_gen3_copy(name, wom_show->name, OT_NAME_GEN3_SIZE, OT_NAME_GEN3_SIZE+1);
        store_tid_tv_show((struct tv_show_t*)wom_show, tid);
        if(is_jp)
            wom_show->language = JAPANESE_LANGUAGE;
        else
            wom_show->language = ENGLISH_LANGUAGE;
    }
}

void try_airing_number_one_show(struct tv_show_t* tv_shows, u8 index, u16 value, u16 tid, u8* name, u8 is_jp) {
    exists_record_mix_tv_slot(tv_shows, TV_SHOW_NUMO_ID, 1, tid);
    int curr_slot = find_first_empty_record_mix_tv_slot(tv_shows);
    if(curr_slot != -1) {
        struct tv_show_numo_t* numo_show = (struct tv_show_numo_t*)&tv_shows[curr_slot];
        numo_show->kind = TV_SHOW_NUMO_ID;
        numo_show->is_active = 0; // Show is not active until passed via Record Mix
        text_gen3_copy(name, numo_show->name, OT_NAME_GEN3_SIZE, OT_NAME_GEN3_SIZE+1);
        numo_show->index = index;
        numo_show->value = value;
        store_tid_tv_show((struct tv_show_t*)numo_show, tid);
        if(is_jp)
            numo_show->language = JAPANESE_LANGUAGE;
        else
            numo_show->language = ENGLISH_LANGUAGE;
    }
}

void schedule_world_of_masters_show(struct tv_show_t* tv_shows, u32 steps, u16 tid, u8* name, u8 is_jp) {
    if(tv_shows[TV_SHOW_WOM_POS].kind == TV_SHOW_WOM_ID) {
        struct tv_show_wom_t* wom_show = (struct tv_show_wom_t*)&tv_shows[TV_SHOW_WOM_POS];
        if(wom_show->num_caught >= MIN_CAUGHT_WOM)
            try_airing_world_of_masters_show(tv_shows, steps, tid, name, is_jp);

        clear_single_tv_show(&tv_shows[TV_SHOW_WOM_POS]);
    }
}

void schedule_number_one_show(struct tv_show_t* tv_shows, u16* daily_show_vars, u16 tid, u8* name, u8 is_jp) {
    for(int i = 0; i < TOTAL_DAILY_SHOW_VARS; i++) {
        if(daily_show_vars[i] >= daily_show_thresholds[i]) {
            try_airing_number_one_show(tv_shows, i, daily_show_thresholds[i], tid, name, is_jp);
            break;
        }
    }

    for(int i = 0; i < TOTAL_DAILY_SHOW_VARS; i++)
        daily_show_vars[i] = 0;
}

void update_news(struct news_t* news, u8 is_game_cleared, u16 days_increase) {
    for(int i = 0; i < TOTAL_NEWS; i++)
        if(news[i].kind) {
            if(news[i].days >= days_increase) {
                // Progress countdown to news event
                if((!news[i].state) && is_game_cleared)
                    news[i].state = 1;

                news[i].days -= days_increase;
            }
            else
                clear_single_news(&news[i]);
        }
    no_gap_news(news);
}

void update_outbreak_tv(struct tv_show_t* tv_shows, struct outbreak_t* outbreak, u16 days_increase) {
    if(!outbreak->species)
        for(int i = 0; i < (TOTAL_TV_SHOWS-1); i++)
            if((tv_shows[i].kind == TV_SHOW_OUTBREAK_ID) && tv_shows[i].is_active)
            {
                u16 tv_show_days = tv_shows[i].data[TV_SHOW_OUTBREAK_DAYS_DATA_POS] | (tv_shows[i].data[TV_SHOW_OUTBREAK_DAYS_DATA_POS+1]<<8);
                if(tv_show_days < days_increase)
                    tv_show_days = 0;
                else
                    tv_show_days -= days_increase;
                tv_shows[i].data[TV_SHOW_OUTBREAK_DAYS_DATA_POS] = tv_show_days & 0xFF;
                tv_shows[i].data[TV_SHOW_OUTBREAK_DAYS_DATA_POS+1] = (tv_show_days >> 8) & 0xFF;
                break;
            }
}

void update_outbreak(struct outbreak_t* outbreak, u16 days_increase) {
    if(outbreak->days <= days_increase)
        for(size_t i = 0; i < sizeof(struct outbreak_t); i++)
            ((u8*)outbreak)[i] = 0;
    else
        outbreak->days -= days_increase;
}

void update_tv_shows(struct game_data_t* game_data, u16 days_increase) {
    update_outbreak_tv(game_data->clock_events.tv_shows, &game_data->clock_events.outbreak, days_increase);
    update_outbreak(&game_data->clock_events.outbreak, days_increase);
    update_news(game_data->clock_events.news, game_data->game_cleared_flag, days_increase);
    schedule_world_of_masters_show(game_data->clock_events.tv_shows, game_data->clock_events.steps_stat, game_data->trainer_id & 0xFFFF, game_data->trainer_name, game_data->game_identifier.game_is_jp);
    if(game_data->game_identifier.game_main_version == E_MAIN_GAME_CODE)
        schedule_number_one_show(game_data->clock_events.tv_shows, game_data->clock_events.daily_show_vars, game_data->trainer_id & 0xFFFF, game_data->trainer_name, game_data->game_identifier.game_is_jp);
}

void update_weather(struct clock_events_t* clock_events, u16 days_increase) {
    clock_events->weather_stage += days_increase;
    SWI_DivMod(clock_events->weather_stage, NUM_WEATHER_STAGES);
}

void update_pokerus_daily(struct game_data_t* game_data, u16 days_increase) {
    gen3_party_total_t num_mons = game_data->party_3.total;
    if(num_mons > PARTY_SIZE)
        num_mons = PARTY_SIZE;
    for(gen3_party_total_t i = 0; i < num_mons; i++)
        update_pokerus_gen3(&game_data->party_3_undec[i], days_increase);
}

u8 is_daily_update_safe(struct game_data_t* game_data, u16 days_increase) {
    gen3_party_total_t num_mons = game_data->party_3.total;
    if(num_mons > PARTY_SIZE)
        num_mons = PARTY_SIZE;
    for(gen3_party_total_t i = 0; i < num_mons; i++)
        if(would_update_end_pokerus_gen3(&game_data->party_3_undec[i], days_increase))
            return 0;
    return 1;
}

void update_mirage_island(struct clock_events_t* clock_events, u16 days_increase) {
    u32 val = clock_events->mirage_low_var | (clock_events->mirage_high_var << 16);
    for(int i = 0; i < days_increase; i++)
        val = get_next_seed_time(val);
    clock_events->mirage_low_var = val & 0xFFFF;
    clock_events->mirage_high_var = val >> 16;
}

void update_birch_state(struct clock_events_t* clock_events, u16 days_increase) {
    clock_events->birch_state_var += days_increase;
    SWI_DivMod(clock_events->birch_state_var, NUM_BIRCH_STATES);
}

void update_frontier_maniac(struct clock_events_t* clock_events, u16 days_increase) {
    clock_events->frontier_maniac_var += days_increase;
    SWI_DivMod(clock_events->frontier_maniac_var, NUM_FRONTIER_MANIAC_STATES);
}

void update_frontier_gambler(struct clock_events_t* clock_events, u16 days_increase) {
    clock_events->frontier_gambler_var += days_increase;
    SWI_DivMod(clock_events->frontier_gambler_var, NUM_FRONTIER_GAMBLER_STATES);
}

void update_dewford_trends(struct dewford_trend_t* dewford_trends, u16 days_increase) {
    u32 base_inc = 5 * days_increase;
    for(int i = 0; i < TOTAL_DEWFORD_TRENDS; i++) {
        u32 inc = base_inc;
        struct dewford_trend_t *trend = &dewford_trends[i];

        // Prevent division by 0
        if(trend->max_trendiness) {
            if (!trend->gaining_trendiness) {
                // This trend is "boring"
                // Lose trendiness until it becomes 0
                if (trend->trendiness >= ((u16)inc)) {
                    trend->trendiness -= inc;
                    if (trend->trendiness == 0)
                        trend->gaining_trendiness = 1;
                    continue;
                }
                inc -= trend->trendiness;
                trend->trendiness = 0;
                trend->gaining_trendiness = 1;
            }

            u32 trendiness = trend->trendiness + inc;
            if (((u16)trendiness) > trend->max_trendiness) {
                // Reached limit, reset trendiness
                u32 new_trendiness = 0;
                trendiness = SWI_DivDivMod(trendiness, trend->max_trendiness, (int*)&new_trendiness);

                trend->gaining_trendiness = trendiness ^ 1;
                if (trend->gaining_trendiness)
                    trend->trendiness = new_trendiness;
                else
                    trend->trendiness = trend->max_trendiness - new_trendiness;
            }
            else {
                // Increase trendiness
                trend->trendiness = trendiness;

                // Trend has reached its max, becoming "boring" and start losing trendiness
                if (trend->trendiness == trend->max_trendiness)
                    trend->gaining_trendiness = 0;
            }
        }
    }

    // Sort trends by trendiness
    for(int i = 0; i < TOTAL_DEWFORD_TRENDS; i++) {
        for(int j = 0; j < i; j++) {
            if(dewford_trends[i].trendiness > dewford_trends[j].trendiness) {
                struct dewford_trend_t support_trends;
                copy_dewford_trend(&dewford_trends[j], &support_trends);
                copy_dewford_trend(&dewford_trends[i], &dewford_trends[j]);
                copy_dewford_trend(&support_trends, &dewford_trends[i]);
            }
        }
    }
}

void reset_shoal_cave_items(struct clock_events_t* clock_events) {
    clock_events->shoal_cave_items_reset_flag = 1;
}

void reset_lottery(struct clock_events_t* clock_events) {
    u32 val = get_rng();
    clock_events->lottery_low_var = val & 0xFFFF;
    clock_events->lottery_high_var = val >> 16;
    clock_events->lottery_consumed_var = 0;
}
#endif
