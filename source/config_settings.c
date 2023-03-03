#include <gba.h>
#include "config_settings.h"
#include "version_identifier.h"
#include "party_handler.h"
#include "useful_qualifiers.h"

#include "default_colours_bin.h"

#define DEFAULT_SYS_LANGUAGE ENGLISH_LANGUAGE
#define DEFAULT_TARGET_INT_LANGUAGE UNKNOWN_LANGUAGE
#define DEFAULT_CONVERSION_COLO_XD 1
#define DEFAULT_DEFAULT_CONVERSION_GAME FR_VERSION_ID
#define DEFAULT_GEN1_EVERSTONE 0
#define DEFAULT_ALLOW_CROSS_GEN_EVOS 0

static u8 sys_language;
static u8 target_int_language;
static u8 conversion_colo_xd;
static u8 default_conversion_game;
static u8 default_colours[NUM_COLOURS][NUM_SUB_COLOURS];
static u8 gen1_everstone;
static u8 allow_cross_gen_evos;

void set_default_settings() {
    set_sys_language(DEFAULT_SYS_LANGUAGE);
    set_target_int_language(DEFAULT_TARGET_INT_LANGUAGE);
    set_conversion_colo_xd(DEFAULT_CONVERSION_COLO_XD);
    set_default_conversion_game(DEFAULT_DEFAULT_CONVERSION_GAME);
    for(size_t i = 0; i < NUM_COLOURS; i++)
        for(size_t j = 0; j < NUM_SUB_COLOURS; j++)
            set_single_colour(i, j, default_colours_bin[(i*NUM_SUB_COLOURS)+j]);
    set_gen1_everstone(DEFAULT_GEN1_EVERSTONE);
    set_allow_cross_gen_evos(DEFAULT_ALLOW_CROSS_GEN_EVOS);
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
}

void set_gen1_everstone(u8 new_val) {
    gen1_everstone = new_val;
}

void set_allow_cross_gen_evos(u8 new_val) {
    allow_cross_gen_evos = new_val;
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
