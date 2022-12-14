#include <gba.h>
#include "gb_dump_receiver.h"
#include "sio.h"

#define NORMAL_BYTE 0x10
#define CHECK_BYTE 0x40
#define VCOUNT_TIMEOUT 28
#define FLAG_CHECK 0x100
#define OK_SIGNAL 1
#define BUFFER_SIZE 0x100
#define DUMP_POSITION EWRAM
#define SRAM_TRANSFER 2
#define ROM_TRANSFER 1

const u8 rom_bank_sizes[] = {64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64};
const u16 rom_banks[] = {2,4,8,16,32,64,128,256,512,0,0,0,0,0,0,0,0,0,72,80,96};
const u8 sram_bank_sizes[] = {0,8,0x20,0x20,0x20,0x20,2};
const u16 sram_banks[] = {0,1,1,4,16,8,1};

int read_gb_dump_val(int);
int read_sector(u8*, int, int*);

int read_dump(int max_size) {
    u8 buffer[BUFFER_SIZE];
    int result = 0, size = 0, keep_going = 0, bank_size, banks, total_sections;
    
    init_sio_normal(SIO_MASTER, SIO_8);
    
    while (!keep_going) {
        size = read_sector(buffer, BUFFER_SIZE, &result);
        
        if(size > 2)
            return GENERIC_DUMP_ERROR;
        if(size == 2)
            keep_going = 1;
    }
    
    if(buffer[0] == ROM_TRANSFER) {
        bank_size = rom_bank_sizes[buffer[1]];
        banks = rom_banks[buffer[1]];
    }
    else if(buffer[0] == SRAM_TRANSFER) {
        bank_size = sram_bank_sizes[buffer[1]];
        banks = sram_banks[buffer[1]];
    }
    else
        return GENERIC_DUMP_ERROR;
    
    total_sections = banks * bank_size;
    
    if(BUFFER_SIZE * total_sections > max_size)
        return SIZE_DUMP_ERROR;
    
    for (int i = 0; i < total_sections; i++) {
        iprintf("\x1b[2J");
        iprintf("Dumping: %d/%d\n", i + 1, total_sections);
        size = read_sector(buffer, BUFFER_SIZE, &result);
        if(size != BUFFER_SIZE)
            return GENERIC_DUMP_ERROR;
        for(int j = 0; j < BUFFER_SIZE; j++)
            *((u8*)(DUMP_POSITION + (i * BUFFER_SIZE) + j)) = buffer[j];
    }
    
    return total_sections * BUFFER_SIZE;
}

int read_gb_dump_val(int data) {
    int flag_value = 0;
    u8 received[2], envelope[2];
    
    for(int i = 0; i < 2; i++) {
        received[i] = timed_sio_normal_master((data & (0xF0 >> (i * 4))) >> (4 - (i * 4)), SIO_8, VCOUNT_TIMEOUT);
        envelope[i] = received[i] & 0xF0;
        received[i] = (received[i] & 0xF) << (4 - (i * 4));
        
        if ((envelope[i] != NORMAL_BYTE) && (envelope[i] != CHECK_BYTE))
            i--;
    }
    
    if (envelope[1] != envelope[0]) {
        timed_sio_normal_master(received[0], SIO_8, VCOUNT_TIMEOUT);
        return -1;
    }
    
    if(envelope[0] == CHECK_BYTE)
        flag_value = FLAG_CHECK;
    
    return flag_value | (received[0] | received[1]);
}

int read_sector(u8* buffer, int max_size, int* result) {
    int index = 0, got_section = 0;
    int curr_result;
    
    while(!got_section) {
        curr_result = read_gb_dump_val(*result);
        if(curr_result != -1) {
            *result = curr_result & 0xFF;
            
            if (!(curr_result & FLAG_CHECK)) {
                if (index >= max_size)
                    index++;
                else
                    buffer[index++] = *result;
                if(index > (max_size + 1))
                    index = 0;
            }
            else if(*result == OK_SIGNAL)
                got_section = 1;
            else
                index = 0;
        }
        else
            index = 0;
    }
    
    return index - 1;
}
