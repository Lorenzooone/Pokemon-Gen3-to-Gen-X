#ifndef COMMUNICATOR_
#define COMMUNICATOR_

#include "party_handler.h"
#include "gen3_save.h"

// We oversize it to have some wiggle room
#define BUFFER_SIZE 0x500

#define DEFAULT_FILLER 0

#define NUM_SIZES 5
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
} __attribute__ ((packed));

struct gen2_party_info {
    u8 num_mons;
    u8 mons_index[MON_INDEX_SIZE];
    u16 trainer_id;
    struct gen2_mon_data mons_data[PARTY_SIZE];
} __attribute__ ((packed));

struct gen1_party_info {
    u8 num_mons;
    u8 mons_index[MON_INDEX_SIZE];
    struct gen1_mon_data mons_data[PARTY_SIZE];
} __attribute__ ((packed));

struct trainer_data_gen2_int {
    u8 trainer_name[STRING_GEN2_INT_SIZE];
    struct gen2_party_info party_info;
    u8 ot_names[PARTY_SIZE][STRING_GEN2_INT_SIZE];
    u8 nicknames[PARTY_SIZE][STRING_GEN2_INT_SIZE];
    u8 safety_bytes[SAFETY_BYTES_NUM];
} __attribute__ ((packed));

struct trainer_data_gen2_jp {
    u8 trainer_name[STRING_GEN2_JP_SIZE];
    struct gen2_party_info party_info;
    u8 ot_names[PARTY_SIZE][STRING_GEN2_JP_SIZE];
    u8 nicknames[PARTY_SIZE][STRING_GEN2_JP_SIZE];
    u8 safety_bytes[SAFETY_BYTES_NUM];
} __attribute__ ((packed));

struct trainer_data_gen1_int {
    u8 trainer_name[STRING_GEN2_INT_SIZE];
    struct gen1_party_info party_info;
    u8 ot_names[PARTY_SIZE][STRING_GEN2_INT_SIZE];
    u8 nicknames[PARTY_SIZE][STRING_GEN2_INT_SIZE];
    u8 safety_bytes[SAFETY_BYTES_NUM];
} __attribute__ ((packed));

struct trainer_data_gen1_jp {
    u8 trainer_name[STRING_GEN2_JP_SIZE];
    struct gen1_party_info party_info;
    u8 ot_names[PARTY_SIZE][STRING_GEN2_JP_SIZE];
    u8 nicknames[PARTY_SIZE][STRING_GEN2_JP_SIZE];
    u8 safety_bytes[SAFETY_BYTES_NUM];
} __attribute__ ((packed));

struct patch_set_trainer_data_gen12 {
    u8 patch_set[PATCH_SET_SIZE];
} __attribute__ ((packed));

struct party_mail_data_gen2_int {
    u8 gen2_mail_data[MAIL_GEN2_INT_SIZE];
    u8 patch_set[MAIL_PATCH_SET_INT_SIZE];
} __attribute__ ((packed));

struct party_mail_data_gen2_jp {
    // Need to look into it, again for the size...
    u8 gen2_mail_data[MAIL_GEN2_JP_SIZE];
    u8 patch_set[MAIL_PATCH_SET_JP_SIZE];
} __attribute__ ((packed));

struct gen2_trade_data_int {
    struct random_data_t random_data;
    struct trainer_data_gen2_int trainer_info;
    struct patch_set_trainer_data_gen12 patch_set;
    struct party_mail_data_gen2_int mail;
} __attribute__ ((packed));

struct gen2_trade_data_jp {
    struct random_data_t random_data;
    struct trainer_data_gen2_jp trainer_info;
    struct patch_set_trainer_data_gen12 patch_set;
    struct party_mail_data_gen2_jp mail;
} __attribute__ ((packed));

struct gen1_trade_data_int {
    struct random_data_t random_data;
    struct trainer_data_gen1_int trainer_info;
    struct patch_set_trainer_data_gen12 patch_set;
} __attribute__ ((packed));

struct gen1_trade_data_jp {
    struct random_data_t random_data;
    struct trainer_data_gen1_jp trainer_info;
    struct patch_set_trainer_data_gen12 patch_set;
} __attribute__ ((packed));

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
} __attribute__ ((packed));

void load_comm_buffer(struct game_data_t*, u16*, int, u8);
u32* get_communication_buffer(u8);

#endif