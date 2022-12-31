#include <gba.h>
#include "menu_text_handler.h"
#include "text_handler.h"
#include "options_handler.h"
#include "input_handler.h"
#include "version_identifier.h"

#define NUM_LINES 10

const char* person_strings[] = {" - You", " - Other"};
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

const u8 ribbon_print_pos[NUM_LINES*2] = {0,1,2,3,4,5,6,7,8,9,14,15,13,16,10,0xFF,11,0xFF,12,0xFF};

void print_game_info(struct game_data_t* game_data, int index) {
    iprintf("\n Game: ");
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
    iprintf("%s\n", chosen_str);
}

void print_trade_menu(struct game_data_t* game_data, u8 update, u8 curr_gen, u8 load_sprites, u8 is_own) {
    if(!update)
        return;
    
    if(curr_gen < 1)
        curr_gen = 3;
    if(curr_gen > 3)
        curr_gen = 3;
    
    iprintf("\x1b[2J");
    
    u8 num_parties = 2;
    if(is_own)
        num_parties = 1;
    
    u8 printable_string[X_TILES+1];
    u8 tmp_buffer[X_TILES+1];
    
    if(is_own) {
        text_gen3_to_generic(game_data[0].trainer_name, tmp_buffer, OT_NAME_GEN3_SIZE+1, X_TILES, game_data[0].game_identifier.game_is_jp, 0);
        iprintf("%s - Gen %d\n", tmp_buffer, curr_gen);
    }
    else {
        text_generic_terminator_fill(printable_string, X_TILES+1);
        for(int i = 0; i < num_parties; i++) {
            text_gen3_to_generic(game_data[i].trainer_name, tmp_buffer, OT_NAME_GEN3_SIZE+1, X_TILES, game_data[i].game_identifier.game_is_jp, 0);
            text_generic_concat(tmp_buffer, person_strings[i], printable_string + (i*(X_TILES>>1)), OT_NAME_GEN3_SIZE, (X_TILES >> 1)-OT_NAME_GEN3_SIZE, X_TILES>>1);
        }
        text_generic_replace(printable_string, X_TILES, GENERIC_EOL, GENERIC_SPACE);
        iprintf("%s", printable_string);
    }

    if(load_sprites)
        reset_sprites_to_cursor();

    u8* options[2];
    for(int i = 0; i < 2; i++)
        options[i] = get_options_trade(i);
    for(int i = 0; i < PARTY_SIZE; i++) {
        text_generic_terminator_fill(printable_string, X_TILES+1);
        for(int j = 0; j < num_parties; j++) {
            // These two values are for debug only - They should be j and i
            u8 party_index = j;
            u8 party_option_index = i;
            if(options[party_index][party_option_index] != 0xFF) {
                struct gen3_mon_data_unenc* mon = &game_data[party_index].party_3_undec[options[party_index][party_option_index]];
                // I tried just using printf here with left padding, but it's EXTREMELY slow
                text_generic_copy(get_pokemon_name_raw(mon), printable_string + 5 + ((X_TILES>>1)*j), NAME_SIZE, (X_TILES>>1) - 5);
                if(load_sprites)
                    load_pokemon_sprite_raw(mon, BASE_Y_SPRITE_TRADE_MENU + (i*BASE_Y_SPRITE_INCREMENT_TRADE_MENU), (((X_TILES >> 1) << 3)*j) + BASE_X_SPRITE_TRADE_MENU);
            }
        }
        text_generic_replace(printable_string, X_TILES, GENERIC_EOL, GENERIC_SPACE);
        iprintf("\n%s\n", printable_string);
    }
    iprintf("  Cancel");
}

void print_pokemon_base_info(u8 load_sprites, struct gen3_mon_data_unenc* mon, u8 page) {
    u8 printable_string[X_TILES+1];
    
    u8 is_shiny = is_shiny_gen3_raw(mon, 0);
    u8 has_pokerus = has_pokerus_gen3_raw(mon);
    u8 is_jp = mon->src->language;
    u8 is_egg = mon->is_egg;
    
    iprintf("\x1b[2J");
    
    u8 page_total = PAGES_TOTAL-FIRST_PAGE+1;
    
    if(is_egg)
        page_total = FIRST_PAGE;
    
    if(page == FIRST_PAGE)
        iprintf(" ");
    else
        iprintf("<");
        
    iprintf("%d / %d", page, page_total);
    
    if(page < page_total)
        iprintf(">");

    if(load_sprites) {
        reset_sprites_to_cursor();
        load_pokemon_sprite_raw(mon, BASE_Y_SPRITE_INFO_PAGE, BASE_X_SPRITE_INFO_PAGE);
    }
    
    if(!is_egg) {
        text_gen3_to_generic(mon->src->nickname, printable_string, NICKNAME_GEN3_SIZE, X_TILES, is_jp, 0);
        iprintf("\n\n    %s - %s %c\n", printable_string, get_pokemon_name_raw(mon), get_pokemon_gender_char_raw(mon));
    
        iprintf("    ");
        if(is_shiny)
            iprintf("Shiny");
    
        if(is_shiny && has_pokerus)
            iprintf(" - ");
    
        if(has_pokerus == HAS_POKERUS)
            iprintf("Has Pokerus");
        else if(has_pokerus == HAD_POKERUS)
            iprintf("Had Pokerus");
    }
    else
        iprintf("\n\n    %s\n", get_pokemon_name_raw(mon));
    
}

void print_bottom_info(){
    iprintf("\nB: Go Back");
}

void print_pokemon_page1(struct gen3_mon_data_unenc* mon) {
    u8 printable_string[X_TILES+1];
    u8 is_jp = (mon->src->language == JAPANESE_LANGUAGE);
    u8 is_egg = mon->is_egg;
    
    if(!is_egg) {
        
        iprintf("\nLevel: %d\n", to_valid_level_gen3(mon->src));
        
        if(is_jp)
            iprintf("\nLanguage: Japanese\n");
        else
            iprintf("\nLanguage: International\n");
        
        text_gen3_to_generic(mon->src->ot_name, printable_string, OT_NAME_GEN3_SIZE, X_TILES, is_jp, 0);
        iprintf("\nOT: %s - %c - %05d\n", printable_string, get_trainer_gender_char_raw(mon), (mon->src->ot_id)&0xFFFF);
        iprintf("\nItem: %s\n", get_item_name_raw(mon));
        
        iprintf("\nMet in: %s\n", get_met_location_name_gen3_raw(mon));
        u8 met_level = get_met_level_gen3_raw(mon);
        if(met_level > 0)
            iprintf("\nCaught at Level %d\n\nCaught in %s Ball\n\n", met_level, get_pokeball_base_name_gen3_raw(mon));
        else
            iprintf("\nHatched in %s Ball\n\n\n\n", get_pokeball_base_name_gen3_raw(mon));
    }
    else
        iprintf("\nHatches in : %d Egg Cycles\n\nHatches in: %d Steps\n\n\n\n\n\n\n\n\n\n\n\n", mon->growth.friendship, mon->growth.friendship * 0x100);
}

void print_pokemon_page2(struct gen3_mon_data_unenc* mon) {
        
    iprintf("\nSTAT    VALUE    EV    IV\n");
    for(int i = 0; i < GEN2_STATS_TOTAL; i++) {
        iprintf("\n %-3s", stat_strings[i]);
        if(i == HP_STAT_INDEX) {
            u16 hp = calc_stats_gen3_raw(mon, i);
            u16 curr_hp = mon->src->curr_hp;
            if(curr_hp > hp)
                curr_hp = hp;
            iprintf("  % 4d/%-4d", curr_hp, hp);
        }
        else
            iprintf("    % 4d   ", calc_stats_gen3_raw(mon,i));
        iprintf("% 4d   % 3d\n", get_evs_gen3(&mon->evs, i), get_ivs_gen3(&mon->misc, i));
    }
    
    iprintf("\n   Hidden Power %s: %d", get_hidden_power_type_name_gen3(&mon->misc), get_hidden_power_power_gen3(&mon->misc));
    //print_game_info();
}

void print_pokemon_page3(struct gen3_mon_data_unenc* mon) {

    iprintf("\nMOVES            PP UP\n");
    for(int i = 0; i < (MOVES_SIZE); i++){
        iprintf("\n %-14s    %d\n", get_move_name_gen3(&mon->attacks, i), (mon->growth.pp_bonuses >> (2*i)) & 3);
    }
    
    iprintf("\nAbility: %s\n", get_ability_name_raw(mon));
    
    iprintf("\nExperience: %d\n", get_proper_exp_raw(mon));
    
    if(to_valid_level_gen3(mon->src) < MAX_LEVEL)
        iprintf("\nNext Lv. in: %d > Lv. %d", get_level_exp_mon_index(get_mon_index_raw(mon), to_valid_level_gen3(mon->src)+1) - get_proper_exp_raw(mon), to_valid_level_gen3(mon->src)+1);
    else
        iprintf("\n");
}

void print_pokemon_page4(struct gen3_mon_data_unenc* mon) {

    iprintf("\nCONTEST STAT     VALUE\n");
    for(int i = 0; i < CONTEST_STATS_TOTAL; i++){
        iprintf("\n %-16s% 4d\n", contest_strings[i], mon->evs.contest[i]);
    }

    iprintf("\n");
}

void print_pokemon_page5(struct gen3_mon_data_unenc* mon) {
    u8 printable_string[X_TILES+1];

    iprintf("\nRIBBONS\n");
    for(int i = 0; i < NUM_LINES; i++){
        // CANNOT USE SNPRINTF OR SPRINTF! THEY ADD 20 KB!
        text_generic_concat(get_ribbon_name(&mon->misc, ribbon_print_pos[(i*2)]), get_ribbon_rank_name(&mon->misc, ribbon_print_pos[(i*2)]), printable_string, X_TILES, X_TILES, X_TILES);
        if(ribbon_print_pos[(i*2)+1] != 0xFF) {
            iprintf("\n %-13s ", printable_string);
            text_generic_concat(get_ribbon_name(&mon->misc, ribbon_print_pos[(i*2)+1]), get_ribbon_rank_name(&mon->misc, ribbon_print_pos[(i*2)+1]), printable_string, X_TILES, X_TILES, X_TILES);
            iprintf(" %-13s", printable_string);
        }
        else
            iprintf("\n %s", printable_string);
    }

    iprintf("\n\n\n");
}

typedef void (*print_info_functions_t)(struct gen3_mon_data_unenc*);

print_info_functions_t print_info_functions[PAGES_TOTAL] = {print_pokemon_page1, print_pokemon_page2, print_pokemon_page3, print_pokemon_page4, print_pokemon_page5};

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

    iprintf("\x1b[2J");
    
    if(!get_valid_options_main()) {
        iprintf("\n  Error reading the data!\n\n\n\n\n\n\n");
        iprintf("\n  Send Multiboot\n");
    }
    else {
        if(curr_gen >= TOTAL_GENS)
            curr_gen = 2;
        curr_gen = options[curr_gen];
        if(get_number_of_higher_ordered_options(options, curr_gen, TOTAL_GENS) > 0 && get_number_of_lower_ordered_options(options, curr_gen, TOTAL_GENS) > 0)
            iprintf("\n  Target: <%s>\n", target_strings[curr_gen-1]);
        else if(get_number_of_higher_ordered_options(options, curr_gen, TOTAL_GENS) > 0)
            iprintf("\n  Target:  %s>\n", target_strings[curr_gen-1]);
        else if(get_number_of_lower_ordered_options(options, curr_gen, TOTAL_GENS) > 0)
            iprintf("\n  Target: <%s\n", target_strings[curr_gen-1]);
        else
            iprintf("\n  Target:  %s\n", target_strings[curr_gen-1]);
        if(curr_gen < 3) {
            if(!is_jp)
                iprintf("\n  Target Region:  %s>\n", region_strings[0]);
            else
            iprintf("\n  Target Region: <%s\n", region_strings[1]);
        }
        else
            iprintf("\n\n");
        if(!is_master)
            iprintf("\n  Act as:  %s>\n", actor_strings[1]);
        else
            iprintf("\n  Act as: <%s\n", actor_strings[0]);
        iprintf("\n  Start Trade\n");
        iprintf("\n  Send Multiboot\n");
        iprintf("\n\n\n\n\n\n\n  View Party %s\n", target_strings[curr_gen-1]);
    }
}

void print_multiboot(enum MULTIBOOT_RESULTS result) {

    iprintf("\x1b[2J");
    
    if(result == MB_SUCCESS)
        iprintf("\nMultiboot successful!\n\n\n");
    else if(result == MB_NO_INIT_SYNC)
        iprintf("\nCouldn't sync.\n\nTry again!\n");
    else
        iprintf("\nThere was an error.\n\nTry again!\n");
    iprintf("\nA: To the previous menu");
}