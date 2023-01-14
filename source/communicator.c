#include <gba.h>
#include "communicator.h"
#include "sio.h"
#include "sio_buffers.h"
#include "print_system.h"

#define GEN2_ENTER_STATES_NUM 4
#define GEN2_START_STATES_NUM 5
#define GEN1_ENTER_STATES_NUM 5
#define GEN1_START_STATES_NUM 2
#define SKIP_SENDS 0x10

#define MAX_WAIT_FOR_SYN 30
#define MIN_WAIT_FOR_SYN 3
#define MAX_NO_NEW_INFO 30

#define NUMBER_OF_ENTITIES 2

const u8 gen2_start_trade_enter_room[NUMBER_OF_ENTITIES][GEN2_ENTER_STATES_NUM] = {
    {ENTER_TRADE_SLAVE, 0x61, 0xD1, 0x00},
    {ENTER_TRADE_MASTER, 0x61, 0xD1, 0x00}
};
const u8 gen2_start_trade_enter_room_next[NUMBER_OF_ENTITIES][GEN2_ENTER_STATES_NUM] = {
    {ENTER_TRADE_MASTER, 0x61, 0xD1, 0x00},
    {0x61, 0xD1, 0x00, 0x00}
};
const u8 gen2_start_trade_start_trade_procedure[NUMBER_OF_ENTITIES][GEN2_START_STATES_NUM] = {
    {START_TRADE_BYTE_GEN2, START_TRADE_BYTE_GEN2, 0, 0x76, 0x76},
    {START_TRADE_BYTE_GEN2, START_TRADE_BYTE_GEN2, 0x76, SEND_NO_INFO, SEND_NO_INFO}
};
const u8 gen2_start_trade_start_trade_procedure_next[NUMBER_OF_ENTITIES][GEN2_START_STATES_NUM] = {
    {START_TRADE_BYTE_GEN2, START_TRADE_BYTE_GEN2, 0x76, 0x76, 0x76},
    {START_TRADE_BYTE_GEN2, 0, SYNCHRONIZE_BYTE, SEND_NO_INFO, SEND_NO_INFO}
};

const u8 gen1_start_trade_enter_room[NUMBER_OF_ENTITIES][GEN1_ENTER_STATES_NUM] = {
    {ENTER_TRADE_SLAVE, 0x60, CHOICE_BYTE_GEN1, END_CHOICE_BYTE_GEN1, END_CHOICE_BYTE_GEN1},
    {ENTER_TRADE_MASTER, 0x60, CHOICE_BYTE_GEN1, END_CHOICE_BYTE_GEN1, SEND_NO_INFO}
};
const u8 gen1_start_trade_enter_room_next[NUMBER_OF_ENTITIES][GEN1_ENTER_STATES_NUM] = {
    {ENTER_TRADE_MASTER, 0x60, CHOICE_BYTE_GEN1, CHOICE_BYTE_GEN1, CHOICE_BYTE_GEN1},
    {0x60, CHOICE_BYTE_GEN1, CHOICE_BYTE_GEN1, START_TRADE_BYTE_GEN1, SEND_NO_INFO}
};
const u8 gen1_start_trade_start_trade_procedure[NUMBER_OF_ENTITIES][GEN1_START_STATES_NUM] = {
    {START_TRADE_BYTE_GEN1, START_TRADE_BYTE_GEN1},
    {START_TRADE_BYTE_GEN1, START_TRADE_BYTE_GEN1}
};
const u8 gen1_start_trade_start_trade_procedure_next[NUMBER_OF_ENTITIES][GEN1_START_STATES_NUM] = {
    {START_TRADE_BYTE_GEN1, START_TRADE_BYTE_GEN1},
    {START_TRADE_BYTE_GEN1, SYNCHRONIZE_BYTE}
};

u8 stored_curr_gen;
u8* out_buffer;
u8* in_buffer;
u16* transfer_sizes;
u16 base_pos;
u8 sizes_index;
u8 syn_transmitted;
u8 has_transmitted_syn;
int last_transfer_counter;
int buffer_counter;
int buffer_counter_out;
u8 start_state_updated;
u8 start_state;
u32 prepared_value;
int skip_sends = 0;
u8 base_pos_out = 0;
u8 increment_out = 0;
u8 last_filtered = 0;
u8 since_last_recv_gen3 = 0;
int own_end_gen3;
u16 other_pos_gen3;
u16 other_end_gen3;
u8 is_done_gen3;
u8 received_gen3[(sizeof(struct gen3_trade_data)>>4)+1];

void set_start_state(u8 new_val) {
    start_state = new_val;
    start_state_updated = 1;
}

u16 get_transferred(u8 index) {
    if(sizes_index > index)
        return get_buffer_size(index);
    if(sizes_index < index)
        return 0;
    if(stored_curr_gen == 3)
        return buffer_counter<<1;
    return buffer_counter;
}

void init_received_gen3() {
    for(int i = 0; i < ((sizeof(struct gen3_trade_data) >> 4)+1); i++)
        received_gen3[i] = 0;
    buffer_counter = 0;
    own_end_gen3 = -1;
    is_done_gen3 = 0;
    since_last_recv_gen3 = 0;
}

u16 get_pos_bytes_from_index(u16 index) {
    int pos_byte = START_SENDING_DATA;
    while(index >= DATA_CHUNK_SIZE) {
        pos_byte++;
        index -= DATA_CHUNK_SIZE;
    }
    return index | (pos_byte << 8);
}

u16 get_index_from_pos_bytes(u8 position, u8 control_byte) {
    u16 final_pos = position;
    if(final_pos >= DATA_CHUNK_SIZE)
        final_pos = 0;
    return final_pos + (DATA_CHUNK_SIZE * ((control_byte&0xF) - START_SENDING_DATA));
}

u8 set_received_gen3(u16 index) {
    if(index >= (sizeof(struct gen3_trade_data)>>1))
        return 1;
    int i = index >> 3;
    int j = index & 7;
    if((received_gen3[i]>>j)&1)
        return 1;
    received_gen3[i] |= 1<<j;
    return 0;
}

u16 get_first_not_received_gen3() {
    for(int i = 0; i < ((sizeof(struct gen3_trade_data) >> 4)+1); i++)
        for(int j = 0; j < 8; j++) {
            if(((i<<3) + j) >= (sizeof(struct gen3_trade_data)>>1))
                return sizeof(struct gen3_trade_data)>>1;
            if(!((received_gen3[i]>>j)&1))
                return (i<<3) + j;
        }
    return sizeof(struct gen3_trade_data)>>1;
}

u16 get_not_received_length_gen3(u16 index) {
    if(index >= (sizeof(struct gen3_trade_data)>>1))
        return 0;
    int i_pos = index >> 3;
    int j_pos = index & 7;
    int j = j_pos;
    int total = 0;
    for(int i = i_pos; i < ((sizeof(struct gen3_trade_data) >> 4)+1); i++) {
        for(; j < 8; j++) {
            if(((i<<3) + j) >= (sizeof(struct gen3_trade_data)>>1))
                return total;
            if(!((received_gen3[i]>>j)&1))
                total++;
            else
                return total;
        }
        j = 0;
    }
    return total;
}

int increment_last_tranfer() {
    return last_transfer_counter++;
}

u8 get_start_state() {
    if(!start_state_updated)
        return START_TRADE_NO_UPDATE;
    start_state_updated = 0;
    return start_state;
}

void reset_transfer_data_between_sync() {
    has_transmitted_syn = 0;
    syn_transmitted = 0;
    increment_out = 0;
    buffer_counter = 0;
    buffer_counter_out = 0;
    last_transfer_counter = 0;
}

void init_transfer_data() {
    base_pos = 0;
    reset_transfer_data_between_sync();
    sizes_index = 0;
    set_start_state(START_TRADE_PAR);
}

u8 get_start_state_raw() {
    return start_state;
}

void init_start_state() {
    set_start_state(START_TRADE_UNK);
}

__attribute__((optimize(3))) void set_next_vcount_interrupt(void){
    int next_stop = REG_VCOUNT + VCOUNT_WAIT_LINES;
    if(next_stop >= 0xE4)
        next_stop -= 0xE4;
    REG_DISPSTAT  = (REG_DISPSTAT &0xFF) | (next_stop<<8);
}

IWRAM_CODE __attribute__((optimize(3))) int communicate_buffer(u8 data, u8 is_master) {
    u8 ignore_data = 0;
    if(!has_transmitted_syn) {
        if((syn_transmitted > MAX_WAIT_FOR_SYN) && (!is_master)) {
            if(data == SYNCHRONIZE_BYTE) {
                base_pos_out = 0;
                ignore_data = 1;
            }
            else
                has_transmitted_syn = 1;
        }
        else {
            if((is_master) && (syn_transmitted >= MAX_WAIT_FOR_SYN))
                syn_transmitted = MAX_WAIT_FOR_SYN;
            if(syn_transmitted < MIN_WAIT_FOR_SYN) {
                if((data != SEND_NO_INFO) && (data == SYNCHRONIZE_BYTE))
                    syn_transmitted++;
                return SYNCHRONIZE_BYTE;
            }
            if((data != SEND_NO_INFO) && (data != SYNCHRONIZE_BYTE)) {
                has_transmitted_syn = 1;
                base_pos_out = 1;
            }
            else {
                if(data != SEND_NO_INFO)
                    syn_transmitted++;
                return SYNCHRONIZE_BYTE;
            }
        }
    }
    if((!ignore_data) && (data != SEND_NO_INFO)) {
        in_buffer[buffer_counter + base_pos] = data;
        last_transfer_counter = 0;
        buffer_counter++;
        if((buffer_counter == (transfer_sizes[sizes_index])) || ((!is_master) && (sizes_index == get_number_of_buffers()-1) && (buffer_counter == transfer_sizes[sizes_index]-base_pos_out))) {
            base_pos += transfer_sizes[sizes_index];
            sizes_index++;
            reset_transfer_data_between_sync();
            if(sizes_index >= get_number_of_buffers())
                set_start_state(START_TRADE_DON);
            if(buffer_counter_out < transfer_sizes[sizes_index-1])
                return out_buffer[base_pos-1];
            return SEND_0_INFO;
        }
    }

    buffer_counter_out++;
    if(buffer_counter_out > transfer_sizes[sizes_index])
        buffer_counter_out = transfer_sizes[sizes_index];
    
    return out_buffer[buffer_counter_out - 1 + base_pos];
}

IWRAM_CODE __attribute__((optimize(3))) int check_if_continue(u8 data, u8* sends, u8* recvs, int size, int new_state, int new_send, u8 filter) {
    if((filter) && (data >= 0x10))
        data = data & 0xF0;
    if(data == recvs[buffer_counter]) {
        buffer_counter++;
        if(((buffer_counter) >= size) || (sends[buffer_counter] == SEND_NO_INFO)) {
            buffer_counter = 0;
            set_start_state(new_state);
            return new_send;
        }
    }
    return sends[buffer_counter];
}

IWRAM_CODE __attribute__((optimize(3))) int process_data_arrived_gen1(u8 data, u8 is_master) {
    if(is_master)
        is_master = 1;
    switch(start_state) {
        case START_TRADE_ENT:
            return check_if_continue(data, gen1_start_trade_enter_room[is_master], gen1_start_trade_enter_room_next[is_master], GEN1_ENTER_STATES_NUM, START_TRADE_WAI, (START_TRADE_BYTE_GEN1 | 1), 1);
        case START_TRADE_STA:
            return check_if_continue(data, gen1_start_trade_start_trade_procedure[is_master], gen1_start_trade_start_trade_procedure_next[is_master], GEN1_START_STATES_NUM, START_TRADE_SYN, SEND_NO_INFO, 1);
        case START_TRADE_PAR:
            return communicate_buffer(data, is_master);
        case START_TRADE_DON:
            return SEND_NO_INFO;
        case START_TRADE_SYN:
            if(is_master)
                init_transfer_data();
            else if((data == SYNCHRONIZE_BYTE) || (buffer_counter == MAX_WAIT_FOR_SYN))
                init_transfer_data();
            else
                buffer_counter++;
            return SEND_NO_INFO;
        default:
            if((data == ENTER_TRADE_MASTER) || (data == ENTER_TRADE_SLAVE)) {
                buffer_counter = 0;
                last_filtered = 0;
                set_start_state(START_TRADE_ENT);
                return gen1_start_trade_enter_room[is_master][0];
            }
            if(data == SYNCHRONIZE_BYTE) {
                buffer_counter = 0;
                last_filtered = 0;
                init_transfer_data();
                return SEND_NO_INFO;
            }
            u8 filtered_data = data & 0xF0;
            if(last_filtered != filtered_data) {
                buffer_counter = 0;
                last_filtered = filtered_data;
            }
            if((!is_master) && (filtered_data == START_TRADE_BYTE_GEN1)) {
                buffer_counter++;
                if(buffer_counter == MAX_WAIT_FOR_SYN) {
                    buffer_counter = 0;
                    last_filtered = 0;
                    init_transfer_data();
                }
                return data;
            }
            if((!is_master) && (filtered_data == CHOICE_BYTE_GEN1)) {
                buffer_counter++;
                if(buffer_counter == MAX_WAIT_FOR_SYN) {
                    buffer_counter = 0;
                    last_filtered = 0;
                    set_start_state(START_TRADE_STA);
                    return END_CHOICE_BYTE_GEN1;
                }
                if(buffer_counter > 4)
                    return END_CHOICE_BYTE_GEN1;
                return CHOICE_BYTE_GEN1;
            }
            if(is_master && (filtered_data == START_TRADE_BYTE_GEN1)) {
                buffer_counter = 0;
                return data;
            }
            if(is_master && (filtered_data == CHOICE_BYTE_GEN1)) {
                buffer_counter = 2;
                set_start_state(START_TRADE_ENT);
                return SEND_NO_INFO;
            }
            if(is_master) {
                if(start_state == START_TRADE_UNK)
                    return gen1_start_trade_enter_room[is_master][0];
                return (START_TRADE_BYTE_GEN1 | 1);
            }
            return SEND_NO_INFO;
    }
}

IWRAM_CODE __attribute__((optimize(3))) int process_data_arrived_gen2(u8 data, u8 is_master) {
    if(is_master)
        is_master = 1;
    switch(start_state) {
        case START_TRADE_ENT:
            return check_if_continue(data, gen2_start_trade_enter_room[is_master], gen2_start_trade_enter_room_next[is_master], GEN2_ENTER_STATES_NUM, START_TRADE_WAI, gen2_start_trade_start_trade_procedure[is_master][0], 0);
        case START_TRADE_STA:
            return check_if_continue(data, gen2_start_trade_start_trade_procedure[is_master], gen2_start_trade_start_trade_procedure_next[is_master], GEN2_START_STATES_NUM, START_TRADE_SYN, SEND_NO_INFO, 0);
        case START_TRADE_PAR:
            return communicate_buffer(data, is_master);
        case START_TRADE_DON:
            return SEND_NO_INFO;
        case START_TRADE_SYN:
            if(is_master)
                init_transfer_data();
            else if((data == SYNCHRONIZE_BYTE) || (buffer_counter == MAX_WAIT_FOR_SYN))
                init_transfer_data();
            else
                buffer_counter++;
            return SEND_NO_INFO;
        default:
            if((data == ENTER_TRADE_MASTER) || (data == ENTER_TRADE_SLAVE)) {
                buffer_counter = 0;
                set_start_state(START_TRADE_ENT);
                return gen2_start_trade_enter_room[is_master][0];
            }
            if(data == END_TRADE_BYTE_GEN2) {
                buffer_counter = 0;
                set_start_state(START_TRADE_END);
                return END_TRADE_BYTE_GEN2;
            }
            if(is_master && (data == gen2_start_trade_enter_room_next[is_master][0])) {
                buffer_counter = 0;
                set_start_state(START_TRADE_ENT);
                return gen2_start_trade_enter_room[is_master][0];
            }
            if(data == START_TRADE_BYTE_GEN2) {
                buffer_counter = 0;
                set_start_state(START_TRADE_STA);
                return gen2_start_trade_start_trade_procedure[is_master][0];
            }
            if(is_master) {
                if(start_state == START_TRADE_UNK)
                    return gen2_start_trade_enter_room[is_master][0];
                return gen2_start_trade_start_trade_procedure[is_master][0];
            }
            return SEND_NO_INFO;
    }
}

u32 prepare_out_data_gen3() {
    u16* out_buffer_gen3 = (u16*)out_buffer;
    u8 out_control_byte = 0;
    u8 out_position = 0;
    u16 out_data = 0;
    if(start_state != START_TRADE_DON) {
        if(is_done_gen3)
            out_control_byte |= DONE_GEN3;
        else
            out_control_byte |= NOT_DONE_GEN3;
        if((own_end_gen3 == -1) && (!is_done_gen3)) {
            out_control_byte = (out_control_byte & 0xF0) | ASKING_DATA;
            u32 out_positions = 0;
            u16 first_not_recv = get_first_not_received_gen3() & 0xFFF;
            u16 end_not_recv = (get_not_received_length_gen3(first_not_recv) + first_not_recv) & 0xFFF;
            own_end_gen3 = end_not_recv - 1;
            // This should NEVER come true
            if(first_not_recv == end_not_recv) {
                if(are_checksum_same_gen3((struct gen3_trade_data*)in_buffer))
                    is_done_gen3 = 1;
                else
                    init_received_gen3();
            }
            out_data = first_not_recv | ((end_not_recv & 0xF)<<12);
            out_position = end_not_recv >> 4;
        }
        else if(other_pos_gen3 != other_end_gen3) {
            out_control_byte |= SENDING_DATA_GEN3;
            u16 out_pos_control = get_pos_bytes_from_index(other_pos_gen3);
            out_position = out_pos_control & 0xFF;
            out_control_byte |= out_pos_control >> 8;
            out_data = out_buffer_gen3[other_pos_gen3++];
        }
        return out_data | (out_position<<16) | (out_control_byte<<24);
    }
    else {
        // Do trading stuff here
        return ((IN_PARTY_TRADE_GEN3 | DONE_GEN3) << 24);
    }
}

void process_in_data_gen3(u32 data) {
    last_transfer_counter = 0;
    u16* in_buffer_gen3 = (u16*)in_buffer;
    u16 recv_data = data & 0xFFFF;
    u8 position = (data >> 16) & 0xFF;
    u8 control_byte = (data >> 24) & 0xFF;
    if(start_state != START_TRADE_DON) {
        if((control_byte & 0xF) >= ASKING_DATA) {
            control_byte &= ~SENDING_DATA_GEN3;
            if(control_byte & NOT_DONE_GEN3) {
                other_pos_gen3 = data & 0xFFF;
                other_end_gen3 = (data >> 12) & 0xFFF;
                if(other_end_gen3 > (sizeof(struct gen3_trade_data) >> 1))
                    other_end_gen3 = sizeof(struct gen3_trade_data) >> 1;
                if(other_pos_gen3 >= other_end_gen3)
                    other_pos_gen3 = other_end_gen3;
            }
            else if(control_byte & DONE_GEN3)
                other_pos_gen3 = other_end_gen3;
        }
        if((!is_done_gen3) && (control_byte & SENDING_DATA_GEN3)) {
            u16 recv_pos = get_index_from_pos_bytes(position, control_byte);
            if(recv_pos >= (sizeof(struct gen3_trade_data) >> 1)) {
                recv_pos = 0;
                control_byte &= ~SENDING_DATA_GEN3;
            }
            if(control_byte & SENDING_DATA_GEN3) {
                in_buffer_gen3[recv_pos] = recv_data;
                since_last_recv_gen3 = 0;
                if(!set_received_gen3(recv_pos))
                    buffer_counter++;
                if(buffer_counter == sizeof(struct gen3_trade_data)) {
                    if(are_checksum_same_gen3((struct gen3_trade_data*)in_buffer_gen3))
                        is_done_gen3 = 1;
                    else
                        init_received_gen3();
                }
                else if(own_end_gen3 == recv_pos)
                    own_end_gen3 = -1;
            }
        }
        if(!is_done_gen3) {
            since_last_recv_gen3++;
            if(since_last_recv_gen3 > MAX_NO_NEW_INFO) {
                since_last_recv_gen3 = 0;
                own_end_gen3 = -1;
            }
        }
        if((control_byte & DONE_GEN3) && is_done_gen3)
            set_start_state(START_TRADE_DON);
    }
    else {
        // Do trading stuff here
    }
}

IWRAM_CODE __attribute__((optimize(3))) void slave_routine(void) {
    if(!(REG_IF & IRQ_SERIAL)){
        REG_IF |= IRQ_SERIAL;
        int value;
        if(stored_curr_gen == 3){
            int data = sio_read(SIO_32);
            process_in_data_gen3(data);
            value = prepare_out_data_gen3();
            //PRINT_FUNCTION("0x%08X - 0x%08X\n", value, data);
        }
        else if(stored_curr_gen == 2)
            value = process_data_arrived_gen2(sio_read(SIO_8), 0);
        else
            value = process_data_arrived_gen1(sio_read(SIO_8), 0);
        sio_handle_irq_slave(value);
        //PRINT_FUNCTION("0x%02X - 0x%02X\n", value, data);
        //sio_handle_irq_slave(process_data_arrived_gen12(sio_read(SIO_8), 0));
    }
}

IWRAM_CODE __attribute__((optimize(3))) void master_routine_gen3(void) {
	REG_IF |= IRQ_VCOUNT;
    int data;
    u8 success = 0;
    
    data = sio_send_if_ready_master(prepared_value, SIO_32, &success);
    if(success) {
        //PRINT_FUNCTION("0x%08X - 0x%08X\n", prepared_value, data);
        process_in_data_gen3(data);
        prepared_value = prepare_out_data_gen3();
    }

    set_next_vcount_interrupt();
}

IWRAM_CODE __attribute__((optimize(3))) void master_routine_gen12(void) {
	REG_IF |= IRQ_VCOUNT;
    int data;
    
    if(!skip_sends) {
        data = sio_send_master(prepared_value, SIO_8);
        if(data != SEND_NO_INFO) {
            //PRINT_FUNCTION("0x%02X - 0x%02X\n", prepared_value, data);
            if(stored_curr_gen == 2)
                prepared_value = process_data_arrived_gen2(data, 1);
            else
                prepared_value = process_data_arrived_gen1(data, 1);
        }
        else
            skip_sends = SKIP_SENDS;
    }
    else
        skip_sends--;

    set_next_vcount_interrupt();
}

void start_transfer(u8 is_master, u8 curr_gen)
{
    out_buffer = (u8*)get_communication_buffer(OWN_BUFFER);
    in_buffer = (u8*)get_communication_buffer(OTHER_BUFFER);
    buffer_counter = 0;
    skip_sends = 0;
    transfer_sizes = get_buffer_sizes();
    base_pos = 0;
    sizes_index = 0;
    syn_transmitted = 0;
    has_transmitted_syn = 0;
    last_filtered = 0;
    other_pos_gen3 = 0;
    other_end_gen3 = 0;
    last_transfer_counter = 0;
    stored_curr_gen = curr_gen;
    if(curr_gen != 3) {
        if(!is_master) {
            init_sio_normal(SIO_SLAVE, SIO_8);
            irqSet(IRQ_SERIAL, slave_routine);
            irqEnable(IRQ_SERIAL);
            sio_normal_prepare_irq_slave(SEND_NO_INFO);
        }
        else{
            set_next_vcount_interrupt();
            init_sio_normal(SIO_MASTER, SIO_8);
            irqSet(IRQ_VCOUNT, master_routine_gen12);
            prepared_value = ENTER_TRADE_MASTER;
            irqEnable(IRQ_VCOUNT);
            REG_DISPSTAT |= LCDC_VCNT;
        }
    }
    else {
        init_received_gen3();
        set_start_state(START_TRADE_PAR);
        if(!is_master) {
            init_sio_normal(SIO_SLAVE, SIO_32);
            irqSet(IRQ_SERIAL, slave_routine);
            irqEnable(IRQ_SERIAL);
            sio_normal_prepare_irq_slave(SEND_0_INFO);
        }
        else {
            set_next_vcount_interrupt();
            init_sio_normal(SIO_MASTER, SIO_32);
            irqSet(IRQ_VCOUNT, master_routine_gen3);
            prepared_value = SEND_0_INFO;
            irqEnable(IRQ_VCOUNT);
            REG_DISPSTAT |= LCDC_VCNT;
        }
    }
}

void stop_transfer(u8 is_master)
{
    buffer_counter = 0;
    if(!is_master) {
        irqDisable(IRQ_SERIAL);
        sio_stop_irq_slave();
    }
    else{
        irqDisable(IRQ_VCOUNT);
        REG_DISPSTAT &= ~LCDC_VCNT;
    }
}
