#ifndef WINDOW_HANDLER__
#define WINDOW_HANDLER__

#define TRADE_OPTIONS_WINDOW_SCREEN 1
#define TRADE_OPTIONS_WINDOW_X 0
#define TRADE_OPTIONS_WINDOW_Y 0x13
#define TRADE_OPTIONS_WINDOW_X_SIZE 0x1E
#define TRADE_OPTIONS_WINDOW_Y_SIZE 1

#define WAITING_WINDOW_SCREEN 3
#define WAITING_WINDOW_X 5
#define WAITING_WINDOW_Y 0x8
#define WAITING_WINDOW_X_SIZE 10
#define WAITING_WINDOW_Y_SIZE 1

#define OFFER_WINDOW_SCREEN 1
#define OFFER_WINDOW_X 15
#define OFFER_WINDOW_Y 0
#define OFFER_WINDOW_X_SIZE (0x1E - OFFER_WINDOW_X)
#define OFFER_WINDOW_Y_SIZE (0x14)

#define OFFER_OPTIONS_WINDOW_SCREEN 2
#define OFFER_OPTIONS_WINDOW_X 1
#define OFFER_OPTIONS_WINDOW_Y 0xD
#define OFFER_OPTIONS_WINDOW_X_SIZE (0x1E - OFFER_OPTIONS_WINDOW_X - 1)
#define OFFER_OPTIONS_WINDOW_Y_SIZE (0x14 - OFFER_OPTIONS_WINDOW_Y - 1)

void init_trade_options_window();
void clear_trade_options_window();

void init_offer_window();
void clear_offer_window();

void init_offer_options_window();
void clear_offer_options_window();

void init_waiting_window();
void clear_waiting_window();

#endif
