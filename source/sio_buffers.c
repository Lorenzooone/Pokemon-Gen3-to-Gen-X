#include <gba.h>
#include "sio_buffers.h"
#include "text_handler.h"
#include "options_handler.h"
#include "party_handler.h"
#include "gen_converter.h"
#include "gen3_save.h"
#include "config_settings.h"
#include <stddef.h>

#include "default_gift_ribbons_bin.h"

void prepare_number_of_sizes(void);
u8 get_number_of_buffers(void);
void copy_bytes(const void*, void*, size_t, u8, u8);
void prepare_random_data_gen12(struct random_data_t*);
void prepare_patch_set(u8*, u8*, size_t, size_t, size_t, size_t);
void apply_patch_set(u8*, u8*, size_t, size_t, size_t, size_t);
void prepare_mail_gen2(u8*, size_t, u8*, size_t, size_t);
void load_names_gen12(struct game_data_t*, struct gen2_party*, struct gen1_party*, u8*, u8*, u8*, u8);
void load_party_info_gen2(struct gen2_party*, struct gen2_party_info*);
void load_party_info_gen1(struct gen1_party*, struct gen1_party_info*);
void prepare_gen2_trade_data(struct game_data_t*, struct gen2_party*, u32*, u8, size_t*);
void prepare_gen1_trade_data(struct game_data_t*, struct gen1_party*, u32*, u8, size_t*);
void prepare_gen3_trade_data(struct game_data_t*, u32*, size_t*);
void read_gen12_trade_data(struct game_data_t*, u32*, u8, u8);
void read_gen3_trade_data(struct game_data_t* game_data, u32*);

u32 communication_buffers[2][BUFFER_SIZE>>2];
size_t buffer_sizes[NUM_SIZES];
u8 number_of_sizes;

u32* get_communication_buffer(u8 requested) {
    if(!requested)
        return communication_buffers[OWN_BUFFER];
    return communication_buffers[OTHER_BUFFER];
}

void prepare_number_of_sizes() {
    number_of_sizes = 0;
    for(int i = 0; i < NUM_SIZES; i++)
        if(buffer_sizes[i] != SIZE_STOP)
            number_of_sizes++;
        else
            return;
}

u8 get_number_of_buffers() {
    return number_of_sizes;
}

size_t* get_buffer_sizes() {
    return buffer_sizes;
}

size_t get_buffer_size(int index) {
    if((index < 0) || (index >= number_of_sizes))
        index = 0;
    return buffer_sizes[index];
}

void copy_bytes(const void* src, void* dst, size_t size, u8 src_offset, u8 dst_offset) {
    const u8* src_data = (const u8*)src;
    u8* dst_data = (u8*)dst;
    for(size_t i = 0; i < size; i++)
        dst_data[dst_offset+i] = src_data[src_offset+i];
}

void prepare_random_data_gen12(struct random_data_t* random_data) {
    for(size_t i = 0; i < RANDOM_DATA_SIZE; i++)
        random_data->data[i] = DEFAULT_FILLER;
}

void prepare_patch_set(u8* buffer, u8* patch_set_buffer, size_t size, size_t start_pos, size_t patch_set_size, size_t base_pos) {
    size_t cursor_data = base_pos;
    
    for(size_t i = 0; i < patch_set_size; i++)
        patch_set_buffer[i] = 0;
    
    u32 base = start_pos;
    for(size_t i = 0; i < size; i++) {
        if(buffer[start_pos + i] == NO_ACTION_BYTE) {
            buffer[start_pos + i] = 0xFF;
            patch_set_buffer[cursor_data++] = start_pos + i + 1 -base;
            if(cursor_data >= patch_set_size) {
                cursor_data -= 1;
                patch_set_buffer[cursor_data] = 0xFF;
                break;
            }
        }
        if((start_pos + i - base) == (NO_ACTION_BYTE-2)) {
            base += NO_ACTION_BYTE-1;
            patch_set_buffer[cursor_data++] = 0xFF;
            if(cursor_data >= patch_set_size) {
                cursor_data -= 1;
                patch_set_buffer[cursor_data] = 0xFF;
                break;
            }
        }
    }
    
    if((size+start_pos-base) > 0)
        patch_set_buffer[cursor_data] = 0xFF;
}

void apply_patch_set(u8* buffer, u8* patch_set_buffer, size_t size, size_t start_pos, size_t patch_set_size, size_t base_pos) {    
    size_t base = 0;
    for(size_t i = base_pos; i < patch_set_size; i++) {
        if(patch_set_buffer[i]) {
            if(patch_set_buffer[i] == 0xFF) {
                base += NO_ACTION_BYTE-1;
                if(base >= size)
                    return;
            }
            else if(patch_set_buffer[i] <= NO_ACTION_BYTE-2)
                if((patch_set_buffer[i]+base-1) < size)
                    buffer[patch_set_buffer[i]+start_pos+base-1] = 0xFE;
        }
    }
}

void prepare_mail_gen2(u8* buffer, size_t size, u8* patch_set_buffer, size_t patch_set_buffer_size, size_t start_pos) {
    for(size_t i = 0; i < size; i++)
        buffer[i] = 0;
    prepare_patch_set(buffer, patch_set_buffer, size, start_pos, patch_set_buffer_size, 0);
}

void load_names_gen12(struct game_data_t* game_data, struct gen2_party* party_2, struct gen1_party* party_1, u8* trainer_name, u8* ot_names, u8* nicknames, u8 is_jp){
    size_t size = STRING_GEN2_INT_SIZE;
    if(is_jp)
        size = STRING_GEN2_JP_SIZE;
    
    u8 curr_gen = 0;
    if(party_2 != NULL)
        curr_gen = 2;
    if(party_1 != NULL)
        curr_gen = 1;
    
    u8 language = get_filtered_target_int_language();
    if(is_jp)
        language = JAPANESE_LANGUAGE;
    
    convert_trainer_name_gen3_to_gen12(game_data->trainer_name, trainer_name, game_data->game_identifier.language, language);
    
    for(size_t i = 0; i < size; i++)
        if((trainer_name[i] == NO_ACTION_BYTE) || (trainer_name[i] == (NO_ACTION_BYTE-1)))
            trainer_name[i] = NO_ACTION_BYTE-2;
    trainer_name[size-1] = GEN2_EOL;
    
    if(!curr_gen)
        return;
    
    for(gen3_party_total_t i = 0; i < PARTY_SIZE; i++) {
        u8* src_ot_name;
        u8* src_nickname;
        if(is_jp && curr_gen == 2) {
            src_ot_name = party_2->mons[i].ot_name_jp;
            src_nickname = party_2->mons[i].nickname_jp;
        }
        else if(curr_gen == 2) {
            src_ot_name = party_2->mons[i].ot_name;
            src_nickname = party_2->mons[i].nickname;
        }
        else if(is_jp) {
            src_ot_name = party_1->mons[i].ot_name_jp;
            src_nickname = party_1->mons[i].nickname_jp;
        }
        else {
            src_ot_name = party_1->mons[i].ot_name;
            src_nickname = party_1->mons[i].nickname;
        }
        copy_bytes(src_ot_name, ot_names, size, 0, i*size);
        ot_names[(i*size)+size-1] = GEN2_EOL;
        copy_bytes(src_nickname, nicknames, size, 0, i*size);
        nicknames[(i*size)+size-1] = GEN2_EOL;
    }
}

void load_party_info_gen2(struct gen2_party* party_2, struct gen2_party_info* party_info){
    u8 num_options = party_2->total;
    if(num_options > PARTY_SIZE)
        num_options = PARTY_SIZE;
    party_info->num_mons = num_options;
    
    for(int i = 0; i < num_options; i++) {
        copy_bytes(&party_2->mons[i].data, &(party_info->mons_data[i]), sizeof(struct gen2_mon_data), 0, 0);
        party_info->mons_index[i] = party_2->mons[i].data.species;
        if(party_2->mons[i].is_egg)
            party_info->mons_index[i] = GEN2_EGG;
    }
    
    for(int i = num_options; i < MON_INDEX_SIZE; i++)
        party_info->mons_index[i] = GEN2_NO_MON;
}

void load_party_info_gen1(struct gen1_party* party_1, struct gen1_party_info* party_info){
    u8 num_options = party_1->total;
    if(num_options > PARTY_SIZE)
        num_options = PARTY_SIZE;
    party_info->num_mons = num_options;
    
    for(int i = 0; i < num_options; i++) {
        copy_bytes(&party_1->mons[i].data, &(party_info->mons_data[i]), sizeof(struct gen1_mon_data), 0, 0);
        party_info->mons_index[i] = party_1->mons[i].data.species;
    }
    
    for(int i = num_options; i < MON_INDEX_SIZE; i++)
        party_info->mons_index[i] = GEN2_NO_MON;
}

void prepare_gen2_trade_data(struct game_data_t* game_data, struct gen2_party* party_2, u32* buffer, u8 is_jp, size_t* sizes) {
    struct gen2_trade_data_int* td_int = (struct gen2_trade_data_int*)buffer;
    struct gen2_trade_data_jp* td_jp = (struct gen2_trade_data_jp*)buffer;
    
    prepare_random_data_gen12(&td_int->random_data);
    
    u8* trainer_name = (u8*)td_int->trainer_info.trainer_name;
    if(is_jp)
        trainer_name = (u8*)td_jp->trainer_info.trainer_name;
    
    u8* ot_names = (u8*)td_int->trainer_info.ot_names;
    if(is_jp)
    ot_names = (u8*)td_jp->trainer_info.ot_names;
    
    u8* nicknames = (u8*)td_int->trainer_info.nicknames;
    if(is_jp)
        nicknames = (u8*)td_jp->trainer_info.nicknames;
    
    load_names_gen12(game_data, party_2, NULL, trainer_name, ot_names, nicknames, is_jp);
    
    struct gen2_party_info* party_info = (struct gen2_party_info*)&td_int->trainer_info.party_info;
    if(is_jp)
        party_info = (struct gen2_party_info*)&td_jp->trainer_info.party_info;
    
    load_party_info_gen2(party_2, party_info);

    party_info->trainer_id = (game_data->trainer_id&0xFFFF);
    
    u8* safety_bytes = (u8*)td_int->trainer_info.safety_bytes;
    if(is_jp)
        safety_bytes = (u8*)td_jp->trainer_info.safety_bytes;
    for(int i = 0; i < SAFETY_BYTES_NUM; i++)
        safety_bytes[i] = DEFAULT_FILLER;
    
    if(!is_jp)
        prepare_patch_set((u8*)(&td_int->trainer_info), td_int->patch_set.patch_set, sizeof(struct trainer_data_gen2_int)-(STRING_GEN2_INT_SIZE + MON_INDEX_SIZE), STRING_GEN2_INT_SIZE + MON_INDEX_SIZE, PATCH_SET_SIZE, PATCH_SET_BASE_POS);
    else
        prepare_patch_set((u8*)(&td_jp->trainer_info), td_jp->patch_set.patch_set, sizeof(struct trainer_data_gen2_jp)-(STRING_GEN2_JP_SIZE + MON_INDEX_SIZE), STRING_GEN2_JP_SIZE + MON_INDEX_SIZE, PATCH_SET_SIZE, PATCH_SET_BASE_POS);
    
    u8* useless_sync_bytes = td_int->useless_sync;
    if(is_jp)
        useless_sync_bytes = td_jp->useless_sync;
    for(int i = 0; i < USELESS_SYNC_BYTES; i++)
        useless_sync_bytes[i] = USELESS_SYNC_VALUE;
    
    if(!is_jp)
        prepare_mail_gen2(td_int->mail.gen2_mail_data, MAIL_GEN2_INT_SIZE, td_int->mail.patch_set, MAIL_PATCH_SET_INT_SIZE, MAIL_PATCH_SET_INT_START);
    else
        prepare_mail_gen2(td_jp->mail.gen2_mail_data, MAIL_GEN2_JP_SIZE, td_jp->mail.patch_set, MAIL_PATCH_SET_JP_SIZE, MAIL_PATCH_SET_JP_START);
    
    sizes[0] = sizeof(struct random_data_t);
    if(!is_jp)
        sizes[1] = sizeof(struct trainer_data_gen2_int);
    else
        sizes[1] = sizeof(struct trainer_data_gen2_jp);
    sizes[2] = sizeof(struct patch_set_trainer_data_gen12);
    if(!is_jp)
        sizes[2] += USELESS_SYNC_BYTES + sizeof(struct party_mail_data_gen2_int);
    else
        sizes[2] += USELESS_SYNC_BYTES + sizeof(struct party_mail_data_gen2_jp);
}

void prepare_gen1_trade_data(struct game_data_t* game_data, struct gen1_party* party_1, u32* buffer, u8 is_jp, size_t* sizes) {
    struct gen1_trade_data_int* td_int = (struct gen1_trade_data_int*)buffer;
    struct gen1_trade_data_jp* td_jp = (struct gen1_trade_data_jp*)buffer;
    
    prepare_random_data_gen12(&td_int->random_data);
    
    u8* trainer_name = (u8*)td_int->trainer_info.trainer_name;
    if(is_jp)
        trainer_name = (u8*)td_jp->trainer_info.trainer_name;
    
    u8* ot_names = (u8*)td_int->trainer_info.ot_names;
    if(is_jp)
    ot_names = (u8*)td_jp->trainer_info.ot_names;
    
    u8* nicknames = (u8*)td_int->trainer_info.nicknames;
    if(is_jp)
        nicknames = (u8*)td_jp->trainer_info.nicknames;
    
    load_names_gen12(game_data, NULL, party_1, trainer_name, ot_names, nicknames, is_jp);
    
    struct gen1_party_info* party_info = (struct gen1_party_info*)&td_int->trainer_info.party_info;
    if(is_jp)
        party_info = (struct gen1_party_info*)&td_jp->trainer_info.party_info;
    
    load_party_info_gen1(party_1, party_info);
    
    u8* safety_bytes = (u8*)td_int->trainer_info.safety_bytes;
    if(is_jp)
        safety_bytes = (u8*)td_jp->trainer_info.safety_bytes;
    for(int i = 0; i < SAFETY_BYTES_NUM; i++)
        safety_bytes[i] = DEFAULT_FILLER;
    
    if(!is_jp)
        prepare_patch_set((u8*)(&td_int->trainer_info), td_int->patch_set.patch_set, sizeof(struct trainer_data_gen1_int)-(STRING_GEN2_INT_SIZE + MON_INDEX_SIZE), STRING_GEN2_INT_SIZE + MON_INDEX_SIZE, PATCH_SET_SIZE, PATCH_SET_BASE_POS);
    else
        prepare_patch_set((u8*)(&td_jp->trainer_info), td_jp->patch_set.patch_set, sizeof(struct trainer_data_gen1_jp)-(STRING_GEN2_JP_SIZE + MON_INDEX_SIZE), STRING_GEN2_JP_SIZE + MON_INDEX_SIZE, PATCH_SET_SIZE, PATCH_SET_BASE_POS);
    
    sizes[0] = sizeof(struct random_data_t);
    if(!is_jp)
        sizes[1] = sizeof(struct trainer_data_gen1_int);
    else
        sizes[1] = sizeof(struct trainer_data_gen1_jp);
    sizes[2] = sizeof(struct patch_set_trainer_data_gen12);
}

u8 are_checksum_same_gen3(struct gen3_trade_data* td) {
    u32 checksum = 0;
    for(gen3_party_total_t i = 0; i < PARTY_SIZE; i++) {
        u32* mail_buf = (u32*)&td->mails_3[i];
        for(size_t j = 0; j < (sizeof(struct mail_gen3)>>2); j++)
            checksum += mail_buf[j];
    }
    
    if(td->checksum_mail != checksum)
        return 0;

    checksum = 0;
    
    u32* party_buf = (u32*)&td->party_3;
    for(size_t i = 0; i < (sizeof(struct gen3_party)>>2); i++)
        checksum += party_buf[i];
    
    if(td->checksum_party != checksum)
        return 0;
    
    checksum = 0;
    u32* buffer = (u32*)td;
    
    for(size_t i = 0; i < ((sizeof(struct gen3_trade_data) - 4)>>2); i++)
        checksum += buffer[i];
    
    if(td->final_checksum != checksum)
        return 0;
    
    return 1;
}

void prepare_gen3_trade_data(struct game_data_t* game_data, u32* buffer, size_t* sizes) {
    struct gen3_trade_data* td = (struct gen3_trade_data*)buffer;
    
    u32 checksum = 0;
    for(gen3_party_total_t i = 0; i < PARTY_SIZE; i++) {
        copy_bytes(&game_data->mails_3[i], &td->mails_3[i], sizeof(struct mail_gen3), 0, 0);
        u32* mail_buf = (u32*)&td->mails_3[i];
        for(size_t j = 0; j < (sizeof(struct mail_gen3)>>2); j++)
            checksum += mail_buf[j];
    }
    
    td->checksum_mail = checksum;
    checksum = 0;
    
    if(game_data->party_3.total > PARTY_SIZE)
        game_data->party_3.total = PARTY_SIZE;
    
    copy_bytes(&game_data->party_3, &td->party_3, sizeof(struct gen3_party), 0, 0);
    for(gen3_party_total_t i = 0; i < game_data->party_3.total; i++)
        game_data->party_3_undec[i].comm_pid = game_data->party_3_undec[i].src->pid;
    u32* party_buf = (u32*)&td->party_3;
    for(size_t i = 0; i < (sizeof(struct gen3_party)>>2); i++)
        checksum += party_buf[i];
    
    td->checksum_party = checksum;
    checksum = 0;
    
    copy_bytes(&game_data->game_identifier, &td->game_identifier, sizeof(struct game_identity), 0, 0);
    
    copy_bytes(game_data->giftRibbons, td->giftRibbons, GIFT_RIBBONS, 0, 0);
    copy_bytes(game_data->trainer_name, td->trainer_name, OT_NAME_GEN3_MAX_SIZE+1, 0, 0);
    td->trainer_gender = game_data->trainer_gender;
    
    for(int i = 0; i < NUM_EXTRA_PADDING_BYTES_GEN3; i++)
        td->extra_tmp_padding[i] = 0;
    
    td->trainer_id = game_data->trainer_id;
    
    for(size_t i = 0; i < ((sizeof(struct gen3_trade_data) - 4)>>2); i++)
        checksum += buffer[i];
    
    td->final_checksum = checksum;
    
    sizes[0] = sizeof(struct gen3_trade_data);
}

void read_gen12_trade_data(struct game_data_t* game_data, u32* buffer, u8 curr_gen, u8 is_jp) {
    struct gen2_trade_data_int* td_int2 = (struct gen2_trade_data_int*) buffer;
    struct gen2_trade_data_jp* td_jp2 = (struct gen2_trade_data_jp*) buffer;
    struct gen1_trade_data_int* td_int1 = (struct gen1_trade_data_int*) buffer;
    struct gen1_trade_data_jp* td_jp1 = (struct gen1_trade_data_jp*) buffer;
    
    size_t names_size = STRING_GEN2_INT_SIZE;
    u8* patch_target = (u8*)(&td_int2->trainer_info);
    u8* patch_set = (u8*)td_int2->patch_set.patch_set;
    size_t target_size = sizeof(struct trainer_data_gen2_int);
    u8* trainer_name = (u8*)td_int2->trainer_info.trainer_name;
    struct gen2_party_info* party_info2 = &td_int2->trainer_info.party_info;
    struct gen1_party_info* party_info1 = &td_int1->trainer_info.party_info;
    u8* ot_names = (u8*)td_int2->trainer_info.ot_names;
    u8* nicknames = (u8*)td_int2->trainer_info.nicknames;
    
    if(is_jp) {
        names_size = STRING_GEN2_JP_SIZE;
        if(curr_gen == 2) {
            patch_target = (u8*)(&td_jp2->trainer_info);
            patch_set = (u8*)td_jp2->patch_set.patch_set;
            target_size = sizeof(struct trainer_data_gen2_jp);
            trainer_name = (u8*)td_jp2->trainer_info.trainer_name;
            party_info2 = &td_jp2->trainer_info.party_info;
            ot_names = (u8*)td_jp2->trainer_info.ot_names;
            nicknames = (u8*)td_jp2->trainer_info.nicknames;
        }
    }
    
    if(curr_gen != 2) {
        if(is_jp) {
            patch_target = (u8*)(&td_jp1->trainer_info);
            patch_set = (u8*)td_jp1->patch_set.patch_set;
            target_size = sizeof(struct trainer_data_gen1_jp);
            trainer_name = (u8*)td_jp1->trainer_info.trainer_name;
            party_info1 = &td_jp1->trainer_info.party_info;
            ot_names = (u8*)td_jp1->trainer_info.ot_names;
            nicknames = (u8*)td_jp1->trainer_info.nicknames;
        }
        else {
            patch_target = (u8*)(&td_int1->trainer_info);
            patch_set = (u8*)td_int1->patch_set.patch_set;
            target_size = sizeof(struct trainer_data_gen1_int);
            trainer_name = (u8*)td_int1->trainer_info.trainer_name;
            ot_names = (u8*)td_int1->trainer_info.ot_names;
            nicknames = (u8*)td_int1->trainer_info.nicknames;
        }
    }

    init_game_data(game_data);
    
    game_data->game_identifier.language = get_filtered_target_int_language();
    if(is_jp)
        game_data->game_identifier.language = JAPANESE_LANGUAGE;
    
    copy_bytes(default_gift_ribbons_bin, game_data->giftRibbons, GIFT_RIBBONS, 0, 0);
    game_data->trainer_gender = 0;
    
    apply_patch_set(patch_target, patch_set, target_size-(names_size + MON_INDEX_SIZE), names_size + MON_INDEX_SIZE, PATCH_SET_SIZE, PATCH_SET_BASE_POS);
    
    convert_trainer_name_gen12_to_gen3(trainer_name, game_data->trainer_name, is_jp, game_data->game_identifier.language, OT_NAME_GEN3_MAX_SIZE+1);
    
    if(curr_gen == 2)
        game_data->trainer_id = ((party_info2->trainer_id & 0xFF) << 8) | (party_info2->trainer_id >> 8);
    
    if(curr_gen == 2)
        game_data->party_3.total = party_info2->num_mons;
    else
        game_data->party_3.total = party_info1->num_mons;
        
    if(game_data->party_3.total > PARTY_SIZE)
        game_data->party_3.total = PARTY_SIZE;
    
    u8 found = 0;
    for(gen3_party_total_t i = 0; i < game_data->party_3.total; i++) {
        game_data->party_3_undec[i].src = &game_data->party_3.mons[i];
        u8 conversion_success = 0;
        if(curr_gen == 2)
            conversion_success = gen2_to_gen3(&party_info2->mons_data[i], &game_data->party_3_undec[i], party_info2->mons_index[i], ot_names + (i*names_size), nicknames + (i*names_size), is_jp);
        else
            conversion_success = gen1_to_gen3(&party_info1->mons_data[i], &game_data->party_3_undec[i], party_info1->mons_index[i], ot_names + (i*names_size), nicknames + (i*names_size), is_jp);
        if(conversion_success) {
            const u16* learnable_moves = game_data->party_3_undec[i].learnable_moves;
            process_gen3_data(&game_data->party_3.mons[i], &game_data->party_3_undec[i], game_data->game_identifier.game_main_version, game_data->game_identifier.game_sub_version);
            if(game_data->party_3_undec[i].is_valid_gen3) {
                game_data->party_3_undec[i].learnable_moves = learnable_moves;
                found = 1;
            }
        }
    }
    if (!found)
        game_data->party_3.total = 0;
}

void read_gen3_trade_data(struct game_data_t* game_data, u32* buffer) {
    struct gen3_trade_data* td = (struct gen3_trade_data*)buffer;
    init_game_data(game_data);
    
    for(gen3_party_total_t i = 0; i < PARTY_SIZE; i++)
        copy_bytes(&td->mails_3[i], &game_data->mails_3[i], sizeof(struct mail_gen3), 0, 0);
    
    copy_bytes(&td->party_3, &game_data->party_3, sizeof(struct gen3_party), 0, 0);
    copy_bytes(&td->game_identifier, &game_data->game_identifier, sizeof(struct game_identity), 0, 0);
    
    copy_bytes(td->giftRibbons, game_data->giftRibbons, GIFT_RIBBONS, 0, 0);
    copy_bytes(td->trainer_name, game_data->trainer_name, OT_NAME_GEN3_MAX_SIZE+1, 0, 0);
    game_data->trainer_gender = td->trainer_gender;
    
    game_data->trainer_id = td->trainer_id;
    
    if(game_data->party_3.total > PARTY_SIZE)
        game_data->party_3.total = PARTY_SIZE;
    
    u8 found = 0;
    for(gen3_party_total_t i = 0; i < game_data->party_3.total; i++) {
        game_data->party_3_undec[i].comm_pid = game_data->party_3.mons[i].pid;
        process_gen3_data(&game_data->party_3.mons[i], &game_data->party_3_undec[i], game_data->game_identifier.game_main_version, game_data->game_identifier.game_sub_version);
        if(game_data->party_3_undec[i].is_valid_gen3)
            found = 1;
    }
    if (!found)
        game_data->party_3.total = 0;
}

void load_comm_buffer(struct game_data_t* game_data, struct gen2_party* party_2, struct gen1_party* party_1, int curr_gen, u8 is_jp) {

    for(int i = 0; i < NUM_SIZES; i++)
        buffer_sizes[i] = SIZE_STOP;

    switch(curr_gen) {
        case 1:
            prepare_gen1_trade_data(game_data, party_1, communication_buffers[OWN_BUFFER], is_jp, buffer_sizes);
            break;
        case 2:
            prepare_gen2_trade_data(game_data, party_2, communication_buffers[OWN_BUFFER], is_jp, buffer_sizes);
            break;
        default:
            prepare_gen3_trade_data(game_data, communication_buffers[OWN_BUFFER], buffer_sizes);
            break;
    }
    prepare_number_of_sizes();
}

void read_comm_buffer(struct game_data_t* game_data, int curr_gen, u8 is_jp) {

    switch(curr_gen) {
        case 1:
            read_gen12_trade_data(game_data, communication_buffers[OTHER_BUFFER], 1, is_jp);
            break;
        case 2:
            read_gen12_trade_data(game_data, communication_buffers[OTHER_BUFFER], 2, is_jp);
            break;
        default:
            read_gen3_trade_data(game_data, communication_buffers[OTHER_BUFFER]);
            break;
    }
    sanitize_ot_name(game_data->trainer_name, OT_NAME_GEN3_MAX_SIZE+1, game_data->game_identifier.language, 0);
}
