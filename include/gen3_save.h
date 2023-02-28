#ifndef GEN3_SAVE__
#define GEN3_SAVE__

#include "party_handler.h"
#include "version_identifier.h"
#include "gen3_clock_events_t.h"
#include "useful_qualifiers.h"
#include "save.h"

#define SAVE_SLOT_SIZE 0xE000
#define SECTION_SIZE SECTOR_SIZE
#define MAX_SECTION_SIZE 0xF80
#define SECTION_SYS_FLAGS_ID 2
#define SECTION_VARS_ID 2
#define SECTION_GAME_STATS_ID 2

struct game_data_priv_t {
    u32 seen_unown_pid;
    u32 seen_spinda_pid;
    u8 pokedex_owned[DEX_BYTES];
    u8 pokedex_seen[DEX_BYTES];
    u16 nat_dex_var;
    u8 nat_dex_flag : 1;
    u8 full_link_flag : 1;
    u8 dex_obtained_flag : 1;
    u8 game_cleared_flag : 1;
    u8 nat_dex_magic;
    u8 save_warp_flags;
    u16 curr_map;
    u32 stat_enc_key;
    u32 num_saves_stat;
    u32 num_trades_stat;
    struct gen2_party party_2;
    struct gen1_party party_1;
    struct clock_events_t clock_events;
};

struct game_data_t {
    struct game_identity game_identifier;
    u8 giftRibbons[GIFT_RIBBONS];
    u8 trainer_name[OT_NAME_GEN3_MAX_SIZE+1];
    u8 trainer_gender;
    u32 trainer_id;
    struct mail_gen3 mails_3[PARTY_SIZE];
    struct gen3_party party_3;
    struct gen3_mon_data_unenc party_3_undec[PARTY_SIZE];
};

enum TRADE_POSSIBILITY {FULL_TRADE_POSSIBLE, PARTIAL_TRADE_POSSIBLE, TRADE_IMPOSSIBLE};

void init_game_data(struct game_data_t*);
void init_save_data(void);
u8 has_cartridge_been_removed(void);
u8 get_is_cartridge_loaded(void);
enum TRADE_POSSIBILITY can_trade(struct game_data_priv_t*, u8);
u8 is_in_pokemon_center(struct game_data_priv_t*, u8);
u8 read_gen_3_data(struct game_data_t*, struct game_data_priv_t*);
void process_party_data(struct game_data_t* game_data, struct gen2_party*, struct gen1_party*);
struct game_data_t* get_own_game_data(void);
void set_default_gift_ribbons(struct game_data_t*);
u8 trade_mons(struct game_data_t*, struct game_data_priv_t*, u8, u8, u8);
u8 is_invalid_offer(struct game_data_t*, u8, u8, u8, u16);
u8 pre_write_gen_3_data(struct game_data_t*, struct game_data_priv_t*, u8);
u8 pre_write_updated_moves_gen_3_data(struct game_data_t*, struct game_data_priv_t*);
u8 complete_write_gen_3_data(struct game_data_t*);
u8 get_sys_flag_save(u8, int, u8, u16);
void set_sys_flag_save(u8*, u8, u16, u8);
u16 get_var_save(u8, int, u8, u16);
void set_var_save(u8*, u8, u16, u16);
u32 get_stat_save(u8, int, u8, u16);
void set_stat_save(u8*, u8, u16, u32);
u8 get_sys_flag_byte_save(u8, int, u8, u16);
void set_sys_flag_byte_save(u8*, u8, u16, u8);

#endif
