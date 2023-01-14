#ifndef PARTY_HANDLER__
#define PARTY_HANDLER__

#define TOTAL_GENS 3
#define FIRST_GEN 1

#define LAST_VALID_GEN_3_MON 411
#define LAST_VALID_GEN_2_MON 251
#define LAST_VALID_GEN_1_MON 151
#define LAST_VALID_GEN_3_MOVE 354
#define LAST_VALID_GEN_2_MOVE 251
#define LAST_VALID_GEN_1_MOVE 165
#define LAST_VALID_GEN_3_ITEM 376
#define GEN2_STATS_TOTAL 6
#define GEN1_STATS_TOTAL 5

#define GEN3_NO_ITEM 0xFFFF
#define GEN2_NO_ITEM 0xFF
#define GEN2_MAIL 0xFE

#define M_GENDER 0
#define F_GENDER 1
#define U_GENDER 2
#define NIDORAN_M_GENDER_INDEX 8
#define NIDORAN_F_GENDER_INDEX 9

#define NO_POKERUS 0
#define HAS_POKERUS 1
#define HAD_POKERUS 2

#define LAST_RIBBON_CONTEST 4
#define LAST_RIBBON 16
#define NUM_RIBBONS (LAST_RIBBON+1)
#define NO_RANK_ID 1
#define NO_RIBBON_ID 21
#define CONTEST_STATS_TOTAL 6

#define COLOSSEUM_CODE 15
#define RUBY_CODE 2
#define EMERALD_CODE 3
#define BATTLE_FACILITY 0x3A
#define HIDEOUT 0x42
#define DEPT_STORE 0xC4
#define EMPTY_LOCATION 0xD5
#define BATTLE_FACILITY_ALT 0x101
#define HIDEOUT_ALT 0x100
#define DEPT_STORE_ALT 0x102
#define COLOSSEUM_ALT 0x103
#define TRADE_MET 0xFE

#define NIDORAN_M_SPECIES 32
#define NIDORAN_F_SPECIES 29

#define MR_MIME_SPECIES 122
#define MR_MIME_OLD_NAME_POS 445
#define UNOWN_SPECIES 201
#define UNOWN_REAL_NAME_POS 446
#define DEOXYS_SPECIES 410
#define DEOXYS_NORMAL 0
#define DEOXYS_ATK 1
#define DEOXYS_DEF 2
#define DEOXYS_SPE 3
#define DEOXYS_FORMS_POS 442
#define MEW_SPECIES 151
#define RAIKOU_SPECIES 243
#define SUICUNE_SPECIES 245
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

#define MAX_EVS 510
#define HP_STAT_INDEX 0

#define NUM_UNOWN_LETTERS_GEN3 28
#define NUM_UNOWN_LETTERS_GEN2 26
#define NUM_NATURES 25

#define NAME_SIZE 11
#define ITEM_NAME_SIZE 15
#define NICKNAME_GEN3_SIZE 10
#define NICKNAME_JP_GEN3_SIZE 5
#define OT_NAME_GEN3_SIZE 7
#define OT_NAME_JP_GEN3_SIZE 5
#define STRING_GEN2_INT_SIZE 11
#define STRING_GEN2_JP_SIZE 6
#define STRING_GEN2_INT_CAP (STRING_GEN2_INT_SIZE-1)
#define STRING_GEN2_JP_CAP (STRING_GEN2_JP_SIZE-1)

#define GEN2_EGG 253
#define GEN2_NO_MON 255

#define POKEBALL_ID 4

#define EGG_ENCOUNTER 0
#define STATIC_ENCOUNTER 1
#define ROAMER_ENCOUNTER 2
#define UNOWN_ENCOUNTER 3

#define JAPANESE_LANGUAGE 1
#define ENGLISH_LANGUAGE 2

struct mail_gen3 {
    u16 words[MAIL_WORDS_SIZE];
    u8 ot_name[OT_NAME_GEN3_SIZE+1];
    u32 ot_id;
    u16 species;
    u16 item;
    u16 unk;
} __attribute__ ((packed));

struct special_met_data_gen2 {
    u8 location;
    u8 level;
};

struct exp_level {
    u32 exp_kind[6];
};

struct stats_gen_23 {
    u8 stats[GEN2_STATS_TOTAL];
};

struct stats_gen_1 {
    u8 stats[GEN1_STATS_TOTAL];
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
    u8 evs[GEN2_STATS_TOTAL];
    u8 contest[CONTEST_STATS_TOTAL];
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
    u32 ribbons : 31;
    u32 obedience : 1;
};

struct gen3_mon_data_unenc {
    struct gen3_mon* src;
    struct gen3_mon_growth growth;
    struct gen3_mon_attacks attacks;
    struct gen3_mon_evs evs;
    struct gen3_mon_misc misc;
    u8 is_valid_gen3 :1;
    u8 is_valid_gen2 :1;
    u8 is_valid_gen1 :1;
    u8 is_egg :1;
    u8 deoxys_form :2;
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
    u16 curr_hp;
    u16 stats[GEN2_STATS_TOTAL];
} __attribute__ ((packed)) __attribute__ ((aligned(4)));

struct gen2_mon_data {
    u8 species;
    u8 item;
    u8 moves[MOVES_SIZE];
    u16 ot_id;
    u8 exp[3];
    u16 evs[GEN1_STATS_TOTAL];
    u16 ivs;
    u8 pps[MOVES_SIZE];
    u8 friendship;
    u8 pokerus;
    u16 met_level : 6;
    u16 time : 2;
    u16 location : 7;
    u16 ot_gender : 1;
    u8 level;
    u8 status;
    u8 unused;
    u16 curr_hp;
    u16 stats[GEN2_STATS_TOTAL];
} __attribute__ ((packed));

struct gen2_mon {
    struct gen2_mon_data data;
    u8 is_egg; // Extra byte of data we keep
    u8 ot_name[STRING_GEN2_INT_SIZE]; // The last byte of all of these must be set to 0x50
    u8 ot_name_jp[STRING_GEN2_JP_SIZE];
    u8 nickname[STRING_GEN2_INT_SIZE];
    u8 nickname_jp[STRING_GEN2_JP_SIZE];
};

struct gen1_mon_data {
    u8 species;
    u16 curr_hp;
    u8 bad_level;
    u8 status;
    u8 type[2];
    u8 item;
    u8 moves[MOVES_SIZE];
    u16 ot_id;
    u8 exp[3];
    u16 evs[GEN1_STATS_TOTAL];
    u16 ivs;
    u8 pps[MOVES_SIZE];
    u8 level;
    u16 stats[GEN1_STATS_TOTAL];
} __attribute__ ((packed));

struct gen1_mon {
    struct gen1_mon_data data;
    u8 ot_name[STRING_GEN2_INT_SIZE]; // The last byte of all of these must be set to 0x50
    u8 ot_name_jp[STRING_GEN2_JP_SIZE];
    u8 nickname[STRING_GEN2_INT_SIZE];
    u8 nickname_jp[STRING_GEN2_JP_SIZE];
};

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

void process_gen3_data(struct gen3_mon*, struct gen3_mon_data_unenc*, u8, u8);
u8 gen3_to_gen2(struct gen2_mon*, struct gen3_mon_data_unenc*, u32);
u8 gen3_to_gen1(struct gen1_mon*, struct gen3_mon_data_unenc*, u32);
u8 gen2_to_gen3(struct gen2_mon_data*, struct gen3_mon_data_unenc*, u8, u8*, u8*, u8);
u8 gen1_to_gen3(struct gen1_mon_data*, struct gen3_mon_data_unenc*, u8, u8*, u8*, u8);

const u8* get_pokemon_name_raw(struct gen3_mon_data_unenc*);
u16 get_mon_index_raw(struct gen3_mon_data_unenc*);
const u8* get_item_name_raw(struct gen3_mon_data_unenc*);
const u8* get_met_location_name_gen3_raw(struct gen3_mon_data_unenc*);
u8 get_met_level_gen3_raw(struct gen3_mon_data_unenc*);
const u8* get_pokeball_base_name_gen3_raw(struct gen3_mon_data_unenc*);
u8 get_trainer_gender_char_raw(struct gen3_mon_data_unenc*);
u8 is_egg_gen3_raw(struct gen3_mon_data_unenc*);
u8 has_pokerus_gen3_raw(struct gen3_mon_data_unenc*);
void load_pokemon_sprite_raw(struct gen3_mon_data_unenc*, u16, u16);
u8 get_pokemon_gender_raw(struct gen3_mon_data_unenc*);
char get_pokemon_gender_char_raw(struct gen3_mon_data_unenc*);
u8 is_shiny_gen3_raw(struct gen3_mon_data_unenc*, u32);
u8 to_valid_level_gen3(struct gen3_mon*);
u16 calc_stats_gen3_raw(struct gen3_mon_data_unenc*, u8);
u8 get_evs_gen3(struct gen3_mon_evs*, u8);
u8 get_ivs_gen3(struct gen3_mon_misc*, u8);
u8 get_hidden_power_power_gen3(struct gen3_mon_misc*);
const u8* get_hidden_power_type_name_gen3(struct gen3_mon_misc*);
const u8* get_nature_name(u32);
char get_nature_symbol(u32, u8);
const u8* get_move_name_gen3(struct gen3_mon_attacks*, u8);
const u8* get_ability_name_raw(struct gen3_mon_data_unenc*);
const u8* get_ribbon_name(struct gen3_mon_misc*, u8);
const u8* get_ribbon_rank_name(struct gen3_mon_misc*, u8);
u32 get_proper_exp_raw(struct gen3_mon_data_unenc*);
u32 get_level_exp_mon_index(u16, u8);
u8 get_pokemon_gender_gen2(u8, u8, u8, u8);
u8 get_pokemon_gender_kind_gen3(int, u32, u8, u8);
u8 get_pokemon_gender_kind_gen2(u8, u8, u8);

#endif