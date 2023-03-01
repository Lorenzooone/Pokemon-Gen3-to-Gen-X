#include <gba.h>
#include "config_settings.h"
#include "party_handler.h"
#include "useful_qualifiers.h"

#define DEFAULT_SYS_LANGUAGE ENGLISH_LANGUAGE
#define DEFAULT_TARGET_INT_LANGUAGE UNKNOWN_LANGUAGE

static u8 sys_language;
static u8 target_int_language;

void set_default_settings() {
    set_sys_language(DEFAULT_SYS_LANGUAGE);
    set_target_int_language(DEFAULT_TARGET_INT_LANGUAGE);
}

void set_sys_language(u8 new_val) {
    new_val = get_valid_language(new_val);
    sys_language = new_val;
}

void set_target_int_language(u8 new_val) {
    if(new_val != UNKNOWN_LANGUAGE)
        new_val = get_valid_language(new_val);
    target_int_language = new_val;
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
