#ifndef ANIMATIONS_HANDLER__
#define ANIMATIONS_HANDLER__

void setup_trade_animation(struct gen3_mon_data_unenc*, struct gen3_mon_data_unenc*, u8, u8);
void setup_evolution_animation(struct gen3_mon_data_unenc*, u8);
void advance_trade_animation(void);
u8 has_animation_completed(void);
void start_with_evolution_animation(void);
void prepare_evolution_animation_only(struct gen3_mon_data_unenc* own_mon, u8);

#endif
