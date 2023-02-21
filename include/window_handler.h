#ifndef WINDOW_HANDLER__
#define WINDOW_HANDLER__

#define TOTAL_Y_SIZE (SCREEN_HEIGHT>>3)
#define TOTAL_X_SIZE (SCREEN_WIDTH>>3)

#define TRADE_OPTIONS_WINDOW_SCREEN 1
#define TRADE_OPTIONS_WINDOW_X 0
#define TRADE_OPTIONS_WINDOW_Y (TOTAL_Y_SIZE-1)
#define TRADE_OPTIONS_WINDOW_X_SIZE TOTAL_X_SIZE
#define TRADE_OPTIONS_WINDOW_Y_SIZE 1

#define TRADE_ANIMATION_SEND_WINDOW_SCREEN 1
#define TRADE_ANIMATION_SEND_WINDOW_X 1
#define TRADE_ANIMATION_SEND_WINDOW_Y_SIZE 3
#define TRADE_ANIMATION_SEND_WINDOW_Y (TOTAL_Y_SIZE-1-TRADE_ANIMATION_SEND_WINDOW_Y_SIZE)
#define TRADE_ANIMATION_SEND_WINDOW_X_SIZE (TOTAL_X_SIZE-TRADE_ANIMATION_SEND_WINDOW_X-1)

#define TRADE_ANIMATION_RECV_WINDOW_SCREEN 2
#define TRADE_ANIMATION_RECV_WINDOW_X 1
#define TRADE_ANIMATION_RECV_WINDOW_Y_SIZE 3
#define TRADE_ANIMATION_RECV_WINDOW_Y (TOTAL_Y_SIZE-1-TRADE_ANIMATION_RECV_WINDOW_Y_SIZE)
#define TRADE_ANIMATION_RECV_WINDOW_X_SIZE (TOTAL_X_SIZE-TRADE_ANIMATION_RECV_WINDOW_X-1)

#define LEARN_MOVE_MESSAGE_WINDOW_SCREEN 1
#define LEARN_MOVE_MESSAGE_WINDOW_X 1
#define LEARN_MOVE_MESSAGE_WINDOW_Y_SIZE 6
#define LEARN_MOVE_MESSAGE_WINDOW_Y (TOTAL_Y_SIZE-1-LEARN_MOVE_MESSAGE_WINDOW_Y_SIZE)
#define LEARN_MOVE_MESSAGE_WINDOW_X_SIZE (TOTAL_X_SIZE-LEARN_MOVE_MESSAGE_WINDOW_X-1)

#define EVOLUTION_ANIMATION_WINDOW_SCREEN 3
#define EVOLUTION_ANIMATION_WINDOW_X 1
#define EVOLUTION_ANIMATION_WINDOW_Y_SIZE 3
#define EVOLUTION_ANIMATION_WINDOW_Y (TOTAL_Y_SIZE-1-TRADE_ANIMATION_SEND_WINDOW_Y_SIZE)
#define EVOLUTION_ANIMATION_WINDOW_X_SIZE (TOTAL_X_SIZE-TRADE_ANIMATION_SEND_WINDOW_X-1)

#define WAITING_WINDOW_SCREEN 3
#define WAITING_WINDOW_X_SIZE 10
#define WAITING_WINDOW_Y_SIZE 1
#define WAITING_WINDOW_X ((TOTAL_X_SIZE>>1)-((WAITING_WINDOW_X_SIZE+1)>>1))
#define WAITING_WINDOW_Y ((TOTAL_Y_SIZE>>1)-((WAITING_WINDOW_Y_SIZE+1)>>1))

#define CRASH_WINDOW_SCREEN 3
#define CRASH_WINDOW_X_SIZE 21
#define CRASH_WINDOW_Y_SIZE 5
#define CRASH_WINDOW_X ((TOTAL_X_SIZE>>1)-((CRASH_WINDOW_X_SIZE+1)>>1))
#define CRASH_WINDOW_Y ((TOTAL_Y_SIZE>>1)-((CRASH_WINDOW_Y_SIZE+1)>>1))

#define SAVING_WINDOW_SCREEN 3
#define SAVING_WINDOW_X_SIZE 9
#define SAVING_WINDOW_Y_SIZE 1
#define SAVING_WINDOW_X ((TOTAL_X_SIZE>>1)-((SAVING_WINDOW_X_SIZE+1)>>1))
#define SAVING_WINDOW_Y ((TOTAL_Y_SIZE>>1)-((SAVING_WINDOW_Y_SIZE+1)>>1))

#define LOADING_WINDOW_SCREEN 3
#define LOADING_WINDOW_X_SIZE 10
#define LOADING_WINDOW_Y_SIZE 1
#define LOADING_WINDOW_X ((TOTAL_X_SIZE>>1)-((LOADING_WINDOW_X_SIZE+1)>>1))
#define LOADING_WINDOW_Y ((TOTAL_Y_SIZE>>1)-((LOADING_WINDOW_Y_SIZE+1)>>1))

#define MESSAGE_WINDOW_SCREEN 2
#define MESSAGE_WINDOW_X 1
#define MESSAGE_WINDOW_Y 0x10
#define MESSAGE_WINDOW_X_SIZE (TOTAL_X_SIZE - MESSAGE_WINDOW_X - 1)
#define MESSAGE_WINDOW_Y_SIZE (TOTAL_Y_SIZE - MESSAGE_WINDOW_Y - 1)

#define OFFER_WINDOW_SCREEN 1
#define OFFER_WINDOW_X (TOTAL_X_SIZE>>1)
#define OFFER_WINDOW_Y 0
#define OFFER_WINDOW_X_SIZE (TOTAL_X_SIZE - OFFER_WINDOW_X)
#define OFFER_WINDOW_Y_SIZE (TOTAL_Y_SIZE)

#define OFFER_OPTIONS_WINDOW_SCREEN 2
#define OFFER_OPTIONS_WINDOW_X 1
#define OFFER_OPTIONS_WINDOW_Y 0xD
#define OFFER_OPTIONS_WINDOW_X_SIZE (TOTAL_X_SIZE - OFFER_OPTIONS_WINDOW_X - 1)
#define OFFER_OPTIONS_WINDOW_Y_SIZE (TOTAL_Y_SIZE - OFFER_OPTIONS_WINDOW_Y - 1)

void init_trade_options_window(void);
void clear_trade_options_window(void);

void init_offer_window(void);
void clear_offer_window(void);

void init_message_window(void);
void clear_message_window(void);

void init_offer_options_window(void);
void clear_offer_options_window(void);

void init_waiting_window(s8);
void clear_waiting_window(s8);

void init_crash_window(void);
void clear_crash_window(void);

void init_learn_move_message_window(void);
void clear_learn_move_message_window(void);

void init_trade_animation_send_window(void);
void clear_trade_animation_send_window(void);

void init_trade_animation_recv_window(void);
void clear_trade_animation_recv_window(void);

void init_evolution_animation_window(void);
void clear_evolution_animation_window(void);

void init_saving_window(void);
void clear_saving_window(void);

void init_loading_window(void);
void clear_loading_window(void);

#endif
