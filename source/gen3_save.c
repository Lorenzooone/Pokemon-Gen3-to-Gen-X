#include <gba.h>
#include "save.h"
#include "gen3_save.h"
#include "gen3_clock_events.h"
#include "party_handler.h"
#include "gen_converter.h"
#include "text_handler.h"
#include "timing_basic.h"
#include <stddef.h>

#include "default_gift_ribbons_bin.h"

#define FRAMES_BETWEEN_CHECKS ((1*FPS)/12)

#define MAGIC_NUMBER 0x08012025
#define NUM_SLOTS 2
#define INVALID_SLOT NUM_SLOTS
#define SAVE_SLOT_INDEX_POS 0xDFFC
#define SECTION_ID_POS 0xFF4
#define CHECKSUM_POS 0xFF6
#define MAGIC_NUMBER_POS 0xFF8
#define SAVE_NUMBER_POS 0xFFC
#define SECTION_TOTAL 14

#define SECTION_TRAINER_INFO_ID 0
#define SECTION_PARTY_INFO_ID 1
#define SECTION_SAVE_LOCATION_FLAGS_ID 0
#define SECTION_LOCATION_ID 1
#define SECTION_BASE_DEX_ID 0
#define SECTION_DEX_SEEN_1_ID 1
#define SECTION_DEX_SEEN_2_ID 4
#define SECTION_MAIL_ID 3
#define SECTION_CHALLENGE_STATUS_ID 0
#define SECTION_GIFT_RIBBON_ID 4

#define TRAINER_NAME_POS 0
#define TRAINER_GENDER_POS 8
#define SAVE_LOCATION_FLAGS_POS 9
#define TRAINER_ID_POS 10
#define LOCATION_POS 4
#define IN_POKEMON_CENTER_FLAG 2
#define DEX_BASE_POS 0x18
#define DEX_UNOWN_PID_POS (DEX_BASE_POS+4)
#define DEX_SPINDA_PID_POS (DEX_BASE_POS+8)
#define DEX_POS_OWNED (DEX_BASE_POS+0x10)
#define DEX_POS_SEEN_0 (DEX_BASE_POS+DEX_BYTES+0x10)

#define SAVED_GAME_STAT_NUM 0
#define NUM_TRADES_STAT_NUM 21

#define GAME_STAT_LIMIT 0xFFFFFF

#define RS_BATTLE_TOWER_LEVEL_POS 0x554

#define RSE_PARTY 0x234
#define FRLG_PARTY 0x34

#define VALID_MAPS 35

#define CHALLENGE_STATUS_PAUSED 2

enum SAVING_KIND {FULL_SAVE, NON_PARTY_SAVE, PARTY_ONLY_SAVE};

u16 read_section_id(int, int);
u32 read_slot_index(int);
void read_game_data_trainer_info(int, struct game_data_t*, struct game_data_priv_t*);
void handle_mail_trade(struct game_data_t*, u8, u8);
void read_party(int, struct game_data_t*, struct game_data_priv_t*);
void update_gift_ribbons(struct game_data_t*, const u8*);
u8 validate_slot(int, struct game_identity*);
u8 get_slot(struct game_identity*);
void unload_cartridge(void);
void load_cartridge(void);
u8 pre_update_save(struct game_data_t*, struct game_data_priv_t*, u8, enum SAVING_KIND);
u8 complete_save(u8, struct game_data_t*);
static u8 get_next_slot(u8);
void replace_party_entry(struct game_data_t*, struct gen3_mon_data_unenc*, u8);
void trade_reorder_party_entries(struct game_data_t*, struct gen3_mon_data_unenc*, u8);
u32 calc_checksum_save_buffer(u32*, u16);

const u16 summable_bytes[SECTION_TOTAL] = {3884, MAX_SECTION_SIZE, MAX_SECTION_SIZE, MAX_SECTION_SIZE, 3848, MAX_SECTION_SIZE, MAX_SECTION_SIZE, MAX_SECTION_SIZE, MAX_SECTION_SIZE, MAX_SECTION_SIZE, MAX_SECTION_SIZE, MAX_SECTION_SIZE, MAX_SECTION_SIZE, 2000};
const u16 summable_bytes_section0[NUM_MAIN_GAME_ID] = {0x890, 0xF24, 0xF2C};
const u16 summable_bytes_section4[NUM_MAIN_GAME_ID] = {0xC40, 0xEE8, 0xF08};
const u16 pokedex_extra_pos_1[NUM_MAIN_GAME_ID] = {0x938, 0x5F8, 0x988};
const u16 pokedex_extra_pos_2[NUM_MAIN_GAME_ID] = {0xC0C, 0xB98, 0xCA4};
const u16 nat_dex_magic_pos[NUM_MAIN_GAME_ID] = {DEX_BASE_POS+2, DEX_BASE_POS+3, DEX_BASE_POS+2};
const u8 nat_dex_magic_value[NUM_MAIN_GAME_ID] = {0xDA, 0xB9, 0xDA};
const u16 mail_pos[NUM_MAIN_GAME_ID] = {0xC4C, 0xDD0, 0xCE0};
const u16 ribbon_pos[NUM_MAIN_GAME_ID] = {0x290, 0x21C, 0x328};
const u16 party_pos[NUM_MAIN_GAME_ID] = {RSE_PARTY, FRLG_PARTY, RSE_PARTY};
const u16 num_saves_stat_num[NUM_MAIN_GAME_ID] = {0x62, 0x37, 0x62};
const u16 game_clear_flag_num[NUM_MAIN_GAME_ID] = {4, 0x2C, 4};
const u16 nat_dex_flag_num[NUM_MAIN_GAME_ID] = {0x36, 0x40, 0x36};
const u16 full_link_flag_num[NUM_MAIN_GAME_ID] = {0, 0x44, 0x1F};
const u16 dex_obtained_flag_num[NUM_MAIN_GAME_ID] = {0x1, 0x29, 1};
const u16 nat_dex_var_num[NUM_MAIN_GAME_ID] = {0x46, 0x4E, 0x46};
const u16 nat_dex_var_value[NUM_MAIN_GAME_ID] = {0x302, 0x6258, 0x302};
const u16 sys_flags_pos[NUM_MAIN_GAME_ID] = {0x3A0, 0x60, 0x3FC};
const u16 vars_pos[NUM_MAIN_GAME_ID] = {0x3C0, 0x80, 0x41C};
const u8 has_stat_enc_key[NUM_MAIN_GAME_ID] = {0, 1, 1};
const u16 stat_enc_key_pos[NUM_MAIN_GAME_ID] = {0, 0xF20, 0xAC};
const u16 game_stats_pos[NUM_MAIN_GAME_ID] = {0x5C0, 0x280, 0x61C};
const u16 challenge_status_pos[NUM_MAIN_GAME_ID] = {0x556, 0xAD, 0xCA8};
const u16 rs_valid_maps[VALID_MAPS] = {0x0202, 0x0203, 0x0301, 0x0302, 0x0405, 0x0406, 0x0503, 0x0504, 0x0603, 0x0604, 0x0700, 0x0701, 0x0804, 0x0805, 0x090a, 0x090b, 0x0a05, 0x0a06, 0x0b05, 0x0b06, 0x0c02, 0x0c03, 0x0d06, 0x0d07, 0x0e03, 0x0e04, 0x0f02, 0x0f03, 0x100c, 0x100d, 0x100a, 0x1918, 0x1919, 0x191a, 0x191b};
// 0x1A05 is the Battle Tower's lobby. I think it's fine...?
// Since you have access to a PC and all, so...
// But Emerald does not count it in...

struct game_data_t* own_game_data_ptr;
u8 in_use_slot;
u8 is_cartridge_loaded;
u16 loaded_checksum[SECTION_TOTAL];
u8 currently_checking_section;
u8 time_since_last_check;

struct game_data_t* get_own_game_data() {
    return own_game_data_ptr;
}

void init_game_data(struct game_data_t* game_data) {
    init_game_identifier(&game_data->game_identifier);

    game_data->party_3.total = 0;

    for(size_t i = 0; i < (OT_NAME_GEN3_MAX_SIZE+1); i++)
        game_data->trainer_name[i] = GEN3_EOL;

    for(gen3_party_total_t i = 0; i < PARTY_SIZE; i++) {
        for(size_t j = 0; j < sizeof(struct mail_gen3); j++)
            ((u8*)(&game_data->mails_3[i]))[j] = 0;
        for(size_t j = 0; j < sizeof(struct gen3_mon_data_unenc); j++)
            ((u8*)(&game_data->party_3_undec[i]))[j] = 0;
    }

    for(size_t i = 0; i < sizeof(struct gen3_party); i++)
        ((u8*)(&game_data->party_3))[i] = 0;

    for(gen3_party_total_t i = 0; i < PARTY_SIZE; i++)
        game_data->party_3_undec[i].is_valid_gen3 = 0;
}

enum TRADE_POSSIBILITY can_trade(struct game_data_priv_t* game_data_priv, u8 game_id) {
    if(!game_data_priv->dex_obtained_flag)
        return TRADE_IMPOSSIBLE;
    if((game_data_priv->nat_dex_var == nat_dex_var_value[game_id]) && game_data_priv->nat_dex_flag && (game_data_priv->nat_dex_magic == nat_dex_magic_value[game_id]) && ((game_id == RS_MAIN_GAME_CODE) || (game_data_priv->game_cleared_flag && game_data_priv->full_link_flag)))
        return FULL_TRADE_POSSIBLE;
    return PARTIAL_TRADE_POSSIBLE;
}

u8 is_in_pokemon_center(struct game_data_priv_t* game_data_priv, u8 game_id) {
    if(game_id != RS_MAIN_GAME_CODE)
        return game_data_priv->save_warp_flags & IN_POKEMON_CENTER_FLAG;
    
    for(size_t i = 0; i < VALID_MAPS; i++)
        if(rs_valid_maps[i] == game_data_priv->curr_map)
            return 1;
    return 0;
}

u16 read_section_id(int slot, int section_pos) {
    if(slot)
        slot = 1;
    if(section_pos < 0)
        section_pos = 0;
    if(section_pos >= SECTION_TOTAL)
        section_pos = SECTION_TOTAL - 1;
    
    return read_short_save((slot * SAVE_SLOT_SIZE) + (section_pos * SECTION_SIZE) + SECTION_ID_POS);
}

u32 read_slot_index(int slot) {
    if(slot)
        slot = 1;
    
    return read_int_save((slot * SAVE_SLOT_SIZE) + SAVE_SLOT_INDEX_POS);
}

void read_game_data_trainer_info(int slot, struct game_data_t* game_data, struct game_data_priv_t* game_data_priv) {
    if(slot)
        slot = 1;

    for(int i = 0; i < SECTION_TOTAL; i++)
        if(read_section_id(slot, i) == SECTION_TRAINER_INFO_ID) {
            copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + TRAINER_NAME_POS, game_data->trainer_name, OT_NAME_GEN3_MAX_SIZE+1);
            game_data->trainer_gender = read_byte_save((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + TRAINER_GENDER_POS);
            game_data->trainer_id = read_int_save((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + TRAINER_ID_POS);
            determine_game_with_save(&game_data->game_identifier, slot, i, summable_bytes[SECTION_TRAINER_INFO_ID]);
            if(has_stat_enc_key[game_data->game_identifier.game_main_version])
                game_data_priv->stat_enc_key = read_int_save((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + stat_enc_key_pos[game_data->game_identifier.game_main_version]);
            else
                game_data_priv->stat_enc_key = 0;
            break;
        }
}

void register_dex_entry(struct game_data_priv_t* game_data_priv, struct gen3_mon_data_unenc* data_src) {
    u16 dex_index = get_dex_index_raw(data_src);
    if(dex_index != NO_DEX_INDEX) {
        u8 base_index = dex_index >> 3;
        u8 rest_index = dex_index & 7;
        u8 prev_seen = game_data_priv->pokedex_seen[base_index] & (1 << rest_index);
        game_data_priv->pokedex_seen[base_index] |= (1 << rest_index);
        game_data_priv->pokedex_owned[base_index] |= (1 << rest_index);
        if((!prev_seen) && (data_src->growth.species == UNOWN_SPECIES))
            game_data_priv->seen_unown_pid = data_src->src->pid;
        if((!prev_seen) && (data_src->growth.species == SPINDA_SPECIES))
            game_data_priv->seen_spinda_pid = data_src->src->pid;
    }
}

void handle_mail_trade(struct game_data_t* game_data, u8 own_mon, u8 other_mon) {
    u8 mail_id = get_mail_id_raw(&game_data[0].party_3_undec[own_mon]);
    if((mail_id != GEN3_NO_MAIL) && (mail_id < PARTY_SIZE)) {
        clean_mail_gen3(&game_data[0].mails_3[get_mail_id_raw(&game_data[0].party_3_undec[own_mon])], game_data[0].party_3_undec[own_mon].src);
    }

    mail_id = get_mail_id_raw(&game_data[1].party_3_undec[other_mon]);
    if((mail_id != GEN3_NO_MAIL) && (mail_id < PARTY_SIZE)) {
        u8 is_mail_free[PARTY_SIZE] = {1,1,1,1,1,1};
        for(gen3_party_total_t i = 0; i < game_data[0].party_3.total; i++) {
            u8 inner_mail_id = get_mail_id_raw(&game_data[0].party_3_undec[i]);
            if((inner_mail_id != GEN3_NO_MAIL) && (inner_mail_id < PARTY_SIZE))
                is_mail_free[inner_mail_id] = 0;
        }
        u8 target = PARTY_SIZE-1;
        for(gen3_party_total_t i = 0; i < PARTY_SIZE; i++)
            if(is_mail_free[i]) {
                target = i;
                break;
            }
        u8* dst = (u8*)&game_data[0].mails_3[target];
        u8* src = (u8*)&game_data[1].mails_3[mail_id];
        for(size_t i = 0; i < sizeof(struct mail_gen3); i++)
            dst[i] = src[i];
        game_data[1].party_3_undec[other_mon].src->mail_id = target;
    }
    else 
        game_data[1].party_3_undec[other_mon].src->mail_id = GEN3_NO_MAIL;
}

void update_gift_ribbons(struct game_data_t* game_data, const u8* new_gift_ribbons) {
    for(int i = 0; i < GIFT_RIBBONS; i++)
        if(!game_data->giftRibbons[i])
            game_data->giftRibbons[i] = new_gift_ribbons[i];
}

void set_default_gift_ribbons(struct game_data_t* game_data) {
    update_gift_ribbons(game_data, default_gift_ribbons_bin);
}

u8 get_new_party_entry_index(struct game_data_t* game_data) {
    gen3_party_total_t party_size = game_data->party_3.total;
    if(party_size > PARTY_SIZE)
        party_size = PARTY_SIZE;
    return party_size-1;
}

void replace_party_entry(struct game_data_t* game_data, struct gen3_mon_data_unenc* src_mon, u8 index_dst) {
    gen3_party_total_t party_size = game_data->party_3.total;
    if(party_size > PARTY_SIZE)
        party_size = PARTY_SIZE;
    if(index_dst >= party_size)
        index_dst = party_size-1;
    u8* dst = (u8*)&game_data->party_3.mons[index_dst];
    u8* src = (u8*)src_mon->src;
    for(size_t i = 0; i < sizeof(struct gen3_mon); i++)
        dst[i] = src[i];
    dst = (u8*)&game_data->party_3_undec[index_dst];
    src = (u8*)src_mon;
    for(size_t i = 0; i < sizeof(struct gen3_mon_data_unenc); i++)
        dst[i] = src[i];
    game_data->party_3_undec[index_dst].src = &game_data->party_3.mons[index_dst];
}

void trade_reorder_party_entries(struct game_data_t* game_data, struct gen3_mon_data_unenc* new_mon, u8 old_index) {
    gen3_party_total_t party_size = game_data->party_3.total;
    if(party_size > PARTY_SIZE)
        party_size = PARTY_SIZE;
    if(old_index >= party_size)
        old_index = party_size-1;
    for(gen3_party_total_t i = old_index + 1; i < party_size; i++)
        replace_party_entry(game_data, &game_data->party_3_undec[i], i-1);
    replace_party_entry(game_data, new_mon, party_size-1);
}

u8 trade_mons(struct game_data_t* game_data, struct game_data_priv_t* game_data_priv, u8 own_mon, u8 other_mon, u8 curr_gen) {
    handle_mail_trade(game_data, own_mon, other_mon);

    trade_reorder_party_entries(&game_data[0], &game_data[1].party_3_undec[other_mon], own_mon);
    own_mon = get_new_party_entry_index(&game_data[0]);
    update_gift_ribbons(&game_data[0], game_data[1].giftRibbons);
    register_dex_entry(game_data_priv, &game_data[0].party_3_undec[own_mon]);
    u8 ret_val = trade_evolve(&game_data[0].party_3.mons[own_mon], &game_data[0].party_3_undec[own_mon], curr_gen);
    if(ret_val)
        register_dex_entry(game_data_priv, &game_data[0].party_3_undec[own_mon]);
    return ret_val;
}

u8 get_party_usable_num(struct game_data_t* game_data) {
    if(!get_is_cartridge_loaded())
        return 0;
    u8 found_size = 0;
    gen3_party_total_t party_size = game_data->party_3.total;
    if(party_size > PARTY_SIZE)
        party_size = PARTY_SIZE;
    for(gen3_party_total_t i = 0; i < party_size; i++)
        if((game_data[0].party_3_undec[i].is_valid_gen3) && (!game_data[0].party_3_undec[i].is_egg))
            found_size += 1;
    return found_size;
}

u8 is_invalid_offer(struct game_data_t* game_data, u8 own_mon, u8 other_mon, u8 curr_gen, u16 received_species) {
    // Prevent OOB checks
    if(game_data[1].party_3.total > PARTY_SIZE)
        game_data[1].party_3.total = PARTY_SIZE;
    if(other_mon >= game_data[1].party_3.total)
        return 1 + 0;

    // Check for validity
    if(!game_data[1].party_3_undec[other_mon].is_valid_gen3)
        return 1 + 0;

    // For gen 3, check the correct species from the other actor
    if((curr_gen == 3) && (game_data[1].party_3_undec[other_mon].growth.species != received_species))
        return 1 + 0;

    u8 found_size = get_party_usable_num(&game_data[0]);

    // Check that the receiving party has at least one active mon
    if(!found_size)
        return 1 + 1;

    // Check that the receiving party would have at least one active mon
    // after the trade
    u8 target_value = game_data[1].party_3_undec[other_mon].is_egg ? 1 : 0;
    u8 subtract = game_data[0].party_3_undec[own_mon].is_egg ? 0 : 1;
    if((found_size-subtract) < target_value)
        return 1 + 1;

    return 0;
}

u8 get_sys_flag_save(u8 slot, int section, u8 game_id, u16 flag_num) {
    return (read_byte_save((slot * SAVE_SLOT_SIZE) + (section * SECTION_SIZE) + sys_flags_pos[game_id] + (flag_num>>3)) & (1<<(flag_num & 7))) == 0 ? 0 : 1;
}

u8 get_sys_flag_byte_save(u8 slot, int section, u8 game_id, u16 flag_num) {
    return read_byte_save((slot * SAVE_SLOT_SIZE) + (section * SECTION_SIZE) + sys_flags_pos[game_id] + (flag_num>>3));
}

void set_sys_flag_save(u8* buffer_8, u8 game_id, u16 flag_num, u8 value) {
    u8 val_to_set = 1;
    if(!value)
        val_to_set = 0;
    buffer_8[sys_flags_pos[game_id] + (flag_num>>3)] &= ~(1<<(flag_num & 7));
    buffer_8[sys_flags_pos[game_id] + (flag_num>>3)] |= val_to_set<<(flag_num & 7);
}

void set_sys_flag_byte_save(u8* buffer_8, u8 game_id, u16 flag_num, u8 value) {
    buffer_8[sys_flags_pos[game_id] + (flag_num>>3)] = value;
}

u16 get_var_save(u8 slot, int section, u8 game_id, u16 var_num) {
    return read_short_save((slot * SAVE_SLOT_SIZE) + (section * SECTION_SIZE) + vars_pos[game_id] + (var_num<<1));
}

void set_var_save(u8* buffer_8, u8 game_id, u16 var_num, u16 value) {
    for(size_t j = 0; j < sizeof(u16); j++)
        buffer_8[vars_pos[game_id] + (var_num<<1) + j] = (value >> (8*j)) & 0xFF;
}

u32 get_stat_save(u8 slot, int section, u8 game_id, u16 stat_num) {
    return read_int_save((slot * SAVE_SLOT_SIZE) + (section * SECTION_SIZE) + game_stats_pos[game_id] + (stat_num<<2));
}

void set_stat_save(u8* buffer_8, u8 game_id, u16 stat_num, u32 value) {
    for(size_t j = 0; j < sizeof(u32); j++)
        buffer_8[game_stats_pos[game_id] + (stat_num<<2) + j] = (value >> (8*j)) & 0xFF;
}

void increase_game_stat(u32* game_stat, u32 increase) {
    if((*game_stat) < GAME_STAT_LIMIT)
        *game_stat += increase;
    if((*game_stat) > GAME_STAT_LIMIT)
        *game_stat = GAME_STAT_LIMIT;
}

void process_party_data(struct game_data_t* game_data, struct gen2_party* party_2, struct gen1_party* party_1) {
    if(game_data->party_3.total > PARTY_SIZE)
        game_data->party_3.total = PARTY_SIZE;
    u8 curr_slot = 0;
    u8 found = 0;
    for(gen3_party_total_t i = 0; i < game_data->party_3.total; i++) {
        process_gen3_data(&game_data->party_3.mons[i], &game_data->party_3_undec[i], game_data->game_identifier.game_main_version, game_data->game_identifier.game_sub_version);
        if(game_data->party_3_undec[i].is_valid_gen3)
            found = 1;
    }
    for(gen3_party_total_t i = game_data->party_3.total; i < PARTY_SIZE; i++) {
        game_data->party_3_undec[i].src = &game_data->party_3.mons[i];
        game_data->party_3_undec[i].is_valid_gen3 = 0;
    }
    if (!found)
        game_data->party_3.total = 0;
    for(gen3_party_total_t i = 0; i < game_data->party_3.total; i++)
        if(gen3_to_gen2(&party_2->mons[curr_slot], &game_data->party_3_undec[i], game_data->trainer_id)) {
            curr_slot++;
            game_data->party_3_undec[i].is_valid_gen2 = 1;
        }
        else
            game_data->party_3_undec[i].is_valid_gen2 = 0;
    party_2->total = curr_slot;
    curr_slot = 0;
    for(gen3_party_total_t i = 0; i < game_data->party_3.total; i++)
        if(gen3_to_gen1(&party_1->mons[curr_slot], &game_data->party_3_undec[i], game_data->trainer_id)) {
            curr_slot++;
            game_data->party_3_undec[i].is_valid_gen1 = 1;
        }
        else
            game_data->party_3_undec[i].is_valid_gen1 = 0;
    party_1->total = curr_slot;
}

void alter_party_data_language(struct game_data_t* game_data, struct gen2_party* party_2, struct gen1_party* party_1) {
    if(game_data->party_3.total > PARTY_SIZE)
        game_data->party_3.total = PARTY_SIZE;
    u8 curr_slot_gen2 = 0;
    u8 curr_slot_gen1 = 0;
    // Altering the order is strictly prohibited!!!
    // Saved pointers/indexes would prevent this limitation, but they'd be "extra"
    for(gen3_party_total_t i = 0; i < game_data->party_3.total; i++) {
        if(game_data->party_3_undec[i].is_valid_gen3 && game_data->party_3_undec[i].is_valid_gen2 && (curr_slot_gen2 < party_2->total))
            reconvert_strings_of_gen3_to_gen2(&game_data->party_3_undec[i], &party_2->mons[curr_slot_gen2++]);
        if(game_data->party_3_undec[i].is_valid_gen3 && game_data->party_3_undec[i].is_valid_gen1 && (curr_slot_gen1 < party_1->total))
            reconvert_strings_of_gen3_to_gen1(&game_data->party_3_undec[i], &party_1->mons[curr_slot_gen1++]);
    }
}

void alter_game_data_language(struct game_data_t* game_data, struct game_data_priv_t* game_data_priv) {
    if(game_data->game_identifier.language_is_sys) {
        game_data->game_identifier.language = SYS_LANGUAGE;
        text_gen3_copy(game_data_priv->trainer_name_raw, game_data->trainer_name, OT_NAME_GEN3_MAX_SIZE+1, OT_NAME_GEN3_MAX_SIZE+1);
        sanitize_ot_name(game_data->trainer_name, OT_NAME_GEN3_MAX_SIZE+1, game_data->game_identifier.language, 0);
    }
}

void alter_game_data_version(struct game_data_t* game_data) {
    if(game_data->party_3.total > PARTY_SIZE)
        game_data->party_3.total = PARTY_SIZE;
    for(gen3_party_total_t i = 0; i < game_data->party_3.total; i++)
        set_deoxys_form(&game_data->party_3_undec[i], game_data->game_identifier.game_main_version, game_data->game_identifier.game_sub_version);
}

u8 give_pokerus_to_party(struct game_data_t* game_data) {
    if(game_data->party_3.total > PARTY_SIZE)
        game_data->party_3.total = PARTY_SIZE;
    u8 found = 0;
    for(gen3_party_total_t i = 0; i < game_data->party_3.total; i++)
        found |= give_pokerus_gen3(&game_data->party_3_undec[i]);
    return found;
}

void read_party(int slot, struct game_data_t* game_data, struct game_data_priv_t* game_data_priv) {
    if(slot)
        slot = 1;
    
    read_game_data_trainer_info(slot, game_data, game_data_priv);
    text_gen3_copy(game_data->trainer_name, game_data_priv->trainer_name_raw, OT_NAME_GEN3_MAX_SIZE+1, OT_NAME_GEN3_MAX_SIZE+1);
    if(game_data->game_identifier.language == UNDETERMINED) {
        if(is_trainer_name_japanese(game_data->trainer_name))
            game_data->game_identifier.language = JAPANESE_LANGUAGE;
        else {
            game_data->game_identifier.language = SYS_LANGUAGE;
            game_data->game_identifier.language_is_sys = 1;
        }
    }
    sanitize_ot_name(game_data->trainer_name, OT_NAME_GEN3_MAX_SIZE+1, game_data->game_identifier.language, 0);

    u8 game_id = game_data->game_identifier.game_main_version;

    for(int i = 0; i < SECTION_TOTAL; i++) {
        u16 section_id = read_section_id(slot, i);

        if(section_id == SECTION_PARTY_INFO_ID)
            copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + party_pos[game_id], (u8*)(&game_data->party_3), sizeof(struct gen3_party));

        if(section_id == SECTION_MAIL_ID)
            copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + mail_pos[game_id], (u8*)game_data->mails_3, sizeof(game_data->mails_3));

        if(section_id == SECTION_GIFT_RIBBON_ID)
            copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + ribbon_pos[game_id], (u8*)game_data->giftRibbons, sizeof(game_data->giftRibbons));

        if(section_id == SECTION_SYS_FLAGS_ID) {
            game_data_priv->game_cleared_flag = get_sys_flag_save(slot, i, game_id, game_clear_flag_num[game_id]);
            game_data_priv->nat_dex_flag = get_sys_flag_save(slot, i, game_id, nat_dex_flag_num[game_id]);
            if(game_id == RS_MAIN_GAME_CODE) {
                game_data_priv->full_link_flag = 1;
                game_data_priv->nat_dex_flag = 1;
            }
            else
                game_data_priv->full_link_flag = get_sys_flag_save(slot, i, game_id, full_link_flag_num[game_id]);
            game_data_priv->dex_obtained_flag = get_sys_flag_save(slot, i, game_id, dex_obtained_flag_num[game_id]);
        }

        if(section_id == SECTION_VARS_ID) {
            game_data_priv->nat_dex_var = get_var_save(slot, i, game_id, nat_dex_var_num[game_id]);
            if(game_id == RS_MAIN_GAME_CODE)
                game_data_priv->nat_dex_var = nat_dex_var_value[game_id];
        }

        if(section_id == SECTION_GAME_STATS_ID) {
            game_data_priv->num_saves_stat = get_stat_save(slot, i, game_id, SAVED_GAME_STAT_NUM) ^ game_data_priv->stat_enc_key;
            game_data_priv->num_trades_stat = get_stat_save(slot, i, game_id, NUM_TRADES_STAT_NUM) ^ game_data_priv->stat_enc_key;
        }

        if(section_id == SECTION_SAVE_LOCATION_FLAGS_ID)
            game_data_priv->save_warp_flags = read_byte_save((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + SAVE_LOCATION_FLAGS_POS);

        if(section_id == SECTION_CHALLENGE_STATUS_ID) {
            u8 tower_level = 0;
            if(game_id == RS_MAIN_GAME_CODE)
                tower_level = read_byte_save((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + RS_BATTLE_TOWER_LEVEL_POS) & 1;
            if((game_id == E_MAIN_GAME_CODE) || (game_id == RS_MAIN_GAME_CODE))
                game_data_priv->game_is_suspended = read_byte_save((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + challenge_status_pos[game_id] + tower_level) == CHALLENGE_STATUS_PAUSED;
            else
                game_data_priv->game_is_suspended = 0;
        }

        if(section_id == SECTION_LOCATION_ID)
            game_data_priv->curr_map = read_short_save((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + LOCATION_POS);

        if(section_id == SECTION_BASE_DEX_ID) {
            game_data_priv->nat_dex_magic = read_byte_save((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + nat_dex_magic_pos[game_id]);
            if(game_id == RS_MAIN_GAME_CODE)
                game_data_priv->nat_dex_magic = nat_dex_magic_value[game_id];
            game_data_priv->seen_unown_pid = read_int_save((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + DEX_UNOWN_PID_POS);
            game_data_priv->seen_spinda_pid = read_int_save((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + DEX_SPINDA_PID_POS);
            copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + DEX_POS_OWNED, game_data_priv->pokedex_owned, sizeof(game_data_priv->pokedex_owned));
            copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + DEX_POS_SEEN_0, game_data_priv->pokedex_seen, sizeof(game_data_priv->pokedex_seen));
        }

        load_time_data(&game_data_priv->clock_events, section_id, slot, i, game_id, has_rtc_events(&game_data->game_identifier), game_data_priv->stat_enc_key);
    }
    process_party_data(game_data, &game_data_priv->party_2, &game_data_priv->party_1);
}

u32 calc_checksum_save_buffer(u32* buffer, u16 sum_size) {
    u32 checksum = 0;
    for(int j = 0; j < sum_size; j++)
        checksum += buffer[j];
    return ((checksum & 0xFFFF) + (checksum >> 16)) & 0xFFFF;
}

u8 validate_slot(int slot, struct game_identity* game_identifier) {
    if(slot)
        slot = 1;

    u8 game_id = game_identifier->game_main_version;
    u8 all_valid_game_ids = (1<<NUM_MAIN_GAME_ID)-1;
    u8 valid_game_ids = all_valid_game_ids;
    u8 section_trainer_info_id_pos = 0;
    
    u32 valid_sections = 0;
    u32 buffer[SECTION_SIZE>>2];
    u16* buffer_16 = (u16*)buffer;
    u32 current_save_index = read_slot_index(slot);
    
    for(int i = 0; i < SECTION_TOTAL; i++)
    {
        copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE), (u8*)buffer, SECTION_SIZE);
        if(buffer[MAGIC_NUMBER_POS>>2] != MAGIC_NUMBER)
            return 0;
        if(buffer[SAVE_NUMBER_POS>>2] != current_save_index)
            return 0;
        u16 section_id = buffer_16[SECTION_ID_POS>>1];
        if(section_id >= SECTION_TOTAL)
            return 0;

        if(section_id == SECTION_TRAINER_INFO_ID)
            section_trainer_info_id_pos = i;

        u16 prepared_checksum = buffer_16[CHECKSUM_POS>>1];

        u16 sum_size = summable_bytes[section_id] >> 2;
        if((game_id != UNDETERMINED) && (section_id == 0))
            sum_size = summable_bytes_section0[game_id] >> 2;
        if((game_id != UNDETERMINED) && (section_id == 4))
            sum_size = summable_bytes_section4[game_id] >> 2;

        if(calc_checksum_save_buffer(buffer, sum_size) != prepared_checksum) {
            if(game_id == UNDETERMINED) {
                if(section_id == 0) {
                    for(int j = 0; j < NUM_MAIN_GAME_ID; j++)
                        if(calc_checksum_save_buffer(buffer, summable_bytes_section0[j] >> 2) != prepared_checksum)
                            valid_game_ids &= ~(1<<j);
                }
                else if(section_id == 4) {
                    for(int j = 0; j < NUM_MAIN_GAME_ID; j++)
                        if(calc_checksum_save_buffer(buffer, summable_bytes_section4[j] >> 2) != prepared_checksum)
                            valid_game_ids &= ~(1<<j);   
                }
                else
                    valid_game_ids = 0;
                if(!valid_game_ids)
                    return 0;
            }
            else
                return 0;
        }
        valid_sections |= (1 << section_id);
    }
    
    if(valid_sections != (1 << (SECTION_TOTAL))-1)
        return 0;
    
    if((game_id == UNDETERMINED) && (valid_game_ids != all_valid_game_ids)) {
        u8 possible_game_ids = determine_possible_main_game_for_slot(slot, section_trainer_info_id_pos, summable_bytes[SECTION_TRAINER_INFO_ID]);
        if(!(possible_game_ids & valid_game_ids))
            return 0;
    }

    return 1;
}

ALWAYS_INLINE u8 get_next_slot(u8 base_slot) {
    return (base_slot + 1) % NUM_SLOTS;
}

u8 pre_update_save(struct game_data_t* game_data, struct game_data_priv_t* game_data_priv, u8 base_slot, enum SAVING_KIND saving_kind) {
    u8 target_slot = get_next_slot(base_slot);
    u32 target_slot_save_index = read_slot_index(base_slot) - 1;

    u32 buffer[SECTION_SIZE>>2];
    u16* buffer_16 = (u16*)buffer;
    u8* buffer_8 = (u8*)buffer;
    u8 target_section = 1;
    u8 game_id = game_data->game_identifier.game_main_version;
    
    if(game_id == UNDETERMINED)
        return 0;
    
    for(int i = 0; i < SECTION_TOTAL; i++)
    {
        if(target_section >= SECTION_TOTAL)
            target_section -= SECTION_TOTAL;
        
        u16 section_id = read_section_id(base_slot, i);
        
        if((saving_kind == FULL_SAVE) || ((saving_kind == NON_PARTY_SAVE) && (section_id != SECTION_PARTY_INFO_ID)) || ((saving_kind == PARTY_ONLY_SAVE) && (section_id == SECTION_PARTY_INFO_ID))) {
            copy_save_to_ram((base_slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE), buffer_8, SECTION_SIZE);

            // Don't use this until everything is in order, so the old save is safe
            buffer[SAVE_NUMBER_POS>>2] = target_slot_save_index;

            if(section_id == SECTION_PARTY_INFO_ID)
                for(size_t j = 0; j < sizeof(struct gen3_party); j++)
                    buffer_8[party_pos[game_id]+j] = ((u8*)(&game_data->party_3))[j];

            if(section_id == SECTION_MAIL_ID)
                for(size_t j = 0; j < sizeof(game_data->mails_3); j++)
                    buffer_8[mail_pos[game_id]+j] = ((u8*)game_data->mails_3)[j];

            if(section_id == SECTION_GIFT_RIBBON_ID)
                for(size_t j = 0; j < sizeof(game_data->giftRibbons); j++)
                    buffer_8[ribbon_pos[game_id]+j] = ((u8*)game_data->giftRibbons)[j];

            if(section_id == SECTION_BASE_DEX_ID) {
                buffer_8[nat_dex_magic_pos[game_id]] = game_data_priv->nat_dex_magic;
                for(size_t j = 0; j < sizeof(game_data_priv->seen_unown_pid); j++)
                    buffer_8[DEX_UNOWN_PID_POS+j] = ((u8*)&game_data_priv->seen_unown_pid)[j];
                for(size_t j = 0; j < sizeof(game_data_priv->seen_spinda_pid); j++)
                    buffer_8[DEX_SPINDA_PID_POS+j] = ((u8*)&game_data_priv->seen_spinda_pid)[j];
                for(size_t j = 0; j < sizeof(game_data_priv->pokedex_owned); j++)
                    buffer_8[DEX_POS_OWNED+j] = ((u8*)game_data_priv->pokedex_owned)[j];
                for(size_t j = 0; j < sizeof(game_data_priv->pokedex_seen); j++)
                    buffer_8[DEX_POS_SEEN_0+j] = ((u8*)game_data_priv->pokedex_seen)[j];
            }

            if(section_id == SECTION_SYS_FLAGS_ID)
                set_sys_flag_save(buffer_8, game_id, nat_dex_flag_num[game_id], game_data_priv->nat_dex_flag);

            if(section_id == SECTION_VARS_ID)
                set_var_save(buffer_8, game_id, nat_dex_var_num[game_id], game_data_priv->nat_dex_var);

            if(section_id == SECTION_GAME_STATS_ID) {
                set_stat_save(buffer_8, game_id, SAVED_GAME_STAT_NUM, game_data_priv->num_saves_stat ^ game_data_priv->stat_enc_key);
                set_stat_save(buffer_8, game_id, NUM_TRADES_STAT_NUM, game_data_priv->num_trades_stat ^ game_data_priv->stat_enc_key);
            }

            if(section_id == SECTION_DEX_SEEN_1_ID)
                for(size_t j = 0; j < sizeof(game_data_priv->pokedex_seen); j++)
                    buffer_8[pokedex_extra_pos_1[game_id]+j] = ((u8*)game_data_priv->pokedex_seen)[j];
            
            if(section_id == SECTION_DEX_SEEN_2_ID)
                for(size_t j = 0; j < sizeof(game_data_priv->pokedex_seen); j++)
                    buffer_8[pokedex_extra_pos_2[game_id]+j] = ((u8*)game_data_priv->pokedex_seen)[j];

            store_time_data(&game_data_priv->clock_events, section_id, buffer_8, game_id, has_rtc_events(&game_data->game_identifier), game_data_priv->stat_enc_key);
            
            // Checksum calcs
            u16 sum_size = summable_bytes[section_id] >> 2;
            if(section_id == 0)
                sum_size = summable_bytes_section0[game_id] >> 2;
            if(section_id == 4)
                sum_size = summable_bytes_section4[game_id] >> 2;
            buffer_16[CHECKSUM_POS>>1] = calc_checksum_save_buffer(buffer, sum_size);

            // Copy to new slot
            erase_sector((target_slot * SAVE_SLOT_SIZE) + (target_section * SECTION_SIZE));
            copy_ram_to_save(buffer_8, (target_slot * SAVE_SLOT_SIZE) + (target_section * SECTION_SIZE), SECTION_SIZE);
            // Verify everything went accordingly
            if(!is_save_correct(buffer_8, (target_slot * SAVE_SLOT_SIZE) + (target_section * SECTION_SIZE), SECTION_SIZE))
                return 0;
        }

        target_section++;
    }
    
    return 1;
}

u8 complete_save(u8 base_slot, struct game_data_t* game_data) {
    u8 target_slot = get_next_slot(base_slot);

    for(int i = 0; i < SECTION_TOTAL; i++)
        erase_sector((base_slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE));

    if(get_slot(&game_data->game_identifier) != target_slot)
        return 0;
    return 1;
}

u8 get_slot(struct game_identity* game_identifier){
    u32 last_valid_save_index = 0;
    u8 slot = INVALID_SLOT;

    for(int i = 0; i < NUM_SLOTS; i++) {
        u32 current_save_index = read_slot_index(i);
        if(validate_slot(i, game_identifier)) {
            u8 success = 0;

            if(slot == INVALID_SLOT)
                success = 1;
            else if(((last_valid_save_index + 1) == 0) && (current_save_index == 0))
                success = 1;
            else if((last_valid_save_index == 0) && ((current_save_index + 1) == 0))
                success = 0;
            else if(current_save_index > last_valid_save_index)
                success = 1;
            else
                success = 0;

            if(success) {
                slot = i;
                last_valid_save_index = current_save_index;
            }
        }
    }
    return slot;
}

void init_save_data(){
    unload_cartridge();
}

void unload_cartridge(){
    is_cartridge_loaded = 0;
}

void load_cartridge(){
    for(size_t i = 0; i < SECTION_TOTAL; i++)
        loaded_checksum[i] = read_short_save((in_use_slot * SAVE_SLOT_SIZE) + CHECKSUM_POS + (i * SECTION_SIZE));
    currently_checking_section = 0;
    time_since_last_check = 0;
    is_cartridge_loaded = 1;
}

u8 loaded_data_has_warnings(struct game_data_t* game_data, struct game_data_priv_t* game_data_priv) {
    if((!get_is_cartridge_loaded()) || (can_trade(game_data_priv, game_data->game_identifier.game_main_version) == TRADE_IMPOSSIBLE))
        return 0;
    return (!is_in_pokemon_center(game_data_priv, game_data->game_identifier.game_main_version)) || (can_trade(game_data_priv, game_data->game_identifier.game_main_version) == PARTIAL_TRADE_POSSIBLE) || (get_party_usable_num(game_data) < MIN_ACTIVE_MON_TRADING);
}

IWRAM_CODE u8 get_is_cartridge_loaded(){
    return is_cartridge_loaded;
}

IWRAM_CODE u32 read_magic_number(u8 slot, u8 section){
    return read_int_save((slot * SAVE_SLOT_SIZE) + MAGIC_NUMBER_POS + (section * SECTION_SIZE));
}

IWRAM_CODE u8 has_cartridge_been_removed(){
    u8 retval = 0;
    if(get_is_cartridge_loaded()) {
        if(time_since_last_check < FRAMES_BETWEEN_CHECKS)
            time_since_last_check++;
        else {
            if(currently_checking_section >= SECTION_TOTAL)
                currently_checking_section = 0;
            if(read_short_save((in_use_slot * SAVE_SLOT_SIZE) + CHECKSUM_POS + (currently_checking_section * SECTION_SIZE)) != loaded_checksum[currently_checking_section])
                retval = 1;
            if(read_magic_number(in_use_slot, currently_checking_section) != MAGIC_NUMBER)
                retval = 1;
            currently_checking_section++;
            time_since_last_check = 0;
        }
    }
    return retval;
}

u8 read_gen_3_data(struct game_data_t* game_data, struct game_data_priv_t* game_data_priv){
    unload_cartridge();
    init_bank();
    
    game_data_priv->game_is_suspended = 0;
    
    u8 slot = get_slot(&game_data->game_identifier);
    in_use_slot = slot;
    if(slot == INVALID_SLOT)
        return 0;
    
    read_party(slot, game_data, game_data_priv);
    own_game_data_ptr = game_data;
    
    if(game_data_priv->game_is_suspended)
        return 0;
    
    load_cartridge();
    
    return 1;
}

u8 pre_write_gen_3_data(struct game_data_t* game_data, struct game_data_priv_t* game_data_priv, u8 is_full){
    unload_cartridge();
    init_bank();

    u8 base_slot = in_use_slot;
    if(base_slot == INVALID_SLOT)
        return 0;

    if(is_full) {
        if(!pre_update_save(game_data, game_data_priv, base_slot, FULL_SAVE))
            return 0;

        if(!validate_slot(get_next_slot(base_slot), &game_data->game_identifier))
            return 0;
    }
    else {
        if(!pre_update_save(game_data, game_data_priv, base_slot, NON_PARTY_SAVE))
            return 0;
    }

    load_cartridge();

    return 1;
}

u8 pre_write_updated_moves_gen_3_data(struct game_data_t* game_data, struct game_data_priv_t* game_data_priv){
    unload_cartridge();
    init_bank();

    u8 base_slot = in_use_slot;
    if(base_slot == INVALID_SLOT)
        return 0;

    if(!pre_update_save(game_data, game_data_priv, base_slot, PARTY_ONLY_SAVE))
        return 0;

    if(!validate_slot(get_next_slot(base_slot), &game_data->game_identifier))
        return 0;

    load_cartridge();

    return 1;
}

u8 complete_write_gen_3_data(struct game_data_t* game_data){
    unload_cartridge();
    init_bank();

    u8 base_slot = in_use_slot;
    if(base_slot == INVALID_SLOT)
        return 0;

    if(!complete_save(base_slot, game_data))
        return 0;
    
    in_use_slot = get_next_slot(in_use_slot);

    load_cartridge();

    return 1;
}
