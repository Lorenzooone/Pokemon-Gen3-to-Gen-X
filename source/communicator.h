#ifndef COMMUNICATOR__
#define COMMUNICATOR__

#define VCOUNT_WAIT_LINES 11

#define START_TRADE_STATES 8
#define START_TRADE_UNK 0
#define START_TRADE_ENT 1
#define START_TRADE_STA 2
#define START_TRADE_END 3
#define START_TRADE_WAI 4
#define START_TRADE_PAR 5
#define START_TRADE_SYN 6
#define START_TRADE_DON 7
#define START_TRADE_NO_UPDATE START_TRADE_STATES

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
#define END_TRADE_BYTE_GEN2 0x7F
#define END_TRADE_BYTE_GEN1 0x6F

#define NO_INFO_LIMIT (60*5)

void start_transfer(u8, u8);
void set_next_vcount_interrupt();
void stop_transfer(u8);
int increment_last_tranfer();
u8 get_start_state();
void init_start_state();
u8 get_start_state_raw();
u16 get_transferred(u8);
void slave_routine(void);
void master_routine_gen12(void);

#endif