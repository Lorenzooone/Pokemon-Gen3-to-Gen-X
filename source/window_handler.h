#ifndef WINDOW_HANDLER__
#define WINDOW_HANDLER__

#define TRADE_OPTIONS_WINDOW_SCREEN 1
#define TRADE_OPTIONS_WINDOW_X 0
#define TRADE_OPTIONS_WINDOW_Y 0x13
#define TRADE_OPTIONS_WINDOW_X_SIZE 0x1E
#define TRADE_OPTIONS_WINDOW_Y_SIZE 1

#define WAITING_WINDOW_SCREEN 2
#define WAITING_WINDOW_X 5
#define WAITING_WINDOW_Y 0x8
#define WAITING_WINDOW_X_SIZE 10
#define WAITING_WINDOW_Y_SIZE 1

void init_trade_options_window();
void clear_trade_options_window();

void init_waiting_window();
void clear_waiting_window();

#endif
