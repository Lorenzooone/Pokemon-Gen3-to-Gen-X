#ifndef PARTY_HANDLER__
#define PARTY_HANDLER__

#define LAST_VALID_GEN_3_MON 411
#define LAST_VALID_GEN_2_MON 251
#define LAST_VALID_GEN_1_MON 151
#define LAST_VALID_GEN_3_MOVE 354
#define LAST_VALID_GEN_2_MOVE 251
#define LAST_VALID_GEN_1_MOVE 165
#define LAST_VALID_GEN_3_ITEM 376
#define MAX_LEVEL 100
#define MIN_LEVEL 1
#define BASE_FRIENDSHIP 70
#define ENC_DATA_SIZE 48

struct exp_level {
    u32 exp_kind[6];
};

struct stats_gen_23 {
    u8 stats[6];
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
    u16 moves[4];
    u8 pp[4];
};

struct gen3_mon_evs {
    u8 evs[6];
    u8 contest[6];
};

struct gen3_mon_misc {
    u8 pokerus;
    u8 met_location;
    u16 origins_info;
    u8 hp_ivs : 5;
    u8 atk_ivs : 5;
    u8 def_ivs : 5;
    u8 spe_ivs : 5;
    u8 spa_ivs : 5;
    u8 spd_ivs : 5;
    u8 is_egg : 1;
    u8 ability : 1;
    u32 ribbons;
};

struct gen3_mon {
    u32 pid;
    u32 ot_id;
    u8 nickname[10];
    u8 language;
    u8 is_bad_egg : 1;
    u8 has_species : 1;
    u8 use_egg_name : 1;
    u8 unused : 5;
    u8 ot_name[7];
    u8 marks;
    u16 checksum;
    u16 unk;
    u32 enc_data[ENC_DATA_SIZE>>2];
    u32 status;
    u8 level;
    u8 pokerus_rem;
    u16 stats[7];
};

struct gen2_mon {
    u8 species;
    u8 item;
    u8 moves[4];
    u16 ot_id;
    u8 exp[3];
    u16 evs[5];
    u16 ivs;
    u8 pps[4];
    u8 friendship;
    u8 pokerus;
    u16 data;
    u8 level;
    u8 status;
    u8 unused;
    u16 stats[7];
    u8 is_egg; // Extra byte of data we keep
    u8 ot_name[11];
    u8 nickname[11];
};

struct gen1_mon {
    u8 species;
    u16 curr_hp;
    u8 bad_level;
    u8 status;
    u8 type[2];
    u8 item;
    u8 moves[4];
    u16 ot_id;
    u8 exp[3];
    u16 evs[5];
    u16 ivs;
    u8 pps[4];
    u8 level;
    u16 stats[5];
    u8 ot_name[11];
    u8 nickname[11];
};

struct gen3_party {
    u32 total;
    struct gen3_mon mons[6];
};

struct gen2_party {
    u8 total;
    struct gen2_mon mons[6];
};

struct gen1_party {
    u8 total;
    struct gen1_mon mons[6];
};

u8 gen3_to_gen2(struct gen2_mon* dst, struct gen3_mon* src);
void init_sprite_counter();
u8 get_sprite_counter();

#endif