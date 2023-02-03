#include <gba.h>
#include "menu_text_handler.h"
#include "text_handler.h"
#include "options_handler.h"
#include "input_handler.h"
#include "sprite_handler.h"
#include "version_identifier.h"
#include "print_system.h"
#include "sio_buffers.h"
#include "communicator.h"
#include "window_handler.h"

#define NUM_LINES 10
#define MAIN_MENU_DISTANCE_FROM_BORDER 2

#define SUMMARY_LINE_MAX_SIZE 18
#define PRINTABLE_INVALID_STRINGS 3

void print_pokemon_base_info(u8, struct gen3_mon_data_unenc*, u8);
void print_bottom_info(void);
void print_pokemon_page1(struct gen3_mon_data_unenc*);
void print_pokemon_page2(struct gen3_mon_data_unenc*);
void print_pokemon_page3(struct gen3_mon_data_unenc*);
void print_pokemon_page4(struct gen3_mon_data_unenc*);
void print_pokemon_page5(struct gen3_mon_data_unenc*);

const char* person_strings[] = {"You", "Other"};
const char* game_strings[] = {"RS", "FRLG", "E"};
const char* unidentified_string = "Unidentified";
const char* subgame_rs_strings[] = {"R", "S"};
const char* subgame_frlg_strings[] = {"FR", "LG"};
const char* actor_strings[] = {"Master", "Slave"};
const char* region_strings[] = {"Int", "Jap"};
const char* slash_string = {"/"};
const char* target_strings[] = {"Gen 1", "Gen 2", "Gen 3"};
const char* stat_strings[] = {"Hp", "Atk", "Def", "SpA", "SpD", "Spe"};
const char* contest_strings[] = {"Coolness", "Beauty", "Cuteness", "Smartness", "Toughness", "Feel"};
const char* trade_start_state_strings[] = {"Unknown", "Entering Room", "Starting Trade", "Ending Trade", "Waiting Trade", "Trading Party Data", "Synchronizing", "Completed"};
const char* offer_strings[] = {" Sending ", "Receiving"};
const char* invalid_strings[][PRINTABLE_INVALID_STRINGS] = {{"      TRADE REJECTED!", "The Pokémon offered by the", "other player has issues!"}, {"      TRADE REJECTED!", "The trade would leave you", "with no usable Pokémon!"}};

const u8 ribbon_print_pos[NUM_LINES*2] = {0,1,2,3,4,5,6,7,8,9,14,15,13,16,10,0xFF,11,0xFF,12,0xFF};
typedef void (*print_info_functions_t)(struct gen3_mon_data_unenc*);
print_info_functions_t print_info_functions[PAGES_TOTAL] = {print_pokemon_page1, print_pokemon_page2, print_pokemon_page3, print_pokemon_page4, print_pokemon_page5};

void print_game_info(struct game_data_t* game_data, int index) {
    PRINT_FUNCTION("\n Game: ");
    const char* chosen_str = game_strings[game_data[index].game_identifier.game_main_version];
    switch(game_data[index].game_identifier.game_main_version) {
        case RS_MAIN_GAME_CODE:
            if(game_data[index].game_identifier.game_sub_version != UNDETERMINED)
                chosen_str = subgame_rs_strings[game_data[index].game_identifier.game_sub_version];
            break;
        case FRLG_MAIN_GAME_CODE:
            if(game_data[index].game_identifier.game_sub_version != UNDETERMINED)
                chosen_str = subgame_frlg_strings[game_data[index].game_identifier.game_sub_version];
            break;
        case E_MAIN_GAME_CODE:
            break;
        default:
            chosen_str = unidentified_string;
            break;
    }
    PRINT_FUNCTION("\x01\n", chosen_str);
}

void print_trade_menu(struct game_data_t* game_data, u8 update, u8 curr_gen, u8 load_sprites, u8 is_own) {
    if(!update)
        return;
    
    // This is for debug only - it should be is_own
    is_own = is_own;
    
    if(curr_gen < 1)
        curr_gen = 3;
    if(curr_gen > 3)
        curr_gen = 3;
    
    default_reset_screen();
    
    u8 num_parties = 2;
    if(is_own)
        num_parties = 1;
    
    if(is_own)
        PRINT_FUNCTION("\x05 - Gen \x03\n", game_data[0].trainer_name, OT_NAME_GEN3_SIZE+1, game_data[0].game_identifier.game_is_jp, curr_gen);
    else {
        for(int i = 0; i < num_parties; i++) {
            set_text_x((X_LIMIT>>1) * i);
            PRINT_FUNCTION("\x05 - \x01", game_data[i].trainer_name, OT_NAME_GEN3_SIZE+1, game_data[i].game_identifier.game_is_jp, person_strings[i]);
        }
    }

    if(load_sprites)
        reset_sprites_to_cursor();

    u8* options[2];
    for(int i = 0; i < 2; i++)
        options[i] = get_options_trade(i);
    for(int i = 0; i < PARTY_SIZE; i++) {
        set_text_y(2+(3 * i));
        for(int j = 0; j < num_parties; j++) {
            // These two values are for debug only - They should be j and i
            set_text_x(5 + ((X_LIMIT>>1) * j));
            u8 party_index = j;
            u8 party_option_index = i;
            if(options[party_index][party_option_index] != 0xFF) {
                struct gen3_mon_data_unenc* mon = &game_data[party_index].party_3_undec[options[party_index][party_option_index]];
                // I tried just using printf here with left padding, but it's EXTREMELY slow
                PRINT_FUNCTION("\x01", get_pokemon_name_raw(mon));
                if(load_sprites)
                    load_pokemon_sprite_raw(mon, BASE_Y_SPRITE_TRADE_MENU + (i*BASE_Y_SPRITE_INCREMENT_TRADE_MENU), (((X_LIMIT >> 1) << 3)*j) + BASE_X_SPRITE_TRADE_MENU);
            }
        }
    }
}

void print_trade_menu_cancel(u8 update) {
    if(!update)
        return;

    reset_screen(BLANK_FILL);

    set_text_y(Y_LIMIT-1);
    set_text_x(2);
    PRINT_FUNCTION("Cancel");
}

void print_offer_screen(struct game_data_t* game_data, u8 own_mon, u8 other_mon) {
    default_reset_screen();
    init_offer_window();
    u8 possible_mons[2] = {own_mon, other_mon};
    
    for(int i = 0; i < 2; i++) {
        
        struct gen3_mon_data_unenc* mon = &game_data[i].party_3_undec[possible_mons[i]];
        u8 is_shiny = is_shiny_gen3_raw(mon, 0);
        //u8 has_pokerus = has_pokerus_gen3_raw(mon);
        //u8 is_jp = mon->src->language == JAPANESE_LANGUAGE;
        u8 is_egg = mon->is_egg;
        u8 curr_text_y = 0;
        
        set_text_y(curr_text_y++);
        set_text_x(3+(OFFER_WINDOW_X*i));
        PRINT_FUNCTION("\x01", offer_strings[i]);
        load_pokemon_sprite_raw(mon, BASE_Y_SPRITE_OFFER_MENU, (OFFER_WINDOW_X*8*i) + BASE_X_SPRITE_OFFER_MENU);
        
        if(!is_egg) {
            set_text_y(curr_text_y++);
            set_text_x(4+(OFFER_WINDOW_X*i));
            PRINT_FUNCTION("\x01", get_pokemon_name_raw(mon));
        
            set_text_y(curr_text_y++);
            set_text_x(4+(OFFER_WINDOW_X*i));
            PRINT_FUNCTION("Lv. \x03 \x02", to_valid_level_gen3(mon->src), get_pokemon_gender_char_raw(mon));
            if(is_shiny) {
                set_text_y(curr_text_y++);
                set_text_x(4+(OFFER_WINDOW_X*i));
                PRINT_FUNCTION("Shiny");
            }
            else
                curr_text_y++;
            set_text_y(curr_text_y++);
            set_text_x(OFFER_WINDOW_X*i);
            PRINT_FUNCTION("\x01 Nature", get_nature_name(mon->src->pid));
            set_text_y(curr_text_y++);
            //set_text_x(OFFER_WINDOW_X*i);
            //PRINT_FUNCTION("ITEM:");
            set_text_y(curr_text_y++);
            set_text_x(OFFER_WINDOW_X*i);
            if(has_item_raw(mon))
                PRINT_FUNCTION("\x01", get_item_name_raw(mon));
            else
                PRINT_FUNCTION("No Item");
            //set_text_y(curr_text_y++);
            //set_text_x(OFFER_WINDOW_X*i);
            //PRINT_FUNCTION("ABILITY:");
            //set_text_y(curr_text_y++);
            //set_text_x(OFFER_WINDOW_X*i);
            //PRINT_FUNCTION("\x01", get_ability_name_raw(mon));
            set_text_y(curr_text_y++);
            set_text_x(OFFER_WINDOW_X*i);
            //PRINT_FUNCTION("MOVES:");
            for(int j = 0; j < MOVES_SIZE; j++) {
                set_text_y(curr_text_y++);
                set_text_x(OFFER_WINDOW_X*i);
                PRINT_FUNCTION("\x01", get_move_name_gen3(&mon->attacks, j));
            }
        }
        else {
            curr_text_y++;
            set_text_y(curr_text_y++);
            set_text_x(4+(OFFER_WINDOW_X*i));
            PRINT_FUNCTION("\x01", get_pokemon_name_raw(mon));
        }
    }
}

void print_invalid(u8 reason) {
    reset_screen(BLANK_FILL);
    init_message_window();
    clear_message_window();
    for(int i = 0; i < PRINTABLE_INVALID_STRINGS; i++) {
        set_text_y(MESSAGE_WINDOW_Y + (i*1));
        set_text_x(MESSAGE_WINDOW_X);
        PRINT_FUNCTION("\x01", invalid_strings[reason][i]);
    }
}

void print_offer_options_screen(struct game_data_t* game_data, u8 own_mon, u8 other_mon) {
    reset_screen(BLANK_FILL);
    init_offer_options_window();
    clear_offer_options_window();
    set_text_y(OFFER_OPTIONS_WINDOW_Y);
    set_text_x(OFFER_OPTIONS_WINDOW_X);
    PRINT_FUNCTION("Trade \x01\n", get_pokemon_name_raw(&game_data[0].party_3_undec[own_mon]));
    set_text_x(OFFER_OPTIONS_WINDOW_X);
    PRINT_FUNCTION("for \x01?\n\n", get_pokemon_name_raw(&game_data[1].party_3_undec[other_mon]));
    set_text_x(OFFER_OPTIONS_WINDOW_X+2);
    PRINT_FUNCTION("Yes");
    set_text_x(OFFER_OPTIONS_WINDOW_X + (OFFER_OPTIONS_WINDOW_X_SIZE - SUMMARY_LINE_MAX_SIZE));
    PRINT_FUNCTION("Summary: Sending\n\n");
    set_text_x(OFFER_OPTIONS_WINDOW_X+2);
    PRINT_FUNCTION("No");
    set_text_x(OFFER_OPTIONS_WINDOW_X + (OFFER_OPTIONS_WINDOW_X_SIZE - SUMMARY_LINE_MAX_SIZE));
    PRINT_FUNCTION("Summary: Receiving");
}

void print_trade_options(u8 cursor_x_pos){
    reset_screen(BLANK_FILL);
    init_trade_options_window();
    clear_trade_options_window();
    set_text_y(TRADE_OPTIONS_WINDOW_Y);
    set_text_x(TRADE_OPTIONS_WINDOW_X);
    PRINT_FUNCTION("  Summary");
    set_text_x(TRADE_OPTIONS_WINDOW_X + (TRADE_OPTIONS_WINDOW_X_SIZE>>1));
    if(cursor_x_pos)
        PRINT_FUNCTION("  Set Nature");
    else 
        PRINT_FUNCTION("  Offer");
}

void print_waiting(){
    reset_screen(BLANK_FILL);
    init_waiting_window();
    clear_waiting_window();
    set_text_y(WAITING_WINDOW_Y);
    set_text_x(WAITING_WINDOW_X);
    PRINT_FUNCTION("Waiting...");
}

void print_start_trade(){
    enum START_TRADE_STATE state = get_start_state();
    enum START_TRADE_STATE raw_state = get_start_state_raw();
    //if((state == START_TRADE_NO_UPDATE) && (raw_state != START_TRADE_PAR))
    //    return;
    if(((state == START_TRADE_NO_UPDATE) && (raw_state != START_TRADE_PAR))||(raw_state == START_TRADE_DON))
        return;
    //if(state == START_TRADE_NO_UPDATE)
    //    return;
    
    default_reset_screen();
    PRINT_FUNCTION("\nState: \x01\n", trade_start_state_strings[raw_state]);
    if(raw_state != START_TRADE_PAR) {
        set_text_y(Y_LIMIT-1);
        PRINT_FUNCTION("B: Go Back");
    }
    else {
        for(int i = 0; i < get_number_of_buffers(); i++) {
            PRINT_FUNCTION("\nSection \x03: \x09/\x03\n", i+1, get_transferred(i), 3, get_buffer_size(i));
        }
        if(get_transferred(0) == 0) {
            set_text_y(Y_LIMIT-1);
            PRINT_FUNCTION("B: Go Back");
        }
    }
}

void print_pokemon_base_info(u8 load_sprites, struct gen3_mon_data_unenc* mon, u8 page) {
    
    u8 is_shiny = is_shiny_gen3_raw(mon, 0);
    u8 has_pokerus = has_pokerus_gen3_raw(mon);
    u8 is_jp = mon->src->language == JAPANESE_LANGUAGE;
    u8 is_egg = mon->is_egg;
    
    default_reset_screen();
    
    u8 page_total = PAGES_TOTAL-FIRST_PAGE+1;
    
    if(is_egg)
        page_total = FIRST_PAGE;
    
    if(page == FIRST_PAGE)
        PRINT_FUNCTION(" ");
    else
        PRINT_FUNCTION("<");
        
    PRINT_FUNCTION("\x03 / \x03", page, page_total);
    
    if(page < page_total)
        PRINT_FUNCTION(">");

    if(load_sprites) {
        reset_sprites_to_party();
        load_pokemon_sprite_raw(mon, BASE_Y_SPRITE_INFO_PAGE, BASE_X_SPRITE_INFO_PAGE);
    }
    
    if(!is_egg) {
        PRINT_FUNCTION("\n\n    \x05 - \x01 \x02\n", mon->src->nickname, NICKNAME_GEN3_SIZE, is_jp, get_pokemon_name_raw(mon), get_pokemon_gender_char_raw(mon));
    
        PRINT_FUNCTION("    ");
        if(is_shiny)
            PRINT_FUNCTION("Shiny");
    
        if(is_shiny && has_pokerus)
            PRINT_FUNCTION(" - ");
    
        if(has_pokerus == HAS_POKERUS)
            PRINT_FUNCTION("Has Pokerus");
        else if(has_pokerus == HAD_POKERUS)
            PRINT_FUNCTION("Had Pokerus");
    }
    else
        PRINT_FUNCTION("\n\n    \x01\n", get_pokemon_name_raw(mon));
    
}

void print_bottom_info(){
    set_text_y(Y_LIMIT-1);
    PRINT_FUNCTION("B: Go Back");
}

void print_pokemon_page1(struct gen3_mon_data_unenc* mon) {
    u8 is_jp = (mon->src->language == JAPANESE_LANGUAGE);
    u8 is_egg = mon->is_egg;
    
    if(!is_egg) {
        PRINT_FUNCTION("\nLevel: \x13Nature: \x01\n", to_valid_level_gen3(mon->src), 6, get_nature_name(mon->src->pid));
        
        if(is_jp)
            PRINT_FUNCTION("\nLanguage: Japanese\n");
        else
            PRINT_FUNCTION("\nLanguage: International\n");
        
        PRINT_FUNCTION("\nOT: \x05 - \x02 - \x0B\n", mon->src->ot_name, OT_NAME_GEN3_SIZE, is_jp, get_trainer_gender_char_raw(mon), (mon->src->ot_id)&0xFFFF, 5);
        PRINT_FUNCTION("\nItem: \x01\n", get_item_name_raw(mon));
        
        PRINT_FUNCTION("\nMet in: \x01\n", get_met_location_name_gen3_raw(mon));
        u8 met_level = get_met_level_gen3_raw(mon);
        if(met_level > 0)
            PRINT_FUNCTION("\nCaught at Level \x03\n\nCaught in \x01 Ball", met_level, get_pokeball_base_name_gen3_raw(mon));
        else
            PRINT_FUNCTION("\nHatched in \x01 Ball", get_pokeball_base_name_gen3_raw(mon));
    }
    else
        PRINT_FUNCTION("\nHatches in : \x03 Egg Cycles\n\nHatches in: \x03 Steps", mon->growth.friendship, mon->growth.friendship * 0x100);
}

void print_pokemon_page2(struct gen3_mon_data_unenc* mon) {
        
    PRINT_FUNCTION("\nSTAT    VALUE    EV    IV\n");
    for(int i = 0; i < GEN2_STATS_TOTAL; i++) {
        PRINT_FUNCTION("\n \x11", stat_strings[i], 3);
        if(i == HP_STAT_INDEX) {
            u16 hp = calc_stats_gen3_raw(mon, i);
            u16 curr_hp = mon->src->curr_hp;
            if(curr_hp > hp)
                curr_hp = hp;
            PRINT_FUNCTION("  \x09/\x13", curr_hp, 4, hp, 4);
        }
        else
            PRINT_FUNCTION("    \x09\x02  ", calc_stats_gen3_raw(mon,i), 4, get_nature_symbol(mon->src->pid, i));
        PRINT_FUNCTION("\x09   \x09\n", get_evs_gen3(&mon->evs, i), 4, get_ivs_gen3(&mon->misc, i), 3);
    }
    
    PRINT_FUNCTION("\n   Hidden Power \x01: \x03", get_hidden_power_type_name_gen3(&mon->misc), get_hidden_power_power_gen3(&mon->misc));
}

void print_pokemon_page3(struct gen3_mon_data_unenc* mon) {

    PRINT_FUNCTION("\nMOVES");
    set_text_x(18);
    PRINT_FUNCTION("PP UP\n");
    for(int i = 0; i < (MOVES_SIZE); i++){
        PRINT_FUNCTION("\n \x11\x03\n", get_move_name_gen3(&mon->attacks, i), 19, (mon->growth.pp_bonuses >> (2*i)) & 3);
    }
    
    PRINT_FUNCTION("\nAbility: \x01\n", get_ability_name_raw(mon));
    
    PRINT_FUNCTION("\nExperience: \x03\n", get_proper_exp_raw(mon));
    
    if(to_valid_level_gen3(mon->src) < MAX_LEVEL)
        PRINT_FUNCTION("\nNext Lv. in: \x09 > Lv. \x09", get_level_exp_mon_index(get_mon_index_raw(mon), to_valid_level_gen3(mon->src)+1) - get_proper_exp_raw(mon), 5, to_valid_level_gen3(mon->src)+1, 3);
}

void print_pokemon_page4(struct gen3_mon_data_unenc* mon) {
    PRINT_FUNCTION("\nCONTEST STAT     VALUE\n");
    for(int i = 0; i < CONTEST_STATS_TOTAL; i++){
        PRINT_FUNCTION("\n \x11\x09\n", contest_strings[i], 16, mon->evs.contest[i], 4);
    }
}

void print_pokemon_page5(struct gen3_mon_data_unenc* mon) {
    PRINT_FUNCTION("\nRIBBONS");
    for(int i = 0; i < NUM_LINES; i++){
        set_text_y(6+i);
        PRINT_FUNCTION(" \x01\x01", get_ribbon_name(&mon->misc, ribbon_print_pos[(i*2)]), get_ribbon_rank_name(&mon->misc, ribbon_print_pos[(i*2)]));
        if(ribbon_print_pos[(i*2)+1] != 0xFF) {
            set_text_x(X_LIMIT>>1);
            PRINT_FUNCTION(" \x01\x01", get_ribbon_name(&mon->misc, ribbon_print_pos[(i*2)+1]), get_ribbon_rank_name(&mon->misc, ribbon_print_pos[(i*2)+1]));
        }
    }
}

void print_pokemon_pages(u8 update, u8 load_sprites, struct gen3_mon_data_unenc* mon, u8 page_num) {
    if(!update)
        return;
    
    if(mon->is_egg)
        page_num = FIRST_PAGE;
    if((page_num < FIRST_PAGE) || (page_num > PAGES_TOTAL))
        page_num = FIRST_PAGE;

    print_pokemon_base_info(load_sprites, mon, page_num);
    
    print_info_functions[page_num-FIRST_PAGE](mon);
    
    print_bottom_info();
}

void print_main_menu(u8 update, u8 curr_gen, u8 is_jp, u8 is_master) {
    if(!update)
        return;
    
    u8* options = get_options_main();

    default_reset_screen();
    
    if(!get_valid_options_main()) {
        set_text_y(1);
        set_text_x(MAIN_MENU_DISTANCE_FROM_BORDER);
        PRINT_FUNCTION("Error reading the data!");
        set_text_y(9);
        set_text_x(MAIN_MENU_DISTANCE_FROM_BORDER);
        PRINT_FUNCTION("Send Multiboot");
    }
    else {
        if(curr_gen >= TOTAL_GENS)
            curr_gen = 2;
        set_text_y(1);
        set_text_x(MAIN_MENU_DISTANCE_FROM_BORDER);
        curr_gen = options[curr_gen];
        if(get_number_of_higher_ordered_options(options, curr_gen, TOTAL_GENS) > 0 && get_number_of_lower_ordered_options(options, curr_gen, TOTAL_GENS) > 0)
            PRINT_FUNCTION("Target: <\x01>", target_strings[curr_gen-1]);
        else if(get_number_of_higher_ordered_options(options, curr_gen, TOTAL_GENS) > 0)
            PRINT_FUNCTION("Target:  \x01>", target_strings[curr_gen-1]);
        else if(get_number_of_lower_ordered_options(options, curr_gen, TOTAL_GENS) > 0)
            PRINT_FUNCTION("Target: <\x01", target_strings[curr_gen-1]);
        else
            PRINT_FUNCTION("Target:  \x01", target_strings[curr_gen-1]);
        set_text_y(3);
        set_text_x(MAIN_MENU_DISTANCE_FROM_BORDER);
        if(curr_gen < 3) {
            if(!is_jp)
                PRINT_FUNCTION("Target Region:  \x01>", region_strings[0]);
            else
            PRINT_FUNCTION("Target Region: <\x01", region_strings[1]);
        }
        set_text_y(5);
        set_text_x(MAIN_MENU_DISTANCE_FROM_BORDER);
        if(!is_master)
            PRINT_FUNCTION("Act as:  \x01>", actor_strings[1]);
        else
            PRINT_FUNCTION("Act as: <\x01", actor_strings[0]);
        set_text_y(7);
        set_text_x(MAIN_MENU_DISTANCE_FROM_BORDER);
        PRINT_FUNCTION("Start Trade");
        set_text_y(9);
        set_text_x(MAIN_MENU_DISTANCE_FROM_BORDER);
        PRINT_FUNCTION("Send Multiboot");
        set_text_y(0x11);
        set_text_x(MAIN_MENU_DISTANCE_FROM_BORDER);
        PRINT_FUNCTION("View Party \x01", target_strings[curr_gen-1]);
    }
}

void print_multiboot_mid_process(u8 initial_handshake) {
    default_reset_screen();
    PRINT_FUNCTION("\nInitiating handshake!\n");
    
    if(initial_handshake)
        PRINT_FUNCTION("\nHandshake successful!\n\nGiving control to SWI 0x25!");
}

void print_multiboot(enum MULTIBOOT_RESULTS result) {

    default_reset_screen();
    
    if(result == MB_SUCCESS)
        PRINT_FUNCTION("\nMultiboot successful!");
    else if(result == MB_NO_INIT_SYNC)
        PRINT_FUNCTION("\nCouldn't sync.\n\nTry again!");
    else
        PRINT_FUNCTION("\nThere was an error.\n\nTry again!");
    
    set_text_y(5);
    PRINT_FUNCTION("A: To the previous menu");
}
