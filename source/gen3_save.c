#include <gba.h>
#include "save.h"
#include "gen3_save.h"
#include "party_handler.h"
#include "gen_converter.h"
#include "text_handler.h"
#include <stddef.h>

#include "default_gift_ribbons_bin.h"

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
#define SECTION_BASE_DEX_ID 0
#define SECTION_DEX_SEEN_1_ID 1
#define SECTION_DEX_SEEN_2_ID 4
#define SECTION_MAIL_ID 3
#define SECTION_GIFT_RIBBON_ID 4

#define TRAINER_NAME_POS 0
#define TRAINER_GENDER_POS 8
#define TRAINER_ID_POS 10
#define DEX_BASE_POS 0x18
#define DEX_UNOWN_PID_POS (DEX_BASE_POS+4)
#define DEX_SPINDA_PID_POS (DEX_BASE_POS+8)
#define DEX_POS_OWNED (DEX_BASE_POS+0x10)
#define DEX_POS_SEEN_0 (DEX_BASE_POS+DEX_BYTES+0x10)

#define RSE_PARTY 0x234
#define FRLG_PARTY 0x34

#define VALID_MAPS 36

enum SAVING_KIND {FULL_SAVE, NON_PARTY_SAVE, PARTY_ONLY_SAVE};

u16 read_section_id(int, int);
u32 read_slot_index(int);
void read_game_data_trainer_info(int, struct game_data_t*);
void register_dex_entry(struct game_data_t*, struct gen3_mon_data_unenc*);
void handle_mail_trade(struct game_data_t*, u8, u8);
void read_party(int, struct game_data_t*);
void update_gift_ribbons(struct game_data_t*, const u8*);
u8 validate_slot(int);
u8 get_slot(void);
void unload_cartridge(void);
void load_cartridge(void);
u8 pre_update_save(struct game_data_t*, u8, enum SAVING_KIND);
u8 complete_save(u8);
static u8 get_next_slot(u8);

const u16 summable_bytes[SECTION_TOTAL] = {3884, 3968, 3968, 3968, 3848, 3968, 3968, 3968, 3968, 3968, 3968, 3968, 3968, 2000};
const u16 pokedex_extra_pos_1 [] = {0x938, 0x5F8, 0x988};
const u16 pokedex_extra_pos_2 [] = {0xC0C, 0xB98, 0xCA4};
const u16 mail_pos [] = {0xC4C, 0xDD0, 0xCE0};
const u16 ribbon_pos [] = {0x290, 0x21C, 0x328};
const u16 party_pos[] = {RSE_PARTY, FRLG_PARTY, RSE_PARTY};
const u16 special_area [] = {0, 9, 9};
const u16 rs_valid_maps[VALID_MAPS] = {0x0202, 0x0203, 0x0301, 0x0302, 0x0405, 0x0406, 0x0503, 0x0504, 0x0603, 0x0604, 0x0700, 0x0701, 0x0804, 0x0805, 0x090a, 0x090b, 0x0a05, 0x0a06, 0x0b05, 0x0b06, 0x0c02, 0x0c03, 0x0d06, 0x0d07, 0x0e03, 0x0e04, 0x0f02, 0x0f03, 0x100c, 0x100d, 0x100a, 0x1918, 0x1919, 0x191a, 0x191b, 0x1a05};

struct game_data_t* own_game_data_ptr;
u8 in_use_slot;
u8 is_cartridge_loaded;
u16 loaded_checksum;

struct game_data_t* get_own_game_data() {
    return own_game_data_ptr;
}

void init_game_data(struct game_data_t* game_data) {
    init_game_identifier(&game_data->game_identifier);
    
    game_data->party_1.total = 0;
    game_data->party_2.total = 0;
    game_data->party_3.total = 0;
    
    for(size_t i = 0; i < (OT_NAME_GEN3_SIZE+1); i++)
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

void read_game_data_trainer_info(int slot, struct game_data_t* game_data) {
    if(slot)
        slot = 1;

    for(int i = 0; i < SECTION_TOTAL; i++)
        if(read_section_id(slot, i) == SECTION_TRAINER_INFO_ID) {
            copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + TRAINER_NAME_POS, game_data->trainer_name, OT_NAME_GEN3_SIZE+1);
            game_data->trainer_gender = read_byte_save((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + TRAINER_GENDER_POS);
            game_data->trainer_id = read_int_save((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + TRAINER_ID_POS);
            determine_game_with_save(&game_data->game_identifier, slot, i, summable_bytes[SECTION_TRAINER_INFO_ID]);

            break;
        }
}

void register_dex_entry(struct game_data_t* game_data, struct gen3_mon_data_unenc* data_src) {
    u16 dex_index = get_dex_index_raw(data_src);
    if(dex_index != NO_DEX_INDEX) {
        u8 base_index = dex_index >> 3;
        u8 rest_index = dex_index & 7;
        u8 prev_seen = game_data->pokedex_seen[base_index] & (1 << rest_index);
        game_data->pokedex_seen[base_index] |= (1 << rest_index);
        game_data->pokedex_owned[base_index] |= (1 << rest_index);
        if((!prev_seen) && (data_src->growth.species == UNOWN_SPECIES))
            game_data->seen_unown_pid = data_src->src->pid;
        if((!prev_seen) && (data_src->growth.species == SPINDA_SPECIES))
            game_data->seen_spinda_pid = data_src->src->pid;
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

u8 trade_mons(struct game_data_t* game_data, u8 own_mon, u8 other_mon, u8 curr_gen) {
    handle_mail_trade(game_data, own_mon, other_mon);

    u8* dst = (u8*)&game_data[0].party_3.mons[own_mon];
    u8* src = (u8*)&game_data[1].party_3.mons[other_mon];
    for(size_t i = 0; i < sizeof(struct gen3_mon); i++)
        dst[i] = src[i];
    dst = (u8*)&game_data[0].party_3_undec[own_mon];
    src = (u8*)&game_data[1].party_3_undec[other_mon];
    for(size_t i = 0; i < sizeof(struct gen3_mon_data_unenc); i++)
        dst[i] = src[i];
    game_data[0].party_3_undec[own_mon].src = &game_data[0].party_3.mons[own_mon];
    update_gift_ribbons(&game_data[0], game_data[1].giftRibbons);
    register_dex_entry(&game_data[0], &game_data[0].party_3_undec[own_mon]);
    u8 ret_val = trade_evolve(&game_data[0].party_3.mons[own_mon], &game_data[0].party_3_undec[own_mon], curr_gen);
    if(ret_val)
        register_dex_entry(&game_data[0], &game_data[0].party_3_undec[own_mon]);
    return ret_val;
}

u8 is_invalid_offer(struct game_data_t* game_data, u8 own_mon, u8 other_mon, u8 curr_gen, u16 received_species) {
    // Check for validity
    if(!game_data[1].party_3_undec[other_mon].is_valid_gen3)
        return 1 + 0;
    // For gen 3, check the correct species from the other actor
    if((curr_gen == 3) && (game_data[1].party_3_undec[other_mon].growth.species != received_species))
        return 1 + 0;
    // Check that the receiving party has at least one active mon
    if(game_data[1].party_3_undec[other_mon].is_egg) {
        u8 found_normal = 0;
        for(gen3_party_total_t i = 0; i < PARTY_SIZE; i++) {
            if((i != own_mon) && (game_data[0].party_3_undec[i].is_valid_gen3) && (!game_data[0].party_3_undec[i].is_egg)) {
                found_normal = 1;
                break;
            }
        }
        if(!found_normal)
            return 1 + 1;
    }
    return 0;
}

void process_party_data(struct game_data_t* game_data) {
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
        if(gen3_to_gen2(&game_data->party_2.mons[curr_slot], &game_data->party_3_undec[i], game_data->trainer_id)) {
            curr_slot++;
            game_data->party_3_undec[i].is_valid_gen2 = 1;
        }
        else
            game_data->party_3_undec[i].is_valid_gen2 = 0;
    game_data->party_2.total = curr_slot;
    curr_slot = 0;
    for(gen3_party_total_t i = 0; i < game_data->party_3.total; i++)
        if(gen3_to_gen1(&game_data->party_1.mons[curr_slot], &game_data->party_3_undec[i], game_data->trainer_id)) {
            curr_slot++;
            game_data->party_3_undec[i].is_valid_gen1 = 1;
        }
        else
            game_data->party_3_undec[i].is_valid_gen1 = 0;
    game_data->party_1.total = curr_slot;
}

void read_party(int slot, struct game_data_t* game_data) {
    if(slot)
        slot = 1;
    
    read_game_data_trainer_info(slot, game_data);
    if(game_data->game_identifier.game_is_jp == UNDETERMINED) {
        if(game_data->trainer_name[OT_NAME_JP_GEN3_SIZE+1] == 0)
            game_data->game_identifier.game_is_jp = 1;
        else
            game_data->game_identifier.game_is_jp = 0;
    }
    
    u8 game_id = game_data->game_identifier.game_main_version;

    for(int i = 0; i < SECTION_TOTAL; i++) {
        u16 section_id = read_section_id(slot, i);
        if(section_id == SECTION_PARTY_INFO_ID)
            copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + party_pos[game_id], (u8*)(&game_data->party_3), sizeof(struct gen3_party));
        if(section_id == SECTION_MAIL_ID)
            copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + mail_pos[game_id], (u8*)game_data->mails_3, sizeof(game_data->mails_3));
        if(section_id == SECTION_GIFT_RIBBON_ID)
            copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + ribbon_pos[game_id], (u8*)game_data->giftRibbons, sizeof(game_data->giftRibbons));
        if(section_id == SECTION_BASE_DEX_ID) {
            game_data->seen_unown_pid = read_int_save((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + DEX_UNOWN_PID_POS);
            game_data->seen_spinda_pid = read_int_save((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + DEX_SPINDA_PID_POS);
            copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + DEX_POS_OWNED, game_data->pokedex_owned, sizeof(game_data->pokedex_owned));
            copy_save_to_ram((slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE) + DEX_POS_SEEN_0, game_data->pokedex_seen, sizeof(game_data->pokedex_seen));
        }
    }
    process_party_data(game_data);
}

u8 validate_slot(int slot) {
    if(slot)
        slot = 1;
    
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
        u16 prepared_checksum = buffer_16[CHECKSUM_POS>>1];
        u32 checksum = 0;
        for(int j = 0; j < (summable_bytes[section_id] >> 2); j++)
            checksum += buffer[j];
        checksum = ((checksum & 0xFFFF) + (checksum >> 16)) & 0xFFFF;
        if(checksum != prepared_checksum)
            return 0;
        valid_sections |= (1 << section_id);
    }
    
    if(valid_sections != (1 << (SECTION_TOTAL))-1)
        return 0;

    return 1;
}

ALWAYS_INLINE u8 get_next_slot(u8 base_slot) {
    return (base_slot + 1) % NUM_SLOTS;
}

u8 pre_update_save(struct game_data_t* game_data, u8 base_slot, enum SAVING_KIND saving_kind) {
    u8 target_slot = get_next_slot(base_slot);
    u32 target_slot_save_index = read_slot_index(base_slot) - 1;

    u32 buffer[SECTION_SIZE>>2];
    u16* buffer_16 = (u16*)buffer;
    u8* buffer_8 = (u8*)buffer;
    u8 target_section = 1;
    u8 game_id = game_data->game_identifier.game_main_version;
    
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
                for(size_t j = 0; j < sizeof(game_data->seen_unown_pid); j++)
                    buffer_8[DEX_UNOWN_PID_POS+j] = ((u8*)&game_data->seen_unown_pid)[j];
                for(size_t j = 0; j < sizeof(game_data->seen_spinda_pid); j++)
                    buffer_8[DEX_SPINDA_PID_POS+j] = ((u8*)&game_data->seen_spinda_pid)[j];
                for(size_t j = 0; j < sizeof(game_data->pokedex_owned); j++)
                    buffer_8[DEX_POS_OWNED+j] = ((u8*)game_data->pokedex_owned)[j];
                for(size_t j = 0; j < sizeof(game_data->pokedex_seen); j++)
                    buffer_8[DEX_POS_SEEN_0+j] = ((u8*)game_data->pokedex_seen)[j];
            }
            
            if(section_id == SECTION_DEX_SEEN_1_ID)
                for(size_t j = 0; j < sizeof(game_data->pokedex_seen); j++)
                    buffer_8[pokedex_extra_pos_1[game_id]+j] = ((u8*)game_data->pokedex_seen)[j];
            
            if(section_id == SECTION_DEX_SEEN_2_ID)
                for(size_t j = 0; j < sizeof(game_data->pokedex_seen); j++)
                    buffer_8[pokedex_extra_pos_2[game_id]+j] = ((u8*)game_data->pokedex_seen)[j];
            
            // Checksum calcs
            u32 checksum = 0;
            for(int j = 0; j < (summable_bytes[section_id] >> 2); j++)
                checksum += buffer[j];
            checksum = ((checksum & 0xFFFF) + (checksum >> 16)) & 0xFFFF;
            buffer_16[CHECKSUM_POS>>1] = checksum;

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

u8 complete_save(u8 base_slot) {
    u8 target_slot = get_next_slot(base_slot);

    for(int i = 0; i < SECTION_TOTAL; i++)
        erase_sector((base_slot * SAVE_SLOT_SIZE) + (i * SECTION_SIZE));

    if(get_slot() != target_slot)
        return 0;
    return 1;
}

u8 get_slot(){
    u32 last_valid_save_index = 0;
    u8 slot = INVALID_SLOT;

    for(int i = 0; i < NUM_SLOTS; i++) {
        u32 current_save_index = read_slot_index(i);
        if(validate_slot(i)) {
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
    loaded_checksum = read_short_save((in_use_slot * SAVE_SLOT_SIZE) + CHECKSUM_POS);
    is_cartridge_loaded = 1;
}

u8 has_cartridge_been_removed(){
    u8 retval = 0;
    if(is_cartridge_loaded)
        if(read_short_save((in_use_slot * SAVE_SLOT_SIZE) + CHECKSUM_POS) != loaded_checksum)
            retval = 1;
    return retval;
}

u8 read_gen_3_data(struct game_data_t* game_data){
    unload_cartridge();
    init_bank();
    
    u8 slot = get_slot();
    in_use_slot = slot;
    if(slot == INVALID_SLOT)
        return 0;
    
    read_party(slot, game_data);
    own_game_data_ptr = game_data;

    load_cartridge();

    return 1;
}

u8 pre_write_gen_3_data(struct game_data_t* game_data, u8 is_full){
    unload_cartridge();
    init_bank();

    u8 base_slot = in_use_slot;
    if(base_slot == INVALID_SLOT)
        return 0;

    if(is_full) {
        if(!pre_update_save(game_data, base_slot, FULL_SAVE))
            return 0;

        if(!validate_slot(get_next_slot(base_slot)))
            return 0;
    }
    else {
        if(!pre_update_save(game_data, base_slot, NON_PARTY_SAVE))
            return 0;
    }

    load_cartridge();

    return 1;
}

u8 pre_write_updated_moves_gen_3_data(struct game_data_t* game_data){
    unload_cartridge();
    init_bank();

    u8 base_slot = in_use_slot;
    if(base_slot == INVALID_SLOT)
        return 0;

    if(!pre_update_save(game_data, base_slot, PARTY_ONLY_SAVE))
        return 0;

    if(!validate_slot(get_next_slot(base_slot)))
        return 0;

    load_cartridge();

    return 1;
}

u8 complete_write_gen_3_data(){
    unload_cartridge();
    init_bank();

    u8 base_slot = in_use_slot;
    if(base_slot == INVALID_SLOT)
        return 0;

    if(!complete_save(base_slot))
        return 0;
    
    in_use_slot = get_next_slot(in_use_slot);

    load_cartridge();

    return 1;
}
