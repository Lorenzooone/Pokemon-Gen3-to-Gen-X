#ifndef PARTY_HANDLER__
#define PARTY_HANDLER__

#define LAST_VALID_GEN_3_MON 411
#define LAST_VALID_GEN_2_MON 251
#define LAST_VALID_GEN_1_MON 151
#define LAST_VALID_GEN_3_MOVE 354
#define LAST_VALID_GEN_2_MOVE 251
#define LAST_VALID_GEN_1_MOVE 165
#define LAST_VALID_GEN_3_ITEM 376

#define M_GENDER 0
#define F_GENDER 1
#define U_GENDER 2

#define NIDORAN_M_SPECIES 32
#define NIDORAN_F_SPECIES 29

#define MR_MIME_SPECIES 122
#define MR_MIME_OLD_NAME_POS 445
#define UNOWN_SPECIES 201
#define UNOWN_REAL_NAME_POS 446
#define DEOXYS_SPECIES 410
#define MEW_SPECIES 151
#define GEN2_DOT 0xE8
#define GEN1_DOT 0xF2

#define MAX_LEVEL 100
#define MIN_LEVEL 1
#define BASE_FRIENDSHIP 70
#define ENC_DATA_SIZE 48
#define PARTY_SIZE 6
#define MOVES_SIZE 4

#define DEX_BYTES ((LAST_VALID_GEN_3_MON & 7) == 0 ? LAST_VALID_GEN_3_MON >> 3 : (LAST_VALID_GEN_3_MON >> 3) + 1)
#define GIFT_RIBBONS 11
#define MAIL_WORDS_SIZE 9

#define NAME_SIZE 11
#define ITEM_NAME_SIZE 15
#define NICKNAME_GEN3_SIZE 10
#define OT_NAME_GEN3_SIZE 7
#define OT_NAME_JP_GEN3_SIZE 5
#define STRING_GEN2_INT_SIZE 11
#define STRING_GEN2_JP_SIZE 6
#define STRING_GEN2_INT_CAP (STRING_GEN2_INT_SIZE-1)
#define STRING_GEN2_JP_CAP (STRING_GEN2_JP_SIZE-1)

#define JAPANESE_LANGUAGE 1

struct mail_gen3 {
    u16 words[MAIL_WORDS_SIZE];
    u8 ot_name[OT_NAME_GEN3_SIZE+1];
    u32 ot_id;
    u16 species;
    u16 item;
    u16 unk;
} __attribute__ ((packed)) __attribute__ ((aligned(4)));

struct exp_level {
    u32 exp_kind[6];
};

struct stats_gen_23 {
    u8 stats[6];
};

struct stats_gen_1 {
    u8 stats[5];
};

struct gen3_mon_growth {
    u16 species;
    u16 item;
    u32 exp;
    u8 pp_bonuses;
    u8 friendship;
    u16 unk;
};

struct gen3_mon_attacks {
    u16 moves[MOVES_SIZE];
    u8 pp[MOVES_SIZE];
};

struct gen3_mon_evs {
    u8 evs[6];
    u8 contest[6];
};

struct gen3_mon_misc {
    u8 pokerus;
    u8 met_location;
    u16 origins_info;
    u32 hp_ivs : 5;
    u32 atk_ivs : 5;
    u32 def_ivs : 5;
    u32 spe_ivs : 5;
    u32 spa_ivs : 5;
    u32 spd_ivs : 5;
    u32 is_egg : 1;
    u32 ability : 1;
    u32 ribbons;
};

struct gen3_mon_data_undec {
    struct gen3_mon* src;
    struct gen3_mon_growth growth;
    struct gen3_mon_attacks attacks;
    struct gen3_mon_evs evs;
    struct gen3_mon_misc misc;
    u8 is_valid;
};

struct gen3_mon {
    u32 pid;
    u32 ot_id;
    u8 nickname[NICKNAME_GEN3_SIZE];
    u8 language;
    u8 is_bad_egg : 1;
    u8 has_species : 1;
    u8 use_egg_name : 1;
    u8 unused : 5;
    u8 ot_name[OT_NAME_GEN3_SIZE];
    u8 marks;
    u16 checksum;
    u16 unk;
    u32 enc_data[ENC_DATA_SIZE>>2];
    u32 status;
    u8 level;
    u8 pokerus_rem;
    u16 stats[7];
} __attribute__ ((packed)) __attribute__ ((aligned(4)));

struct gen2_mon {
    u8 species;
    u8 item;
    u8 moves[MOVES_SIZE];
    u16 ot_id;
    u8 exp[3];
    u16 evs[5];
    u16 ivs;
    u8 pps[MOVES_SIZE];
    u8 friendship;
    u8 pokerus;
    u16 data;
    u8 level;
    u8 status;
    u8 unused;
    u16 curr_hp;
    u16 stats[6];
    u8 is_egg; // Extra byte of data we keep
    u8 ot_name[STRING_GEN2_INT_SIZE]; // The last byte of all of these must be set to 0x50
    u8 ot_name_jp[STRING_GEN2_JP_SIZE];
    u8 nickname[STRING_GEN2_INT_SIZE];
    u8 nickname_jp[STRING_GEN2_JP_SIZE];
} __attribute__ ((packed)) __attribute__ ((aligned(2)));

struct gen1_mon {
    u8 species;
    u16 curr_hp;
    u8 bad_level;
    u8 status;
    u8 type[2];
    u8 item;
    u8 moves[MOVES_SIZE];
    u16 ot_id;
    u8 exp[3];
    u16 evs[5];
    u16 ivs;
    u8 pps[MOVES_SIZE];
    u8 level;
    u16 stats[5];
    u8 ot_name[STRING_GEN2_INT_SIZE]; // The last byte of all of these must be set to 0x50
    u8 ot_name_jp[STRING_GEN2_JP_SIZE];
    u8 nickname[STRING_GEN2_INT_SIZE];
    u8 nickname_jp[STRING_GEN2_JP_SIZE];
} __attribute__ ((packed)) __attribute__ ((aligned(2)));

struct gen3_party {
    u32 total;
    struct gen3_mon mons[PARTY_SIZE];
};

struct gen2_party {
    u8 total;
    struct gen2_mon mons[PARTY_SIZE];
};

struct gen1_party {
    u8 total;
    struct gen1_mon mons[PARTY_SIZE];
};

void process_gen3_data(struct gen3_mon*, struct gen3_mon_data_undec*);
u8 gen3_to_gen2(struct gen2_mon*, struct gen3_mon_data_undec*, u32);
u8 gen3_to_gen1(struct gen1_mon*, struct gen3_mon_data_undec*, u32);
const u8* get_pokemon_name_raw(struct gen3_mon_data_undec*);
u16 get_mon_index_raw(struct gen3_mon_data_undec*);
const u8* get_item_name_raw(struct gen3_mon_data_undec*);
u8 is_egg_gen3_raw(struct gen3_mon_data_undec*);
void load_pokemon_sprite_raw(struct gen3_mon_data_undec*, u16, u16);
u8 get_pokemon_gender_raw(struct gen3_mon_data_undec*);
char get_pokemon_gender_char_raw(struct gen3_mon_data_undec*);

#endif