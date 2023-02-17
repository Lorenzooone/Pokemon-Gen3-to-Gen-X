#include <gba.h>
#include "party_handler.h"
#include "sprite_handler.h"
#include "animations_handler.h"
#include "graphics_handler.h"
#include "timing_basic.h"
#include <stddef.h>

#define OWN_SPRITE_INDEX 0
#define OTHER_SPRITE_INDEX 1
#define EVOLUTION_SPRITE_INDEX 2
#define TOTAL_SPRITES 3

#define WAIT_TRADE_END (2*FPS)
#define WAIT_EVO_START (2*FPS)
#define EVO_PHASE_TIME (2*FPS)
#define EVO_FADES_NUM 16
#define EVO_FADES_LIMIT (EVO_FADES_NUM + 1)
#define EVO_SINGLE_FADES_TIME ((EVO_PHASE_TIME+EVO_FADES_LIMIT-1)/EVO_FADES_LIMIT)
#define WAIT_EVO_END (3*FPS)

#define FADE_VAL_BASE 1

#define FIRST_SPEEDUP_THRESHOLD (BASE_Y_SPRITE_TRADE_ANIMATION_SEND+BASE_Y_SPRITE_TRADE_ANIMATION_END_RUN_INC-8)
#define SECOND_SPEEDUP_THRESHOLD (FIRST_SPEEDUP_THRESHOLD-16)
#define THIRD_SPEEDUP_THRESHOLD (SECOND_SPEEDUP_THRESHOLD-32)

void advance_evolution_animation(void);

u8 sprite_indexes[TOTAL_SPRITES];
u8 sprite_ys[TOTAL_SPRITES];
u8 screen_nums[TOTAL_SPRITES];
u16 animation_counter;
u8 animation_completed;
u8 animate_evolution;
u8 is_animating_evolution;
u8 curr_fade_val;

enum EVOLUTION_ANIMATION_STATE {START, FADE_IN, REVERSE_FADE_IN, END};
enum EVOLUTION_ANIMATION_STATE curr_evolution_animation_state;

void setup_evolution_animation(struct gen3_mon_data_unenc* own_mon, u8 evo_screen) {
    sprite_indexes[EVOLUTION_SPRITE_INDEX] = get_next_sprite_index();
    screen_nums[EVOLUTION_SPRITE_INDEX] = evo_screen;
    load_pokemon_sprite_raw(own_mon, 0, BASE_Y_SPRITE_TRADE_ANIMATION_RECV, BASE_X_SPRITE_TRADE_ANIMATION);
    animate_evolution = 1;
}

void setup_trade_animation(struct gen3_mon_data_unenc* own_mon, struct gen3_mon_data_unenc* other_mon, u8 own_screen, u8 other_screen) {
    sprite_indexes[OWN_SPRITE_INDEX] = get_next_sprite_index();
    sprite_ys[OWN_SPRITE_INDEX] = BASE_Y_SPRITE_TRADE_ANIMATION_SEND;
    screen_nums[OWN_SPRITE_INDEX] = own_screen;
    load_pokemon_sprite_raw(own_mon, 0, BASE_Y_SPRITE_TRADE_ANIMATION_SEND, BASE_X_SPRITE_TRADE_ANIMATION);
    sprite_indexes[OTHER_SPRITE_INDEX] = get_next_sprite_index();
    sprite_ys[OTHER_SPRITE_INDEX] = BASE_Y_SPRITE_TRADE_ANIMATION_RECV;
    screen_nums[OTHER_SPRITE_INDEX] = other_screen;
    load_pokemon_sprite_raw(other_mon, 0, BASE_Y_SPRITE_TRADE_ANIMATION_RECV, BASE_X_SPRITE_TRADE_ANIMATION);
    animation_counter = 0;
    animation_completed = 0;
    animate_evolution = 0;
    is_animating_evolution = 0;
}

u8 get_animation_state() {
    return animation_completed;
}

void advance_evolution_animation() {

    if(animation_completed)
        return;

    animation_counter++;
    
    switch(curr_evolution_animation_state) {
        case START:
            if(animation_counter == WAIT_EVO_START) {
                curr_evolution_animation_state = FADE_IN;
                swap_screen_enabled_state(screen_nums[EVOLUTION_SPRITE_INDEX]);
                prepare_flush();
                animation_counter = 0;
                curr_fade_val = 0;
                fade_all_sprites_to_white(curr_fade_val + FADE_VAL_BASE);
            }
            break;
        case FADE_IN:
            if(animation_counter == EVO_SINGLE_FADES_TIME) {
                curr_fade_val++;
                if(curr_fade_val == EVO_FADES_NUM) {
                    curr_evolution_animation_state = REVERSE_FADE_IN;
                    raw_update_sprite_y(sprite_indexes[EVOLUTION_SPRITE_INDEX], sprite_ys[OTHER_SPRITE_INDEX]);
                    raw_update_sprite_y(sprite_indexes[OTHER_SPRITE_INDEX], sprite_ys[OWN_SPRITE_INDEX]);
                }
                else
                    fade_all_sprites_to_white(curr_fade_val + FADE_VAL_BASE);
                animation_counter = 0;
            }
            break;
        case REVERSE_FADE_IN:
            if(animation_counter == EVO_SINGLE_FADES_TIME) {
                if(!curr_fade_val) {
                    curr_evolution_animation_state = END;
                    remove_fade_all_sprites();
                    swap_buffer_screen(screen_nums[EVOLUTION_SPRITE_INDEX], 0);
                    swap_screen_enabled_state(screen_nums[EVOLUTION_SPRITE_INDEX]);
                    prepare_flush();
                }
                else{
                    curr_fade_val--;
                    fade_all_sprites_to_white(curr_fade_val + FADE_VAL_BASE);
                }
                animation_counter = 0;
            }
            break;
        case END:
            if(animation_counter == WAIT_EVO_END) {
                animation_completed = 1;
                swap_screen_enabled_state(screen_nums[EVOLUTION_SPRITE_INDEX]);
                prepare_flush();
                animation_counter = 0;
                is_animating_evolution = 0;
            }
            break;
        default:
            break;
    }

}

void advance_trade_animation() {

    if(animation_completed)
        return;
    
    if(is_animating_evolution) {
        advance_evolution_animation();
        return;
    }

    animation_counter++;
    
    if(sprite_ys[OTHER_SPRITE_INDEX] == BASE_Y_SPRITE_TRADE_ANIMATION_SEND) {
        if(animation_counter == WAIT_TRADE_END) {
            if(!animate_evolution)
                animation_completed = 1;
            else {
                is_animating_evolution = 1;
                swap_screen_enabled_state(screen_nums[EVOLUTION_SPRITE_INDEX]);
                curr_evolution_animation_state = START;
            }
            swap_screen_enabled_state(screen_nums[OTHER_SPRITE_INDEX]);
            prepare_flush();
            animation_counter = 0;
        }
        return;
    }

    u8 index = OWN_SPRITE_INDEX;
    s32 inc_type = -1;
    if(sprite_ys[OWN_SPRITE_INDEX] == BASE_Y_SPRITE_TRADE_ANIMATION_RECV) {
        index = OTHER_SPRITE_INDEX;
        inc_type = +1;
    }
    
    u8 curr_y = sprite_ys[index] + BASE_Y_SPRITE_TRADE_ANIMATION_END_RUN_INC;
    u8 scheduled_change = 0;

    if(curr_y < THIRD_SPEEDUP_THRESHOLD)
        scheduled_change = 1;
    else if(curr_y < SECOND_SPEEDUP_THRESHOLD) {
        if(animation_counter == 2)
            scheduled_change = 1;
    }
    else if(curr_y < FIRST_SPEEDUP_THRESHOLD) {
        if(animation_counter == 4)
            scheduled_change = 1;
    }
    else {
        if(animation_counter == 8)
            scheduled_change = 1;
    }
    
    if((scheduled_change) && (curr_y == THIRD_SPEEDUP_THRESHOLD)) {
        swap_screen_enabled_state(screen_nums[index]);
        prepare_flush();
    }
    
    if(scheduled_change) {
        sprite_ys[index] += inc_type;
        animation_counter = 0;
        raw_update_sprite_y(sprite_indexes[index], sprite_ys[index]);
    }
}
