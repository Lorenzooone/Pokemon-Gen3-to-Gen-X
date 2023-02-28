#ifndef GEN_CONVERTER__
#define GEN_CONVERTER__

#include "party_handler.h"

void alter_nature(struct gen3_mon_data_unenc*, u8);
void set_alter_data(struct gen3_mon_data_unenc*, struct alternative_data_gen3*);
void preload_if_fixable(struct gen3_mon_data_unenc*);

void sanitize_ot_name_gen3_to_gen12(u8*, u8*, u8, u8);
void sanitize_ot_name_gen12_to_gen3(u8*, u8*, u8);

u8 gen3_to_gen2(struct gen2_mon*, struct gen3_mon_data_unenc*, u32);
u8 gen3_to_gen1(struct gen1_mon*, struct gen3_mon_data_unenc*, u32);
u8 gen2_to_gen3(struct gen2_mon_data*, struct gen3_mon_data_unenc*, u8, u8*, u8*, u8);
u8 gen1_to_gen3(struct gen1_mon_data*, struct gen3_mon_data_unenc*, u8, u8*, u8*, u8);

#endif
