#include <gba.h>
#include "sio.h"

void sio_normal_inner_slave(void);
void sio_normal_inner_master(void);

int timed_sio_normal_master(int data, int is_32, int vCountWait) {
    u8 curr_vcount, target_vcount;
    
    if(is_32)
        REG_SIODATA32 = data;
    else
        REG_SIODATA8 = (data & 0xFF);
        
    
    // - Wait at least 36 us between sends (this is a bit more, but it works)
    curr_vcount = REG_VCOUNT;
    target_vcount = curr_vcount + vCountWait;
    if(target_vcount >= 0xE4)
        target_vcount -= 0xE4;
    while (target_vcount != REG_VCOUNT);
    
    // - Set Start flag.
    REG_SIOCNT |= SIO_START;
    // - Wait for IRQ (or for Start bit to become zero).
    while (REG_SIOCNT & SIO_START);

    // - Process received data.
    if(is_32)
        return REG_SIODATA32;
    else
        return (REG_SIODATA8 & 0xFF);
}

void sio_normal_inner_slave() {
    // - Set Start=0 and SO=0 (SO=LOW indicates that slave is (almost) ready).
    REG_SIOCNT &= ~(SIO_START | SIO_SO_HIGH);
    // - Set Start=1 and SO=1 (SO=HIGH indicates not ready, applied after transfer).
    //   (Expl. Old SO=LOW kept output until 1st clock bit received).
    //   (Expl. New SO=HIGH is automatically output at transfer completion).
    REG_SIOCNT |= SIO_START | SIO_SO_HIGH;
    // - Set SO to LOW to indicate that master may start now.
    REG_SIOCNT &= ~SIO_SO_HIGH;
    // - Wait for IRQ (or for Start bit to become zero). (Check timeout here!)
    while (REG_SIOCNT & SIO_START);
    
    //Stop next transfer
    REG_SIOCNT |= SIO_SO_HIGH;
}

void sio_handle_irq_slave() {
	REG_IF |= IRQ_SERIAL;

    REG_SIOCNT &= ~SIO_SO_HIGH;
	
	REG_SIODATA32 = SIO_DEFAULT_VALUE;
	REG_SIODATA8 = SIO_DEFAULT_VALUE;
	
	REG_SIOCNT &= ~(SIO_START | SIO_SO_HIGH);
    // - Set Start=1 and SO=1 (SO=HIGH indicates not ready, applied after transfer).
    //   (Expl. Old SO=LOW kept output until 1st clock bit received).
    //   (Expl. New SO=HIGH is automatically output at transfer completion).
    REG_SIOCNT |= SIO_START | SIO_SO_HIGH;
    // - Set SO to LOW to indicate that master may start now.
    REG_SIOCNT &= ~SIO_SO_HIGH;
}

void sio_stop_irq_slave() {
    REG_SIOCNT &= ~SIO_SO_HIGH;
    REG_SIOCNT &= ~SIO_IRQ;
}

void sio_normal_prepare_irq_slave(int data, int is_32) {
    // - Initialize data which is to be sent to master.
    if(is_32)
        REG_SIODATA32 = data;
    else
        REG_SIODATA8 = (data & 0xFF);
    
    REG_SIOCNT |= SIO_IRQ;

    // - Set Start=0 and SO=0 (SO=LOW indicates that slave is (almost) ready).
    REG_SIOCNT &= ~(SIO_START | SIO_SO_HIGH);
    // - Set Start=1 and SO=1 (SO=HIGH indicates not ready, applied after transfer).
    //   (Expl. Old SO=LOW kept output until 1st clock bit received).
    //   (Expl. New SO=HIGH is automatically output at transfer completion).
    REG_SIOCNT |= SIO_START | SIO_SO_HIGH;
    // - Set SO to LOW to indicate that master may start now.
    REG_SIOCNT &= ~SIO_SO_HIGH;
}

void sio_normal_inner_master() {
    // - Wait for SI to become LOW (slave ready). (Check timeout here!)
    while (REG_SIOCNT & SIO_RDY);
    // - Set Start flag.
    REG_SIOCNT |= SIO_START;
    // - Wait for IRQ (or for Start bit to become zero).
    while (REG_SIOCNT & SIO_START);
}

void init_sio_normal(int is_master, int is_32) {
    u16 sio_cnt_val = 0;
    
    if(is_32)
        sio_cnt_val |= SIO_32BIT;
    else
        sio_cnt_val |= SIO_8BIT;
    
    if(is_master)
        sio_cnt_val |= SIO_CLK_INT;
    else
        sio_cnt_val |= SIO_SO_HIGH;
        
    
    REG_RCNT = R_NORMAL;
    REG_SIOCNT = sio_cnt_val;
}

int sio_normal(int data, int is_master, int is_32) {
    // - Initialize data which is to be sent to master.
    if(is_32)
        REG_SIODATA32 = data;
    else
        REG_SIODATA8 = (data & 0xFF);
    
    if(is_master)
        sio_normal_inner_master();
    else
        sio_normal_inner_slave();
    
    // - Process received data.
    if(is_32)
        return REG_SIODATA32;
    else
        return (REG_SIODATA8 & 0xFF);
}
