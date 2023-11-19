#include "base_include.h"
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
#include "config_settings.h"
#include "agbabi.h"
#include <stddef.h>

#define NUM_LINES 10
#define MAIN_MENU_DISTANCE_FROM_BORDER 2

#define ROM_SIZE 0x1000000

#define SUMMARY_LINE_MAX_SIZE 18
#define PRINTABLE_INVALID_STRINGS 3

#if ENABLED_PRINT_INFO
#if ENABLED_LARGE_CRC_TABLE
const u32 poly8_lookup[256] =
{
 0, 0x77073096, 0xEE0E612C, 0x990951BA,
 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
 0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
 0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
 0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
 0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
 0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
 0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
 0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
 0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
 0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
 0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
 0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
 0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
 0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
 0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
 0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
 0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
 0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
 0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
 0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
 0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};
#endif
#endif

u32 crc32b(u8* target, size_t size);
void print_basic_alter_conf_data(struct gen3_mon_data_unenc*, struct alternative_data_gen3*);
void print_pokemon_base_data(u8, struct gen3_mon_data_unenc*, u8, u8, u8);
void print_pokemon_base_info(u8, struct gen3_mon_data_unenc*, u8);
void print_bottom_info(void);
void print_pokemon_page1(struct gen3_mon_data_unenc*);
void print_pokemon_page2(struct gen3_mon_data_unenc*);
void print_pokemon_page3(struct gen3_mon_data_unenc*);
void print_pokemon_page4(struct gen3_mon_data_unenc*);
void print_pokemon_page5(struct gen3_mon_data_unenc*);
void print_evolution_animation_internal(struct gen3_mon_data_unenc*, u8);
const u8* get_language_string(u8);
void print_single_colour_info(u8);

const char* person_strings[] = {"You", "Other"};
const char* maingame_strings[] = {"RS", "FRLG", "E"};
const char* unidentified_string = "Unidentified";
const char* subgame_rs_strings[] = {"R", "S"};
const char* subgame_frlg_strings[] = {"FR", "LG"};
const char* game_strings[] = {"???", "Sapphire", "Ruby", "Emerald", "Fire Red", "Leaf Green"};
const char* actor_strings[] = {"Master", "Slave"};
const char* region_strings[] = {"Int", "Jpn"};
const char* target_strings[] = {"Gen 1", "Gen 2", "Gen 3"};
const char* stat_strings[] = {"Hp", "Atk", "Def", "SpA", "SpD", "Spe"};
const char* contest_strings[] = {"Coolness", "Beauty", "Cuteness", "Smartness", "Toughness", "Feel"};
const char* language_strings[NUM_LANGUAGES+1] = {"???", "Japanese", "English", "French", "Italian", "German", "Korean", "Spanish", "Unknown"};
const char* trade_start_state_strings[] = {"Unknown", "Entering Room", "Starting Trade", "Ending Trade", "Waiting Trade", "Trading Party Data", "Synchronizing", "Completed"};
const char* offer_strings[] = {"Sending ", "Receiving"};
const char colours_chars[] = {'R', 'G', 'B'};
const char* invalid_strings[][PRINTABLE_INVALID_STRINGS] = {{"      TRADE REJECTED!", "The Pok\xE9mon offered by the", "other player has issues!"}, {"      TRADE REJECTED!", "The trade would leave you", "with no usable Pok\xE9mon!"}};

const u8 ribbon_print_pos[NUM_LINES*2] = {0,1,2,3,4,5,6,7,8,9,14,15,13,16,10,0xFF,11,0xFF,12,0xFF};
typedef void (*print_info_functions_t)(struct gen3_mon_data_unenc*);
print_info_functions_t print_info_functions[PAGES_TOTAL] = {print_pokemon_page1, print_pokemon_page2, print_pokemon_page3, print_pokemon_page4, print_pokemon_page5};

// calculate a checksum on a buffer -- start address = p, length = bytelength
#if ENABLED_PRINT_INFO
u32 crc32b(u8* target, size_t size)
#else
u32 crc32b(u8* UNUSED(target), size_t UNUSED(size))
#endif
{
    u32 crc = 0xffffffff;
    #if ENABLED_PRINT_INFO
    while (size-- !=0) {
        #if ENABLED_LARGE_CRC_TABLE
        crc = poly8_lookup[((u8) crc ^ *(target++))] ^ (crc >> 8);
        #else
        crc ^= *(target++);
        for(int i = 0; i < 8; i++)
        {
            u32 t = ~((crc & 1) - 1);
            crc = (crc >> 1) ^ (0xEDB88320 & t);
        }
        #endif
    }
    #endif
    return (crc ^ 0xffffffff);
}

void print_game_info(struct game_data_t* game_data, int index) {
    default_reset_screen();
    PRINT_FUNCTION("\n Game: ");
    const char* chosen_str = maingame_strings[game_data[index].game_identifier.game_main_version];
    switch(game_data[index].game_identifier.game_main_version) {
        case RS_MAIN_GAME_CODE:
            if(!game_data[index].game_identifier.game_sub_version_undetermined)
                chosen_str = subgame_rs_strings[game_data[index].game_identifier.game_sub_version];
            break;
        case FRLG_MAIN_GAME_CODE:
            if(!game_data[index].game_identifier.game_sub_version_undetermined)
                chosen_str = subgame_frlg_strings[game_data[index].game_identifier.game_sub_version];
            break;
        case E_MAIN_GAME_CODE:
            break;
        default:
            chosen_str = unidentified_string;
            break;
    }
    PRINT_FUNCTION("\x01\n", chosen_str);
    PRINT_FUNCTION("\n0x\x04\n", crc32b((u8*)ROM, ROM_SIZE));
    PRINT_FUNCTION("\n0x\x04\n", crc32b((u8*)EWRAM, EWRAM_SIZE));
    PRINT_FUNCTION("\n0x\x04\n", read_magic_number(0, 0));
    PRINT_FUNCTION("\n0x\x04\n", read_magic_number(1, 0));
    PRINT_FUNCTION("\n0x\x04\n", read_magic_number(1, 6));
    
    PRINT_FUNCTION("\nPress A to go back!");
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
        PRINT_FUNCTION("\x05 - Gen \x03\n", game_data[0].trainer_name, GET_LANGUAGE_OT_NAME_LIMIT(game_data[0].game_identifier.language), GET_LANGUAGE_IS_JAPANESE(game_data[0].game_identifier.language), curr_gen);
    else {
        for(int i = 0; i < num_parties; i++) {
            set_text_x(SCREEN_HALF_X * i);
            PRINT_FUNCTION("\x05 - \x01", game_data[i].trainer_name, GET_LANGUAGE_OT_NAME_LIMIT(game_data[i].game_identifier.language), GET_LANGUAGE_IS_JAPANESE(game_data[i].game_identifier.language), person_strings[i]);
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
                PRINT_FUNCTION("\x05", get_pokemon_name_raw(mon), get_pokemon_name_raw_language_limit(mon), IS_SYS_LANGUAGE_JAPANESE);
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

    u16 move = mon->learnable_moves->moves[index];
    
    set_text_y(LEARN_MOVE_MESSAGE_WINDOW_Y+JAPANESE_Y_AUTO_INCREASE);
    set_text_x(LEARN_MOVE_MESSAGE_WINDOW_X);

    switch(moves_printing_type) {
        case LEARNT_P:
            PRINT_FUNCTION("\x05 learnt\n\n", get_pokemon_name_raw(mon), get_pokemon_name_raw_language_limit(mon), IS_SYS_LANGUAGE_JAPANESE);
            set_text_x(LEARN_MOVE_MESSAGE_WINDOW_X);
            PRINT_FUNCTION("\x01!", get_move_name_raw(move));
            break;
        case DID_NOT_LEARN_P:
            PRINT_FUNCTION("\x05 did not\n\n", get_pokemon_name_raw(mon), get_pokemon_name_raw_language_limit(mon), IS_SYS_LANGUAGE_JAPANESE);
            set_text_x(LEARN_MOVE_MESSAGE_WINDOW_X);
            PRINT_FUNCTION("learn \x01!", get_move_name_raw(move));
            break;
        case LEARNABLE_P:
            PRINT_FUNCTION("\x05 wants to\n", get_pokemon_name_raw(mon), get_pokemon_name_raw_language_limit(mon), IS_SYS_LANGUAGE_JAPANESE);
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
        set_text_x(POKEMON_SPRITE_X_TILES+(OFFER_WINDOW_X*i));
        PRINT_FUNCTION("\x01", offer_strings[i]);
        load_pokemon_sprite_raw(mon, 1, BASE_Y_SPRITE_OFFER_MENU, (OFFER_WINDOW_X*8*i) + BASE_X_SPRITE_OFFER_MENU);
        curr_text_y++;
        set_text_y(curr_text_y++);
        set_text_x(POKEMON_SPRITE_X_TILES+(OFFER_WINDOW_X*i));

        if(!is_egg) {
            PRINT_FUNCTION("\x05", get_pokemon_name_raw(mon), get_pokemon_name_raw_language_limit(mon), IS_SYS_LANGUAGE_JAPANESE);

            set_text_y(curr_text_y++);
            set_text_x((OFFER_WINDOW_X*i));
            PRINT_FUNCTION("Lv.\x03 \x02", to_valid_level_gen3(mon->src), get_pokemon_gender_char_raw(mon));
            if(is_shiny)
                PRINT_FUNCTION(" Shiny");
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
        else
            PRINT_FUNCTION("\x05", get_pokemon_name_raw(mon), get_pokemon_name_raw_language_limit(mon), IS_SYS_LANGUAGE_JAPANESE);
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
    set_text_y(OFFER_OPTIONS_WINDOW_Y+JAPANESE_Y_AUTO_INCREASE);
    set_text_x(OFFER_OPTIONS_WINDOW_X);
    struct gen3_mon_data_unenc* own_mon_data = &game_data[0].party_3_undec[own_mon];
    struct gen3_mon_data_unenc* other_mon_data = &game_data[1].party_3_undec[other_mon];
    if(!IS_SYS_LANGUAGE_JAPANESE) {
        PRINT_FUNCTION("Trade \x05\n", get_pokemon_name_raw(own_mon_data), get_pokemon_name_raw_language_limit(own_mon_data), IS_SYS_LANGUAGE_JAPANESE);
        set_text_x(OFFER_OPTIONS_WINDOW_X);
        PRINT_FUNCTION("for \x05?\n\n", get_pokemon_name_raw(other_mon_data), get_pokemon_name_raw_language_limit(other_mon_data), IS_SYS_LANGUAGE_JAPANESE);
    }
    else {
        PRINT_FUNCTION("Trade \x05 ", get_pokemon_name_raw(own_mon_data), get_pokemon_name_raw_language_limit(own_mon_data), IS_SYS_LANGUAGE_JAPANESE);
        PRINT_FUNCTION("for \x05?\n\n", get_pokemon_name_raw(other_mon_data), get_pokemon_name_raw_language_limit(other_mon_data), IS_SYS_LANGUAGE_JAPANESE);
    }
    set_text_x(OFFER_OPTIONS_WINDOW_X+2);
    PRINT_FUNCTION("Yes");
    set_text_x(OFFER_OPTIONS_WINDOW_X + (OFFER_OPTIONS_WINDOW_X_SIZE - SUMMARY_LINE_MAX_SIZE));
    PRINT_FUNCTION("Summary: Sending\n\n");
    set_text_x(OFFER_OPTIONS_WINDOW_X+2);
    PRINT_FUNCTION("No");
    set_text_x(OFFER_OPTIONS_WINDOW_X + (OFFER_OPTIONS_WINDOW_X_SIZE - SUMMARY_LINE_MAX_SIZE));
    PRINT_FUNCTION("Summary: Receiving");
}

const u8* get_language_string(u8 language) {
    if(language > NUM_LANGUAGES)
        language = NUM_LANGUAGES;
    return (const u8*)language_strings[language];
}

void print_gen12_settings_menu(u8 update) {
    if(!update)
        return;

    default_reset_screen();
    PRINT_FUNCTION("\n  Target Language: <\x01>\n\n", get_language_string(get_target_int_language()));
    PRINT_FUNCTION("  Convert to: <\x01>\n\n", game_strings[get_default_conversion_game()]);
    PRINT_FUNCTION("  Prioritize Colo/XD:");
    if(get_conversion_colo_xd())
        PRINT_FUNCTION(" <True>\n\n");
    else
        PRINT_FUNCTION(" <False>\n\n");
    PRINT_FUNCTION("  Gen 1 Everstone: ");
    if(get_gen1_everstone())
        PRINT_FUNCTION("<Enabled>\n\n");
    else
        PRINT_FUNCTION("<Disabled>\n\n");
    PRINT_FUNCTION("  Caught in: <\x01 Ball>\n\n", get_pokeball_base_name_gen3_pure(get_applied_ball()));
    PRINT_FUNCTION("  Hatched at: ");
    PRINT_FUNCTION("<\x0B> +/- 1\n\n", get_egg_met_location(), 3);
    PRINT_FUNCTION("  Hatched at: ");
    PRINT_FUNCTION("<\x0B> +/- 10\n\n\n", get_egg_met_location(), 3);
    PRINT_FUNCTION("Hatched at: ");
    PRINT_FUNCTION("\x01\n\n", get_met_location_name_gen3_pure(get_egg_met_location(), get_default_conversion_game()));

    print_bottom_info();
}

void print_base_settings_menu(struct game_identity* game_identifier, u8 is_loaded, u8 update) {
    if(!update)
        return;

    default_reset_screen();
    PRINT_FUNCTION("\n  System Language: <\x01>\n\n", get_language_string(get_sys_language()));
    PRINT_FUNCTION("  Gen 1/2 Settings\n\n");
    u8 curr_y = get_text_y();
    if(is_loaded) {
        if(has_rtc_events(game_identifier))
            PRINT_FUNCTION("  Clock Settings\n\n");
        set_text_y(curr_y+2);
        if(game_identifier->game_sub_version_undetermined)
            PRINT_FUNCTION("  Game Loaded: <\x01>\n\n", game_strings[id_to_version(game_identifier)]);
        /*
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
        */
    }

    set_text_y(curr_y+4);
    PRINT_FUNCTION("  Color Settings");

    set_text_y(Y_LIMIT-3);
    PRINT_FUNCTION("  Cheats");

    print_bottom_info();
    const struct version_t* version = get_version();
    set_text_x(X_LIMIT-(4+(3*3)+1));
    PRINT_FUNCTION("V.\x03.\x03.\x03\x02", version->main_version, version->sub_version, version->revision_version, version->revision_letter);
}

void print_clock_menu(struct clock_events_t* clock_events, struct saved_time_t* time_change, u8 update) {
    if(!update)
        return;

    init_rtc_time();
    struct saved_time_t base_time;
    struct saved_time_t new_time;

    get_clean_time(&clock_events->saved_time, &base_time);
    get_increased_time(&clock_events->saved_time, time_change, &new_time);

    default_reset_screen();

    PRINT_FUNCTION("\n  Time: ");
    if(is_daytime(clock_events, time_change))
        PRINT_FUNCTION("<Day>\n\n");
    else
        PRINT_FUNCTION("<Night>\n\n");
    PRINT_FUNCTION("  Shoal Cave Tide: ");
    if(is_high_tide(clock_events, time_change))
        PRINT_FUNCTION("<High>\n\n");
    else
        PRINT_FUNCTION("<Low>\n\n");
    u8 curr_y = get_text_y();
    PRINT_FUNCTION("  Clock Reset Menu: ");
    if(is_rtc_reset_enabled(clock_events))
        PRINT_FUNCTION("<Enabled>");
    else
        PRINT_FUNCTION("<Disabled>");
    set_text_y(curr_y+2);

    PRINT_FUNCTION("  Days Increase:    <\x0B>\n", time_change->d, 5);
    PRINT_FUNCTION("  Hours Increase:   <\x0B>\n", time_change->h, 2);
    PRINT_FUNCTION("  Minutes Increase: <\x0B>\n", time_change->m, 2);
    PRINT_FUNCTION("  Seconds Increase: <\x0B>\n", time_change->s, 2);

    set_text_y(Y_LIMIT-8);
    PRINT_FUNCTION("  Save and Exit\n\n");
    PRINT_FUNCTION("Base Time: \x0B-\x0B:\x0B:\x0B\n", base_time.d, 5, base_time.h, 2, base_time.m, 2, base_time.s, 2);
    PRINT_FUNCTION("New Time:  \x0B-\x0B:\x0B:\x0B\n", new_time.d, 5, new_time.h, 2, new_time.m, 2, new_time.s, 2);
    set_text_y(Y_LIMIT-4);
    PRINT_FUNCTION("Issues may arise when changing");
    set_text_y(Y_LIMIT-3);
    PRINT_FUNCTION("the time of a Working Battery.");
    set_text_y(Y_LIMIT-1);
    PRINT_FUNCTION("B: Exit Without Saving");
}

void print_clock(void) {
    default_reset_screen();

    #if ACTIVE_RTC_FUNCTIONS
    REG_IME = 0;
	*(vu16*)0x9FE0000 = 0xD200;
	*(vu16*)0x8000000 = 0x1500;
	*(vu16*)0x8020000 = 0xD200;
	*(vu16*)0x8040000 = 0x1500;
	*(vu16*)0x9880000 = 0x8002; //Kernel mode - Kills ROM, so be careful where this is
	*(vu16*)0x9FC0000 = 0x1500;
    *(vu16*)0x9FE0000 = 0xD200;
    *(vu16*)0x8000000 = 0x1500;
    *(vu16*)0x8020000 = 0xD200;
    *(vu16*)0x8040000 = 0x1500;
    *(vu16*)0x96A0000 = 0x0001; // Enable clock
    *(vu16*)0x9FC0000 = 0x1500;
    if(!__agbabi_rtc_init()) {
        __agbabi_rtc_setdatetime((volatile __agbabi_datetime_t) {
                0x100978,
                0x563412
        }); /* Set date & time to 2078-09-10, 12:34:56 */
        //__agbabi_rtc_settime((volatile unsigned int)0x563412); /* Set time to 12:34:56 */
        __agbabi_datetime_t datetime = __agbabi_rtc_datetime();
        REG_IME = 1;
        u16 year = bcd_decode(datetime[0], 1);
        u8 month = bcd_decode(datetime[0] >> 8, 1);
        u8 day = bcd_decode(datetime[0] >> 16, 1);
        u8 wday = bcd_decode(datetime[0] >> 24, 1);
        u8 hour = bcd_decode(datetime[1], 1);
        u8 minute = bcd_decode(datetime[1] >> 8, 1);
        u8 second = bcd_decode(datetime[1] >> 16, 1);
        PRINT_FUNCTION("Year: \x0B\n", year, 2);
        PRINT_FUNCTION("Month: \x0B\n", month, 2);
        PRINT_FUNCTION("Day: \x0B\n", day, 2);
        PRINT_FUNCTION("Hour: \x0B\n", hour, 2);
        PRINT_FUNCTION("Minute: \x0B\n", minute, 2);
        PRINT_FUNCTION("Second: \x0B\n", second, 2);
        PRINT_FUNCTION("WDay: \x0B\n", wday, 2);
    }
    else
        REG_IME = 1;
    #endif
}

void print_cheats_menu(u8 update) {
    if(!update)
        return;

    default_reset_screen();
    PRINT_FUNCTION("\n  Cross-Gen Evo.:");
    if(get_allow_cross_gen_evos())
        PRINT_FUNCTION(" <Enabled>\n\n");
    else
        PRINT_FUNCTION(" <Disabled>\n\n");
    PRINT_FUNCTION("  Tradeless Evo.:");
    if(get_evolve_without_trade())
        PRINT_FUNCTION(" <Enabled>\n\n");
    else
        PRINT_FUNCTION(" <Disabled>\n\n");
    PRINT_FUNCTION("  Undistr. Events:");
    if(get_allow_undistributed_events())
        PRINT_FUNCTION(" <Enabled>\n\n");
    else
        PRINT_FUNCTION(" <Disabled>\n\n");
    PRINT_FUNCTION("  Fast Hatch Eggs:");
    if(get_fast_hatch_eggs())
        PRINT_FUNCTION(" <Enabled>\n\n");
    else
        PRINT_FUNCTION(" <Disabled>\n\n");
    PRINT_FUNCTION("  Give Pok\xE9rus to Party\n\n");

    print_bottom_info();
}

void print_single_colour_info(u8 colour) {
    for(int i = 0; i < NUM_SUB_COLOURS; i++) {
        set_text_x((BASE_X_CURSOR_COLOURS_SETTINGS_MENU_IN>>3)+(i*(BASE_X_CURSOR_INCREMENT_COLOURS_SETTINGS_MENU_IN>>3))+2);
        if(colour < NUM_COLOURS)
            PRINT_FUNCTION("\x0B", get_single_colour(colour, i), 2);
        else
            PRINT_FUNCTION(" \x02", colours_chars[i]);
    }
}

void print_colour_settings_menu(u8 update) {
    if(!update)
        return;

    default_reset_screen();
    PRINT_FUNCTION("\n  Colour");
    print_single_colour_info(NUM_COLOURS);
    set_text_y(BASE_Y_CURSOR_COLOURS_SETTINGS_MENU>>3);
    PRINT_FUNCTION("  Background:");
    print_single_colour_info(BACKGROUND_COLOUR_POS);
    set_text_y((BASE_Y_CURSOR_COLOURS_SETTINGS_MENU>>3)+(BASE_Y_CURSOR_INCREMENT_COLOURS_SETTINGS_MENU>>3));
    PRINT_FUNCTION("  Font:");
    print_single_colour_info(FONT_COLOUR_POS);
    set_text_y((BASE_Y_CURSOR_COLOURS_SETTINGS_MENU>>3)+((BASE_Y_CURSOR_INCREMENT_COLOURS_SETTINGS_MENU>>3)*2));
    PRINT_FUNCTION("  Window 1:");
    print_single_colour_info(WINDOW_COLOUR_1_POS);
    set_text_y((BASE_Y_CURSOR_COLOURS_SETTINGS_MENU>>3)+((BASE_Y_CURSOR_INCREMENT_COLOURS_SETTINGS_MENU>>3)*3));
    PRINT_FUNCTION("  Window 2:");
    print_single_colour_info(WINDOW_COLOUR_2_POS);
    set_text_y((BASE_Y_CURSOR_COLOURS_SETTINGS_MENU>>3)+((BASE_Y_CURSOR_INCREMENT_COLOURS_SETTINGS_MENU>>3)*4));
    PRINT_FUNCTION("  Cursor:");
    print_single_colour_info(SPRITE_COLOUR_POS);
    
    init_colour_window();
    clear_colour_window();
    
    set_text_y(COLOURS_WINDOW_Y+(COLOURS_WINDOW_Y_SIZE>>1));
    set_text_x(COLOURS_WINDOW_X+(COLOURS_WINDOW_X_SIZE>>1)-((11+1)>>1));
    PRINT_FUNCTION("Test Window");

    print_bottom_info();
    PRINT_FUNCTION(" - UP/DOWN: Change");
}

void print_trade_options(u8 cursor_x_pos, u8 own_menu, u8 evolution){
    reset_screen(BLANK_FILL);
    init_trade_options_window();
    clear_trade_options_window();
    set_text_y(TRADE_OPTIONS_WINDOW_Y);
    set_text_x(TRADE_OPTIONS_WINDOW_X);
    PRINT_FUNCTION("  Summary");
    set_text_x(TRADE_OPTIONS_WINDOW_X + (TRADE_OPTIONS_WINDOW_X_SIZE>>1));
    if(own_menu && evolution)
        PRINT_FUNCTION("  Evolve");
    else if(own_menu)
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
    set_text_y(EVOLUTION_ANIMATION_WINDOW_Y+JAPANESE_Y_AUTO_INCREASE);
    set_text_x(EVOLUTION_ANIMATION_WINDOW_X);
    if(!is_second_run) {
        PRINT_FUNCTION("Huh?! \x05\n\n", mon->pre_evo_string, mon->pre_evo_string_length, IS_SYS_LANGUAGE_JAPANESE);
        set_text_x(EVOLUTION_ANIMATION_WINDOW_X);
        PRINT_FUNCTION("is evolving?!");
    }
    else {
        PRINT_FUNCTION("\x05 evolved\n\n", mon->pre_evo_string, mon->pre_evo_string_length, IS_SYS_LANGUAGE_JAPANESE);
        set_text_x(EVOLUTION_ANIMATION_WINDOW_X);
        PRINT_FUNCTION("into \x05!", get_pokemon_name_raw(mon), get_pokemon_name_raw_language_limit(mon), IS_SYS_LANGUAGE_JAPANESE);
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
    set_text_y(TRADE_ANIMATION_SEND_WINDOW_Y+JAPANESE_Y_AUTO_INCREASE);
    set_text_x(TRADE_ANIMATION_SEND_WINDOW_X);
    if(!is_egg) {
        PRINT_FUNCTION("Sending \x05 away...\n\n", get_pokemon_name_raw(mon), get_pokemon_name_raw_language_limit(mon), IS_SYS_LANGUAGE_JAPANESE);
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
    set_text_y(TRADE_ANIMATION_RECV_WINDOW_Y+JAPANESE_Y_AUTO_INCREASE);
    set_text_x(TRADE_ANIMATION_RECV_WINDOW_X);
    if(!is_egg) {
        PRINT_FUNCTION("Received \x05.\n\n", get_pokemon_name_raw(mon), get_pokemon_name_raw_language_limit(mon), IS_SYS_LANGUAGE_JAPANESE);
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
    if((state == START_TRADE_NO_UPDATE) && (raw_state != START_TRADE_PAR) && (raw_state != START_TRADE_DON))
        return;
    //if(state == START_TRADE_NO_UPDATE)
    //    return;
    
    default_reset_screen();
    PRINT_FUNCTION("\nState: \x01\n", trade_start_state_strings[raw_state]);
    if((raw_state != START_TRADE_PAR) && (raw_state != START_TRADE_DON))
        print_bottom_info();
    else {
        for(int i = 0; i < get_number_of_buffers(); i++) {
            if(raw_state != START_TRADE_DON)
                PRINT_FUNCTION("\nSection \x03: \x09/\x03\n", i+1, get_transferred(i), 3, get_buffer_size(i));
            else
                PRINT_FUNCTION("\nSection \x03: \x09/\x03\n", i+1, get_buffer_size(i), 3, get_buffer_size(i));
        }
        if(!get_transferred(0))
            print_bottom_info();
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
    PRINT_FUNCTION("\n\nOld ");
    PRINT_FUNCTION("Hidden Power: \x01 \x03", get_hidden_power_type_name_gen3(&mon->misc), get_hidden_power_power_gen3(&mon->misc));
    PRINT_FUNCTION("\nNew ");
    PRINT_FUNCTION("Hidden Power: \x01 \x03", get_hidden_power_type_name_gen3_pure(altered->ivs), get_hidden_power_power_gen3_pure(altered->ivs));
}

void print_set_nature(u8 load_sprites, struct gen3_mon_data_unenc* mon) {
    default_reset_screen();

    if((!mon->is_valid_gen3) || (mon->is_egg)) {
        print_bottom_info();
        return;
    }

    print_pokemon_base_data(load_sprites, mon, BASE_Y_SPRITE_NATURE_PAGE, BASE_X_SPRITE_NATURE_PAGE, 1);

    print_basic_alter_conf_data(mon, &mon->alter_nature);

    u8 ability_text_y = 15;
    u8 nature_text_y = ability_text_y + 2;

    if(mon->growth.species == DUNSPARCE_SPECIES) {
        ability_text_y -= 1;
        set_text_y(ability_text_y + 1);
        PRINT_FUNCTION("Dudunsparce Segments: \x03", get_dudunsparce_segments(mon->alter_nature.pid));
    }

    set_text_y(ability_text_y);
    PRINT_FUNCTION("Ability: \x01 (\x03)", get_ability_name_raw_alternative(mon, &mon->alter_nature), get_ability_num_gen_4_5(mon->alter_nature.pid));

    set_text_y(nature_text_y);
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

    print_pokemon_base_data(1, mon, BASE_Y_SPRITE_IV_FIX_PAGE, BASE_X_SPRITE_IV_FIX_PAGE, 1);
    u16 species = mon->growth.species;

    print_basic_alter_conf_data(mon, &mon->fixed_ivs);

    if((species == LATIAS_SPECIES) || (species == LATIOS_SPECIES))
        PRINT_FUNCTION("\nMet in: \x01", get_met_location_name_gen3_raw(mon));
    else
        PRINT_FUNCTION("\nOld Met in: \x01", get_met_location_name_gen3_raw(mon));
    if((species == RAIKOU_SPECIES) || (species == ENTEI_SPECIES) || (species == SUICUNE_SPECIES))
        PRINT_FUNCTION("\nNew Met in: Deep Colosseum");

    if(get_met_level_gen3_raw(mon) != (mon->fixed_ivs.origins_info&0x7F))
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

    u16 move = mon->learnable_moves->moves[index];

    print_pokemon_base_data(1, mon, BASE_Y_SPRITE_LEARN_MOVE_PAGE, BASE_X_SPRITE_LEARN_MOVE_PAGE, 1);

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
    PRINT_FUNCTION("Ability: \x01 (\x03)", get_ability_name_raw(mon), get_ability_num_gen_4_5(mon->src->pid));
    PRINT_FUNCTION("\nHidden Power: \x01 \x03\n", get_hidden_power_type_name_gen3(&mon->misc), get_hidden_power_power_gen3(&mon->misc));
    base_y = get_text_y();
    u8 offset_text_y = 0;
    if(base_y < (((BASE_Y_CURSOR_LEARN_MOVE_MENU)>>3)-(3)))
        offset_text_y = 1;
    set_text_y(((BASE_Y_CURSOR_LEARN_MOVE_MENU)>>3)-(3 + offset_text_y));
    PRINT_FUNCTION("Which move will be forgotten?");
    set_text_y(((BASE_Y_CURSOR_LEARN_MOVE_MENU)>>3)-2);
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

void print_evolution_window(struct gen3_mon_data_unenc* mon) {
    u8 useless = 0;
    reset_screen(BLANK_FILL);

    if((!mon->is_valid_gen3) || (mon->is_egg))
        return;

    u16 num_species = can_own_menu_evolve(mon);
    if(!num_species)
        return;

    init_evolution_window(num_species);
    clear_evolution_window(num_species);

    set_text_y(EVOLUTION_WINDOW_Y-(EVOLUTION_WINDOW_Y_SIZE_INCREMENT*num_species));
    set_text_x(EVOLUTION_WINDOW_X);
    PRINT_FUNCTION("Evolve into:\n\n");
    for(size_t i = 0; i < num_species; i++) {
        u16 new_species = get_own_menu_evolution_species(mon, i, &useless);
        set_text_x(EVOLUTION_WINDOW_X);
        if(is_species_valid(new_species))
            PRINT_FUNCTION("  \x05\n\n", get_pokemon_name_pure(new_species, 0, SYS_LANGUAGE), SYS_LANGUAGE_LIMIT, IS_SYS_LANGUAGE_JAPANESE);
    }
}

void print_evolution_menu(struct gen3_mon_data_unenc* mon, u16 index, u8 screen, u8 update) {
    if(!update)
        return;

    u8 old_screen = get_screen_num();
    set_screen(screen);
    default_reset_screen();

    if((!mon->is_valid_gen3) || (mon->is_egg))
        return;

    u8 needs_levelup = 0;
    u16 old_species = mon->growth.species;
    u16 new_species = get_own_menu_evolution_species(mon, index, &needs_levelup);
    mon->src->level = to_valid_level_gen3(mon->src);
    u8 old_level = mon->src->level;
    if((!new_species) || (!is_species_valid(new_species)))
        return;
    
    if(needs_levelup)
        mon->src->level += 1;
    if(mon->src->level > MAX_LEVEL_GEN3)
        mon->src->level = MAX_LEVEL_GEN3;
    mon->growth.species = new_species;

    print_pokemon_base_data(1, mon, BASE_Y_SPRITE_EVOLUTION_PAGE, BASE_X_SPRITE_EVOLUTION_PAGE, 0);

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
    PRINT_FUNCTION("Ability: \x01 (\x03)", get_ability_name_raw(mon), get_ability_num_gen_4_5(mon->src->pid));
    PRINT_FUNCTION("\nHidden Power: \x01 \x03\n", get_hidden_power_type_name_gen3(&mon->misc), get_hidden_power_power_gen3(&mon->misc));
    if(needs_levelup)
        PRINT_FUNCTION("Level: from \x03 to \x03\n", old_level, mon->src->level);
    else
        PRINT_FUNCTION("\n");
    PRINT_FUNCTION("MOVES\n\n");
    for(size_t i = 0; i < MOVES_SIZE; i++)
        PRINT_FUNCTION("\x01\n\n", get_move_name_gen3(&mon->attacks, i));

    set_text_y(Y_LIMIT-1);
    PRINT_FUNCTION("A: Confirm - B: Go Back");

    mon->growth.species = old_species;
    mon->src->level = old_level;
    set_screen(old_screen);
}

void print_pokemon_base_data(u8 load_sprites, struct gen3_mon_data_unenc* mon, u8 y, u8 x, u8 print_nickname) {
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
        if(print_nickname)
            PRINT_FUNCTION("\x05 - ", mon->src->nickname, GET_LANGUAGE_NICKNAME_LIMIT(language), GET_LANGUAGE_IS_JAPANESE(language));
        PRINT_FUNCTION("\x05 \x02\n", get_pokemon_name_raw(mon), get_pokemon_name_raw_language_limit(mon), IS_SYS_LANGUAGE_JAPANESE, get_pokemon_gender_char_raw(mon));
    
        set_text_x((x>>3) + POKEMON_SPRITE_X_TILES);
        
        if(is_shiny)
            PRINT_FUNCTION("Shiny");
    
        if(is_shiny && has_pokerus)
            PRINT_FUNCTION(" - ");
    }
    else {
        PRINT_FUNCTION("\x05\n", get_pokemon_name_raw(mon), get_pokemon_name_raw_language_limit(mon), IS_SYS_LANGUAGE_JAPANESE);
        set_text_x((x>>3) + POKEMON_SPRITE_X_TILES);
    }
    
    if(has_pokerus == HAS_POKERUS)
        PRINT_FUNCTION("Has Pok\xE9rus");
    else if(has_pokerus == HAD_POKERUS)
        PRINT_FUNCTION("Had Pok\xE9rus");
}

void print_warning_when_clock_changed() {
    default_reset_screen();
    PRINT_FUNCTION("\nAdvancing the clock will\n");
    PRINT_FUNCTION("result in the Pok\xE9rus\n");
    PRINT_FUNCTION("expiring for at least\n");
    PRINT_FUNCTION("one Pok\xE9mon!\n\n");
    PRINT_FUNCTION("Continue Anyway?");
    set_text_y(BASE_Y_CURSOR_CLOCK_WARNING>>3);
    PRINT_FUNCTION("  Yes");
    set_text_x(BASE_X_CURSOR_INCREMENT_CLOCK_WARNING>>3);
    PRINT_FUNCTION("  No");
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
    
    print_pokemon_base_data(load_sprites, mon, BASE_Y_SPRITE_INFO_PAGE, BASE_X_SPRITE_INFO_PAGE, 1);
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
        
        u8 met_level = get_met_level_gen3_raw(mon);
        if(met_level > 0) {
            PRINT_FUNCTION("\nMet in: \x01\n", get_met_location_name_gen3_raw(mon));
            PRINT_FUNCTION("\nCaught at Level \x03\n\nCaught in \x01 Ball", met_level, get_pokeball_base_name_gen3_raw(mon));
        }
        else {
            PRINT_FUNCTION("\nHatched at: \x01\n", get_met_location_name_gen3_raw(mon));
            PRINT_FUNCTION("\nHatched in \x01 Ball", get_pokeball_base_name_gen3_raw(mon));
        }
        if(mon->growth.species == DUNSPARCE_SPECIES)
            PRINT_FUNCTION("\n\n   Dudunsparce Segments: \x03", get_dudunsparce_segments(mon->src->pid));
    }
    else
        PRINT_FUNCTION("\nHatches in : \x03 Egg Cycle\x02\n\nHatches in: \x03 Steps", mon->growth.friendship + 1, mon->growth.friendship ? 's' : ' ', (mon->growth.friendship + 1) * 0x100);
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
    
    PRINT_FUNCTION("\nAbility: \x01 (\x03)\n", get_ability_name_raw(mon), get_ability_num_gen_4_5(mon->src->pid));
    
    PRINT_FUNCTION("\nExperience: \x03\n", get_proper_exp_raw(mon));
    
    if(to_valid_level_gen3(mon->src) < MAX_LEVEL_GEN3)
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

void print_load_warnings(struct game_data_t* game_data, struct game_data_priv_t* game_data_priv) {
    default_reset_screen();

    set_text_x((X_LIMIT-8)>>1);
    PRINT_FUNCTION("WARNING!\n\n");
    if(!is_in_pokemon_center(game_data_priv, game_data->game_identifier.game_main_version)){
        PRINT_FUNCTION("The player did not save\n");
        PRINT_FUNCTION("in a Pok\xE9mon Center!\n\n");
    }
    if(can_trade(game_data_priv, game_data->game_identifier.game_main_version) == PARTIAL_TRADE_POSSIBLE) {
        PRINT_FUNCTION("Normally, the game should not\n");
        PRINT_FUNCTION("be able to trade to certain\n");
        PRINT_FUNCTION("other versions!\n\n");
    }
    if(get_party_usable_num(game_data) < MIN_ACTIVE_MON_TRADING) {
        PRINT_FUNCTION("Normally, the game requires\n");
        PRINT_FUNCTION("having more active Pok\xE9mon in\n");
        PRINT_FUNCTION("the party in order to trade!\n\n");
    }
        
    set_text_y(Y_LIMIT-3);
    PRINT_FUNCTION("  Proceed at your own risk!\n\n");
    PRINT_FUNCTION("Press any button to continue!");
}

void print_main_menu(u8 update, u8 curr_gen, u8 is_jp, u8 is_master, struct game_data_t* game_data, struct game_data_priv_t* game_data_priv) {
    if(!update)
        return;
    
    u8* options = get_options_main();

    default_reset_screen();
    
    if(!get_valid_options_main()) {
        set_text_y(1);
        set_text_x(MAIN_MENU_DISTANCE_FROM_BORDER);
        if(game_data_priv->game_is_suspended)
            PRINT_FUNCTION("Error: Found Suspend data!");
        else if(!get_is_cartridge_loaded())
            PRINT_FUNCTION("Error reading the data!");
        else if(can_trade(game_data_priv, game_data->game_identifier.game_main_version) == TRADE_IMPOSSIBLE)
            PRINT_FUNCTION("Pok\xE9""dex not obtained!");
        else
            PRINT_FUNCTION("No valid Pok\xE9mon found!");
        set_text_y(9);
        set_text_x(MAIN_MENU_DISTANCE_FROM_BORDER);
        PRINT_FUNCTION("Send Multiboot");
        #if ENABLED_PRINT_INFO
        set_text_y(Y_LIMIT-7);
        set_text_x(MAIN_MENU_DISTANCE_FROM_BORDER);
        PRINT_FUNCTION("Currently Read Info");
        #endif
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
        PRINT_FUNCTION("Target: ");
        if(get_number_of_higher_ordered_options(options, curr_gen, TOTAL_GENS) > 0 && get_number_of_lower_ordered_options(options, curr_gen, TOTAL_GENS) > 0)
            PRINT_FUNCTION("<\x01>", target_strings[curr_gen-1]);
        else if(get_number_of_higher_ordered_options(options, curr_gen, TOTAL_GENS) > 0)
            PRINT_FUNCTION(" \x01>", target_strings[curr_gen-1]);
        else if(get_number_of_lower_ordered_options(options, curr_gen, TOTAL_GENS) > 0)
            PRINT_FUNCTION("<\x01", target_strings[curr_gen-1]);
        else
            PRINT_FUNCTION(" \x01", target_strings[curr_gen-1]);
        set_text_y(3);
        set_text_x(MAIN_MENU_DISTANCE_FROM_BORDER);
        if(curr_gen < 3) {
            PRINT_FUNCTION("Target Region: ");
            if(!is_jp)
                PRINT_FUNCTION(" \x01>", region_strings[0]);
            else
            PRINT_FUNCTION("<\x01", region_strings[1]);
        }
        set_text_y(5);
        set_text_x(MAIN_MENU_DISTANCE_FROM_BORDER);
        PRINT_FUNCTION("Act as: ");
        if(!is_master)
            PRINT_FUNCTION(" \x01>", actor_strings[1]);
        else
            PRINT_FUNCTION("<\x01", actor_strings[0]);
        set_text_y(7);
        set_text_x(MAIN_MENU_DISTANCE_FROM_BORDER);
        PRINT_FUNCTION("Start Trade");
        set_text_y(9);
        set_text_x(MAIN_MENU_DISTANCE_FROM_BORDER);
        PRINT_FUNCTION("Send Multiboot");
        #if (ENABLED_PRINT_INFO && PRINT_INFO_ALWAYS)
        set_text_y(Y_LIMIT-7);
        set_text_x(MAIN_MENU_DISTANCE_FROM_BORDER);
        PRINT_FUNCTION("Currently Read Info");
        #endif
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

void print_multiboot_settings(u8 is_normal, u8 update) {
    if(!update)
        return;

    default_reset_screen();
    PRINT_FUNCTION("\nCable type: ");
    if(is_normal)
        PRINT_FUNCTION(" GBC> (Fast)");
    else
        PRINT_FUNCTION("<GBA  (Slow)");
    set_text_y(Y_LIMIT-1);
    PRINT_FUNCTION("A: Start - B: Go Back");
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
    
    set_text_y(7);
    PRINT_FUNCTION("A: To the previous menu\n\n");
    PRINT_FUNCTION("The cartridge will be\n");
    PRINT_FUNCTION("read once more.");
}
