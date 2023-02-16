#ifndef ANIMATIONS_HANDLER__
#define ANIMATIONS_HANDLER__

void setup_trade_animation(struct gen3_mon_data_unenc*, struct gen3_mon_data_unenc*, u8, u8);
void advance_trade_animation(void);
u8 get_trade_animation_state(void);

#endif
