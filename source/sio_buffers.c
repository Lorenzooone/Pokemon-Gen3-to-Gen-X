#include <stddef.h>
#include <gba.h>
#include "sio_buffers.h"
#include "text_handler.h"
#include "options_handler.h"
#include "party_handler.h"
#include "gen3_save.h"

u32 communication_buffers[2][BUFFER_SIZE>>2];
u16 buffer_sizes[NUM_SIZES];
u8 number_of_sizes;

#define EACH_DIFFERENT 1

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

u16* get_buffer_sizes() {
    return buffer_sizes;
}

u16 get_buffer_size(int index) {
    if((index < 0) || (index >= number_of_sizes))
        index = 0;
    return buffer_sizes[index];
}

void copy_bytes(void* src, void* dst, int size, u8 src_offset, u8 dst_offset) {
    u8* src_data = (u8*)src;
    u8* dst_data = (u8*)dst;
    for(int i = 0; i < size; i++)
        dst_data[dst_offset+i] = src_data[src_offset+i];
}

void prepare_random_data_gen12(struct random_data_t* random_data) {
    for(int i = 0; i < RANDOM_DATA_SIZE; i++)
        random_data->data[i] = DEFAULT_FILLER;
}

void prepare_patch_set(u8* buffer, u8* patch_set_buffer, int size, int start_pos, int patch_set_size, int base_pos) {
    int cursor_data = base_pos;
    
    for(int i = 0; i < patch_set_size; i++)
        patch_set_buffer[i] = 0;
    
    u32 base = 0;
    for(int i = 0; i < size; i++) {
        if(buffer[i] == NO_ACTION_BYTE) {
            buffer[i] = 0xFF;
            patch_set_buffer[cursor_data++] = i+1-base;
            if(cursor_data >= patch_set_size) {
                cursor_data -= 1;
                patch_set_buffer[cursor_data] = 0xFF;
                break;
            }
        }
        if((i-base) == (NO_ACTION_BYTE-2)) {
            base += NO_ACTION_BYTE-1;
            patch_set_buffer[cursor_data++] = 0xFF;
            if(cursor_data >= patch_set_size) {
                cursor_data -= 1;
                patch_set_buffer[cursor_data] = 0xFF;
                break;
            }
        }
    }
    
    if((size-base) > 0)
        patch_set_buffer[cursor_data] = 0xFF;
}

void prepare_mail_gen2(u8* buffer, int size, u8* patch_set_buffer, u8 patch_set_buffer_size, u32 start_pos) {
    for(int i = 0; i < size; i++)
        buffer[i] = 0;
    prepare_patch_set(buffer, patch_set_buffer, size, start_pos, patch_set_buffer_size, 0);
}

#if !(EACH_DIFFERENT)

#define POS_PARTY_MEMBERS 0
#define SIZE_MEMBER 1
#define POS_TOTAL_MONS 2
#define POS_MON_DATA 3
#define SIZE_MON_DATA 4
#define POS_OT_NAME 5
#define SIZE_NAMES 6
#define POS_NICKNAME 7
#define SIZE_MAIL 8
#define POS_MAIL_PATCH 9
#define SIZE_MAIL_PATCH 10
#define START_MAIL_PATCH 11
#define POS_IS_EGG 12
// Address of party - Size of party member - Address of total size -
// Address of single mon data - Size of single mon data -
// Address of OT name - Size of names -
// Address of Nickname -
// Size of mail - Address of mail patch_set - Size of mail patch_set -
// Position of is_egg 
const u32 positions_party_multi[4][13] = {
    {offsetof(struct game_data_t, party_1) + offsetof(struct gen1_party, mons),
    sizeof(struct gen1_mon),
    offsetof(struct game_data_t, party_1) + offsetof(struct gen1_party, total),
    offsetof(struct gen1_mon, data), sizeof(struct gen1_mon_data),
    offsetof(struct gen1_mon, ot_name), STRING_GEN2_INT_SIZE+1,
    offsetof(struct gen1_mon, nickname), 0, 0, 0, 0, 0},
    {offsetof(struct game_data_t, party_1) + offsetof(struct gen1_party, mons),
    sizeof(struct gen1_mon),
    offsetof(struct game_data_t, party_1) + offsetof(struct gen1_party, total),
    offsetof(struct gen1_mon, data), sizeof(struct gen1_mon_data),
    offsetof(struct gen1_mon, ot_name_jp), STRING_GEN2_JP_SIZE+1,
    offsetof(struct gen1_mon, nickname_jp), 0, 0, 0, 0, 0},
    {offsetof(struct game_data_t, party_2) + offsetof(struct gen2_party, mons),
    sizeof(struct gen2_mon),
    offsetof(struct game_data_t, party_2) + offsetof(struct gen2_party, total),
    offsetof(struct gen2_mon, data), sizeof(struct gen2_mon_data),
    offsetof(struct gen2_mon, ot_name), STRING_GEN2_INT_SIZE+1,
    offsetof(struct gen2_mon, nickname), MAIL_GEN2_INT_SIZE,
    MAIL_GEN2_INT_SIZE, MAIL_PATCH_SET_INT_SIZE, MAIL_PATCH_SET_INT_START,
    offsetof(struct gen2_mon, is_egg)},
    {offsetof(struct game_data_t, party_2) + offsetof(struct gen2_party, mons),
    sizeof(struct gen2_mon),
    offsetof(struct game_data_t, party_2) + offsetof(struct gen2_party, total),
    offsetof(struct gen2_mon, data), sizeof(struct gen2_mon_data),
    offsetof(struct gen2_mon, ot_name_jp), STRING_GEN2_JP_SIZE+1,
    offsetof(struct gen2_mon, nickname_jp), MAIL_GEN2_JP_SIZE,
    MAIL_GEN2_JP_SIZE, MAIL_PATCH_SET_JP_SIZE, MAIL_PATCH_SET_JP_START,
    offsetof(struct gen2_mon, is_egg)}
};

// Implemented this in case it may save some bytes...?

void buffer_loader(struct game_data_t* gd, u32* buffer, u16* sizes, u8 index) {
    int pos = 0;
    u8* td = (u8*)buffer;
    
    const u32* positions_party = positions_party_multi[index&3];
    
    u8* party_members = ((u8*)gd) + positions_party[POS_PARTY_MEMBERS];
    u8 total_mons = *(((u8*)gd) + positions_party[POS_TOTAL_MONS]);
    if(total_mons > PARTY_SIZE)
        total_mons = PARTY_SIZE;
    int names_size = positions_party[SIZE_NAMES];
    int member_size = positions_party[SIZE_MEMBER];
    int data_size = positions_party[SIZE_MON_DATA];
    int mail_size = positions_party[SIZE_MAIL];
    int mail_patch_size = positions_party[SIZE_MAIL_PATCH];
    u8 is_jp = index & 1;
    u8 curr_gen = ((index>>1) & 1)+1;
    
    for(int i = 0; i < RANDOM_DATA_SIZE; i++)
        td[i+pos] = DEFAULT_FILLER;
    
    pos += RANDOM_DATA_SIZE;
    
    text_gen3_to_gen12(gd->trainer_name, td+pos, OT_NAME_GEN3_SIZE+1, names_size, gd->game_identifier.game_is_jp, is_jp);
    td[pos+names_size-1] = GEN2_EOL;
    pos += names_size;
    td[pos++] = total_mons;
    for(int i = 0; i < PARTY_SIZE; i++) {
        if(i < total_mons) {
            if((curr_gen == 2) && (party_members + (i*member_size) + positions_party[POS_IS_EGG])[0])
                td[pos++] = GEN2_EGG;
            else
                td[pos++] = (party_members + (i*member_size) + positions_party[POS_MON_DATA])[0];
        }
        else
            td[pos++] = GEN2_NO_MON;
    }
    td[pos++] = GEN2_NO_MON;
    
    int start_patch_set = pos;
    
    if(curr_gen == 2) {
        td[pos++] = (gd->trainer_id&0xFFFF)>>8;
        td[pos++] = (gd->trainer_id&0xFF);
    }
    
    for(int i = 0; i < PARTY_SIZE; i++) {
        copy_bytes(party_members + (i*member_size) + positions_party[POS_MON_DATA], td + pos, data_size, 0, 0);
        pos += data_size;
    }
    
    for(int i = 0; i < PARTY_SIZE; i++) {
        copy_bytes(party_members + (i*member_size) + positions_party[POS_OT_NAME], td + pos, names_size, 0, 0);
        td[pos+names_size-1] = GEN2_EOL;
        pos += names_size;
    }
    
    for(int i = 0; i < PARTY_SIZE; i++) {
        copy_bytes(party_members + (i*member_size) + positions_party[POS_NICKNAME], td + pos, names_size, 0, 0);
        td[pos+names_size-1] = GEN2_EOL;
        pos += names_size;
    }
    
    for(int i = 0; i < SAFETY_BYTES_NUM; i++)
        td[pos++] = DEFAULT_FILLER;
    
    int size_of_patch_section = pos - start_patch_set;
    int pos_of_patch_section = pos;
    prepare_patch_set(td + RANDOM_DATA_SIZE, td + pos, size_of_patch_section, start_patch_set-RANDOM_DATA_SIZE, PATCH_SET_SIZE, PATCH_SET_BASE_POS);
    
    pos += PATCH_SET_SIZE;
    
    if(curr_gen == 2) {
        for(int i = 0; i < USELESS_SYNC_BYTES; i++)
            td[pos++] = USELESS_SYNC_VALUE;
        prepare_mail_gen2(td+pos, mail_size, td+pos+positions_party[POS_MAIL_PATCH], mail_patch_size, positions_party[START_MAIL_PATCH]);
        pos += mail_size + mail_patch_size;
    }
    
    sizes[0] = RANDOM_DATA_SIZE;
    sizes[1] = pos_of_patch_section-RANDOM_DATA_SIZE;
    sizes[2] = pos - pos_of_patch_section;
}

#else

void load_names_gen12(struct game_data_t* game_data, u8* trainer_name, u8* ot_names, u8* nicknames, u8 is_jp, u8 curr_gen){
    u8 size = STRING_GEN2_INT_SIZE;
    if(is_jp)
        size = STRING_GEN2_JP_SIZE;
        
    text_gen3_to_gen12(game_data->trainer_name, trainer_name, OT_NAME_GEN3_SIZE+1, size, game_data->game_identifier.game_is_jp, is_jp);
    trainer_name[size-1] = GEN2_EOL;
    
    for(int i = 0; i < PARTY_SIZE; i++) {
        u8* src_ot_name;
        u8* src_nickname;
        if(is_jp && curr_gen == 2) {
            src_ot_name = (u8*)&game_data->party_2.mons[i].ot_name_jp;
            src_nickname = (u8*)&game_data->party_2.mons[i].nickname_jp;
        }
        else if(curr_gen == 2) {
            src_ot_name = (u8*)&game_data->party_2.mons[i].ot_name;
            src_nickname = (u8*)&game_data->party_2.mons[i].nickname;
        }
        else if(is_jp) {
            src_ot_name = (u8*)&game_data->party_1.mons[i].ot_name_jp;
            src_nickname = (u8*)&game_data->party_1.mons[i].nickname_jp;
        }
        else {
            src_ot_name = (u8*)&game_data->party_1.mons[i].ot_name;
            src_nickname = (u8*)&game_data->party_1.mons[i].nickname;
        }
        copy_bytes(src_ot_name, ot_names, size, 0, i*size);
        ot_names[(i*size)+size-1] = GEN2_EOL;
        copy_bytes(src_nickname, nicknames, size, 0, i*size);
        nicknames[(i*size)+size-1] = GEN2_EOL;
    }
}

void load_party_info_gen2(struct game_data_t* game_data, struct gen2_party_info* party_info){
    u8 num_options = game_data->party_2.total;
    if(num_options > PARTY_SIZE)
        num_options = PARTY_SIZE;
    party_info->num_mons = num_options;
    
    for(int i = 0; i < num_options; i++) {
        copy_bytes(&game_data->party_2.mons[i].data, &(party_info->mons_data[i]), sizeof(struct gen2_mon_data), 0, 0);
        party_info->mons_index[i] = game_data->party_2.mons[i].data.species;
        if(game_data->party_2.mons[i].is_egg)
            party_info->mons_index[i] = GEN2_EGG;
    }
    
    for(int i = num_options; i < MON_INDEX_SIZE; i++)
        party_info->mons_index[i] = GEN2_NO_MON;
    
    party_info->trainer_id = (game_data->trainer_id&0xFFFF);
}

void load_party_info_gen1(struct game_data_t* game_data, struct gen1_party_info* party_info){
    u8 num_options = game_data->party_1.total;
    if(num_options > PARTY_SIZE)
        num_options = PARTY_SIZE;
    party_info->num_mons = num_options;
    
    for(int i = 0; i < num_options; i++) {
        copy_bytes(&game_data->party_1.mons[i].data, &(party_info->mons_data[i]), sizeof(struct gen1_mon_data), 0, 0);
        party_info->mons_index[i] = game_data->party_1.mons[i].data.species;
    }
    
    for(int i = num_options; i < MON_INDEX_SIZE; i++)
        party_info->mons_index[i] = GEN2_NO_MON;
}

void prepare_gen2_trade_data(struct game_data_t* game_data, u32* buffer, u8 is_jp, u16* sizes) {
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
    
    load_names_gen12(game_data, trainer_name, ot_names, nicknames, is_jp, 2);
    
    struct gen2_party_info* party_info = (struct gen2_party_info*)&td_int->trainer_info.party_info;
    if(is_jp)
        party_info = (struct gen2_party_info*)&td_jp->trainer_info.party_info;
    
    load_party_info_gen2(game_data, party_info);
    
    u8* safety_bytes = (u8*)td_int->trainer_info.safety_bytes;
    if(is_jp)
        safety_bytes = (u8*)td_jp->trainer_info.safety_bytes;
    for(int i = 0; i < SAFETY_BYTES_NUM; i++)
        safety_bytes[i] = DEFAULT_FILLER;
    
    if(!is_jp)
        prepare_patch_set((u8*)(&td_int->trainer_info), td_int->patch_set.patch_set, sizeof(struct trainer_data_gen2_int)-(STRING_GEN2_INT_SIZE + MON_INDEX_SIZE + 1), STRING_GEN2_INT_SIZE + MON_INDEX_SIZE + 1, PATCH_SET_SIZE, PATCH_SET_BASE_POS);
    else
        prepare_patch_set((u8*)(&td_jp->trainer_info), td_jp->patch_set.patch_set, sizeof(struct trainer_data_gen2_jp)-(STRING_GEN2_JP_SIZE + MON_INDEX_SIZE + 1), STRING_GEN2_JP_SIZE + MON_INDEX_SIZE + 1, PATCH_SET_SIZE, PATCH_SET_BASE_POS);
    
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

void prepare_gen1_trade_data(struct game_data_t* game_data, u32* buffer, u8 is_jp, u16* sizes) {
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
    
    load_names_gen12(game_data, trainer_name, ot_names, nicknames, is_jp, 1);
    
    struct gen1_party_info* party_info = (struct gen1_party_info*)&td_int->trainer_info.party_info;
    if(is_jp)
        party_info = (struct gen1_party_info*)&td_jp->trainer_info.party_info;
    
    load_party_info_gen1(game_data, party_info);
    
    u8* safety_bytes = (u8*)td_int->trainer_info.safety_bytes;
    if(is_jp)
        safety_bytes = (u8*)td_jp->trainer_info.safety_bytes;
    for(int i = 0; i < SAFETY_BYTES_NUM; i++)
        safety_bytes[i] = DEFAULT_FILLER;
    
    if(!is_jp)
        prepare_patch_set((u8*)(&td_int->trainer_info), td_int->patch_set.patch_set, sizeof(struct trainer_data_gen1_int)-(STRING_GEN2_INT_SIZE + MON_INDEX_SIZE + 1), STRING_GEN2_INT_SIZE + MON_INDEX_SIZE + 1, PATCH_SET_SIZE, PATCH_SET_BASE_POS);
    else
        prepare_patch_set((u8*)(&td_jp->trainer_info), td_jp->patch_set.patch_set, sizeof(struct trainer_data_gen1_jp)-(STRING_GEN2_JP_SIZE + MON_INDEX_SIZE + 1), STRING_GEN2_JP_SIZE + MON_INDEX_SIZE + 1, PATCH_SET_SIZE, PATCH_SET_BASE_POS);
    
    sizes[0] = sizeof(struct random_data_t);
    if(!is_jp)
        sizes[1] = sizeof(struct trainer_data_gen1_int);
    else
        sizes[1] = sizeof(struct trainer_data_gen1_jp);
    sizes[2] = sizeof(struct patch_set_trainer_data_gen12);
}

#endif

u8 are_checksum_same_gen3(struct gen3_trade_data* td) {
    u32 checksum = 0;
    for(int i = 0; i < PARTY_SIZE; i++) {
        u32* mail_buf = &td->mails_3[i];
        for(int i = 0; i < (sizeof(struct mail_gen3)>>2); i++)
            checksum += mail_buf[i];
    }
    
    if(td->checksum_mail != checksum)
        return 0;

    checksum = 0;
    
    u32* party_buf = &td->party_3;
    for(int i = 0; i < (sizeof(struct gen3_party)>>2); i++)
        checksum += party_buf[i];
    
    if(td->checksum_party != checksum)
        return 0;
    
    checksum = 0;
    u32* buffer = (u32*)td;
    
    for(int i = 0; i < ((sizeof(struct gen3_trade_data) - 4)>>2); i++)
        checksum += buffer[i];
    
    if(td->final_checksum != checksum)
        return 0;
    
    return 1;
}

void prepare_gen3_trade_data(struct game_data_t* game_data, u32* buffer, u16* sizes) {
    struct gen3_trade_data* td = (struct gen3_trade_data*)buffer;
    
    u32 checksum = 0;
    for(int i = 0; i < PARTY_SIZE; i++) {
        copy_bytes(&game_data->mails_3[i], &td->mails_3[i], sizeof(struct mail_gen3), 0, 0);
        u32* mail_buf = &td->mails_3[i];
        for(int i = 0; i < (sizeof(struct mail_gen3)>>2); i++)
            checksum += mail_buf[i];
    }
    
    td->checksum_mail = checksum;
    checksum = 0;
    
    copy_bytes(&game_data->party_3, &td->party_3, sizeof(struct gen3_party), 0, 0);
    u32* party_buf = &td->party_3;
    for(int i = 0; i < (sizeof(struct gen3_party)>>2); i++)
        checksum += party_buf[i];
    
    td->checksum_party = checksum;
    checksum = 0;
    
    td->game_main_code = game_data->game_identifier.game_main_version;
    td->game_sub_code = game_data->game_identifier.game_sub_version;
    td->game_is_jp = game_data->game_identifier.game_is_jp;
    
    copy_bytes(game_data->giftRibbons, td->giftRibbons, GIFT_RIBBONS, 0, 0);
    copy_bytes(game_data->trainer_name, td->trainer_name, OT_NAME_GEN3_SIZE+1, 0, 0);
    td->trainer_gender = game_data->trainer_gender;
    
    for(int i = 0; i < NUM_EXTRA_PADDING_BYTES_GEN3; i++) {
        td->extra_tmp_padding[i] = 0;
    }
    
    td->trainer_id = game_data->trainer_id;
    
    for(int i = 0; i < ((sizeof(struct gen3_trade_data) - 4)>>2); i++)
        checksum += buffer[i];
    
    td->final_checksum = checksum;
    
    sizes[0] = sizeof(struct gen3_trade_data);
}

void load_comm_buffer(struct game_data_t* game_data, int curr_gen, u8 is_jp) {
    
    for(int i = 0; i < NUM_SIZES; i++)
        buffer_sizes[i] = SIZE_STOP;

    #if EACH_DIFFERENT
    switch(curr_gen) {
        case 1:
            prepare_gen1_trade_data(game_data, communication_buffers[OWN_BUFFER], is_jp, buffer_sizes);
            break;
        case 2:
            prepare_gen2_trade_data(game_data, communication_buffers[OWN_BUFFER], is_jp, buffer_sizes);
            break;
        default:
            prepare_gen3_trade_data(game_data, communication_buffers[OWN_BUFFER], buffer_sizes);
            break;
    }
    #else
    switch(curr_gen) {
        case 1:
            buffer_loader(game_data, communication_buffers[OWN_BUFFER], buffer_sizes, (is_jp&1));
            break;
        case 2:
            buffer_loader(game_data, communication_buffers[OWN_BUFFER], buffer_sizes, 2|(is_jp&1));
            break;
        default:
            prepare_gen3_trade_data(game_data, communication_buffers[OWN_BUFFER], buffer_sizes);
            break;
    }
    #endif
    prepare_number_of_sizes();
}
