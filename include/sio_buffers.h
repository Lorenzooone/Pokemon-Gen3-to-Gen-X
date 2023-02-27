#ifndef SIO_BUFFERS__
#define SIO_BUFFERS__

#include "party_handler.h"
#include "gen3_save.h"
#include "useful_qualifiers.h"
#include <stddef.h>

// We oversize it to have some wiggle room
#define BUFFER_SIZE 0x500

#define DEFAULT_FILLER 0

#define USELESS_SYNC_BYTES 6
#define USELESS_SYNC_VALUE 0x20

#define NUM_SIZES 4
#define SIZE_STOP 0xFFFF

#define NUM_EXTRA_PADDING_BYTES_GEN3 0x1F

#define OWN_BUFFER 0
#define OTHER_BUFFER 1

#define RANDOM_DATA_SIZE 10
#define PATCH_SET_SIZE 0xC5
#define PATCH_SET_BASE_POS 7
#define MAIL_GEN2_INT_SIZE 0x11A
#define MAIL_PATCH_SET_INT_SIZE 0x67
#define MAIL_PATCH_SET_INT_START 0xC6
// These should be right
#define MAIL_GEN2_JP_SIZE 0xFC
#define MAIL_PATCH_SET_JP_SIZE 0x21
#define MAIL_PATCH_SET_JP_START 0

#define NO_ACTION_BYTE 0xFE

#define MON_INDEX_SIZE (PARTY_SIZE+1)
#define SAFETY_BYTES_NUM 3

struct random_data_t {
    u8 data[RANDOM_DATA_SIZE];
} PACKED;

struct gen2_party_info {
    u8 num_mons;
    u8 mons_index[MON_INDEX_SIZE];
    u16 trainer_id;
    struct gen2_mon_data mons_data[PARTY_SIZE];
} PACKED;

struct gen1_party_info {
    u8 num_mons;
    u8 mons_index[MON_INDEX_SIZE];
    struct gen1_mon_data mons_data[PARTY_SIZE];
} PACKED;

struct trainer_data_gen2_int {
    u8 trainer_name[STRING_GEN2_INT_SIZE];
    struct gen2_party_info party_info;
    u8 ot_names[PARTY_SIZE][STRING_GEN2_INT_SIZE];
    u8 nicknames[PARTY_SIZE][STRING_GEN2_INT_SIZE];
    u8 safety_bytes[SAFETY_BYTES_NUM];
} PACKED;

struct trainer_data_gen2_jp {
    u8 trainer_name[STRING_GEN2_JP_SIZE];
    struct gen2_party_info party_info;
    u8 ot_names[PARTY_SIZE][STRING_GEN2_JP_SIZE];
    u8 nicknames[PARTY_SIZE][STRING_GEN2_JP_SIZE];
    u8 safety_bytes[SAFETY_BYTES_NUM];
} PACKED;

struct trainer_data_gen1_int {
    u8 trainer_name[STRING_GEN2_INT_SIZE];
    struct gen1_party_info party_info;
    u8 ot_names[PARTY_SIZE][STRING_GEN2_INT_SIZE];
    u8 nicknames[PARTY_SIZE][STRING_GEN2_INT_SIZE];
    u8 safety_bytes[SAFETY_BYTES_NUM];
} PACKED;

struct trainer_data_gen1_jp {
    u8 trainer_name[STRING_GEN2_JP_SIZE];
    struct gen1_party_info party_info;
    u8 ot_names[PARTY_SIZE][STRING_GEN2_JP_SIZE];
    u8 nicknames[PARTY_SIZE][STRING_GEN2_JP_SIZE];
    u8 safety_bytes[SAFETY_BYTES_NUM];
} PACKED;

struct patch_set_trainer_data_gen12 {
    u8 patch_set[PATCH_SET_SIZE];
} PACKED;

struct party_mail_data_gen2_int {
    u8 gen2_mail_data[MAIL_GEN2_INT_SIZE];
    u8 patch_set[MAIL_PATCH_SET_INT_SIZE];
} PACKED;

struct party_mail_data_gen2_jp {
    // Need to look into it, again for the size...
    u8 gen2_mail_data[MAIL_GEN2_JP_SIZE];
    u8 patch_set[MAIL_PATCH_SET_JP_SIZE];
} PACKED;

struct gen2_trade_data_int {
    struct random_data_t random_data;
    struct trainer_data_gen2_int trainer_info;
    struct patch_set_trainer_data_gen12 patch_set;
    u8 useless_sync[USELESS_SYNC_BYTES];
    struct party_mail_data_gen2_int mail;
} PACKED;

struct gen2_trade_data_jp {
    struct random_data_t random_data;
    struct trainer_data_gen2_jp trainer_info;
    struct patch_set_trainer_data_gen12 patch_set;
    u8 useless_sync[USELESS_SYNC_BYTES];
    struct party_mail_data_gen2_jp mail;
} PACKED;

struct gen1_trade_data_int {
    struct random_data_t random_data;
    struct trainer_data_gen1_int trainer_info;
    struct patch_set_trainer_data_gen12 patch_set;
} PACKED;

struct gen1_trade_data_jp {
    struct random_data_t random_data;
    struct trainer_data_gen1_jp trainer_info;
    struct patch_set_trainer_data_gen12 patch_set;
} PACKED;

struct gen3_trade_data {
    struct mail_gen3 mails_3[PARTY_SIZE];
    u32 checksum_mail;
    struct gen3_party party_3;
    u32 checksum_party;
    u8 game_main_code : 4;
    u8 game_sub_code : 2;
    u8 game_is_jp : 2;
    u8 giftRibbons[GIFT_RIBBONS];
    u8 trainer_name[OT_NAME_GEN3_SIZE+1];
    u8 trainer_gender;
    u8 extra_tmp_padding[NUM_EXTRA_PADDING_BYTES_GEN3];
    u32 trainer_id;
    u32 final_checksum;
} PACKED ALIGNED(4);

void load_comm_buffer(struct game_data_t*, struct gen2_party*, struct gen1_party*, int, u8);
void read_comm_buffer(struct game_data_t*, int, u8);

u8 are_checksum_same_gen3(struct gen3_trade_data*);
u32* get_communication_buffer(u8);
u8 get_number_of_buffers(void);
size_t get_buffer_size(int);
size_t* get_buffer_sizes(void);

#endif
