#include <stdio.h>
#include <string.h>
#include <gba.h>

#include "multiboot_handler.h"
#include "graphics_handler.h"
#include "party_handler.h"
#include "text_handler.h"
#include "sprite_handler.h"
#include "version_identifier.h"
#include "gen3_save.h"
#include "options_handler.h"
#include "input_handler.h"
#include "menu_text_handler.h"
#include "sio_buffers.h"
#include "rng.h"
#include "pid_iv_tid.h"
#include "print_system.h"
#include "window_handler.h"
#include "communicator.h"
//#include "save.h"

#include "ewram_speed_check_bin.h"

#define REG_MEMORY_CONTROLLER_ADDR 0x4000800
#define HW_SET_REG_MEMORY_CONTROLLER_VALUE 0x0D000020
#define REG_MEMORY_CONTROLLER *((u32*)(REG_MEMORY_CONTROLLER_ADDR))

#define WORST_CASE_EWRAM 1
#define MIN_WAITCYCLE 1

#define BASE_SCREEN 0
#define INFO_SCREEN 3

void main_menu_init(struct game_data_t*, u8, u8, u8, u8*);
void conclude_trade(struct game_data_t*, u8, u8, u8, u8*);

enum STATE {MAIN_MENU, MULTIBOOT, TRADING_MENU, INFO_MENU, START_TRADE, WAITING_DATA, TRADE_OPTIONS, NATURE_SETTING, OFFER_MENU, TRADING_ANIMATION};
enum STATE curr_state;
u32 counter = 0;
u32 input_counter = 0;

void vblank_update_function() {
	REG_IF |= IRQ_VBLANK;

    move_sprites(counter);
    move_cursor_x(counter);
    advance_rng();
    counter++;
    if((REG_SIOCNT & SIO_IRQ) && (!(REG_SIOCNT & SIO_START)))
        slave_routine();
    if((get_start_state_raw() == START_TRADE_PAR) && (REG_SIOCNT & SIO_IRQ) && (increment_last_tranfer() == NO_INFO_LIMIT)) {
        REG_SIODATA8 = SEND_0_INFO;
        REG_SIODATA32 = SEND_0_INFO;
        REG_IF &= ~IRQ_SERIAL;
        slave_routine();
    }
    if((REG_DISPSTAT >> 8) >= 0xA0 && ((REG_DISPSTAT >> 8) <= REG_VCOUNT))
        set_next_vcount_interrupt();
}

IWRAM_CODE void find_optimal_ewram_settings() {
    int size = ewram_speed_check_bin_size>>2;
    u32* ewram_speed_check = (u32*) ewram_speed_check_bin;
    u32 test_data[size];
    
    // Check for unsupported (DS)
    if(REG_MEMORY_CONTROLLER != HW_SET_REG_MEMORY_CONTROLLER_VALUE)
        return;
    
    // Check for worst case testing (Not for final release)
    if(WORST_CASE_EWRAM)
        return;
    
    // Prepare data to test against
    for(int i = 0; i < size; i++)
        test_data[i] = ewram_speed_check[i];
    
    // Detetmine minimum number of stable waitcycles
    for(int i = 0; i < (16-MIN_WAITCYCLE); i++) {
        REG_MEMORY_CONTROLLER &= ~(0xF<<24);
        REG_MEMORY_CONTROLLER |= (15-i-MIN_WAITCYCLE)<<24;
        u8 failed = 0;
        for(int j = 0; (!failed) && (j < size); j++)
            if(test_data[i] != ewram_speed_check[i])
                failed = 1;
        if(!failed)
            return;
    }
}

u8 init_cursor_y_pos_main_menu(){
    if(!get_valid_options_main())
        return 4;
    return 0;
}

void cursor_update_trading_menu(u8 cursor_y_pos, u8 cursor_x_pos) {
    if(cursor_y_pos < PARTY_SIZE) {
        update_cursor_base_x(BASE_X_CURSOR_TRADING_MENU + (BASE_X_CURSOR_INCREMENT_TRADING_MENU * cursor_x_pos), counter);
        update_cursor_y(BASE_Y_CURSOR_TRADING_MENU + (BASE_Y_CURSOR_INCREMENT_TRADING_MENU * cursor_y_pos));
    }
    else {
        update_cursor_base_x(CURSOR_X_POS_CANCEL, counter);
        update_cursor_y(CURSOR_Y_POS_CANCEL);
    }
}

void cursor_update_main_menu(u8 cursor_y_pos) {
    update_cursor_y(BASE_Y_CURSOR_MAIN_MENU + (BASE_Y_CURSOR_INCREMENT_MAIN_MENU * cursor_y_pos));
}

void cursor_update_trade_options(u8 cursor_x_pos) {
    update_cursor_base_x(BASE_X_CURSOR_TRADE_OPTIONS + (cursor_x_pos * BASE_X_CURSOR_INCREMENT_TRADE_OPTIONS), counter);
}

void cursor_update_offer_options(u8 cursor_y_pos, u8 cursor_x_pos) {
    update_cursor_base_x(BASE_X_CURSOR_OFFER_OPTIONS + (cursor_x_pos * BASE_X_CURSOR_INCREMENT_OFFER_OPTIONS), counter);
    update_cursor_y(BASE_Y_CURSOR_OFFER_OPTIONS + (BASE_Y_CURSOR_INCREMENT_OFFER_OPTIONS * cursor_y_pos));
}

void offer_init(struct game_data_t* game_data, u8 own_mon, u8 other_mon, u8* cursor_y_pos, u8* cursor_x_pos) {
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
    *cursor_x_pos = 0;
    *cursor_y_pos = 0;
    cursor_update_offer_options(*cursor_y_pos, *cursor_x_pos);
}

void waiting_init() {
    curr_state = WAITING_DATA;
    set_screen(WAITING_WINDOW_SCREEN);
    print_waiting();
    enable_screen(WAITING_WINDOW_SCREEN);
}

void invalid_init(u8 reason) {
    set_screen(MESSAGE_WINDOW_SCREEN);
    print_invalid(reason);
    enable_screen(MESSAGE_WINDOW_SCREEN);
}

void waiting_offer_init(u8 cancel, u8 cursor_y_pos) {
    waiting_init();
    if(cancel)
        try_to_end_trade();
    else
        try_to_offer(cursor_y_pos);
}

void waiting_accept_init(u8 decline) {
    waiting_init();
    if(decline)
        try_to_decline_offer();
    else
        try_to_accept_offer();
}

void check_bad_trade_received(struct game_data_t* game_data, u8 target, u8 region, u8 master, u8 curr_gen, u8 own_menu, u8* cursor_y_pos) {
    u8 useless = 0;
    // Handle bad received / No valid mons
    if(handle_input_trading_menu(&useless, &useless, 0, curr_gen, own_menu) == CANCEL_TRADING) {
        if(own_menu)
            main_menu_init(&game_data[0], target, region, master, cursor_y_pos);
        else
            waiting_offer_init(1, 0);
    }
}

void trade_options_init(u8 cursor_x_pos, u8* submenu_cursor_x_pos) {
    curr_state = TRADE_OPTIONS;
    set_screen(TRADE_OPTIONS_WINDOW_SCREEN);
    print_trade_options(cursor_x_pos);
    enable_screen(TRADE_OPTIONS_WINDOW_SCREEN);
    *submenu_cursor_x_pos = 0;
    cursor_update_trade_options(*submenu_cursor_x_pos);
    update_cursor_y(BASE_Y_CURSOR_TRADE_OPTIONS);
}

void trade_menu_init(struct game_data_t* game_data, u8 target, u8 region, u8 master, u8 curr_gen, u8 own_menu, u8* cursor_y_pos, u8* cursor_x_pos) {
    curr_state = TRADING_MENU;
    *cursor_y_pos = 0;
    *cursor_x_pos = 0;
    prepare_options_trade(game_data, curr_gen, own_menu);
    print_trade_menu(game_data, 1, curr_gen, 1, own_menu);
    set_party_sprite_counter();
    cursor_update_trading_menu(*cursor_y_pos, *cursor_x_pos);
    check_bad_trade_received(game_data, target, region, master, curr_gen, own_menu, cursor_y_pos);
}

void start_trade_init(struct game_data_t* game_data, u8 target, u8 region, u8 master, u8 curr_gen, u8* cursor_y_pos) {
    if(is_valid_for_gen(game_data, curr_gen)) {
        curr_state = START_TRADE;
        load_comm_buffer(game_data, curr_gen, region);
        init_start_state();
        set_screen(BASE_SCREEN);
        print_start_trade();
        disable_all_screens_but_current();
        reset_sprites_to_cursor();
        disable_all_cursors();
        start_transfer(master, curr_gen);
    }
    else
        conclude_trade(game_data, target, region, master, cursor_y_pos);
}

void main_menu_init(struct game_data_t* game_data, u8 target, u8 region, u8 master, u8* cursor_y_pos) {
    curr_state = MAIN_MENU;
    prepare_main_options(game_data);
    set_screen(BASE_SCREEN);
    print_main_menu(1, target, region, master);
    disable_all_screens_but_current();
    reset_sprites_to_cursor();
    disable_all_cursors();
    *cursor_y_pos = init_cursor_y_pos_main_menu();
    update_cursor_base_x(BASE_X_CURSOR_MAIN_MENU, counter);
    cursor_update_main_menu(*cursor_y_pos);
}

void info_menu_init(struct game_data_t* game_data, u8 cursor_x_pos, u8 curr_mon, u8* curr_page) {
    curr_state = INFO_MENU;
    *curr_page = 1;
    set_screen(INFO_SCREEN);
    disable_all_sprites();
    print_pokemon_pages(1, 1, &game_data[cursor_x_pos].party_3_undec[curr_mon], *curr_page);
    enable_screen(INFO_SCREEN);
}

void conclude_trade(struct game_data_t* game_data, u8 target, u8 region, u8 master, u8* cursor_y_pos) {
    stop_transfer(master);
    main_menu_init(game_data, target, region, master, cursor_y_pos);
}

void return_to_trade_menu(struct game_data_t* game_data, u8 target, u8 region, u8 master, u8 curr_gen, u8 own_menu, u8* cursor_y_pos, u8* cursor_x_pos) {
    curr_state = TRADING_MENU;
    set_screen(BASE_SCREEN);
    reset_sprites_to_party();
    disable_all_screens_but_current();
    disable_all_cursors();
    enable_all_valid_sprites();
    cursor_update_trading_menu(*cursor_y_pos, *cursor_x_pos);
    check_bad_trade_received(game_data, target, region, master, curr_gen, own_menu, cursor_y_pos);
}

int main(void)
{
    counter = 0;
    input_counter = 0;
    find_optimal_ewram_settings();
    init_text_system();
    init_rng(0,0);
    u16 keys;
    enum MULTIBOOT_RESULTS result;
    struct game_data_t game_data[2];
    
    init_game_data(&game_data[0]);
    init_game_data(&game_data[1]);
    
    get_game_id(&game_data[0].game_identifier);
    
    init_sprites();
    REG_DISPCNT |= OBJ_ON | OBJ_1D_MAP;
    init_numbers();
    
    init_unown_tsv();
    init_oam_palette();
    init_sprite_counter();
    irqInit();
    irqSet(IRQ_VBLANK, vblank_update_function);
    irqEnable(IRQ_VBLANK);
    
    read_gen_3_data(&game_data[0]);
    
    init_item_icon();
    init_cursor();
    
    u16** learnset_ptr = NULL;
    u8 evolved = 0;
    u8 returned_val;
    u8 update = 0;
    u8 target = 1;
    u8 region = 0;
    u8 master = 0;
    u8 curr_gen = 0;
    u8 own_menu = 0;
    u8 cursor_y_pos = 0;
    u8 cursor_x_pos = 0;
    u8 submenu_cursor_y_pos = 0;
    u8 submenu_cursor_x_pos = 0;
    u8 prev_val = 0;
    u8 curr_mon = 0;
    u8 other_mon = 0;
    u8 curr_page = 0;
    
    main_menu_init(&game_data[0], target, region, master, &cursor_y_pos);
    
    //PRINT_FUNCTION("\n\n0x\x0D: 0x\x0D\n", REG_MEMORY_CONTROLLER_ADDR, 8, REG_MEMORY_CONTROLLER, 8);
    
    while(1) {
        scanKeys();
        keys = keysDown();
        
        while ((!(keys & KEY_LEFT)) && (!(keys & KEY_RIGHT)) && (!(keys & KEY_A)) && (!(keys & KEY_B)) && (!(keys & KEY_UP)) && (!(keys & KEY_DOWN))) {
            VBlankIntrWait();
            scanKeys();
            keys = keysDown();
            if(curr_state == START_TRADE) {
                if(get_start_state_raw() == START_TRADE_DON) {
                    read_comm_buffer(&game_data[1], curr_gen, region);
                    own_menu = 0;
                    trade_menu_init(game_data, target, region, master, curr_gen, own_menu, &cursor_y_pos, &cursor_x_pos);
                }
                else {
                    print_start_trade();
                }
            }
            if(curr_state == WAITING_DATA) {
                if(get_trading_state() == RECEIVED_OFFER) {
                    int result = get_received_trade_offer();
                    if(result == TRADE_CANCELLED)
                        conclude_trade(&game_data[0], target, region, master, &cursor_y_pos);
                    else if(result == WANTS_TO_CANCEL)
                        return_to_trade_menu(game_data, target, region, master, curr_gen, own_menu, &cursor_y_pos, &cursor_x_pos);
                    else {
                        u8 is_invalid = is_invalid_offer(game_data, curr_mon, result);
                        if(!is_invalid) {
                            other_mon = result;
                            offer_init(game_data, curr_mon, other_mon, &submenu_cursor_y_pos, &submenu_cursor_x_pos);
                        }
                        else {
                            is_invalid -= 1;
                            return_to_trade_menu(game_data, target, region, master, curr_gen, own_menu, &cursor_y_pos, &cursor_x_pos);
                            invalid_init(is_invalid);
                            waiting_accept_init(1);
                        }
                    }
                }
                else if(get_trading_state() == RECEIVED_ACCEPT) {
                    if(has_accepted_offer()) {
                        evolved = trade_mons(game_data, curr_mon, other_mon, learnset_ptr, curr_gen);
                        process_party_data(&game_data[0]);
                        curr_state = TRADING_ANIMATION;
                    }
                    else
                        return_to_trade_menu(game_data, target, region, master, curr_gen, own_menu, &cursor_y_pos, &cursor_x_pos);
                }
            }
        }
        //PRINT_FUNCTION("%p %p\n", get_communication_buffer(0));
        //worst_case_conversion_tester(&counter);
        input_counter++;
        switch(curr_state) {
            case MAIN_MENU:
                returned_val = handle_input_main_menu(&cursor_y_pos, keys, &update, &target, &region, &master);
                print_main_menu(update, target, region, master);
                cursor_update_main_menu(cursor_y_pos);
                if(returned_val == START_MULTIBOOT) {
                    curr_state = MULTIBOOT;
                    irqDisable(IRQ_SERIAL);
                    disable_cursor();
                    result = multiboot_normal((u16*)EWRAM, (u16*)(EWRAM + 0x3FF40));
                    print_multiboot(result);
                }
                else if(returned_val > VIEW_OWN_PARTY && returned_val <= VIEW_OWN_PARTY + TOTAL_GENS) {
                    curr_gen = returned_val - VIEW_OWN_PARTY;
                    own_menu = 1;
                    trade_menu_init(game_data, target, region, master, curr_gen, own_menu, &cursor_y_pos, &cursor_x_pos);
                }
                else if(returned_val > 0 && returned_val <= TOTAL_GENS) {
                    curr_gen = returned_val;
                    start_trade_init(&game_data[0], target, region, master, curr_gen, &cursor_y_pos);
                }
                break;
            case TRADING_MENU:
                returned_val = handle_input_trading_menu(&cursor_y_pos, &cursor_x_pos, keys, curr_gen, own_menu);
                print_trade_menu(game_data, update, curr_gen, 0, own_menu);
                cursor_update_trading_menu(cursor_y_pos, cursor_x_pos);
                curr_mon = returned_val -1;
                
                if(own_menu) {
                    if(returned_val == CANCEL_TRADING)
                        main_menu_init(&game_data[0], target, region, master, &cursor_y_pos);
                    else if(returned_val)
                        info_menu_init(game_data, cursor_x_pos, curr_mon, &curr_page);
                }
                else {
                    if(returned_val == CANCEL_TRADING)
                        waiting_offer_init(1, cursor_y_pos);
                    else if(returned_val) {
                        if(cursor_x_pos && curr_gen ==3)
                            info_menu_init(game_data, cursor_x_pos, curr_mon, &curr_page);
                        else
                            trade_options_init(cursor_x_pos, &submenu_cursor_x_pos);
                    }
                }
                break;
            case INFO_MENU:
                prev_val = curr_mon;
                returned_val = handle_input_info_menu(game_data, &cursor_y_pos, cursor_x_pos, keys, &curr_mon, curr_gen, &curr_page);
                if(returned_val == CANCEL_INFO)
                    return_to_trade_menu(game_data, target, region, master, curr_gen, own_menu, &cursor_y_pos, &cursor_x_pos);
                else
                    print_pokemon_pages(returned_val, curr_mon != prev_val, &game_data[cursor_x_pos].party_3_undec[curr_mon], curr_page);
                break;
            case MULTIBOOT:
                if(handle_input_multiboot_menu(keys))
                    main_menu_init(&game_data[0], target, region, master, &cursor_y_pos);
                break;
            case START_TRADE:
                if(handle_input_trade_setup(keys, curr_gen))
                    conclude_trade(&game_data[0], target, region, master, &cursor_y_pos);
                break;
            case TRADE_OPTIONS:
                returned_val = handle_input_trade_options(keys, &submenu_cursor_x_pos);
                if(returned_val) {
                    if(returned_val == CANCEL_TRADE_OPTIONS)
                        return_to_trade_menu(game_data, target, region, master, curr_gen, own_menu, &cursor_y_pos, &cursor_x_pos);
                    else {
                        if(!submenu_cursor_x_pos)
                            info_menu_init(game_data, cursor_x_pos, curr_mon, &curr_page);
                        else if(!cursor_x_pos)
                            waiting_offer_init(0, cursor_y_pos);
                        else {
                            // TODO: Print the nature settings menu here
                            curr_state = NATURE_SETTING;
                        }
                            
                    }
                }
                else
                    cursor_update_trade_options(submenu_cursor_x_pos);
                break;
            case WAITING_DATA:
                break;
            case NATURE_SETTING:
                // TODO: Nature settings
                break;
            case OFFER_MENU:
                returned_val = handle_input_offer_options(keys, &submenu_cursor_y_pos, &submenu_cursor_x_pos);
                if(returned_val) {
                    if(returned_val >= OFFER_INFO_DISPLAY) {
                        returned_val -= OFFER_INFO_DISPLAY;
                        if(returned_val)
                            returned_val = 1;
                        // TODO: Handle the summaries here
                        
                    }
                    else
                        waiting_accept_init(returned_val-1);
                }
                else
                    cursor_update_offer_options(submenu_cursor_y_pos, submenu_cursor_x_pos);
                break;
            default:
                main_menu_init(&game_data[0], target, region, master, &cursor_y_pos);
                break;
        }
        update = 0;
    }

    return 0;
}
