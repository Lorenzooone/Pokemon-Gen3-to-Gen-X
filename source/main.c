#include <gba.h>
#include "multiboot_handler.h"
#include "graphics_handler.h"
#include "party_handler.h"
#include "text_handler.h"
#include "sprite_handler.h"
#include "version_identifier.h"
#include "gen3_save.h"
#include "gen3_clock_events.h"
#include "options_handler.h"
#include "input_handler.h"
#include "menu_text_handler.h"
#include "sio_buffers.h"
#include "rng.h"
#include "pid_iv_tid.h"
#include "print_system.h"
#include "window_handler.h"
#include "communicator.h"
#include "gen_converter.h"
#include "sio.h"
#include "vcount_basic.h"
#include "animations_handler.h"
#include <stddef.h>
#include "optimized_swi.h"
#include "timing_basic.h"
#include "config_settings.h"
//#include "save.h"

#include "ewram_speed_check_bin.h"

#define REG_MEMORY_CONTROLLER_ADDR 0x4000800
#define HW_SET_REG_MEMORY_CONTROLLER_VALUE 0x0D000020
#define REG_MEMORY_CONTROLLER *((u32*)(REG_MEMORY_CONTROLLER_ADDR))

#define WORST_CASE_EWRAM 0
#define TEST_LAG_EWRAM 0
#define MIN_WAITCYCLE 1

#define WAITING_TIME_MOVE_MESSAGES (2*FPS)
#define WAITING_TIME_BEFORE_RESPONSE (1*FPS)
#define WAITING_TIME_REJECTED (2*FPS)
#define MAX_RANDOM_WAIT_TIME (1*FPS)

#define BASE_SCREEN 0
#define LEARNABLE_MOVES_MENU_SCREEN 2
#define TRADE_CANCEL_SCREEN 1
#define INFO_SCREEN 3
#define NATURE_SCREEN 2
#define IV_FIX_SCREEN 2
#define EVOLUTION_MENU_SCREEN 2

void vblank_update_function(void);
void find_optimal_ewram_settings(void);
void disable_all_irqs(void);
u8 init_cursor_y_pos_main_menu(void);
void cursor_update_learnable_move_message_menu(u8);
void cursor_update_learnable_move_menu(u8);
void cursor_update_trading_menu(u8, u8);
void cursor_update_main_menu(u8);
void cursor_update_cheats_menu(u8);
void cursor_update_trade_options(u8);
void cursor_update_offer_options(u8, u8);
void cursor_update_base_settings_menu(u8);
void cursor_update_clock_settings_menu(u8);
void cursor_update_clock_warning(u8);
void cursor_update_colours_settings_menu(u8, u8);
void cursor_update_evolutions_menu(u8, u16);
void handle_learnable_moves_message(struct game_data_t*, u8, u8*, u32*, enum MOVES_PRINTING_TYPE);
void change_nature(struct game_data_t*, u8, u8, u8*, u8);
void check_bad_trade_received(struct game_data_t*, struct game_data_priv_t*, u8, u8, u8, u8, u8, u8*);
void trade_cancel_print_screen(u8);
void saving_print_screen(void);
void loading_print_screen(void);
void rejected_print_screen(u8);
void trading_animation_init(struct game_data_t*, u8, u8, u8);
void evolution_animation_init(struct game_data_t*, u8);
void offer_init(struct game_data_t*, u8, u8, u8*, u8*, u8);
void waiting_init(s8);
void invalid_init(u8);
void waiting_offer_init(u8, u8);
void waiting_accept_init(u8);
void waiting_success_init(void);
void trade_options_init(u8, u8*, u8, u8);
void trade_menu_init(struct game_data_t*, struct game_data_priv_t*, u8, u8, u8, u8, u8, u8*, u8*);
void start_trade_init(struct game_data_t*, struct game_data_priv_t*, u8, u8, u8, u8, u8*);
void main_menu_init(struct game_data_t*, struct game_data_priv_t*, u8, u8, u8, u8*);
void info_menu_init(struct game_data_t*, u8, u8, u8*, u8);
void nature_menu_init(struct game_data_t*, u8, u8, u8*);
void iv_fix_menu_init(struct game_data_t*, u8, u8);
void base_settings_menu_init(struct game_data_t*, u8*);
void load_warnings_menu_init(struct game_data_t*, struct game_data_priv_t*);
void colours_settings_menu_init(u8*, u8*);
void clock_settings_menu_init(struct game_data_priv_t*, struct saved_time_t*, u8*, u8);
void cheats_menu_init(u8*);
void clock_warning_menu_init(u8*);
void learnable_moves_message_init(struct game_data_t*, u8);
void learnable_move_menu_init(struct game_data_t*, u8, u8, u8*);
void evolution_menu_init(struct game_data_t*, u8, u8, u8*, u16);
void conclude_trade(struct game_data_t*, struct game_data_priv_t*, u8, u8, u8, u8*);
void return_to_trade_menu(struct game_data_t*, struct game_data_priv_t*, u8, u8, u8, u8, u8, u8*, u8*);
void prepare_crash_screen(enum CRASH_REASONS);
void crash_on_cartridge_removed(void);
void crash_on_bad_save(u8, u8);
void crash_on_bad_trade(void);
void wait_frames(int);
void complete_save_menu(struct game_data_t*, struct game_data_priv_t*, u8, u8);
void complete_cartridge_loading(struct game_data_t*, struct game_data_priv_t*, u8, u8, u8, u8*);
int main(void);

enum STATE {MAIN_MENU, MULTIBOOT, TRADING_MENU, INFO_MENU, START_TRADE, WAITING_DATA, TRADE_OPTIONS, NATURE_SETTING, OFFER_MENU, TRADING_ANIMATION, OFFER_INFO_MENU, IV_FIX_MENU, LEARNABLE_MOVES_MESSAGE,  LEARNABLE_MOVES_MESSAGE_MENU, LEARNABLE_MOVES_MENU, SWAP_CARTRIDGE_MENU, BASE_SETTINGS_MENU, COLOURS_SETTINGS_MENU, CLOCK_SETTINGS_MENU, CHEATS_MENU, EVOLUTION_MENU, WARNINGS_WHEN_LOADING, WARNING_WHEN_ADVANCING_CLOCK, PRINT_READ_INFO};
enum STATE curr_state;
u32 counter = 0;
u32 input_counter = 0;

IWRAM_CODE void vblank_update_function() {
	REG_IF |= IRQ_VBLANK;
	
    flush_screens();
    
    if(has_cartridge_been_removed())
        crash_on_cartridge_removed();
    
    move_sprites(counter);
    move_cursor_x(counter);
    advance_rng();
    counter++;
    
    // Handle trading animation
    if(curr_state == TRADING_ANIMATION)
        advance_trade_animation();
    // Handle slave communications
    if((REG_SIOCNT & SIO_IRQ) && (!(REG_SIOCNT & SIO_START)))
        slave_routine();
    // Handle no data received
    if((get_start_state_raw() == START_TRADE_PAR) && (REG_SIOCNT & SIO_IRQ) && (increment_last_tranfer() == NO_INFO_LIMIT)) {
        REG_SIODATA8 = SEND_0_INFO;
        REG_SIODATA32 = SEND_0_INFO;
        REG_IF &= ~IRQ_SERIAL;
        slave_routine();
    }
    // Handle master communications
    if(REG_DISPSTAT & LCDC_VCNT) {
        u16 next_vcount_irq = (REG_DISPSTAT >> 8);
        if(next_vcount_irq < VBLANK_SCANLINES)
            next_vcount_irq += SCANLINES;
        u16 curr_vcount = REG_VCOUNT + 2;
        if(curr_vcount < VBLANK_SCANLINES)
            curr_vcount += SCANLINES;
        if(next_vcount_irq <= curr_vcount)
            set_next_vcount_interrupt();
    }
}

IWRAM_CODE void find_optimal_ewram_settings() {
    size_t size = ewram_speed_check_bin_size>>2;
    const u32* ewram_speed_check = (const u32*)ewram_speed_check_bin;
    u32 test_data[size];
    
    // Check for unsupported (DS)
    if(REG_MEMORY_CONTROLLER != HW_SET_REG_MEMORY_CONTROLLER_VALUE)
        return;
    
    if(TEST_LAG_EWRAM)
        REG_MEMORY_CONTROLLER &= ~(0xF<<24);
    
    // Check for worst case testing (Not for final release)
    if(WORST_CASE_EWRAM)
        return;
    
    // Prepare data to test against
    for(size_t i = 0; i < size; i++)
        test_data[i] = ewram_speed_check[i];
    
    // Detetmine minimum number of stable waitcycles
    for(int i = 0; i < (16-MIN_WAITCYCLE); i++) {
        REG_MEMORY_CONTROLLER &= ~(0xF<<24);
        REG_MEMORY_CONTROLLER |= (15-i-MIN_WAITCYCLE)<<24;
        u8 failed = 0;
        for(size_t j = 0; (!failed) && (j < size); j++)
            if(test_data[j] != ewram_speed_check[j])
                failed = 1;
        if(!failed)
            return;
    }
}

void disable_all_irqs() {
    REG_IME = 0;
    REG_IE = 0;
}

u8 init_cursor_y_pos_main_menu() {
    if(!get_valid_options_main())
        return 4;
    return 0;
}

void cursor_update_trading_menu(u8 cursor_y_pos, u8 cursor_x_pos) {
    if(cursor_y_pos < PARTY_SIZE) {
        update_cursor_base_x(BASE_X_CURSOR_TRADING_MENU + (BASE_X_CURSOR_INCREMENT_TRADING_MENU * cursor_x_pos));
        update_cursor_y(BASE_Y_CURSOR_TRADING_MENU + (BASE_Y_CURSOR_INCREMENT_TRADING_MENU * cursor_y_pos));
    }
    else {
        update_cursor_base_x(CURSOR_X_POS_CANCEL);
        update_cursor_y(CURSOR_Y_POS_CANCEL);
    }
}

void cursor_update_main_menu(u8 cursor_y_pos) {
    update_cursor_y(BASE_Y_CURSOR_MAIN_MENU + (BASE_Y_CURSOR_INCREMENT_MAIN_MENU * cursor_y_pos));
}

void cursor_update_cheats_menu(u8 cursor_y_pos) {
    update_cursor_y(BASE_Y_CURSOR_CHEATS_MENU + (BASE_Y_CURSOR_INCREMENT_CHEATS_MENU * cursor_y_pos));
}

void cursor_update_learnable_move_menu(u8 cursor_y_pos) {
    update_cursor_base_x(BASE_X_CURSOR_LEARN_MOVE_MENU);
    update_cursor_y(BASE_Y_CURSOR_LEARN_MOVE_MENU + (cursor_y_pos*BASE_Y_CURSOR_INCREMENT_LEARN_MOVE_MENU));
}

void cursor_update_learnable_move_message_menu(u8 cursor_x_pos) {
    if(cursor_x_pos)
        update_cursor_base_x(BASE_X_CURSOR_LEARN_MOVE_MESSAGE_NO + (LEARN_MOVE_MESSAGE_WINDOW_X<<3));
    else
        update_cursor_base_x(BASE_X_CURSOR_LEARN_MOVE_MESSAGE_YES + (LEARN_MOVE_MESSAGE_WINDOW_X<<3));
    update_cursor_y(BASE_Y_CURSOR_LEARN_MOVE_MESSAGE);
}

void cursor_update_trade_options(u8 cursor_x_pos) {
    update_cursor_base_x(BASE_X_CURSOR_TRADE_OPTIONS + (cursor_x_pos * BASE_X_CURSOR_INCREMENT_TRADE_OPTIONS));
}

void cursor_update_offer_options(u8 cursor_y_pos, u8 cursor_x_pos) {
    update_cursor_base_x(BASE_X_CURSOR_OFFER_OPTIONS + (cursor_x_pos * BASE_X_CURSOR_INCREMENT_OFFER_OPTIONS));
    update_cursor_y(BASE_Y_CURSOR_OFFER_OPTIONS + (BASE_Y_CURSOR_INCREMENT_OFFER_OPTIONS * cursor_y_pos));
}

void cursor_update_clock_warning(u8 cursor_x_pos) {
    update_cursor_base_x(BASE_X_CURSOR_CLOCK_WARNING + (cursor_x_pos * BASE_X_CURSOR_INCREMENT_CLOCK_WARNING));
}

void cursor_update_base_settings_menu(u8 cursor_y_pos) {
    update_cursor_y(BASE_Y_CURSOR_BASE_SETTINGS_MENU + (BASE_Y_CURSOR_INCREMENT_BASE_SETTINGS_MENU * cursor_y_pos));
}

void cursor_update_clock_settings_menu(u8 cursor_y_pos) {
    update_cursor_y(BASE_Y_CURSOR_CLOCK_SETTINGS_MENU + (BASE_Y_CURSOR_INCREMENT_CLOCK_SETTINGS_MENU * cursor_y_pos));
}

void cursor_update_evolutions_menu(u8 cursor_y_pos, u16 num_evolutions) {
    update_cursor_y(BASE_Y_CURSOR_EVOLUTIONS_MENU - ((num_evolutions-1)*(EVOLUTION_WINDOW_Y_SIZE_INCREMENT<<3)) + (BASE_Y_CURSOR_INCREMENT_EVOLUTIONS_MENU * cursor_y_pos));
}

void cursor_update_colours_settings_menu(u8 cursor_y_pos, u8 cursor_x_pos) {
    if(!cursor_x_pos)
        update_cursor_base_x(BASE_X_CURSOR_COLOURS_SETTINGS_MENU);
    else
        update_cursor_base_x(BASE_X_CURSOR_COLOURS_SETTINGS_MENU_IN + ((cursor_x_pos-1) * BASE_X_CURSOR_INCREMENT_COLOURS_SETTINGS_MENU_IN));
    update_cursor_y(BASE_Y_CURSOR_COLOURS_SETTINGS_MENU + (BASE_Y_CURSOR_INCREMENT_COLOURS_SETTINGS_MENU * cursor_y_pos));
}

void wait_frames(int num_frames) {
    int wait_counter = 0;
    while(wait_counter < num_frames) {
        VBlankIntrWait();
        wait_counter++;
    }
}

void handle_learnable_moves_message(struct game_data_t* game_data, u8 own_mon, u8* cursor_x_pos, u32* curr_move, enum MOVES_PRINTING_TYPE moves_printing_type) {
    if(!game_data[0].party_3_undec[own_mon].is_egg) {
        print_learnable_move(&game_data[0].party_3_undec[own_mon], *curr_move, moves_printing_type);
        enable_screen(LEARN_MOVE_MESSAGE_WINDOW_SCREEN);
        if(moves_printing_type == LEARNABLE_P) {
            *cursor_x_pos = 0;
            cursor_update_learnable_move_message_menu(*cursor_x_pos);
        }
        prepare_flush();
        if(moves_printing_type != LEARNABLE_P) {
            wait_frames(WAITING_TIME_MOVE_MESSAGES);
            *curr_move += 1;
        }
    }
    else
        *curr_move += 1;
}

void change_nature(struct game_data_t* game_data, u8 cursor_x_pos, u8 curr_mon, u8* wanted_nature, u8 is_change_inc) {
    if((!game_data[cursor_x_pos].party_3_undec[curr_mon].is_valid_gen3) || (game_data[cursor_x_pos].party_3_undec[curr_mon].is_egg))
        return;

    u8 base_nature = get_nature(game_data[cursor_x_pos].party_3_undec[curr_mon].alter_nature.pid);
    u8 new_base_nature = base_nature;

    while(get_nature(game_data[cursor_x_pos].party_3_undec[curr_mon].alter_nature.pid) == base_nature) {
        if(is_change_inc)
            new_base_nature += 1;
        else
            new_base_nature += NUM_NATURES-1;
        
        if(new_base_nature >= NUM_NATURES)
            new_base_nature -= NUM_NATURES;

        alter_nature(&game_data[cursor_x_pos].party_3_undec[curr_mon], new_base_nature);
    }

    *wanted_nature = get_nature(game_data[cursor_x_pos].party_3_undec[curr_mon].alter_nature.pid);
}

void check_bad_trade_received(struct game_data_t* game_data, struct game_data_priv_t* game_data_priv, u8 target, u8 region, u8 master, u8 curr_gen, u8 own_menu, u8* cursor_y_pos) {
    u8 useless = 0;
    // Handle bad received / No valid mons
    if(handle_input_trading_menu(&useless, &useless, 0, curr_gen, own_menu) == CANCEL_TRADING) {
        if(own_menu)
            main_menu_init(&game_data[0], game_data_priv, target, region, master, cursor_y_pos);
        else
            waiting_offer_init(1, 0);
    }
}

void trade_cancel_print_screen(u8 update) {
    u8 prev_screen = get_screen_num();
    set_screen(TRADE_CANCEL_SCREEN);
    print_trade_menu_cancel(update);
    enable_screen(TRADE_CANCEL_SCREEN);
    set_screen(prev_screen);
}

void rejected_print_screen(u8 reason) {
    u8 prev_screen = get_screen_num();
    set_screen(REJECTED_WINDOW_SCREEN);
    print_rejected(reason);
    enable_screen(REJECTED_WINDOW_SCREEN);
    set_screen(prev_screen);
    prepare_flush();
}

void saving_print_screen() {
    u8 prev_screen = get_screen_num();
    set_screen(SAVING_WINDOW_SCREEN);
    print_saving();
    enable_screen(SAVING_WINDOW_SCREEN);
    set_screen(prev_screen);
    prepare_flush();
}

void loading_print_screen() {
    u8 prev_screen = get_screen_num();
    set_screen(LOADING_WINDOW_SCREEN);
    print_loading();
    enable_screen(LOADING_WINDOW_SCREEN);
    set_screen(prev_screen);
    prepare_flush();
}

void trading_animation_init(struct game_data_t* game_data, u8 own_mon, u8 other_mon, u8 print_messages) {
    set_screen(BASE_SCREEN);
    reset_sprites_to_party();
    disable_all_screens_but_current();
    disable_all_cursors();
    disable_all_sprites();
    default_reset_screen();
    if(print_messages) {
        set_screen(TRADE_ANIMATION_SEND_WINDOW_SCREEN);
        print_trade_animation_send(&game_data[0].party_3_undec[own_mon]);
        set_screen(TRADE_ANIMATION_RECV_WINDOW_SCREEN);
        print_trade_animation_recv(&game_data[1].party_3_undec[other_mon]);
        enable_screen(TRADE_ANIMATION_SEND_WINDOW_SCREEN);
    }
    set_screen(BASE_SCREEN);
    enable_screen(BASE_SCREEN);
    if(print_messages)
        setup_trade_animation(&game_data[0].party_3_undec[own_mon], &game_data[1].party_3_undec[other_mon], TRADE_ANIMATION_SEND_WINDOW_SCREEN, TRADE_ANIMATION_RECV_WINDOW_SCREEN);
    else
        prepare_evolution_animation_only(&game_data[0].party_3_undec[own_mon], TRADE_ANIMATION_RECV_WINDOW_SCREEN);
    prepare_flush();
}

void evolution_animation_init(struct game_data_t* game_data, u8 own_mon) {
    set_screen(EVOLUTION_ANIMATION_WINDOW_SCREEN);
    print_evolution_animation(&game_data[0].party_3_undec[own_mon]);
    set_screen(BASE_SCREEN);
    setup_evolution_animation(&game_data[0].party_3_undec[own_mon], EVOLUTION_ANIMATION_WINDOW_SCREEN);
    prepare_flush();
}

void offer_init(struct game_data_t* game_data, u8 own_mon, u8 other_mon, u8* cursor_y_pos, u8* cursor_x_pos, u8 reset) {
    curr_state = OFFER_MENU;
    set_screen(BASE_SCREEN);
    reset_sprites_to_party();
    disable_all_screens_but_current();
    disable_all_cursors();
    disable_all_sprites();
    set_screen(OFFER_WINDOW_SCREEN);
    print_offer_screen(game_data, own_mon, other_mon);
    set_screen(OFFER_OPTIONS_WINDOW_SCREEN);
    print_offer_options_screen(game_data, own_mon, other_mon);
    enable_screen(OFFER_WINDOW_SCREEN);
    enable_screen(OFFER_OPTIONS_WINDOW_SCREEN);
    if(reset) {
        *cursor_x_pos = 0;
        *cursor_y_pos = 0;
    }
    cursor_update_offer_options(*cursor_y_pos, *cursor_x_pos);
    prepare_flush();
}

void waiting_init(s8 y_increase) {
    curr_state = WAITING_DATA;
    set_screen(WAITING_WINDOW_SCREEN);
    print_waiting(y_increase);
    enable_screen(WAITING_WINDOW_SCREEN);
    prepare_flush();
}

void invalid_init(u8 reason) {
    set_screen(MESSAGE_WINDOW_SCREEN);
    print_invalid(reason);
    enable_screen(MESSAGE_WINDOW_SCREEN);
    prepare_flush();
}

void waiting_offer_init(u8 cancel, u8 cursor_y_pos) {
    waiting_init(0);
    if(cancel)
        try_to_end_trade();
    else
        try_to_offer(cursor_y_pos);
}

void waiting_accept_init(u8 decline) {
    waiting_init(0);
    if(decline)
        try_to_decline_offer();
    else
        try_to_accept_offer();
}

void waiting_success_init() {
    waiting_init((((BASE_Y_SPRITE_TRADE_ANIMATION_SEND+(POKEMON_SPRITE_Y_TILES<<3))>>3)+1)-WAITING_WINDOW_Y);
    try_to_success();
}

void trade_options_init(u8 cursor_x_pos, u8* submenu_cursor_x_pos, u8 own_menu, u8 evolve) {
    curr_state = TRADE_OPTIONS;
    set_screen(TRADE_OPTIONS_WINDOW_SCREEN);
    print_trade_options(cursor_x_pos, own_menu, evolve);
    enable_screen(TRADE_OPTIONS_WINDOW_SCREEN);
    *submenu_cursor_x_pos = 0;
    cursor_update_trade_options(*submenu_cursor_x_pos);
    update_cursor_y(BASE_Y_CURSOR_TRADE_OPTIONS);
    prepare_flush();
}

void trade_menu_init(struct game_data_t* game_data, struct game_data_priv_t* game_data_priv, u8 target, u8 region, u8 master, u8 curr_gen, u8 own_menu, u8* cursor_y_pos, u8* cursor_x_pos) {
    set_bg_pos(BASE_SCREEN, X_OFFSET_TRADE_MENU, Y_OFFSET_TRADE_MENU);
    curr_state = TRADING_MENU;
    *cursor_y_pos = 0;
    *cursor_x_pos = 0;
    prepare_options_trade(game_data, curr_gen, own_menu);
    print_trade_menu(game_data, 1, curr_gen, 1, own_menu);
    trade_cancel_print_screen(1);
    set_party_sprite_counter();
    cursor_update_trading_menu(*cursor_y_pos, *cursor_x_pos);
    enable_screen(BASE_SCREEN);
    check_bad_trade_received(game_data, game_data_priv, target, region, master, curr_gen, own_menu, cursor_y_pos);
    prepare_flush();
}

void start_trade_init(struct game_data_t* game_data, struct game_data_priv_t* game_data_priv, u8 target, u8 region, u8 master, u8 curr_gen, u8* cursor_y_pos) {
    if(is_valid_for_gen(game_data, game_data_priv, curr_gen)) {
        curr_state = START_TRADE;
        load_comm_buffer(game_data, &game_data_priv->party_2, &game_data_priv->party_1, curr_gen, region);
        init_start_state();
        set_screen(BASE_SCREEN);
        set_bg_pos(BASE_SCREEN, 0, 0);
        print_start_trade();
        disable_all_screens_but_current();
        reset_sprites_to_cursor(1);
        disable_all_cursors();
        enable_screen(BASE_SCREEN);
        prepare_flush();
        start_transfer(master, curr_gen);
    }
    else
        conclude_trade(game_data, game_data_priv, target, region, master, cursor_y_pos);
}

void main_menu_init(struct game_data_t* game_data, struct game_data_priv_t* game_data_priv, u8 target, u8 region, u8 master, u8* cursor_y_pos) {
    curr_state = MAIN_MENU;
    prepare_main_options(game_data, game_data_priv);
    set_screen(BASE_SCREEN);
    set_bg_pos(BASE_SCREEN, 0, 0);
    print_main_menu(1, target, region, master, game_data, game_data_priv);
    disable_all_screens_but_current();
    reset_sprites_to_cursor(1);
    disable_all_cursors();
    *cursor_y_pos = init_cursor_y_pos_main_menu();
    update_cursor_base_x(BASE_X_CURSOR_MAIN_MENU);
    cursor_update_main_menu(*cursor_y_pos);
    enable_screen(BASE_SCREEN);
    prepare_flush();
}

void load_warnings_menu_init(struct game_data_t* game_data, struct game_data_priv_t* game_data_priv) {
    curr_state = WARNINGS_WHEN_LOADING;
    set_screen(BASE_SCREEN);
    set_bg_pos(BASE_SCREEN, 0, 0);
    print_load_warnings(game_data, game_data_priv);
    disable_all_screens_but_current();
    reset_sprites_to_cursor(1);
    disable_all_cursors();
    enable_screen(BASE_SCREEN);
    prepare_flush();
}

void info_menu_init(struct game_data_t* game_data, u8 cursor_x_pos, u8 curr_mon, u8* curr_page, u8 is_offer) {
    if(is_offer)
        curr_state = OFFER_INFO_MENU;
    else
        curr_state = INFO_MENU;
    *curr_page = 1;
    set_screen(INFO_SCREEN);
    disable_all_sprites();
    print_pokemon_pages(1, 1, &game_data[cursor_x_pos].party_3_undec[curr_mon], *curr_page);
    enable_screen(INFO_SCREEN);
    prepare_flush();
}

void nature_menu_init(struct game_data_t* game_data, u8 cursor_x_pos, u8 curr_mon, u8* wanted_nature) {
    curr_state = NATURE_SETTING;
    set_screen(NATURE_SCREEN);
    disable_all_sprites();
    *wanted_nature = get_nature(game_data[cursor_x_pos].party_3_undec[curr_mon].src->pid);
    alter_nature(&game_data[cursor_x_pos].party_3_undec[curr_mon], *wanted_nature);
    print_set_nature(1, &game_data[cursor_x_pos].party_3_undec[curr_mon]);
    enable_screen(NATURE_SCREEN);
    prepare_flush();
}

void iv_fix_menu_init(struct game_data_t* game_data, u8 cursor_x_pos, u8 curr_mon) {
    curr_state = IV_FIX_MENU;
    set_screen(IV_FIX_SCREEN);
    disable_all_sprites();
    print_iv_fix(&game_data[cursor_x_pos].party_3_undec[curr_mon]);
    enable_screen(IV_FIX_SCREEN);
    prepare_flush();
}

void evolution_menu_init(struct game_data_t* game_data, u8 cursor_x_pos, u8 curr_mon, u8* cursor_y_pos, u16 num_evolutions) {
    curr_state = EVOLUTION_MENU;
    disable_all_sprites();
    *cursor_y_pos = 0;
    print_evolution_menu(&game_data[cursor_x_pos].party_3_undec[curr_mon], *cursor_y_pos, EVOLUTION_MENU_SCREEN, 1);
    enable_screen(EVOLUTION_MENU_SCREEN);
    set_screen(EVOLUTION_WINDOW_SCREEN);
    print_evolution_window(&game_data[cursor_x_pos].party_3_undec[curr_mon]);
    enable_screen(EVOLUTION_WINDOW_SCREEN);
    update_cursor_base_x(BASE_X_CURSOR_EVOLUTIONS_MENU);
    cursor_update_evolutions_menu(*cursor_y_pos, num_evolutions);
    prepare_flush();
}

void learnable_move_menu_init(struct game_data_t* game_data, u8 curr_mon, u8 curr_move, u8* cursor_y_pos) {
    curr_state = LEARNABLE_MOVES_MENU;
    set_screen(LEARNABLE_MOVES_MENU_SCREEN);
    disable_all_screens_but_current();
    disable_all_cursors();
    reset_sprites_to_cursor(0);
    print_learnable_moves_menu(&game_data[0].party_3_undec[curr_mon], curr_move);
    enable_screen(LEARNABLE_MOVES_MENU_SCREEN);
    *cursor_y_pos = 0;
    cursor_update_learnable_move_menu(*cursor_y_pos);
    prepare_flush();
}

void learnable_moves_message_init(struct game_data_t* game_data, u8 curr_mon) {
    curr_state = LEARNABLE_MOVES_MESSAGE;
    set_screen(BASE_SCREEN);
    disable_all_screens_but_current();
    disable_all_cursors();
    reset_sprites_to_cursor(0);
    default_reset_screen();
    load_pokemon_sprite_raw(&game_data[0].party_3_undec[curr_mon], 0, BASE_Y_SPRITE_TRADE_ANIMATION_SEND, BASE_X_SPRITE_TRADE_ANIMATION);
    set_screen(LEARN_MOVE_MESSAGE_WINDOW_SCREEN);
    enable_screen(BASE_SCREEN);
    prepare_flush();
}

void base_settings_menu_init(struct game_data_t* game_data, u8* cursor_y_pos) {
    curr_state = BASE_SETTINGS_MENU;
    set_screen(BASE_SCREEN);
    disable_all_screens_but_current();
    disable_all_cursors();
    print_base_settings_menu(&game_data->game_identifier, get_is_cartridge_loaded(), 1);
    enable_screen(BASE_SCREEN);
    *cursor_y_pos = 0;
    update_cursor_base_x(BASE_X_CURSOR_BASE_SETTINGS_MENU);
    cursor_update_base_settings_menu(*cursor_y_pos);
    prepare_flush();
}

void colours_settings_menu_init(u8* cursor_y_pos, u8* cursor_x_pos) {
    curr_state = COLOURS_SETTINGS_MENU;
    set_screen(BASE_SCREEN);
    disable_all_screens_but_current();
    disable_all_cursors();
    print_colour_settings_menu(1);
    enable_screen(BASE_SCREEN);
    *cursor_y_pos = 0;
    *cursor_x_pos = 0;
    cursor_update_colours_settings_menu(*cursor_y_pos, *cursor_x_pos);
    prepare_flush();
}

void clock_settings_menu_init(struct game_data_priv_t* game_data_priv, struct saved_time_t* time_change, u8* cursor_y_pos, u8 reset_time) {
    curr_state = CLOCK_SETTINGS_MENU;
    set_screen(BASE_SCREEN);
    disable_all_screens_but_current();
    disable_all_cursors();
    if(reset_time)
        wipe_time(time_change);
    print_clock_menu(&game_data_priv->clock_events, time_change, 1);
    enable_screen(BASE_SCREEN);
    *cursor_y_pos = 0;
    update_cursor_base_x(BASE_X_CURSOR_CLOCK_SETTINGS_MENU);
    cursor_update_clock_settings_menu(*cursor_y_pos);
    prepare_flush();
}

void clock_warning_menu_init(u8* cursor_x_pos) {
    curr_state = WARNING_WHEN_ADVANCING_CLOCK;
    set_screen(BASE_SCREEN);
    disable_all_screens_but_current();
    disable_all_cursors();
    print_warning_when_clock_changed();
    enable_screen(BASE_SCREEN);
    *cursor_x_pos = 1;
    update_cursor_y(BASE_Y_CURSOR_CLOCK_WARNING);
    cursor_update_clock_warning(*cursor_x_pos);
    prepare_flush();
}

void cheats_menu_init(u8* cursor_y_pos) {
    curr_state = CHEATS_MENU;
    set_screen(BASE_SCREEN);
    disable_all_screens_but_current();
    disable_all_cursors();
    print_cheats_menu(1);
    enable_screen(BASE_SCREEN);
    *cursor_y_pos = 0;
    cursor_update_cheats_menu(*cursor_y_pos);
    prepare_flush();
}

void conclude_trade(struct game_data_t* game_data, struct game_data_priv_t* game_data_priv, u8 target, u8 region, u8 master, u8* cursor_y_pos) {
    stop_transfer(master);
    main_menu_init(game_data, game_data_priv, target, region, master, cursor_y_pos);
}

void return_to_trade_menu(struct game_data_t* game_data, struct game_data_priv_t* game_data_priv, u8 target, u8 region, u8 master, u8 curr_gen, u8 own_menu, u8* cursor_y_pos, u8* cursor_x_pos) {
    curr_state = TRADING_MENU;
    set_screen(BASE_SCREEN);
    reset_sprites_to_party();
    disable_all_screens_but_current();
    disable_all_cursors();
    enable_all_sprites();
    trade_cancel_print_screen(1);
    cursor_update_trading_menu(*cursor_y_pos, *cursor_x_pos);
    enable_screen(BASE_SCREEN);
    check_bad_trade_received(game_data, game_data_priv, target, region, master, curr_gen, own_menu, cursor_y_pos);
    prepare_flush();
}

void prepare_crash_screen(enum CRASH_REASONS reason) {
    set_screen(BASE_SCREEN);
    default_reset_screen();
    enable_screen(BASE_SCREEN);
    reset_sprites_to_cursor(1);
    disable_all_screens_but_current();
    disable_all_cursors();
    set_screen(CRASH_WINDOW_SCREEN);
    print_crash(reason);
    enable_screen(CRASH_WINDOW_SCREEN);
    prepare_flush();
}

void crash_on_cartridge_removed() {
    prepare_crash_screen(CARTRIDGE_REMOVED);
    base_stop_transfer(0);
    base_stop_transfer(1);
    u8 curr_vcount = REG_VCOUNT + 1 + 1;
    if(curr_vcount >= SCANLINES)
        curr_vcount -= SCANLINES;
    while(REG_VCOUNT != curr_vcount);
    while(REG_VCOUNT != VBLANK_SCANLINES);
    disable_sprites_rendering();
    flush_screens();
    disable_all_irqs();
    while(1)
        Halt();
}

void crash_on_bad_save(u8 disable_transfer, u8 master) {
    prepare_crash_screen(BAD_SAVE);
    if(disable_transfer)
        stop_transfer(master);
    VBlankIntrWait();
    disable_all_irqs();
    while(1)
        Halt();
}

void crash_on_bad_trade() {
    prepare_crash_screen(BAD_TRADE);
    VBlankIntrWait();
    irqDisable(IRQ_VBLANK);
    while(1)
        VBlankIntrWait();
}

void complete_save_menu(struct game_data_t* game_data, struct game_data_priv_t* game_data_priv, u8 disable_transfer, u8 master) {
    u8 success = complete_write_gen_3_data(game_data);
    if(!success)
        crash_on_bad_save(disable_transfer, master);
    process_party_data(game_data, &game_data_priv->party_2, &game_data_priv->party_1);
}

void complete_cartridge_loading(struct game_data_t* game_data, struct game_data_priv_t* game_data_priv, u8 target, u8 region, u8 master, u8* cursor_y_pos) {
    init_game_data(game_data);
    get_game_id(&game_data->game_identifier);
    read_gen_3_data(game_data, game_data_priv);
    prepare_main_options(game_data, game_data_priv);
    if((!get_valid_options_main()) || (!loaded_data_has_warnings(game_data, game_data_priv)))
        main_menu_init(game_data, game_data_priv, target, region, master, cursor_y_pos);
    else
        load_warnings_menu_init(game_data, game_data_priv);
}

int main(void)
{
    RegisterRamReset(RESET_SIO|RESET_SOUND|RESET_OTHER);
    disable_all_irqs();
    curr_state = MAIN_MENU;
    counter = 0;
    input_counter = 0;
    find_optimal_ewram_settings();
    set_default_settings();
    init_text_system();
    init_enc_positions();
    init_rng(0,0);
    init_save_data();
    u16 keys;
    struct game_data_t game_data[2];
    struct game_data_priv_t game_data_priv;
    struct saved_time_t time_change;
    
    init_sprites();
    init_oam_palette();
    init_sprite_counter();
    enable_sprites_rendering();
    init_numbers();
    
    init_unown_tsv();
    sio_stop_irq_slave();
    irqInit();
    irqSet(IRQ_VBLANK, vblank_update_function);
    irqEnable(IRQ_VBLANK);
    
    init_item_icon();
    init_cursor();
    
    int result = 0;
    u8 evolved = 0;
    u8 learnable_moves = 0;
    u8 returned_val;
    u8 move_go_on = 0;
    u8 update = 0;
    u8 target = 1;
    u8 region = 0;
    u8 master = 0;
    u8 curr_gen = 0;
    u16 num_evolutions = 0;
    u8 own_menu = 0;
    u8 cursor_y_pos = 0;
    u8 cursor_x_pos = 0;
    u8 submenu_cursor_y_pos = 0;
    u8 submenu_cursor_x_pos = 0;
    u8 prev_val = 0;
    u8 curr_mon = 0;
    u32 curr_move = 0;
    u8 success = 0;
    u8 other_mon = 0;
    u8 failed_entered_menu = 0;
    u8 curr_page = 0;
    const u8* party_selected_mons[2] = {&curr_mon, &other_mon};
    
    complete_cartridge_loading(&game_data[0], &game_data_priv, target, region, master, &cursor_y_pos);
    
    //load_pokemon_sprite_raw(&game_data[1].party_3_undec[0], 1, 0, 0);
    //worst_case_conversion_tester(&counter);
    //PRINT_FUNCTION("\n\n0x\x0D: 0x\x0D\n", REG_MEMORY_CONTROLLER_ADDR, 8, REG_MEMORY_CONTROLLER, 8);
    scanKeys();
    keys = keysDown();
    
    while(1) {
        
        do {
            prepare_flush();
            VBlankIntrWait();
            scanKeys();
            keys = keysDown();
            switch(curr_state) {
                case WARNINGS_WHEN_LOADING:
                    if(keys)
                        main_menu_init(&game_data[0], &game_data_priv, target, region, master, &cursor_y_pos);
                    keys = 0;
                    break;
                case START_TRADE:
                    if(get_start_state_raw() == START_TRADE_DON) {
                        keys = 0;
                        read_comm_buffer(&game_data[1], curr_gen, region);
                        own_menu = 0;
                        print_start_trade();
                        prepare_flush();
                        wait_frames(WAITING_TIME_BEFORE_RESPONSE);
                        trade_menu_init(game_data, &game_data_priv, target, region, master, curr_gen, own_menu, &cursor_y_pos, &cursor_x_pos);
                        failed_entered_menu = curr_state == WAITING_DATA;
                    }
                    else
                        print_start_trade();
                    break;
                case WAITING_DATA:
                    switch(get_trading_state()) {
                        case RECEIVED_OFFER:
                            keys = 0;
                            result = get_received_trade_offer();
                            if(result == TRADE_CANCELLED) {
                                if(failed_entered_menu) {
                                    stop_transfer(master);
                                    wait_frames(WAITING_TIME_BEFORE_RESPONSE);
                                    start_trade_init(&game_data[0], &game_data_priv, target, region, master, curr_gen, &cursor_y_pos);
                                }
                                else
                                    conclude_trade(&game_data[0], &game_data_priv, target, region, master, &cursor_y_pos);
                            }
                            else if(result == WANTS_TO_CANCEL) {
                                rejected_print_screen(1);
                                wait_frames(WAITING_TIME_REJECTED);
                                return_to_trade_menu(game_data, &game_data_priv, target, region, master, curr_gen, own_menu, &cursor_y_pos, &cursor_x_pos);
                            }
                            else {
                                u8 is_invalid = is_invalid_offer(game_data, curr_mon, result, curr_gen, get_gen3_offer());
                                wait_frames(WAITING_TIME_BEFORE_RESPONSE);
                                if(!is_invalid) {
                                    other_mon = result;
                                    offer_init(game_data, curr_mon, other_mon, &submenu_cursor_y_pos, &submenu_cursor_x_pos, 1);
                                }
                                else {
                                    is_invalid -= 1;
                                    return_to_trade_menu(game_data, &game_data_priv, target, region, master, curr_gen, own_menu, &cursor_y_pos, &cursor_x_pos);
                                    invalid_init(is_invalid);
                                    waiting_accept_init(1);
                                }
                            }
                            break;
                        case RECEIVED_ACCEPT:
                            keys = 0;
                            if(has_accepted_offer()) {
                                trading_animation_init(game_data, curr_mon, other_mon, 1);
                                prepare_gen23_success(&game_data[0].party_3_undec[curr_mon], &game_data[1].party_3_undec[other_mon]);
                                evolved = trade_mons(game_data, &game_data_priv, curr_mon, other_mon, curr_gen);
                                curr_mon = get_new_party_entry_index(&game_data[0]);
                                if(evolved)
                                    evolution_animation_init(game_data, curr_mon);
                                increase_game_stat(&game_data_priv.num_trades_stat, 1);
                                increase_game_stat(&game_data_priv.num_saves_stat, -1);
                                curr_state = TRADING_ANIMATION;
                                learnable_moves = game_data[0].party_3_undec[curr_mon].learnable_moves != NULL;
                                success = pre_write_gen_3_data(&game_data[0], &game_data_priv, !learnable_moves);
                                if(!success)
                                    crash_on_bad_save(1, master);
                            }
                            else {
                                rejected_print_screen(0);
                                wait_frames(WAITING_TIME_REJECTED);
                                return_to_trade_menu(game_data, &game_data_priv, target, region, master, curr_gen, own_menu, &cursor_y_pos, &cursor_x_pos);
                            }
                            break;
                        case FAILED_SUCCESS:
                            keys = 0;
                            crash_on_bad_trade();
                            break;
                        case RECEIVED_SUCCESS:
                            keys = 0;
                            complete_save_menu(&game_data[0], &game_data_priv, 1, master);
                            stop_transfer(master);
                            if(curr_gen == 3)
                                wait_frames(SWI_DivMod(get_rng() & 0x7FFFFFFF, MAX_RANDOM_WAIT_TIME));
                            start_trade_init(&game_data[0], &game_data_priv, target, region, master, curr_gen, &cursor_y_pos);
                            break;
                        default:
                            break;
                    }
                    break;
                case TRADING_ANIMATION:
                    if(has_animation_completed()) {
                        keys = 0;
                    
                        if(learnable_moves) {
                            learnable_moves_message_init(game_data, curr_mon);
                            curr_move = 0;
                        }
                        else if(!own_menu)
                            waiting_success_init();
                        else {
                            saving_print_screen();
                            complete_save_menu(&game_data[0], &game_data_priv, 0, master);
                            main_menu_init(&game_data[0], &game_data_priv, target, region, master, &cursor_y_pos);
                        }   
                    }
                    break;
                case LEARNABLE_MOVES_MESSAGE:
                    keys = 0;
                    move_go_on = 0;
                    while(!move_go_on) {
                        switch(learn_if_possible(&game_data[0].party_3_undec[curr_mon], curr_move)) {
                            case SKIPPED:
                                curr_move++;
                                break;
                            case LEARNT:
                                handle_learnable_moves_message(game_data, curr_mon, &cursor_x_pos, &curr_move, LEARNT_P);
                                break;
                            case LEARNABLE:
                                curr_state = LEARNABLE_MOVES_MESSAGE_MENU;
                                move_go_on = 1;
                                handle_learnable_moves_message(game_data, curr_mon, &cursor_x_pos, &curr_move, LEARNABLE_P);
                                break;
                            case COMPLETED:
                                success = pre_write_updated_moves_gen_3_data(&game_data[0], &game_data_priv);
                                if(!success)
                                    crash_on_bad_save(1, master);
                                disable_screen(LEARN_MOVE_MESSAGE_WINDOW_SCREEN);
                                prepare_flush();
                                move_go_on = 1;
                                if(!own_menu)
                                    waiting_success_init();
                                else {
                                    saving_print_screen();
                                    complete_save_menu(&game_data[0], &game_data_priv, 0, master);
                                    main_menu_init(&game_data[0], &game_data_priv, target, region, master, &cursor_y_pos);
                                }
                                break;
                            default:
                                curr_move++;
                                break;
                        }
                    }
                    break;
                default:
                    break;
            }
        } while ((!(keys & KEY_LEFT)) && (!(keys & KEY_RIGHT)) && (!(keys & KEY_A)) && (!(keys & KEY_B)) && (!(keys & KEY_UP)) && (!(keys & KEY_DOWN)));
        
        input_counter++;
        switch(curr_state) {
            case MAIN_MENU:
                returned_val = handle_input_main_menu(&cursor_y_pos, keys, &update, &target, &region, &master);
                print_main_menu(update, target, region, master, &game_data[0], &game_data_priv);
                cursor_update_main_menu(cursor_y_pos);
                if(returned_val == START_MULTIBOOT) {
                    curr_state = MULTIBOOT;
                    sio_stop_irq_slave();
                    irqDisable(IRQ_SERIAL);
                    disable_cursor();
                    init_save_data();
                    print_multiboot(multiboot_normal((u16*)EWRAM, (u16*)(EWRAM + MULTIBOOT_MAX_SIZE)));
                }
                if(returned_val == START_PRINT_READ_INFO) {
                    curr_state = PRINT_READ_INFO;
                    disable_cursor();
                    print_game_info(game_data, 0);
                }
                else if(returned_val == START_SWAP_CARTRIDGE) {
                    curr_state = SWAP_CARTRIDGE_MENU;
                    disable_cursor();
                    init_save_data();
                    print_swap_cartridge_menu();
                }
                else if(returned_val == START_SETTINGS_MENU)
                    base_settings_menu_init(&game_data[0], &cursor_y_pos);
                else if(returned_val > VIEW_OWN_PARTY && returned_val <= VIEW_OWN_PARTY + TOTAL_GENS) {
                    curr_gen = returned_val - VIEW_OWN_PARTY;
                    own_menu = 1;
                    trade_menu_init(game_data, &game_data_priv, target, region, master, curr_gen, own_menu, &cursor_y_pos, &cursor_x_pos);
                }
                else if(returned_val > 0 && returned_val <= TOTAL_GENS) {
                    curr_gen = returned_val;
                    start_trade_init(&game_data[0], &game_data_priv, target, region, master, curr_gen, &cursor_y_pos);
                }
                break;
            case TRADING_MENU:
                returned_val = handle_input_trading_menu(&cursor_y_pos, &cursor_x_pos, keys, curr_gen, own_menu);
                print_trade_menu(game_data, update, curr_gen, 0, own_menu);
                trade_cancel_print_screen(update);
                cursor_update_trading_menu(cursor_y_pos, cursor_x_pos);
                curr_mon = returned_val -1;
                
                if(own_menu) {
                    if(returned_val == CANCEL_TRADING)
                        main_menu_init(&game_data[0], &game_data_priv, target, region, master, &cursor_y_pos);
                    else if(returned_val) {
                        if(game_data[cursor_x_pos].party_3_undec[curr_mon].is_valid_gen3 && (!game_data[cursor_x_pos].party_3_undec[curr_mon].is_egg) && game_data[cursor_x_pos].party_3_undec[curr_mon].can_roamer_fix)
                            trade_options_init(cursor_x_pos, &submenu_cursor_x_pos, own_menu, 0);
                        else if(can_own_menu_evolve(&game_data[cursor_x_pos].party_3_undec[curr_mon]))
                            trade_options_init(cursor_x_pos, &submenu_cursor_x_pos, own_menu, 1);
                        else
                            info_menu_init(game_data, cursor_x_pos, curr_mon, &curr_page, 0);
                    }
                }
                else {
                    if(returned_val == CANCEL_TRADING)
                        waiting_offer_init(1, cursor_y_pos);
                    else if(returned_val) {
                        if(cursor_x_pos && ((curr_gen ==3) || (!game_data[cursor_x_pos].party_3_undec[curr_mon].is_valid_gen3) || game_data[cursor_x_pos].party_3_undec[curr_mon].is_egg))
                            info_menu_init(game_data, cursor_x_pos, curr_mon, &curr_page, 0);
                        else
                            trade_options_init(cursor_x_pos, &submenu_cursor_x_pos, own_menu, 0);
                    }
                }
                break;
            case INFO_MENU:
                prev_val = curr_mon;
                returned_val = handle_input_info_menu(game_data, &cursor_y_pos, cursor_x_pos, keys, &curr_mon, curr_gen, &curr_page);
                if(returned_val == CANCEL_INFO)
                    return_to_trade_menu(game_data, &game_data_priv, target, region, master, curr_gen, own_menu, &cursor_y_pos, &cursor_x_pos);
                else
                    print_pokemon_pages(returned_val, curr_mon != prev_val, &game_data[cursor_x_pos].party_3_undec[curr_mon], curr_page);
                break;
            case OFFER_INFO_MENU:
                prev_val = submenu_cursor_y_pos;
                returned_val = handle_input_offer_info_menu(game_data, &submenu_cursor_y_pos, party_selected_mons, keys, &curr_page) ;
                if(returned_val == CANCEL_INFO)
                    offer_init(game_data, curr_mon, other_mon, &submenu_cursor_y_pos, &submenu_cursor_x_pos, 0);
                else
                    print_pokemon_pages(returned_val, submenu_cursor_y_pos != prev_val, &game_data[submenu_cursor_y_pos].party_3_undec[*party_selected_mons[submenu_cursor_y_pos]], curr_page);
                break;
            case MULTIBOOT:
                if(handle_input_multiboot_menu(keys)) {
                    loading_print_screen();
                    complete_cartridge_loading(&game_data[0], &game_data_priv, target, region, master, &cursor_y_pos);
                }
                break;
            case START_TRADE:
                if(handle_input_trade_setup(keys, curr_gen))
                    conclude_trade(&game_data[0], &game_data_priv, target, region, master, &cursor_y_pos);
                break;
            case TRADE_OPTIONS:
                returned_val = handle_input_trade_options(keys, &submenu_cursor_x_pos);
                if(returned_val) {
                    if(returned_val == CANCEL_TRADE_OPTIONS)
                        return_to_trade_menu(game_data, &game_data_priv, target, region, master, curr_gen, own_menu, &cursor_y_pos, &cursor_x_pos);
                    else {
                        if(!submenu_cursor_x_pos)
                            info_menu_init(game_data, cursor_x_pos, curr_mon, &curr_page, 0);
                        else if(own_menu) {
                            if(game_data[cursor_x_pos].party_3_undec[curr_mon].can_roamer_fix)
                                iv_fix_menu_init(game_data, cursor_x_pos, curr_mon);
                            else {
                                num_evolutions = can_own_menu_evolve(&game_data[cursor_x_pos].party_3_undec[curr_mon]);
                                if(num_evolutions)
                                    evolution_menu_init(game_data, cursor_x_pos, curr_mon, &submenu_cursor_y_pos, num_evolutions);
                            }
                        }
                        else if(!cursor_x_pos) {
                            prepare_gen3_offer(&game_data[0].party_3_undec[curr_mon]);
                            waiting_offer_init(0, cursor_y_pos);
                        }
                        else
                            nature_menu_init(game_data, cursor_x_pos, curr_mon, &submenu_cursor_y_pos);
                    }
                }
                else
                    cursor_update_trade_options(submenu_cursor_x_pos);
                break;
            case WAITING_DATA:
                break;
            case EVOLUTION_MENU:
                returned_val = handle_input_evolution_menu(keys, &submenu_cursor_y_pos, &update, num_evolutions);
                if(returned_val == CANCEL_NATURE)
                    return_to_trade_menu(game_data, &game_data_priv, target, region, master, curr_gen, own_menu, &cursor_y_pos, &cursor_x_pos);
                else if(returned_val) {
                    trading_animation_init(game_data, curr_mon, other_mon, 0);
                    evolved = own_menu_evolve(&game_data[0].party_3_undec[curr_mon], submenu_cursor_y_pos);
                    if(!evolved)
                        main_menu_init(&game_data[0], &game_data_priv, target, region, master, &cursor_y_pos);
                    else {
                        evolution_animation_init(game_data, curr_mon);
                        start_with_evolution_animation();
                        increase_game_stat(&game_data_priv.num_trades_stat, 2);
                        increase_game_stat(&game_data_priv.num_saves_stat, -1);
                        curr_state = TRADING_ANIMATION;
                        learnable_moves = game_data[0].party_3_undec[curr_mon].learnable_moves != NULL;
                        register_dex_entry(&game_data_priv, &game_data[0].party_3_undec[curr_mon]);
                        success = pre_write_gen_3_data(&game_data[0], &game_data_priv, !learnable_moves);
                        if(!success)
                            crash_on_bad_save(0, master);
                    }
                }
                else {
                    print_evolution_menu(&game_data[cursor_x_pos].party_3_undec[curr_mon], submenu_cursor_y_pos, EVOLUTION_MENU_SCREEN, 1);
                    cursor_update_evolutions_menu(submenu_cursor_y_pos, num_evolutions);
                }
                break;
            case NATURE_SETTING:
                returned_val = handle_input_nature_menu(keys);
                if(returned_val) {
                    if(returned_val == CANCEL_NATURE)
                        return_to_trade_menu(game_data, &game_data_priv, target, region, master, curr_gen, own_menu, &cursor_y_pos, &cursor_x_pos);
                    else if(returned_val == CONFIRM_NATURE) {
                        if(game_data[cursor_x_pos].party_3_undec[curr_mon].is_valid_gen3 && (!game_data[cursor_x_pos].party_3_undec[curr_mon].is_egg))
                           set_alter_data(&game_data[cursor_x_pos].party_3_undec[curr_mon], &game_data[cursor_x_pos].party_3_undec[curr_mon].alter_nature);
                        return_to_trade_menu(game_data, &game_data_priv, target, region, master, curr_gen, own_menu, &cursor_y_pos, &cursor_x_pos);
                    }
                    else {
                        change_nature(game_data, cursor_x_pos, curr_mon, &submenu_cursor_y_pos, returned_val == INC_NATURE);
                        print_set_nature(0, &game_data[cursor_x_pos].party_3_undec[curr_mon]);
                    }
                }
                break;
            case IV_FIX_MENU:
                returned_val = handle_input_iv_fix_menu(keys);
                if(returned_val) {
                    if(returned_val == CANCEL_IV_FIX)
                        return_to_trade_menu(game_data, &game_data_priv, target, region, master, curr_gen, own_menu, &cursor_y_pos, &cursor_x_pos);
                    else {
                        if(game_data[cursor_x_pos].party_3_undec[curr_mon].is_valid_gen3 && (!game_data[cursor_x_pos].party_3_undec[curr_mon].is_egg) && game_data[cursor_x_pos].party_3_undec[curr_mon].can_roamer_fix) {
                            set_alter_data(&game_data[cursor_x_pos].party_3_undec[curr_mon], &game_data[cursor_x_pos].party_3_undec[curr_mon].fixed_ivs);
                            game_data[cursor_x_pos].party_3_undec[curr_mon].can_roamer_fix = 0;
                            game_data[cursor_x_pos].party_3_undec[curr_mon].fix_has_altered_ot = 0;
                            set_default_gift_ribbons(&game_data[cursor_x_pos]);
                            if(!cursor_x_pos) {
                                saving_print_screen();
                                success = pre_write_gen_3_data(&game_data[0], &game_data_priv, 1);
                                if(!success)
                                    crash_on_bad_save(0, master);
                                complete_save_menu(&game_data[0], &game_data_priv, 0, master);
                            }
                        }
                        return_to_trade_menu(game_data, &game_data_priv, target, region, master, curr_gen, own_menu, &cursor_y_pos, &cursor_x_pos);
                    }
                }
                break;
            case BASE_SETTINGS_MENU:
                returned_val = handle_input_base_settings_menu(keys, &cursor_y_pos, &update, &game_data[0].game_identifier, get_is_cartridge_loaded());
                if(returned_val) {
                    if(returned_val == ENTER_COLOUR_MENU)
                        colours_settings_menu_init(&cursor_y_pos, &cursor_x_pos);
                    else if(returned_val == ENTER_CLOCK_MENU)
                        clock_settings_menu_init(&game_data_priv, &time_change, &cursor_y_pos, 1);
                    else if(returned_val == ENTER_CHEATS_MENU)
                        cheats_menu_init(&cursor_y_pos);
                    else if(returned_val == EXIT_BASE_SETTINGS) {
                        if(get_is_cartridge_loaded()) {
                            alter_game_data_language(&game_data[0], &game_data_priv);
                            alter_party_data_language(&game_data[0], &game_data_priv.party_2, &game_data_priv.party_1);
                            alter_game_data_version(&game_data[0]);
                        }
                        main_menu_init(&game_data[0], &game_data_priv, target, region, master, &cursor_y_pos);
                    }
                }
                else {
                    print_base_settings_menu(&game_data->game_identifier, get_is_cartridge_loaded(), update);
                    cursor_update_base_settings_menu(cursor_y_pos);
                }
                break;
            case COLOURS_SETTINGS_MENU:
                returned_val = handle_input_colours_menu(keys, &cursor_y_pos, &cursor_x_pos, &update);
                if(returned_val)
                    base_settings_menu_init(&game_data[0], &cursor_y_pos);
                else {
                    if(update) {
                        VBlankIntrWait();
                        set_text_palettes();
                        set_cursor_palette();
                    }
                    print_colour_settings_menu(update);
                    cursor_update_colours_settings_menu(cursor_y_pos, cursor_x_pos);
                }
                break;
            case CHEATS_MENU:
                returned_val = handle_input_cheats_menu(keys, &cursor_y_pos, &update);
                if(returned_val) {
                    if(returned_val == EXIT_CHEAT_SETTINGS)
                        base_settings_menu_init(&game_data[0], &cursor_y_pos);
                    else {
                        if(get_is_cartridge_loaded() && give_pokerus_to_party(&game_data[0])) {
                            saving_print_screen();
                            success = pre_write_gen_3_data(&game_data[0], &game_data_priv, 1);
                            if(!success)
                                crash_on_bad_save(0, master);
                            complete_save_menu(&game_data[0], &game_data_priv, 0, master);
                            cheats_menu_init(&cursor_y_pos);
                        }
                    }
                }
                else {
                    print_cheats_menu(update);
                    cursor_update_cheats_menu(cursor_y_pos);
                }
                break;
            case CLOCK_SETTINGS_MENU:
                returned_val = handle_input_clock_menu(keys, &game_data_priv.clock_events, &time_change, &cursor_y_pos, &update);
                if(returned_val) {
                    if(returned_val == EXIT_CLOCK_SETTINGS)
                        base_settings_menu_init(&game_data[0], &cursor_y_pos);
                    else {
                        if(!is_daily_update_safe(&game_data[0], &game_data_priv.clock_events, &time_change))
                           clock_warning_menu_init(&cursor_x_pos);
                        else {
                            run_daily_update(&game_data[0], &game_data_priv.clock_events, &time_change, game_data_priv.game_cleared_flag);
                            saving_print_screen();
                            success = pre_write_gen_3_data(&game_data[0], &game_data_priv, 1);
                            if(!success)
                                crash_on_bad_save(0, master);
                            complete_save_menu(&game_data[0], &game_data_priv, 0, master);
                            base_settings_menu_init(&game_data[0], &cursor_y_pos);
                        }
                    }
                }
                else {
                    print_clock_menu(&game_data_priv.clock_events, &time_change, update);
                    cursor_update_clock_settings_menu(cursor_y_pos);
                }
                break;
            case WARNING_WHEN_ADVANCING_CLOCK:
                returned_val = handle_input_clock_warning_menu(keys, &cursor_x_pos);
                if(returned_val) {
                    if(returned_val == EXIT_CLOCK_WARNING_SETTINGS)
                        clock_settings_menu_init(&game_data_priv, &time_change, &cursor_y_pos, 0);
                    else {
                        run_daily_update(&game_data[0], &game_data_priv.clock_events, &time_change, game_data_priv.game_cleared_flag);
                        saving_print_screen();
                        success = pre_write_gen_3_data(&game_data[0], &game_data_priv, 1);
                        if(!success)
                            crash_on_bad_save(0, master);
                        complete_save_menu(&game_data[0], &game_data_priv, 0, master);
                        base_settings_menu_init(&game_data[0], &cursor_y_pos);
                    }
                }
                else
                    cursor_update_clock_warning(cursor_x_pos);
                break;
            case SWAP_CARTRIDGE_MENU:
                returned_val = handle_input_swap_cartridge_menu(keys);
                if(returned_val) {
                    loading_print_screen();
                    complete_cartridge_loading(&game_data[0], &game_data_priv, target, region, master, &cursor_y_pos);
                }
                break;
            case WARNINGS_WHEN_LOADING:
                main_menu_init(&game_data[0], &game_data_priv, target, region, master, &cursor_y_pos);
                break;
            case OFFER_MENU:
                returned_val = handle_input_offer_options(keys, &submenu_cursor_y_pos, &submenu_cursor_x_pos);
                if(returned_val) {
                    if(returned_val >= OFFER_INFO_DISPLAY) {
                        if(submenu_cursor_y_pos)
                            submenu_cursor_y_pos = 1;
                        info_menu_init(game_data, submenu_cursor_y_pos, *party_selected_mons[submenu_cursor_y_pos], &curr_page, 1);
                    }
                    else
                        waiting_accept_init(returned_val-1);
                }
                else
                    cursor_update_offer_options(submenu_cursor_y_pos, submenu_cursor_x_pos);
                break;
            case TRADING_ANIMATION:
                break;
            case LEARNABLE_MOVES_MESSAGE:
                break;
            case PRINT_READ_INFO:
                if(handle_input_print_read_info(keys))
                    main_menu_init(&game_data[0], &game_data_priv, target, region, master, &cursor_y_pos);
                break;
            case LEARNABLE_MOVES_MESSAGE_MENU:
                returned_val = handle_input_learnable_message_moves_menu(keys, &cursor_x_pos);
                if(returned_val) {
                    if(returned_val == DENIED_LEARNING) {
                        learnable_moves_message_init(game_data, curr_mon);
                        handle_learnable_moves_message(game_data, curr_mon, &cursor_x_pos, &curr_move, DID_NOT_LEARN_P);
                    }
                    else
                        learnable_move_menu_init(game_data, curr_mon, curr_move, &cursor_y_pos);
                }
                else
                    cursor_update_learnable_move_message_menu(cursor_x_pos);
                break;
            case LEARNABLE_MOVES_MENU:
                returned_val = handle_input_learnable_moves_menu(keys, &cursor_y_pos);
                if(returned_val) {
                    if(returned_val == DO_NOT_FORGET_MOVE) {
                        learnable_moves_message_init(game_data, curr_mon);
                        handle_learnable_moves_message(game_data, curr_mon, &cursor_x_pos, &curr_move, DID_NOT_LEARN_P);
                    }
                    else {
                        forget_and_learn_move(&game_data[0].party_3_undec[curr_mon], curr_move, returned_val-1);
                        learnable_moves_message_init(game_data, curr_mon);
                        handle_learnable_moves_message(game_data, curr_mon, &cursor_x_pos, &curr_move, LEARNT_P);
                    }
                }
                else
                    cursor_update_learnable_move_menu(cursor_y_pos);
                break;
            default:
                main_menu_init(&game_data[0], &game_data_priv, target, region, master, &cursor_y_pos);
                break;
        }
        update = 0;
    }

    return 0;
}
