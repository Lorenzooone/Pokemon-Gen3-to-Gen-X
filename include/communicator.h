#ifndef COMMUNICATOR__
#define COMMUNICATOR__

#include "timing_basic.h"

#define CYCLES_PER_SCANLINE 1232
#define WAIT_TIME_NS 820000
#define VCOUNT_WAIT_LINES (WAIT_TIME_NS/(CYCLES_PER_SCANLINE*NS_PER_CYCLE))

#define IN_PARTY_TRADE_GEN3 0x80
#define NOT_DONE_GEN3 0x40
#define DONE_GEN3 0x20
#define SENDING_DATA_GEN3 0x10
#define START_SENDING_DATA 0x1
#define ASKING_DATA 0xC
#define DATA_CHUNK_SIZE 0xFE

#define SEND_0_INFO 0
#define SEND_NO_INFO 0xFE
#define SYNCHRONIZE_BYTE 0xFD
#define ENTER_TRADE_END 0
#define ENTER_TRADE_MASTER 1
#define ENTER_TRADE_SLAVE 2
#define CHOICE_BYTE_GEN1 0xD0
#define END_CHOICE_BYTE_GEN1 0xD4
#define START_TRADE_BYTE_GEN2 0x75
#define START_TRADE_BYTE_GEN1 0x60

#define CANCEL_VALUE 0xF
#define GEN1_TRADE_OFFER_START 0x60
#define GEN2_TRADE_OFFER_START 0x70
#define END_TRADE_BYTE_GEN2 (GEN2_TRADE_OFFER_START + CANCEL_VALUE)
#define END_TRADE_BYTE_GEN1 (GEN1_TRADE_OFFER_START + CANCEL_VALUE)
#define DECLINE_VALUE 1
#define ACCEPT_VALUE 2

#define WANTS_TO_CANCEL -1
#define TRADE_CANCELLED -2

#define NO_INFO_LIMIT (60*5)

enum START_TRADE_STATE {START_TRADE_UNK, START_TRADE_ENT, START_TRADE_STA, START_TRADE_END, START_TRADE_WAI, START_TRADE_PAR, START_TRADE_SYN, START_TRADE_DON, START_TRADE_NO_UPDATE};
enum TRADING_STATE {NO_INFO, HAVE_OFFER, RECEIVED_OFFER, HAVE_ACCEPT, RECEIVED_ACCEPT, DURING_TRADE, COMPLETED_TRADE};

void try_to_end_trade(void);
void try_to_offer(u8);
void try_to_accept_offer(void);
void try_to_decline_offer(void);
int get_received_trade_offer(void);
int has_accepted_offer(void);
void start_transfer(u8, u8);
void set_next_vcount_interrupt(void);
void stop_transfer(u8);
int increment_last_tranfer(void);
void init_start_state(void);
enum TRADING_STATE get_trading_state(void);
enum START_TRADE_STATE get_start_state(void);
enum START_TRADE_STATE get_start_state_raw(void);
u16 get_transferred(u8);
void slave_routine(void);
void master_routine_gen12(void);
void master_routine_gen3(void);

#endif
