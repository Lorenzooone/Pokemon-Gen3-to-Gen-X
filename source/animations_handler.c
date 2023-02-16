#include <gba.h>
#include "party_handler.h"
#include "sprite_handler.h"
#include "animations_handler.h"
#include "graphics_handler.h"
#include <stddef.h>

#define OWN_SPRITE_INDEX 0
#define OTHER_SPRITE_INDEX 1
#define EVOLUTION_SPRITE_INDEX 0
#define TOTAL_SPRITES 2

#define FIRST_SPEEDUP_THRESHOLD (BASE_Y_SPRITE_TRADE_ANIMATION_SEND+BASE_Y_SPRITE_TRADE_ANIMATION_END_RUN_INC-8)
#define SECOND_SPEEDUP_THRESHOLD (FIRST_SPEEDUP_THRESHOLD-16)
#define THIRD_SPEEDUP_THRESHOLD (SECOND_SPEEDUP_THRESHOLD-32)

u8 sprite_indexes[TOTAL_SPRITES];
u8 sprite_ys[TOTAL_SPRITES];
u8 screen_nums[TOTAL_SPRITES];
u8 trade_animation_counter;
u8 trade_animation_completed;

void setup_trade_animation(struct gen3_mon_data_unenc* own_mon, struct gen3_mon_data_unenc* other_mon, u8 own_screen, u8 other_screen) {
    sprite_indexes[OWN_SPRITE_INDEX] = get_next_sprite_index();
    sprite_ys[OWN_SPRITE_INDEX] = BASE_Y_SPRITE_TRADE_ANIMATION_SEND;
    screen_nums[OWN_SPRITE_INDEX] = own_screen;
    load_pokemon_sprite_raw(own_mon, 0, BASE_Y_SPRITE_TRADE_ANIMATION_SEND, BASE_X_SPRITE_TRADE_ANIMATION);
    sprite_indexes[OTHER_SPRITE_INDEX] = get_next_sprite_index();
    sprite_ys[OTHER_SPRITE_INDEX] = BASE_Y_SPRITE_TRADE_ANIMATION_RECV;
    screen_nums[OTHER_SPRITE_INDEX] = other_screen;
    load_pokemon_sprite_raw(other_mon, 0, BASE_Y_SPRITE_TRADE_ANIMATION_RECV, BASE_X_SPRITE_TRADE_ANIMATION);
    trade_animation_counter = 0;
    trade_animation_completed = 0;
}

u8 get_trade_animation_state() {
    return trade_animation_completed;
}

void advance_trade_animation() {
    u8 index = OWN_SPRITE_INDEX;
    s32 inc_type = -1;
    if(sprite_ys[OWN_SPRITE_INDEX] == BASE_Y_SPRITE_TRADE_ANIMATION_RECV) {
        index = OTHER_SPRITE_INDEX;
        inc_type = +1;
    }
    
    if(trade_animation_completed)
        return;
    
    u8 curr_y = sprite_ys[index] + BASE_Y_SPRITE_TRADE_ANIMATION_END_RUN_INC;
    u8 scheduled_change = 0;
    trade_animation_counter++;

    if(curr_y < THIRD_SPEEDUP_THRESHOLD)
        scheduled_change = 1;
    else if(curr_y < SECOND_SPEEDUP_THRESHOLD) {
        if(trade_animation_counter == 2)
            scheduled_change = 1;
    }
    else if(curr_y < FIRST_SPEEDUP_THRESHOLD) {
        if(trade_animation_counter == 4)
            scheduled_change = 1;
    }
    else {
        if(trade_animation_counter == 8)
            scheduled_change = 1;
    }
    
    if((scheduled_change) && (curr_y == THIRD_SPEEDUP_THRESHOLD)) {
        swap_screen_enabled_state(screen_nums[index]);
        prepare_flush();
    }
    
    if(scheduled_change) {
        sprite_ys[index] += inc_type;
        trade_animation_counter = 0;
        raw_update_sprite_y(sprite_indexes[index], sprite_ys[index]);
    }
    
    if(sprite_ys[OTHER_SPRITE_INDEX] == BASE_Y_SPRITE_TRADE_ANIMATION_SEND)
        trade_animation_completed = 1;
}
