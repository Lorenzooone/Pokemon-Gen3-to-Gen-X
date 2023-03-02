#include <gba.h>
#include "config_settings.h"
#include "version_identifier.h"
#include "party_handler.h"
#include "useful_qualifiers.h"

#define DEFAULT_SYS_LANGUAGE ENGLISH_LANGUAGE
#define DEFAULT_TARGET_INT_LANGUAGE UNKNOWN_LANGUAGE
#define DEFAULT_CONVERSION_COLO_XD 1
#define DEFAULT_DEFAULT_CONVERSION_GAME FR_VERSION_ID

static u8 sys_language;
static u8 target_int_language;
static u8 conversion_colo_xd;
static u8 default_conversion_game;

void set_default_settings() {
    set_sys_language(DEFAULT_SYS_LANGUAGE);
    set_target_int_language(DEFAULT_TARGET_INT_LANGUAGE);
    set_conversion_colo_xd(DEFAULT_CONVERSION_COLO_XD);
    set_default_conversion_game(DEFAULT_DEFAULT_CONVERSION_GAME);
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
