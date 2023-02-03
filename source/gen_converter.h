#ifndef GEN_CONVERTER__
#define GEN_CONVERTER__

#include "party_handler.h"

u8 gen3_to_gen2(struct gen2_mon*, struct gen3_mon_data_unenc*, u32);
u8 gen3_to_gen1(struct gen1_mon*, struct gen3_mon_data_unenc*, u32);
u8 gen2_to_gen3(struct gen2_mon_data*, struct gen3_mon_data_unenc*, u8, u8*, u8*, u8);
u8 gen1_to_gen3(struct gen1_mon_data*, struct gen3_mon_data_unenc*, u8, u8*, u8*, u8);

#endif
