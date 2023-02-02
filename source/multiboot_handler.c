#include <gba.h>
#include "multiboot_handler.h"
#include "sio.h"
#include "menu_text_handler.h"
#include "print_system.h"

#define MULTIBOOT_VCOUNTWAIT 2

int multiboot_normal_send(int);

int multiboot_normal_send(int data) {
    // Only this part of REG_SIODATA32 is used during setup.
    // The rest is handled by SWI $25
    return timed_sio_normal_master(data, SIO_32, MULTIBOOT_VCOUNTWAIT) >> 0x10;
}

enum MULTIBOOT_RESULTS multiboot_normal (u16* data, u16* end) {
    int response;
    u8 clientMask = 0;
    int attempts, sends, halves;
    u8 answer, handshake;
    const u8 palette = 0x81;
    const int paletteCmd = 0x6300 | palette;
    MultiBootParam mp;

    init_sio_normal(SIO_MASTER, SIO_32);

    print_multiboot_mid_process(0);
    prepare_flush();

    for(attempts = 0; attempts < 128; attempts++) {
        for(sends = 0; sends < 16; sends++) {
            response = multiboot_normal_send(0x6200);

            if((response & 0xfff0) == 0x7200) {
                clientMask = (response & 0xf);
                if(clientMask)
                    break;
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
    response = multiboot_normal_send(0x6100 | clientMask);
    if(response != (0x7200 | clientMask))
        return MB_WRONG_ANSWER;

    for(halves = 0; halves < 0x60; ++halves)
        if(multiboot_normal_send(*data++) != ((0x60 - halves) << 8 | clientMask))
            return MB_HEADER_ISSUE;

    response = multiboot_normal_send(0x6200);
    if(response != (clientMask))
        return MB_WRONG_ANSWER;

    response = multiboot_normal_send(0x6200 | clientMask);
    if(response != (0x7200 | clientMask))
        return MB_WRONG_ANSWER;

    while((response & 0xFF00) != 0x7300)
        response = multiboot_normal_send(paletteCmd);

    answer = response&0xFF;
    handshake = 0x11 + 0xFF + 0xFF + answer;

    response = multiboot_normal_send(0x6400 | handshake);
    if((response & 0xFF00) != 0x7300)
        return MB_WRONG_ANSWER;
    
    print_multiboot_mid_process(1);
    prepare_flush();
    VBlankIntrWait();
    
    mp.handshake_data = handshake;
    mp.client_data[0] = answer;
    mp.client_data[1] = 0xFF;
    mp.client_data[2] = 0xFF;
    mp.palette_data = palette;
    mp.client_bit = clientMask;
    mp.boot_srcp = (u8*)data;
    mp.boot_endp = (u8*)end;
    
    if(MultiBoot(&mp, MODE32_NORMAL))
        return MB_SWI_FAILURE;
    
    return MB_SUCCESS;
}
