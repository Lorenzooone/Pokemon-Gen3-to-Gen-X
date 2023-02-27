#include <gba.h>
#include "menu_text_handler.h"
#include "gen3_clock_events.h"
#include "text_handler.h"
#include "options_handler.h"
#include "input_handler.h"
#include "sprite_handler.h"
#include "version_identifier.h"
#include "print_system.h"
#include "sio_buffers.h"
#include "communicator.h"
#include "window_handler.h"
#include <stddef.h>

#define NUM_LINES 10
#define MAIN_MENU_DISTANCE_FROM_BORDER 2

#define SUMMARY_LINE_MAX_SIZE 18
#define PRINTABLE_INVALID_STRINGS 3

void print_basic_alter_conf_data(struct gen3_mon_data_unenc*, struct alternative_data_gen3*);
void print_pokemon_base_data(u8, struct gen3_mon_data_unenc*, u8, u8);
void print_pokemon_base_info(u8, struct gen3_mon_data_unenc*, u8);
void print_bottom_info(void);
void print_pokemon_page1(struct gen3_mon_data_unenc*);
void print_pokemon_page2(struct gen3_mon_data_unenc*);
void print_pokemon_page3(struct gen3_mon_data_unenc*);
void print_pokemon_page4(struct gen3_mon_data_unenc*);
void print_pokemon_page5(struct gen3_mon_data_unenc*);
void print_evolution_animation_internal(struct gen3_mon_data_unenc*, u8);

const char* person_strings[] = {"You", "Other"};
const char* game_strings[] = {"RS", "FRLG", "E"};
const char* unidentified_string = "Unidentified";
const char* subgame_rs_strings[] = {"R", "S"};
const char* subgame_frlg_strings[] = {"FR", "LG"};
const char* actor_strings[] = {"Master", "Slave"};
const char* region_strings[] = {"Int", "Jap"};
const char* target_strings[] = {"Gen 1", "Gen 2", "Gen 3"};
const char* stat_strings[] = {"Hp", "Atk", "Def", "SpA", "SpD", "Spe"};
const char* contest_strings[] = {"Coolness", "Beauty", "Cuteness", "Smartness", "Toughness", "Feel"};
const char* language_strings[NUM_LANGUAGES] = {"???", "Japanese", "English", "French", "Italian", "German", "Korean", "Spanish"};
const char* trade_start_state_strings[] = {"Unknown", "Entering Room", "Starting Trade", "Ending Trade", "Waiting Trade", "Trading Party Data", "Synchronizing", "Completed"};
const char* offer_strings[] = {" Sending ", "Receiving"};
const char* invalid_strings[][PRINTABLE_INVALID_STRINGS] = {{"      TRADE REJECTED!", "The Pok\xE9mon offered by the", "other player has issues!"}, {"      TRADE REJECTED!", "The trade would leave you", "with no usable Pok\xE9mon!"}};

const u8 ribbon_print_pos[NUM_LINES*2] = {0,1,2,3,4,5,6,7,8,9,14,15,13,16,10,0xFF,11,0xFF,12,0xFF};
typedef void (*print_info_functions_t)(struct gen3_mon_data_unenc*);
print_info_functions_t print_info_functions[PAGES_TOTAL] = {print_pokemon_page1, print_pokemon_page2, print_pokemon_page3, print_pokemon_page4, print_pokemon_page5};

void print_game_info(struct game_data_t* game_data, int index) {
    default_reset_screen();
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

void print_crash(enum CRASH_REASONS reason) {
    reset_screen(BLANK_FILL);
    
    init_crash_window();
    clear_crash_window();
    
    set_text_y(CRASH_WINDOW_Y);
    set_text_x(CRASH_WINDOW_X);
    PRINT_FUNCTION("      CRASHED!\n\n");
    set_text_x(CRASH_WINDOW_X);
    switch(reason) {
        case BAD_SAVE:
            PRINT_FUNCTION("ISSUES WHILE SAVING!\n\n");
            break;
        case BAD_TRADE:
            PRINT_FUNCTION("ISSUES WHILE TRADING!\n\n");
            break;
        case CARTRIDGE_REMOVED:
            PRINT_FUNCTION(" CARTRIDGE REMOVED!\n\n");
            break;
        default:
            break;
    }
    set_text_x(CRASH_WINDOW_X);
    PRINT_FUNCTION("TURN OFF THE CONSOLE.");
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
        PRINT_FUNCTION("\x05 - Gen \x03\n", game_data[0].trainer_name, GET_LANGUAGE_OT_NAME_LIMIT_DIRECT(game_data[0].game_identifier.game_is_jp), game_data[0].game_identifier.game_is_jp, curr_gen);
    else {
        for(int i = 0; i < num_parties; i++) {
            set_text_x(SCREEN_HALF_X * i);
            PRINT_FUNCTION("\x05 - \x01", game_data[i].trainer_name, GET_LANGUAGE_OT_NAME_LIMIT_DIRECT(game_data[i].game_identifier.game_is_jp), game_data[i].game_identifier.game_is_jp, person_strings[i]);
        }
    }

    if(load_sprites)
        reset_sprites_to_cursor(1);

    u8* options[2];
    for(int i = 0; i < 2; i++)
        options[i] = get_options_trade(i);
    for(gen3_party_total_t i = 0; i < PARTY_SIZE; i++) {
        set_text_y((BASE_Y_CURSOR_TRADING_MENU>>3)+((BASE_Y_SPRITE_INCREMENT_TRADE_MENU>>3) * i));
        for(int j = 0; j < num_parties; j++) {
            // These two values are for debug only - They should be j and i
            set_text_x(POKEMON_SPRITE_X_TILES + ((BASE_X_SPRITE_TRADE_MENU+7)>>3) + (SCREEN_HALF_X * j));
            u8 party_index = j;
            u8 party_option_index = i;
            if(options[party_index][party_option_index] != 0xFF) {
                struct gen3_mon_data_unenc* mon = &game_data[party_index].party_3_undec[options[party_index][party_option_index]];
                // I tried just using printf here with left padding, but it's EXTREMELY slow
                PRINT_FUNCTION("\x05", get_pokemon_name_raw(mon), SYS_LANGUAGE_LIMIT, IS_SYS_LANGUAGE_JAPANESE);
                if(load_sprites)
                    load_pokemon_sprite_raw(mon, 1, BASE_Y_SPRITE_TRADE_MENU + (i*BASE_Y_SPRITE_INCREMENT_TRADE_MENU), (BASE_X_SPRITE_INCREMENT_TRADE_MENU*j) + BASE_X_SPRITE_TRADE_MENU);
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

void print_swap_cartridge_menu() {
    default_reset_screen();

    PRINT_FUNCTION("Insert the new cartridge,\n\n");
    PRINT_FUNCTION("then press A to continue.\n\n");
}

void print_learnable_move(struct gen3_mon_data_unenc* mon, u16 index, enum MOVES_PRINTING_TYPE moves_printing_type) {
    reset_screen(BLANK_FILL);
    
    init_learn_move_message_window();
    clear_learn_move_message_window();
    
    if((!mon->is_valid_gen3) || (mon->is_egg))
        return;

    if(mon->learnable_moves == NULL)
        return;

    u16 move = mon->learnable_moves[1+index];
    
    set_text_y(LEARN_MOVE_MESSAGE_WINDOW_Y);
    set_text_x(LEARN_MOVE_MESSAGE_WINDOW_X);

    switch(moves_printing_type) {
        case LEARNT_P:
            PRINT_FUNCTION("\x05 learnt\n\n", get_pokemon_name_raw(mon), SYS_LANGUAGE_LIMIT, IS_SYS_LANGUAGE_JAPANESE);
            set_text_x(LEARN_MOVE_MESSAGE_WINDOW_X);
            PRINT_FUNCTION("\x01!", get_move_name_raw(move));
            break;
        case DID_NOT_LEARN_P:
            PRINT_FUNCTION("\x05 did not\n\n", get_pokemon_name_raw(mon), SYS_LANGUAGE_LIMIT, IS_SYS_LANGUAGE_JAPANESE);
            set_text_x(LEARN_MOVE_MESSAGE_WINDOW_X);
            PRINT_FUNCTION("learn \x01!", get_move_name_raw(move));
            break;
        case LEARNABLE_P:
            PRINT_FUNCTION("\x05 wants to\n", get_pokemon_name_raw(mon), SYS_LANGUAGE_LIMIT, IS_SYS_LANGUAGE_JAPANESE);
            set_text_x(LEARN_MOVE_MESSAGE_WINDOW_X);
            PRINT_FUNCTION("learn \x01!\n", get_move_name_raw(move));
            set_text_x(LEARN_MOVE_MESSAGE_WINDOW_X);
            PRINT_FUNCTION("Would you like to replace\n");
            set_text_x(LEARN_MOVE_MESSAGE_WINDOW_X);
            PRINT_FUNCTION("an existing move with it?");
            set_text_y(BASE_Y_CURSOR_LEARN_MOVE_MESSAGE>>3);
            set_text_x((BASE_X_CURSOR_LEARN_MOVE_MESSAGE_YES>>3)+2+LEARN_MOVE_MESSAGE_WINDOW_X);
            PRINT_FUNCTION("Yes");
            set_text_x((BASE_X_CURSOR_LEARN_MOVE_MESSAGE_NO>>3)+2+LEARN_MOVE_MESSAGE_WINDOW_X);
            PRINT_FUNCTION("No");
            break;
        default:
            break;
    }
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
        load_pokemon_sprite_raw(mon, 1, BASE_Y_SPRITE_OFFER_MENU, (OFFER_WINDOW_X*8*i) + BASE_X_SPRITE_OFFER_MENU);
        
        if(!is_egg) {
            set_text_y(curr_text_y++);
            set_text_x(POKEMON_SPRITE_X_TILES+(OFFER_WINDOW_X*i));
            PRINT_FUNCTION("\x05", get_pokemon_name_raw(mon), SYS_LANGUAGE_LIMIT, IS_SYS_LANGUAGE_JAPANESE);
        
            set_text_y(curr_text_y++);
            set_text_x(POKEMON_SPRITE_X_TILES+(OFFER_WINDOW_X*i));
            PRINT_FUNCTION("Lv. \x03 \x02", to_valid_level_gen3(mon->src), get_pokemon_gender_char_raw(mon));
            if(is_shiny) {
                set_text_y(curr_text_y++);
                set_text_x(POKEMON_SPRITE_X_TILES+(OFFER_WINDOW_X*i));
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
            for(size_t j = 0; j < MOVES_SIZE; j++) {
                set_text_y(curr_text_y++);
                set_text_x(OFFER_WINDOW_X*i);
                PRINT_FUNCTION("\x01", get_move_name_gen3(&mon->attacks, j));
            }
        }
        else {
            curr_text_y++;
            set_text_y(curr_text_y++);
            set_text_x(POKEMON_SPRITE_X_TILES+(OFFER_WINDOW_X*i));
            PRINT_FUNCTION("\x05", get_pokemon_name_raw(mon), SYS_LANGUAGE_LIMIT, IS_SYS_LANGUAGE_JAPANESE);
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

void print_rejected(u8 reason) {
    reset_screen(BLANK_FILL);
    init_rejected_window();
    clear_rejected_window();
    set_text_y(REJECTED_WINDOW_Y);
    set_text_x(REJECTED_WINDOW_X);
    if(!reason)
        PRINT_FUNCTION("The trade offer was denied!");
    else
        PRINT_FUNCTION("   The trade was refused!");
}

void print_offer_options_screen(struct game_data_t* game_data, u8 own_mon, u8 other_mon) {
    reset_screen(BLANK_FILL);
    init_offer_options_window();
    clear_offer_options_window();
    set_text_y(OFFER_OPTIONS_WINDOW_Y);
    set_text_x(OFFER_OPTIONS_WINDOW_X);
    PRINT_FUNCTION("Trade \x05\n", get_pokemon_name_raw(&game_data[0].party_3_undec[own_mon]), SYS_LANGUAGE_LIMIT, IS_SYS_LANGUAGE_JAPANESE);
    set_text_x(OFFER_OPTIONS_WINDOW_X);
    PRINT_FUNCTION("for \x05?\n\n", get_pokemon_name_raw(&game_data[1].party_3_undec[other_mon]), SYS_LANGUAGE_LIMIT, IS_SYS_LANGUAGE_JAPANESE);
    set_text_x(OFFER_OPTIONS_WINDOW_X+2);
    PRINT_FUNCTION("Yes");
    set_text_x(OFFER_OPTIONS_WINDOW_X + (OFFER_OPTIONS_WINDOW_X_SIZE - SUMMARY_LINE_MAX_SIZE));
    PRINT_FUNCTION("Summary: Sending\n\n");
    set_text_x(OFFER_OPTIONS_WINDOW_X+2);
    PRINT_FUNCTION("No");
    set_text_x(OFFER_OPTIONS_WINDOW_X + (OFFER_OPTIONS_WINDOW_X_SIZE - SUMMARY_LINE_MAX_SIZE));
    PRINT_FUNCTION("Summary: Receiving");
}

void print_base_settings_menu(struct game_data_priv_t* game_data_priv, u8 is_loaded) {
    default_reset_screen();
    PRINT_FUNCTION("\n  Color Settings\n\n");
    PRINT_FUNCTION("  Color Settings\n\n");
    if(is_loaded) {
        //change_time_of_day(game_data);
        //enable_rtc_reset(&game_data->clock_events);
        if(is_daytime(&game_data_priv->clock_events))
            PRINT_FUNCTION("Time: Day\n\n");
        else
            PRINT_FUNCTION("Time: Night\n\n");
        if(is_high_tide(&game_data_priv->clock_events))
            PRINT_FUNCTION("Tide: High\n\n");
        else
            PRINT_FUNCTION("Tide: Low\n\n");
        PRINT_FUNCTION("\x03 \x03", game_data_priv->clock_events.enable_rtc_reset_flag, game_data_priv->clock_events.enable_rtc_reset_var);
    }
    
    set_text_y(Y_LIMIT-1);
    PRINT_FUNCTION("B: Go Back");
}

void print_trade_options(u8 cursor_x_pos, u8 own_menu){
    reset_screen(BLANK_FILL);
    init_trade_options_window();
    clear_trade_options_window();
    set_text_y(TRADE_OPTIONS_WINDOW_Y);
    set_text_x(TRADE_OPTIONS_WINDOW_X);
    PRINT_FUNCTION("  Summary");
    set_text_x(TRADE_OPTIONS_WINDOW_X + (TRADE_OPTIONS_WINDOW_X_SIZE>>1));
    if(own_menu)
        PRINT_FUNCTION("  Fix IV");
    else if(cursor_x_pos)
        PRINT_FUNCTION("  Set Nature");
    else 
        PRINT_FUNCTION("  Offer");
}

void print_waiting(s8 y_increase){
    reset_screen(BLANK_FILL);
    init_waiting_window(y_increase);
    clear_waiting_window(y_increase);
    set_text_y(WAITING_WINDOW_Y + y_increase);
    set_text_x(WAITING_WINDOW_X);
    PRINT_FUNCTION("Waiting...");
}

void print_evolution_animation_internal(struct gen3_mon_data_unenc* mon, u8 is_second_run){
    reset_screen(BLANK_FILL);
    init_evolution_animation_window();
    clear_evolution_animation_window();
    set_text_y(EVOLUTION_ANIMATION_WINDOW_Y);
    set_text_x(EVOLUTION_ANIMATION_WINDOW_X);
    if(!is_second_run) {
        PRINT_FUNCTION("Huh?! \x01\n\n", mon->pre_evo_string);
        set_text_x(EVOLUTION_ANIMATION_WINDOW_X);
        PRINT_FUNCTION("is evolving?!");
    }
    else {
        PRINT_FUNCTION("\x01 evolved\n\n", mon->pre_evo_string);
        set_text_x(EVOLUTION_ANIMATION_WINDOW_X);
        PRINT_FUNCTION("into \x05!", get_pokemon_name_raw(mon), SYS_LANGUAGE_LIMIT, IS_SYS_LANGUAGE_JAPANESE);
    }
    swap_buffer_screen(get_screen_num(), 1);
}

void print_evolution_animation(struct gen3_mon_data_unenc* mon){
    print_evolution_animation_internal(mon, 0);
    print_evolution_animation_internal(mon, 1);
}

void print_trade_animation_send(struct gen3_mon_data_unenc* mon){
    u8 language = get_valid_language(mon->src->language);
    u8 is_egg = mon->is_egg;

    reset_screen(BLANK_FILL);
    init_trade_animation_send_window();
    clear_trade_animation_send_window();
    set_text_y(TRADE_ANIMATION_SEND_WINDOW_Y);
    set_text_x(TRADE_ANIMATION_SEND_WINDOW_X);
    if(!is_egg) {
        PRINT_FUNCTION("Sending \x05 away...\n\n", get_pokemon_name_raw(mon), SYS_LANGUAGE_LIMIT, IS_SYS_LANGUAGE_JAPANESE);
        set_text_x(TRADE_ANIMATION_SEND_WINDOW_X);
        PRINT_FUNCTION("Goodbye, \x05!", mon->src->nickname, GET_LANGUAGE_NICKNAME_LIMIT(language), GET_LANGUAGE_IS_JAPANESE(language));
    }
    else {
        PRINT_FUNCTION("Sending an Egg away...\n\n");
        set_text_x(TRADE_ANIMATION_SEND_WINDOW_X);
        PRINT_FUNCTION("Who knows what's inside...");
    }
}

void print_trade_animation_recv(struct gen3_mon_data_unenc* mon){
    u8 language = get_valid_language(mon->src->language);
    u8 is_egg = mon->is_egg;

    reset_screen(BLANK_FILL);
    init_trade_animation_recv_window();
    clear_trade_animation_recv_window();
    set_text_y(TRADE_ANIMATION_RECV_WINDOW_Y);
    set_text_x(TRADE_ANIMATION_RECV_WINDOW_X);
    if(!is_egg) {
        PRINT_FUNCTION("Received \x05.\n\n", get_pokemon_name_raw(mon), SYS_LANGUAGE_LIMIT, IS_SYS_LANGUAGE_JAPANESE);
        set_text_x(TRADE_ANIMATION_RECV_WINDOW_X);
        PRINT_FUNCTION("Welcome, \x05!", mon->src->nickname, GET_LANGUAGE_NICKNAME_LIMIT(language), GET_LANGUAGE_IS_JAPANESE(language));
    }
    else {
        PRINT_FUNCTION("Received an Egg.\n\n");
        set_text_x(TRADE_ANIMATION_SEND_WINDOW_X);
        PRINT_FUNCTION("Who knows what's inside!");
    }
}

void print_saving(){
    reset_screen(BLANK_FILL);
    init_saving_window();
    clear_saving_window();
    set_text_y(SAVING_WINDOW_Y);
    set_text_x(SAVING_WINDOW_X);
    PRINT_FUNCTION("Saving...");
}

void print_loading(){
    reset_screen(BLANK_FILL);
    init_loading_window();
    clear_loading_window();
    set_text_y(LOADING_WINDOW_Y);
    set_text_x(LOADING_WINDOW_X);
    PRINT_FUNCTION("Loading...");
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

void print_basic_alter_conf_data(struct gen3_mon_data_unenc* mon, struct alternative_data_gen3* altered) {
    PRINT_FUNCTION("\n       OLD    NEW   OLD  NEW");
    PRINT_FUNCTION("\nSTAT  VALUE  VALUE   IV   IV");
    for(int i = 0; i < GEN2_STATS_TOTAL; i++) {
        PRINT_FUNCTION("\n \x11", stat_strings[i], 3);
        PRINT_FUNCTION("  \x09\x02", calc_stats_gen3_raw(mon,i), 4, get_nature_symbol(mon->src->pid, i));
        PRINT_FUNCTION("  \x09\x02", calc_stats_gen3_raw_alternative(mon, altered, i), 4, get_nature_symbol(altered->pid, i));
        PRINT_FUNCTION("  \x09  \x09", get_ivs_gen3(&mon->misc, i), 3, get_ivs_gen3_pure(altered->ivs, i), 3);
    }
    PRINT_FUNCTION("\n\nOld Hidden Power: \x01 \x03", get_hidden_power_type_name_gen3(&mon->misc), get_hidden_power_power_gen3(&mon->misc));
    PRINT_FUNCTION("\nNew Hidden Power: \x01 \x03", get_hidden_power_type_name_gen3_pure(altered->ivs), get_hidden_power_power_gen3_pure(altered->ivs));
}

void print_set_nature(u8 load_sprites, struct gen3_mon_data_unenc* mon) {
    default_reset_screen();

    if((!mon->is_valid_gen3) || (mon->is_egg)) {
        print_bottom_info();
        return;
    }

    print_pokemon_base_data(load_sprites, mon, BASE_Y_SPRITE_NATURE_PAGE, BASE_X_SPRITE_NATURE_PAGE);

    print_basic_alter_conf_data(mon, &mon->alter_nature);
    
    set_text_y(16);
    PRINT_FUNCTION("\x01 Nature to: <\x01>", get_nature_name(mon->src->pid), get_nature_name(mon->alter_nature.pid));
    
    set_text_y(Y_LIMIT-1);
    PRINT_FUNCTION("A: Confirm - B: Go Back");
}

void print_iv_fix(struct gen3_mon_data_unenc* mon) {
    default_reset_screen();

    if((!mon->is_valid_gen3) || (mon->is_egg) || (!mon->can_roamer_fix)) {
        print_bottom_info();
        return;
    }

    print_pokemon_base_data(1, mon, BASE_Y_SPRITE_IV_FIX_PAGE, BASE_X_SPRITE_IV_FIX_PAGE);
    u16 species = mon->growth.species;

    print_basic_alter_conf_data(mon, &mon->fixed_ivs);

    PRINT_FUNCTION("\nOld Met in: \x01", get_met_location_name_gen3_raw(mon));
    if((species == LATIAS_SPECIES) || (species == LATIOS_SPECIES))
        PRINT_FUNCTION("\nNew Met in: Southern Island");
    if((species == RAIKOU_SPECIES) || (species == ENTEI_SPECIES) || (species == SUICUNE_SPECIES))
        PRINT_FUNCTION("\nNew Met in: Deep Colosseum");
    
    PRINT_FUNCTION("\nCaught Level: from \x03 to \x03", get_met_level_gen3_raw(mon), mon->fixed_ivs.origins_info&0x7F);
    
    if(mon->fix_has_altered_ot) {
        PRINT_FUNCTION("\n           WARNING:");
        PRINT_FUNCTION("\n  CHANGED ORIGINAL TRAINER!");
    }
    
    set_text_y(Y_LIMIT-1);
    PRINT_FUNCTION("A: Confirm - B: Go Back");
}

void print_learnable_moves_menu(struct gen3_mon_data_unenc* mon, u16 index) {
    default_reset_screen();

    if((!mon->is_valid_gen3) || (mon->is_egg))
        return;

    if(mon->learnable_moves == NULL)
        return;

    u16 move = mon->learnable_moves[1+index];

    print_pokemon_base_data(1, mon, BASE_Y_SPRITE_IV_FIX_PAGE, BASE_X_SPRITE_IV_FIX_PAGE);

    u8 base_x = 0;
    u8 base_y = get_text_y() + 1;
    set_text_y(base_y);
    for(int i = 0; i < GEN2_STATS_TOTAL; i++) {
        set_text_x(base_x);
        PRINT_FUNCTION("\x11: \x09\x02\n", stat_strings[i], 3, calc_stats_gen3_raw(mon,i), 4, get_nature_symbol(mon->src->pid, i));
        if(i == ((GEN2_STATS_TOTAL-1)>>1)) {
            set_text_y(base_y);
            base_x += X_LIMIT>>1;
        }
    }
    PRINT_FUNCTION("Ability: \x01", get_ability_name_raw(mon));
    PRINT_FUNCTION("\nHidden Power: \x01 \x03", get_hidden_power_type_name_gen3(&mon->misc), get_hidden_power_power_gen3(&mon->misc));
    PRINT_FUNCTION("\nWhich move will be forgotten?\n");
    set_text_x((BASE_X_CURSOR_LEARN_MOVE_MENU>>3)+2);
    PRINT_FUNCTION("MOVES");
    set_text_x(X_LIMIT-11+(BASE_X_CURSOR_LEARN_MOVE_MENU>>3)+2);
    PRINT_FUNCTION("PP UP");
    for(size_t i = 0; i < MOVES_SIZE; i++){
        set_text_y((BASE_Y_CURSOR_LEARN_MOVE_MENU + (BASE_Y_CURSOR_INCREMENT_LEARN_MOVE_MENU*i))>>3);
        set_text_x((BASE_X_CURSOR_LEARN_MOVE_MENU>>3)+2);
        PRINT_FUNCTION("\x01", get_move_name_gen3(&mon->attacks, i));
        set_text_x((X_LIMIT-11+(BASE_X_CURSOR_LEARN_MOVE_MENU>>3)+2)+2);
        PRINT_FUNCTION("\x03", (mon->growth.pp_bonuses >> (2*i)) & 3);
    }
    set_text_y((BASE_Y_CURSOR_LEARN_MOVE_MENU + (BASE_Y_CURSOR_INCREMENT_LEARN_MOVE_MENU*MOVES_SIZE))>>3);
    set_text_x((BASE_X_CURSOR_LEARN_MOVE_MENU>>3)+2);
    PRINT_FUNCTION("\x01", get_move_name_raw(move));
    set_text_x((X_LIMIT-11+(BASE_X_CURSOR_LEARN_MOVE_MENU>>3)+2)+1);
    PRINT_FUNCTION("NEW");
}

void print_pokemon_base_data(u8 load_sprites, struct gen3_mon_data_unenc* mon, u8 y, u8 x) {
    u8 is_shiny = is_shiny_gen3_raw(mon, 0);
    u8 has_pokerus = has_pokerus_gen3_raw(mon);
    u8 language = get_valid_language(mon->src->language);
    u8 is_egg = mon->is_egg;

    if(load_sprites) {
        reset_sprites_to_party();
        load_pokemon_sprite_raw(mon, 1, y, x);
    }
    
    set_text_y((y>>3) + (POKEMON_SPRITE_Y_TILES>>1));
    set_text_x((x>>3) + POKEMON_SPRITE_X_TILES);
    
    if(!is_egg) {
        PRINT_FUNCTION("\x05 - \x05 \x02\n", mon->src->nickname, GET_LANGUAGE_NICKNAME_LIMIT(language), GET_LANGUAGE_IS_JAPANESE(language), get_pokemon_name_raw(mon), SYS_LANGUAGE_LIMIT, IS_SYS_LANGUAGE_JAPANESE, get_pokemon_gender_char_raw(mon));
    
        set_text_x((x>>3) + POKEMON_SPRITE_X_TILES);
        
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
        PRINT_FUNCTION("\x05\n", get_pokemon_name_raw(mon), SYS_LANGUAGE_LIMIT, IS_SYS_LANGUAGE_JAPANESE);
}

void print_pokemon_base_info(u8 load_sprites, struct gen3_mon_data_unenc* mon, u8 page) {
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
    
    print_pokemon_base_data(load_sprites, mon, BASE_Y_SPRITE_INFO_PAGE, BASE_X_SPRITE_INFO_PAGE);
}

void print_bottom_info(){
    set_text_y(Y_LIMIT-1);
    PRINT_FUNCTION("B: Go Back");
}

void print_pokemon_page1(struct gen3_mon_data_unenc* mon) {
    u8 language = get_valid_language(mon->src->language);
    u8 is_egg = mon->is_egg;
    
    if(!is_egg) {
        PRINT_FUNCTION("\nLevel: \x13Nature: \x01\n", to_valid_level_gen3(mon->src), 6, get_nature_name(mon->src->pid));
        
        PRINT_FUNCTION("\nLanguage: \x01\n", language_strings[language]);
        
        PRINT_FUNCTION("\nOT: \x05 - \x02 - \x0B\n", mon->src->ot_name, GET_LANGUAGE_OT_NAME_LIMIT(language), GET_LANGUAGE_IS_JAPANESE(language), get_trainer_gender_char_raw(mon), (mon->src->ot_id)&0xFFFF, 5);
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
    for(size_t i = 0; i < (MOVES_SIZE); i++){
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
            set_text_x(SCREEN_HALF_X);
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
        set_text_y(Y_LIMIT-5);
        set_text_x(MAIN_MENU_DISTANCE_FROM_BORDER);
        PRINT_FUNCTION("Load Cartridge");
        set_text_y(Y_LIMIT-1);
        set_text_x(MAIN_MENU_DISTANCE_FROM_BORDER);
        PRINT_FUNCTION("Settings");
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
        set_text_y(Y_LIMIT-5);
        set_text_x(MAIN_MENU_DISTANCE_FROM_BORDER);
        PRINT_FUNCTION("Swap Cartridge");
        set_text_y(Y_LIMIT-3);
        set_text_x(MAIN_MENU_DISTANCE_FROM_BORDER);
        PRINT_FUNCTION("View Party \x01", target_strings[curr_gen-1]);
        set_text_y(Y_LIMIT-1);
        set_text_x(MAIN_MENU_DISTANCE_FROM_BORDER);
        PRINT_FUNCTION("Settings");
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
