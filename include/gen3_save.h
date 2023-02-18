#ifndef GEN3_SAVE__
#define GEN3_SAVE__

#include "party_handler.h"
#include "version_identifier.h"
#include "save.h"

#define SAVE_SLOT_SIZE 0xE000
#define SECTION_SIZE SECTOR_SIZE

struct game_data_t {
    u32 seen_unown_pid;
    u32 seen_spinda_pid;
    u8 pokedex_owned[DEX_BYTES];
    u8 pokedex_seen[DEX_BYTES];
    struct game_identity game_identifier;
    u8 giftRibbons[GIFT_RIBBONS];
    u8 trainer_name[OT_NAME_GEN3_SIZE+1];
    u8 trainer_gender;
    u32 trainer_id;
    struct mail_gen3 mails_3[PARTY_SIZE];
    struct gen3_party party_3;
    struct gen3_mon_data_unenc party_3_undec[PARTY_SIZE];
    struct gen2_party party_2;
    struct gen1_party party_1;
};

void init_game_data(struct game_data_t*);
void init_save_data(void);
u8 read_gen_3_data(struct game_data_t*);
void process_party_data(struct game_data_t* game_data);
struct game_data_t* get_own_game_data(void);
void set_default_gift_ribbons(struct game_data_t*);
u8 trade_mons(struct game_data_t*, u8, u8, u8);
u8 is_invalid_offer(struct game_data_t*, u8, u8);
u8 pre_write_gen_3_data(struct game_data_t*, u8);
u8 pre_write_updated_moves_gen_3_data(struct game_data_t*);
u8 complete_write_gen_3_data(void);

#endif
