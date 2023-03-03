#include <gba.h>
#include "print_system.h"
#include "window_handler.h"

#define HORIZONTAL_WINDOW_TILE 2
#define VERTICAL_WINDOW_TILE 3
#define ANGLE_WINDOW_TILE 4

void create_window(u8, u8, u8, u8, u8);
void reset_window(u8, u8, u8, u8, u8);

void init_trade_options_window() {
    create_window(TRADE_OPTIONS_WINDOW_X, TRADE_OPTIONS_WINDOW_Y, TRADE_OPTIONS_WINDOW_X_SIZE, TRADE_OPTIONS_WINDOW_Y_SIZE, TRADE_OPTIONS_WINDOW_SCREEN);
}

void clear_trade_options_window() {
    reset_window(TRADE_OPTIONS_WINDOW_X, TRADE_OPTIONS_WINDOW_Y, TRADE_OPTIONS_WINDOW_X_SIZE, TRADE_OPTIONS_WINDOW_Y_SIZE, TRADE_OPTIONS_WINDOW_SCREEN);
}

void init_message_window() {
    create_window(MESSAGE_WINDOW_X, MESSAGE_WINDOW_Y, MESSAGE_WINDOW_X_SIZE, MESSAGE_WINDOW_Y_SIZE, MESSAGE_WINDOW_SCREEN);
}

void clear_message_window() {
    reset_window(MESSAGE_WINDOW_X, MESSAGE_WINDOW_Y, MESSAGE_WINDOW_X_SIZE, MESSAGE_WINDOW_Y_SIZE, MESSAGE_WINDOW_SCREEN);
}

void init_rejected_window(void) {
    create_window(REJECTED_WINDOW_X, REJECTED_WINDOW_Y, REJECTED_WINDOW_X_SIZE, REJECTED_WINDOW_Y_SIZE, REJECTED_WINDOW_SCREEN);
}

void clear_rejected_window(void) {
    reset_window(REJECTED_WINDOW_X, REJECTED_WINDOW_Y, REJECTED_WINDOW_X_SIZE, REJECTED_WINDOW_Y_SIZE, REJECTED_WINDOW_SCREEN);
}

void init_crash_window() {
    create_window(CRASH_WINDOW_X, CRASH_WINDOW_Y, CRASH_WINDOW_X_SIZE, CRASH_WINDOW_Y_SIZE, CRASH_WINDOW_SCREEN);
}

void clear_crash_window() {
    reset_window(CRASH_WINDOW_X, CRASH_WINDOW_Y, CRASH_WINDOW_X_SIZE, CRASH_WINDOW_Y_SIZE, CRASH_WINDOW_SCREEN);
}

void init_waiting_window(s8 y_increase) {
    create_window(WAITING_WINDOW_X, WAITING_WINDOW_Y + y_increase, WAITING_WINDOW_X_SIZE, WAITING_WINDOW_Y_SIZE, WAITING_WINDOW_SCREEN);
}

void clear_waiting_window(s8 y_increase) {
    reset_window(WAITING_WINDOW_X, WAITING_WINDOW_Y + y_increase, WAITING_WINDOW_X_SIZE, WAITING_WINDOW_Y_SIZE, WAITING_WINDOW_SCREEN);
}

void init_learn_move_message_window() {
    create_window(LEARN_MOVE_MESSAGE_WINDOW_X, LEARN_MOVE_MESSAGE_WINDOW_Y, LEARN_MOVE_MESSAGE_WINDOW_X_SIZE, LEARN_MOVE_MESSAGE_WINDOW_Y_SIZE, LEARN_MOVE_MESSAGE_WINDOW_SCREEN);
}

void clear_learn_move_message_window() {
    reset_window(LEARN_MOVE_MESSAGE_WINDOW_X, LEARN_MOVE_MESSAGE_WINDOW_Y, LEARN_MOVE_MESSAGE_WINDOW_X_SIZE, LEARN_MOVE_MESSAGE_WINDOW_Y_SIZE, LEARN_MOVE_MESSAGE_WINDOW_SCREEN);
}

void init_trade_animation_send_window() {
    create_window(TRADE_ANIMATION_SEND_WINDOW_X, TRADE_ANIMATION_SEND_WINDOW_Y, TRADE_ANIMATION_SEND_WINDOW_X_SIZE, TRADE_ANIMATION_SEND_WINDOW_Y_SIZE, TRADE_ANIMATION_SEND_WINDOW_SCREEN);
}

void clear_trade_animation_send_window() {
    reset_window(TRADE_ANIMATION_SEND_WINDOW_X, TRADE_ANIMATION_SEND_WINDOW_Y, TRADE_ANIMATION_SEND_WINDOW_X_SIZE, TRADE_ANIMATION_SEND_WINDOW_Y_SIZE, TRADE_ANIMATION_SEND_WINDOW_SCREEN);
}

void init_trade_animation_recv_window() {
    create_window(TRADE_ANIMATION_RECV_WINDOW_X, TRADE_ANIMATION_RECV_WINDOW_Y, TRADE_ANIMATION_RECV_WINDOW_X_SIZE, TRADE_ANIMATION_RECV_WINDOW_Y_SIZE, TRADE_ANIMATION_RECV_WINDOW_SCREEN);
}

void clear_trade_animation_recv_window() {
    reset_window(TRADE_ANIMATION_RECV_WINDOW_X, TRADE_ANIMATION_RECV_WINDOW_Y, TRADE_ANIMATION_RECV_WINDOW_X_SIZE, TRADE_ANIMATION_RECV_WINDOW_Y_SIZE, TRADE_ANIMATION_RECV_WINDOW_SCREEN);
}

void init_evolution_animation_window() {
    create_window(EVOLUTION_ANIMATION_WINDOW_X, EVOLUTION_ANIMATION_WINDOW_Y, EVOLUTION_ANIMATION_WINDOW_X_SIZE, EVOLUTION_ANIMATION_WINDOW_Y_SIZE, EVOLUTION_ANIMATION_WINDOW_SCREEN);
}

void clear_evolution_animation_window() {
    reset_window(EVOLUTION_ANIMATION_WINDOW_X, EVOLUTION_ANIMATION_WINDOW_Y, EVOLUTION_ANIMATION_WINDOW_X_SIZE, EVOLUTION_ANIMATION_WINDOW_Y_SIZE, EVOLUTION_ANIMATION_WINDOW_SCREEN);
}

void init_saving_window() {
    create_window(SAVING_WINDOW_X, SAVING_WINDOW_Y, SAVING_WINDOW_X_SIZE, SAVING_WINDOW_Y_SIZE, SAVING_WINDOW_SCREEN);
}

void clear_saving_window() {
    reset_window(SAVING_WINDOW_X, SAVING_WINDOW_Y, SAVING_WINDOW_X_SIZE, SAVING_WINDOW_Y_SIZE, SAVING_WINDOW_SCREEN);
}

void init_colour_window() {
    create_window(COLOURS_WINDOW_X, COLOURS_WINDOW_Y, COLOURS_WINDOW_X_SIZE, COLOURS_WINDOW_Y_SIZE, COLOURS_WINDOW_SCREEN);
}

void clear_colour_window() {
        reset_window(COLOURS_WINDOW_X, COLOURS_WINDOW_Y, COLOURS_WINDOW_X_SIZE, COLOURS_WINDOW_Y_SIZE, COLOURS_WINDOW_SCREEN);
}

void init_evolution_window(u8 num_entries) {
    create_window(EVOLUTION_WINDOW_X, EVOLUTION_WINDOW_Y-(EVOLUTION_WINDOW_Y_SIZE_INCREMENT*num_entries), EVOLUTION_WINDOW_X_SIZE, EVOLUTION_WINDOW_Y_SIZE + (EVOLUTION_WINDOW_Y_SIZE_INCREMENT*num_entries), EVOLUTION_WINDOW_SCREEN);
}

void clear_evolution_window(u8 num_entries) {
    reset_window(EVOLUTION_WINDOW_X, EVOLUTION_WINDOW_Y-(EVOLUTION_WINDOW_Y_SIZE_INCREMENT*num_entries), EVOLUTION_WINDOW_X_SIZE, EVOLUTION_WINDOW_Y_SIZE + (EVOLUTION_WINDOW_Y_SIZE_INCREMENT*num_entries), EVOLUTION_WINDOW_SCREEN);
}

void init_loading_window() {
    create_window(LOADING_WINDOW_X, LOADING_WINDOW_Y, LOADING_WINDOW_X_SIZE, LOADING_WINDOW_Y_SIZE, LOADING_WINDOW_SCREEN);
}

void clear_loading_window() {
    reset_window(LOADING_WINDOW_X, LOADING_WINDOW_Y, LOADING_WINDOW_X_SIZE, LOADING_WINDOW_Y_SIZE, LOADING_WINDOW_SCREEN);
}

void init_offer_window() {
    create_window(OFFER_WINDOW_X, OFFER_WINDOW_Y, OFFER_WINDOW_X_SIZE, OFFER_WINDOW_Y_SIZE, OFFER_WINDOW_SCREEN);
}

void clear_offer_window() {
    reset_window(OFFER_WINDOW_X, OFFER_WINDOW_Y, OFFER_WINDOW_X_SIZE, OFFER_WINDOW_Y_SIZE, OFFER_WINDOW_SCREEN);
}

void init_offer_options_window() {
    create_window(OFFER_OPTIONS_WINDOW_X, OFFER_OPTIONS_WINDOW_Y, OFFER_OPTIONS_WINDOW_X_SIZE, OFFER_OPTIONS_WINDOW_Y_SIZE, OFFER_OPTIONS_WINDOW_SCREEN);
}

void clear_offer_options_window() {
    reset_window(OFFER_OPTIONS_WINDOW_X, OFFER_OPTIONS_WINDOW_Y, OFFER_OPTIONS_WINDOW_X_SIZE, OFFER_OPTIONS_WINDOW_Y_SIZE, OFFER_OPTIONS_WINDOW_SCREEN);
}

void create_window(u8 x, u8 y, u8 x_size, u8 y_size, u8 screen_num) {
    set_updated_screen();
    screen_t* screen = get_screen(screen_num);
    
    if(2+x+x_size > X_SIZE)
        x_size = 0;
    if(2+y+y_size > Y_SIZE)
        y_size = 0;
    
    if((!x_size) || (!y_size))
        return;
    
    u8 start_x = x-1;
    u8 start_y = y-1;
    if(!x)
        start_x = X_SIZE-1;
    if(!y)
        start_y = Y_SIZE-1;
    
    u8 end_x = x + x_size;
    u8 end_y = y + y_size;
    if(end_x >= X_SIZE)
        end_x -= X_SIZE;
    if(end_y >= Y_SIZE)
        end_y -= Y_SIZE;
    
    for(int i = 0; i < x_size; i++) {
        screen[x+(start_y*X_SIZE)+i] = (PALETTE<<12) | HORIZONTAL_WINDOW_TILE;
        screen[x+(end_y*X_SIZE)+i] = (PALETTE<<12) | HORIZONTAL_WINDOW_TILE | VSWAP_TILE;
    }
    
    for(int i = 0; i < y_size; i++) {
        screen[start_x+((y+i)*X_SIZE)] = (PALETTE<<12) | VERTICAL_WINDOW_TILE;
        screen[end_x+((y+i)*X_SIZE)] = (PALETTE<<12) | VERTICAL_WINDOW_TILE | HSWAP_TILE;
    }
    
    screen[start_x+(start_y*X_SIZE)] = (PALETTE<<12) | ANGLE_WINDOW_TILE;
    screen[end_x+(start_y*X_SIZE)] = (PALETTE<<12) | ANGLE_WINDOW_TILE | HSWAP_TILE;
    screen[start_x+(end_y*X_SIZE)] = (PALETTE<<12) | ANGLE_WINDOW_TILE | VSWAP_TILE;
    screen[end_x+(end_y*X_SIZE)] = (PALETTE<<12) | ANGLE_WINDOW_TILE | HSWAP_TILE | VSWAP_TILE;
}

void reset_window(u8 x, u8 y, u8 x_size, u8 y_size, u8 screen_num){
    set_updated_screen();
    screen_t* screen = get_screen(screen_num);

    for(int i = 0; i < y_size; i++) {
        u8 inside_y = y + i;
        if(inside_y >= Y_SIZE)
            inside_y -= Y_SIZE;
        for(int j = 0; j < x_size; j++) {
            u8 inside_x = x + j;
            if(inside_x >= X_SIZE)
                inside_x -= X_SIZE;
            screen[inside_x+(inside_y*X_SIZE)] = (PALETTE<<12) | 0;
        }
    }
}
