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
#include "communicator.h"
//#include "save.h"

// --------------------------------------------------------------------

#define PACKED __attribute__((packed))
#define ALWAYS_INLINE __attribute__((always_inline)) static inline
#define MAX_DUMP_SIZE 0x20000

// --------------------------------------------------------------------

enum STATE {MAIN_MENU, MULTIBOOT, TRADING_MENU, INFO_MENU, START_TRADE};
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

void main_menu_init(struct game_data_t* game_data, u8 target, u8 region, u8 master, u8* cursor_y_pos) {
    curr_state = MAIN_MENU;
    prepare_main_options(game_data);
    print_main_menu(1, target, region, master);
    *cursor_y_pos = init_cursor_y_pos_main_menu();
    reset_sprites_to_cursor();
    update_cursor_base_x(BASE_X_CURSOR_MAIN_MENU, counter);
    cursor_update_main_menu(*cursor_y_pos);
}

int main(void)
{
    counter = 0;
    input_counter = 0;
    init_rng(0,0);
    u16 keys;
    enum MULTIBOOT_RESULTS result;
    struct game_data_t game_data[2];
    
    init_game_data(&game_data[0]);
    init_game_data(&game_data[1]);
    
    get_game_id(&game_data[0].game_identifier);
    
    init_unown_tsv();
    init_oam_palette();
    init_sprite_counter();
    irqInit();
    irqSet(IRQ_VBLANK, vblank_update_function);
    irqEnable(IRQ_VBLANK);

    consoleDemoInit();
    init_gender_symbols();
    REG_DISPCNT |= OBJ_ON | OBJ_1D_MAP;
    
    PRINT_FUNCTION("\x1b[2J");
    
    read_gen_3_data(&game_data[0]);
    prepare_main_options(&game_data[0]);
    
    u8 returned_val;
    u8 update = 0;
    u8 target = 1;
    u8 region = 0;
    u8 master = 0;
    u8 curr_gen = 0;
    u8 own_menu = 0;
    u8 cursor_y_pos = init_cursor_y_pos_main_menu();
    u8 cursor_x_pos = 0;
    u8 prev_val = 0;
    u8 curr_mon = 0;
    u8 curr_page = 0;
    curr_state = MAIN_MENU;
    
    print_main_menu(1, target, region, master);
    
    init_item_icon();
    init_cursor(cursor_y_pos);
    
    while(1) {
        scanKeys();
        keys = keysDown();
        
        while ((!(keys & KEY_LEFT)) && (!(keys & KEY_RIGHT)) && (!(keys & KEY_A)) && (!(keys & KEY_B)) && (!(keys & KEY_UP)) && (!(keys & KEY_DOWN))) {
            VBlankIntrWait();
            scanKeys();
            keys = keysDown();
            if(curr_state == START_TRADE) {
                if(get_start_state_raw() == START_TRADE_DON) {
                    curr_state = TRADING_MENU;
                    cursor_y_pos = 0;
                    cursor_x_pos = 0;
                    own_menu = 0;
                    read_comm_buffer(&game_data[1], curr_gen, region);
                    prepare_options_trade(game_data, curr_gen, own_menu);
                    print_trade_menu(game_data, 1, curr_gen, 1, own_menu);
                    cursor_update_trading_menu(cursor_y_pos, cursor_x_pos);
                }
                else {
                    print_start_trade();
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
                    curr_state = TRADING_MENU;
                    cursor_y_pos = 0;
                    cursor_x_pos = 0;
                    curr_gen = returned_val - VIEW_OWN_PARTY;
                    own_menu = 1;
                    prepare_options_trade(game_data, curr_gen, own_menu);
                    print_trade_menu(game_data, 1, curr_gen, 1, own_menu);
                    cursor_update_trading_menu(cursor_y_pos, cursor_x_pos);
                }
                else if(returned_val > 0 && returned_val <= TOTAL_GENS) {
                    curr_gen = returned_val;
                    curr_state = START_TRADE;
                    init_start_state();
                    load_comm_buffer(&game_data[0], curr_gen, region);
                    start_transfer(master, curr_gen);
                    disable_cursor();
                    print_start_trade();
                }
                break;
            case TRADING_MENU:
                returned_val = handle_input_trading_menu(&cursor_y_pos, &cursor_x_pos, keys, curr_gen, own_menu);
                print_trade_menu(game_data, update, curr_gen, 0, own_menu);
                cursor_update_trading_menu(cursor_y_pos, cursor_x_pos);
                
                // IMPLEMENT THE ELSE LATER
                if(own_menu) {
                    if(returned_val == CANCEL_TRADING)
                        main_menu_init(&game_data[0], target, region, master, &cursor_y_pos);
                    else if(returned_val) {
                        curr_state = INFO_MENU;
                        curr_mon = returned_val-1;
                        curr_page = 1;
                        disable_cursor();
                        print_pokemon_pages(1, 1, &game_data[cursor_x_pos].party_3_undec[curr_mon], curr_page);
                    }
                }
                break;
            case INFO_MENU:
                prev_val = curr_mon;
                returned_val = handle_input_info_menu(game_data, &cursor_y_pos, cursor_x_pos, keys, &curr_mon, curr_gen, &curr_page);
                if(returned_val == CANCEL_INFO){
                    curr_state = TRADING_MENU;
                    print_trade_menu(game_data, 1, curr_gen, 1, own_menu);
                    cursor_update_trading_menu(cursor_y_pos, cursor_x_pos);
                }
                else
                    print_pokemon_pages(returned_val, curr_mon != prev_val, &game_data[cursor_x_pos].party_3_undec[curr_mon], curr_page);
                break;
            case MULTIBOOT:
                if(handle_input_multiboot_menu(keys))
                    main_menu_init(&game_data[0], target, region, master, &cursor_y_pos);
                break;
            case START_TRADE:
                if(handle_input_trade_setup(keys, curr_gen)) {
                    stop_transfer(master);
                    main_menu_init(&game_data[0], target, region, master, &cursor_y_pos);
                }
                break;
            default:
                main_menu_init(&game_data[0], target, region, master, &cursor_y_pos);
                break;
        }
        update = 0;
    }

    return 0;
}
