#include "base_include.h"
#include "config_settings.h"
#include "version_identifier.h"
#include "party_handler.h"
#include "gen_converter.h"
#include "useful_qualifiers.h"

#include "default_colours_bin.h"
#include "egg_locations_bin.h"
#include "valid_egg_locations_e_bin.h"
#include "valid_egg_locations_rs_bin.h"
#include "valid_egg_locations_frlg_bin.h"

#define DEFAULT_SYS_LANGUAGE ENGLISH_LANGUAGE
#define DEFAULT_TARGET_INT_LANGUAGE UNKNOWN_LANGUAGE
#define DEFAULT_CONVERSION_COLO_XD 1
#define DEFAULT_DEFAULT_CONVERSION_GAME FR_VERSION_ID
#define DEFAULT_GEN1_EVERSTONE 0
#define DEFAULT_ALLOW_CROSS_GEN_EVOS 0
#define DEFAULT_EVOLVE_WITHOUT_TRADE 0
#define DEFAULT_ALLOW_UNDISTRIBUTED_EVENTS 0
#define DEFAULT_FAST_HATCH_EGGS 0
#define DEFAULT_BALL POKEBALL_ID

void sanitize_egg_met_location(void);
const u8* get_valid_egg_met_locs(void);

static u8 sys_language;
static u8 target_int_language;
static u8 conversion_colo_xd;
static u8 default_conversion_game;
static u8 default_colours[NUM_COLOURS][NUM_SUB_COLOURS];
static u8 gen1_everstone;
static u8 allow_cross_gen_evos;
static u8 evolve_without_trade;
static u8 allow_undistributed_events;
static u8 fast_hatch_eggs;
static u16 applied_ball;
static u8 egg_met_location;
static u8 first_set_egg_met_location;

const struct version_t version = { .main_version = 1, .sub_version = 1, .revision_version = 1, .revision_letter = CONSOLE_LETTER};
const u8* egg_valid_met_locations[NUMBER_OF_GAMES+FIRST_VERSION_ID] = {valid_egg_locations_rs_bin, valid_egg_locations_rs_bin, valid_egg_locations_rs_bin, valid_egg_locations_e_bin, valid_egg_locations_frlg_bin, valid_egg_locations_frlg_bin};

void set_default_settings() {
    set_sys_language(DEFAULT_SYS_LANGUAGE);
    set_target_int_language(DEFAULT_TARGET_INT_LANGUAGE);
    set_conversion_colo_xd(DEFAULT_CONVERSION_COLO_XD);
    first_set_egg_met_location = 1;
    set_default_conversion_game(DEFAULT_DEFAULT_CONVERSION_GAME);
    for(size_t i = 0; i < NUM_COLOURS; i++)
        for(size_t j = 0; j < NUM_SUB_COLOURS; j++)
            set_single_colour(i, j, default_colours_bin[(i*NUM_SUB_COLOURS)+j]);
    set_gen1_everstone(DEFAULT_GEN1_EVERSTONE);
    set_allow_cross_gen_evos(DEFAULT_ALLOW_CROSS_GEN_EVOS);
    set_evolve_without_trade(DEFAULT_EVOLVE_WITHOUT_TRADE);
    set_allow_undistributed_events(DEFAULT_ALLOW_UNDISTRIBUTED_EVENTS);
    set_fast_hatch_eggs(DEFAULT_FAST_HATCH_EGGS);
    set_applied_ball(DEFAULT_BALL);
}

const u8* get_valid_egg_met_locs() {
    const u8* valid_met_locs = egg_valid_met_locations[FIRST_VERSION_ID];
    u8 default_game_id = get_default_conversion_game();
    if((default_game_id < (NUMBER_OF_GAMES+FIRST_VERSION_ID)) && (default_game_id >= FIRST_VERSION_ID))
        valid_met_locs = egg_valid_met_locations[default_game_id];
    return valid_met_locs;
}

void sanitize_egg_met_location() {
    const u8* valid_met_locs = get_valid_egg_met_locs();
    if(first_set_egg_met_location || (!(valid_met_locs[egg_met_location>>3] & (1 << (egg_met_location & 7))))) {
        u8 default_game_id = get_default_conversion_game() & 0xF;
        egg_met_location = egg_locations_bin[default_game_id];
        first_set_egg_met_location = 0;
    }
}

void set_sys_language(u8 new_val) {
    if((new_val < FIRST_VALID_LANGUAGE) || (new_val == ((u8)(FIRST_VALID_LANGUAGE-1))))
        new_val = NUM_LANGUAGES-1;
    if(new_val >= NUM_LANGUAGES)
        new_val = FIRST_VALID_LANGUAGE;
    new_val = get_valid_language(new_val);
    sys_language = new_val;
}

void set_target_int_language(u8 new_val) {
    if(new_val == ((u8)(UNKNOWN_LANGUAGE+1)))
        new_val = FIRST_INTERNATIONAL_VALID_LANGUAGE;
    if(new_val == ((u8)(UNKNOWN_LANGUAGE-1)))
        new_val = NUM_LANGUAGES-1;
    if((new_val < FIRST_INTERNATIONAL_VALID_LANGUAGE) || (new_val == ((u8)(FIRST_INTERNATIONAL_VALID_LANGUAGE-1))))
        new_val = UNKNOWN_LANGUAGE;
    if(new_val >= NUM_LANGUAGES)
        new_val = UNKNOWN_LANGUAGE;
    if(new_val != UNKNOWN_LANGUAGE)
        new_val = get_valid_language(new_val);
    target_int_language = new_val;
}

void set_conversion_colo_xd(u8 new_val) {
    conversion_colo_xd = new_val;
}

void set_default_conversion_game(u8 new_val) {
    if((new_val < FIRST_VERSION_ID) || (new_val == ((u8)(FIRST_VERSION_ID-1))))
        new_val = NUMBER_OF_GAMES-1+FIRST_VERSION_ID;
    if(new_val >= (NUMBER_OF_GAMES+FIRST_VERSION_ID))
        new_val = FIRST_VERSION_ID;
    default_conversion_game = new_val;
    sanitize_egg_met_location();
}

void set_gen1_everstone(u8 new_val) {
    gen1_everstone = new_val;
}

void set_allow_cross_gen_evos(u8 new_val) {
    allow_cross_gen_evos = new_val;
}

void set_evolve_without_trade(u8 new_val) {
    evolve_without_trade = new_val;
}

void set_allow_undistributed_events(u8 new_val) {
    allow_undistributed_events = new_val;
}

void set_fast_hatch_eggs(u8 new_val) {
    fast_hatch_eggs = new_val;
}

void increase_egg_met_location() {
    const u8* valid_met_locs = get_valid_egg_met_locs();
    u8 base_val = egg_met_location;
    egg_met_location += 1;
    while(base_val != egg_met_location) {
        if(valid_met_locs[egg_met_location>>3] & (1 << (egg_met_location & 7)))
            return;
        egg_met_location += 1;
    }
}

void decrease_egg_met_location() {
    const u8* valid_met_locs = get_valid_egg_met_locs();
    u8 base_val = egg_met_location;
    egg_met_location -= 1;
    while(base_val != egg_met_location) {
        if(valid_met_locs[egg_met_location>>3] & (1 << (egg_met_location & 7)))
            return;
        egg_met_location -= 1;
    }
}

u8 set_applied_ball(u16 new_val) {
    new_val &= 0x1F;
    if(new_val > LAST_BALL_ID)
        new_val = FIRST_BALL_ID;
    if(new_val < FIRST_BALL_ID)
        new_val = LAST_BALL_ID;
    if(VALID_POKEBALL_POSSIBLE & (1 << new_val)) {
        applied_ball = new_val;
        return 1;
    }
    return 0;
}

void set_single_colour(u8 index, u8 sub_index, u8 new_colour) {
    if(index >= NUM_COLOURS)
        index = 0;
    if(sub_index >= NUM_SUB_COLOURS)
        sub_index = 0;
    default_colours[index][sub_index] = new_colour&0x1F;
}

u8 get_sys_language() {
    u8 out = sys_language;
    if(out >= NUM_LANGUAGES)
        out = DEFAULT_NAME_BAD_LANGUAGE;
    return out;
}

u8 get_target_int_language() {
    u8 out = target_int_language;
    if(out >= NUM_LANGUAGES)
        out = UNKNOWN_LANGUAGE;
    return out;
}

u8 get_filtered_target_int_language() {
    u8 out = target_int_language;
    if(out >= NUM_LANGUAGES)
        out = get_sys_language();
    if(out == JAPANESE_LANGUAGE)
        out = ENGLISH_LANGUAGE;
    return out;
}

u8 get_conversion_colo_xd() {
    return conversion_colo_xd;
}

u8 get_default_conversion_game() {
    return default_conversion_game;
}

u16 get_full_colour(u8 index) {
    if(index >= NUM_COLOURS)
        index = 0;
    return RGB5(default_colours[index][0]&0x1F, default_colours[index][1]&0x1F, default_colours[index][2]&0x1F);
}

u8 get_single_colour(u8 index, u8 sub_index) {
    if(index >= NUM_COLOURS)
        index = 0;
    if(sub_index >= NUM_SUB_COLOURS)
        sub_index = 0;
    return default_colours[index][sub_index]&0x1F;
}

u8 get_gen1_everstone() {
    return gen1_everstone;
}

u8 get_allow_cross_gen_evos() {
    return allow_cross_gen_evos;
}

u8 get_evolve_without_trade() {
    return evolve_without_trade;
}

u8 get_allow_undistributed_events() {
    return allow_undistributed_events;
}

u8 get_fast_hatch_eggs() {
    return fast_hatch_eggs;
}

u8 get_egg_met_location() {
    return egg_met_location;
}

u16 get_applied_ball() {
    return applied_ball;
}

const struct version_t* get_version() {
    return &version;
}
