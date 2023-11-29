#include "base_include.h"
#include <agbabi.h>
#include "input_handler.h"
#include "menu_text_handler.h"
#include "print_system.h"
#include "sprite_handler.h"
#include "config_settings.h"
#include "rtc_helpers.h"
#include <stddef.h>

#define BASE_SCREEN 0

void vblank_update_function(void);
void disable_all_irqs(void);
void cursor_update_new_clock_menu(u8);
int main(void);

u32 counter = 0;

IWRAM_CODE void vblank_update_function() {
    REG_IF |= IRQ_VBLANK;

    flush_screens();

    move_sprites(counter);
    move_cursor_x(counter);
    counter++;

    #ifdef __NDS__
    // Increase FPS on NDS
    //__reset_vcount();
    #endif
}

void disable_all_irqs() {
    REG_IME = 0;
    REG_IE = 0;
}

void cursor_update_new_clock_menu(u8 cursor_y_pos) {
    update_cursor_y(BASE_Y_CURSOR_MAIN_MENU + (BASE_Y_CURSOR_INCREMENT_MAIN_MENU * cursor_y_pos));
}

int main(void)
{
    #ifdef __GBA__
    RegisterRamReset(RESET_SIO|RESET_SOUND|RESET_OTHER);
    disable_all_irqs();
    #endif
    counter = 0;
    set_default_settings();
    init_text_system();
    u32 keys;
    
    init_sprites();
    init_oam_palette();
    init_sprite_counter();
    enable_sprites_rendering();
    init_numbers();

    #ifdef __GBA__
    irqInit();
    #endif
    irqSet(IRQ_VBLANK, vblank_update_function);
    irqEnable(IRQ_VBLANK);
    
    init_cursor();
    
    u8 returned_val;
    u8 update = 0;
    u8 cursor_y_pos = 0;
    u32 date = 0x00010100;
    u32 time = 0;

    set_screen(BASE_SCREEN);
    print_clock();
    print_new_clock(date, time, 1);
    enable_screen(BASE_SCREEN);
    enable_screen(BASE_SCREEN + 1);
    update_cursor_base_x(BASE_X_CURSOR_MAIN_MENU);
    cursor_update_new_clock_menu(cursor_y_pos);

    scanKeys();
    keys = keysDown();

    while(!(keys & KEY_B)) {
        prepare_flush();
        VBlankIntrWait();
        if(keys) {
            returned_val = handle_input_new_clock_menu(keys, &date, &time, &cursor_y_pos);
            if(returned_val == SET_NEW_CLOCK && (!__agbabi_rtc_init()))
                __agbabi_rtc_setdatetime((volatile __agbabi_datetime_t) {
                        bcd_encode_bytes(date, 4),
                        bcd_encode_bytes(time, 3)
                }); // Set date & time
            else if(returned_val)
                update = 1;
        }
        print_clock();
        print_new_clock(date, time, update);
        cursor_update_new_clock_menu(cursor_y_pos);
        update = 0;
        scanKeys();
        keys = keysDown();
    }

    return 0;
}
