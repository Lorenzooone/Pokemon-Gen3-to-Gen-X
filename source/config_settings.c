#include "base_include.h"
#include "config_settings.h"
#include "useful_qualifiers.h"

#include "default_colours_bin.h"

static u8 default_colours[NUM_COLOURS][NUM_SUB_COLOURS];

void set_default_settings() {
    for(size_t i = 0; i < NUM_COLOURS; i++)
        for(size_t j = 0; j < NUM_SUB_COLOURS; j++)
            set_single_colour(i, j, default_colours_bin[(i*NUM_SUB_COLOURS)+j]);
}

void set_single_colour(u8 index, u8 sub_index, u8 new_colour) {
    if(index >= NUM_COLOURS)
        index = 0;
    if(sub_index >= NUM_SUB_COLOURS)
        sub_index = 0;
    default_colours[index][sub_index] = new_colour&0x1F;
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
