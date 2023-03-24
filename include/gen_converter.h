#ifndef GEN_CONVERTER__
#define GEN_CONVERTER__

#include "party_handler.h"

#define VALID_POKEBALL_NO_EGG 0x00001FDE
#define VALID_POKEBALL_EGG 0x00000010
#define VALID_POKEBALL_CELEBI 0x00000010
#define VALID_POKEBALL_POSSIBLE (VALID_POKEBALL_NO_EGG)

void alter_nature(struct gen3_mon_data_unenc*, u8);
void set_alter_data(struct gen3_mon_data_unenc*, struct alternative_data_gen3*);
void preload_if_fixable(struct gen3_mon_data_unenc*);

void convert_trainer_name_gen3_to_gen12(u8*, u8*, u8, u8);
void convert_trainer_name_gen12_to_gen3(u8*, u8*, u8, u8, u8);

u8 gen3_to_gen2(struct gen2_mon*, struct gen3_mon_data_unenc*, u32);
u8 gen3_to_gen1(struct gen1_mon*, struct gen3_mon_data_unenc*, u32);
u8 gen2_to_gen3(struct gen2_mon_data*, struct gen3_mon_data_unenc*, u8, u8*, u8*, u8);
u8 gen1_to_gen3(struct gen1_mon_data*, struct gen3_mon_data_unenc*, u8, u8*, u8*, u8);

void reconvert_strings_of_gen3_to_gen2(struct gen3_mon_data_unenc*, struct gen2_mon*);
void reconvert_strings_of_gen3_to_gen1(struct gen3_mon_data_unenc*, struct gen1_mon*);

#endif
