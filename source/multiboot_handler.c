#include "base_include.h"
#include "multiboot_handler.h"
#include "sio.h"
#include "menu_text_handler.h"
#include "print_system.h"
#include "timing_basic.h"
#include "useful_qualifiers.h"

#define MAX_PALETTE_ATTEMPTS 128
#define MULTIBOOT_WAIT_TIME_NS 36000
#define MULTIBOOT_VCOUNTWAIT (((MULTIBOOT_WAIT_TIME_NS/NS_PER_SCANLINE) + ((MULTIBOOT_WAIT_TIME_NS%NS_PER_SCANLINE) == 0 ? 0 : 1))+1)

void multiboot_send(int, int, u16*);

void multiboot_send(int data, int is_normal, u16* out_buffer) {
    // Only this part of REG_SIODATA32 is used during setup.
    // The rest is handled by SWI $25
    for(int i = 0; i < MAX_NUM_SLAVES; i++)
        out_buffer[i] = 0;
    if(is_normal)
        out_buffer[0] = timed_sio_normal_master(data, SIO_32, MULTIBOOT_VCOUNTWAIT) >> 0x10;
    else
        timed_sio_multi_master(data, MULTIBOOT_VCOUNTWAIT, out_buffer);
}

#ifdef HAS_SIO
enum MULTIBOOT_RESULTS multiboot_normal (u16* data, u16* end, int is_normal) {
#else
enum MULTIBOOT_RESULTS multiboot_normal (u16* UNUSED(data), u16* UNUSED(end), int UNUSED(is_normal)) {
#endif
    #ifdef HAS_SIO
    u16 response[MAX_NUM_SLAVES];
    u8 clientMask = 0;
    u8 client_bit;
    int attempts, sends, halves;
    u8 answers[MAX_NUM_SLAVES] = {0xFF, 0xFF, 0xFF};
    u8 handshake;
    u8 sendMask;
    u8 attempt_counter;
    const u8 palette = 0x81;
    enum MULTIBOOT_MODES mb_mode = MODE32_NORMAL;
    MultiBootParam mp;
    
    if(!is_normal)
        mb_mode = MODE16_MULTI;

    if(is_normal)
        init_sio_normal(SIO_MASTER, SIO_32);
    else
        init_sio_multi(SIO_MASTER);

    print_multiboot_mid_process(0);
    prepare_flush();

    for(attempts = 0; attempts < 128; attempts++) {
        for(sends = 0; sends < 16; sends++) {
            multiboot_send(0x6200, is_normal, response);

            for(int i = 0; i < MAX_NUM_SLAVES; i++)
                if((response[i] & 0xFFF0) == 0x7200) {
                    clientMask |= response[i];
                }
        }

        if(clientMask)
            break;
        else
            VBlankIntrWait();
    }

    if(!clientMask) {
        return MB_NO_INIT_SYNC;
    }

    clientMask &= 0xF;
    multiboot_send(0x6100 | clientMask, is_normal, response);
    for(int i = 0; i < MAX_NUM_SLAVES; i++) {
        client_bit = 1 << (i + 1);

        if ((clientMask & client_bit) && (response[i] != (0x7200 | client_bit)))
            return MB_WRONG_ANSWER;
    }

    for(halves = 0; halves < 0x60; ++halves) {
        multiboot_send(*data++, is_normal, response);
        for(int i = 0; i < MAX_NUM_SLAVES; i++) {
            client_bit = 1 << (i + 1);

            if ((clientMask & client_bit) && (response[i] != (((0x60 - halves) << 8) | client_bit)))
                return MB_HEADER_ISSUE;
        }
    }

    multiboot_send(0x6200, is_normal, response);
    for(int i = 0; i < MAX_NUM_SLAVES; i++) {
        client_bit = 1 << (i + 1);

        if ((clientMask & client_bit) && (response[i] != client_bit))
            return MB_WRONG_ANSWER;
    }

    multiboot_send(0x6200 | clientMask, is_normal, response);
    for(int i = 0; i < MAX_NUM_SLAVES; i++) {
        client_bit = 1 << (i + 1);

        if ((clientMask & client_bit) && (response[i] != (0x7200 | client_bit)))
            return MB_WRONG_ANSWER;
    }

    sendMask = clientMask;
    attempt_counter = 0;

    while(sendMask) {
        multiboot_send(0x6300 | palette, is_normal, response);

        for(int i = 0; i < MAX_NUM_SLAVES; i++) {
            client_bit = 1 << (i + 1);

            if ((clientMask & client_bit) && ((response[i] & 0xFF00) == 0x7300)) {
                answers[i] = response[i] & 0xFF;
                sendMask &= ~client_bit;
            }
        }
        attempt_counter++;

        if((attempt_counter == MAX_PALETTE_ATTEMPTS) && sendMask)
            return MB_PALETTE_FAILURE;
    }

    handshake = 0x11;
    for(int i = 0; i < MAX_NUM_SLAVES; i++)
        handshake += answers[i];

    multiboot_send(0x6400 | handshake, is_normal, response);
    for(int i = 0; i < MAX_NUM_SLAVES; i++) {
        client_bit = 1 << (i + 1);

        if ((clientMask & client_bit) && ((response[i] & 0xFF00) != 0x7300))
            return MB_WRONG_ANSWER;
    }
    
    print_multiboot_mid_process(1);
    prepare_flush();
    VBlankIntrWait();
    
    mp.handshake_data = handshake;
    for(int i = 0; i < MAX_NUM_SLAVES; i++)
        mp.client_data[i] = answers[i];
    mp.palette_data = palette;
    mp.client_bit = clientMask;
    mp.boot_srcp = (u8*)data;
    mp.boot_endp = (u8*)end;
    
    if(MultiBoot(&mp, mb_mode))
        return MB_SWI_FAILURE;
    
    #endif
    return MB_SUCCESS;
}
