#ifndef BASE_INCLUDE__
#define BASE_INCLUDE__

#ifdef __GBA__

// GBA defines and all
#include <gba.h>
#include "useful_qualifiers.h"
#define SCANLINES 0xE4
#define ROM 0x8000000
ALWAYS_INLINE MAX_OPTIMIZE void __set_next_vcount_interrupt_gba(int scanline) {
    REG_DISPSTAT  = (REG_DISPSTAT & 0xFF) | (scanline<<8);
}
ALWAYS_INLINE MAX_OPTIMIZE int __get_next_vcount_interrupt(void) {
    u16 reg_val = REG_DISPSTAT;
    return reg_val >> 8;
}
#define __set_next_vcount_interrupt(x) __set_next_vcount_interrupt_gba(x)
#define SCANLINE_IRQ_BIT LCDC_VCNT
#define REG_WAITCNT *(vu16*)(REG_BASE + 0x204) // Wait state Control
#define SRAM_READING_VALID_WAITCYCLES 3
#define SRAM_ACCESS_CYCLES 9
#define NON_SRAM_MASK 0xFFFC
#define BASE_WAITCNT_VAL 0x4314
#define OVRAM_START ((uintptr_t)OBJ_BASE_ADR)
#define TILE_1D_MAP 0
#define ACTIVATE_SCREEN_HW 0
#define EWRAM_SIZE 0x0003FF40
#define DIV_SWI_VAL "0x06"
#define VRAM_0 VRAM
#define HAS_SIO
#define CLOCK_SPEED 16777216
#define SAME_ON_BOTH_SCREENS 0
#define CONSOLE_LETTER 'G'

#endif

#ifdef __NDS__

// NDS defines and all
#include <nds.h>
#include "useful_qualifiers.h"
#include "decompress.h"
#define SCANLINES 0x107
#define ROM GBAROM
ALWAYS_INLINE MAX_OPTIMIZE int __get_next_vcount_interrupt(void) {
    u16 reg_val = REG_DISPSTAT;
    return (reg_val >> 8) | ((reg_val & 0x80) << 1);
}
ALWAYS_INLINE MAX_OPTIMIZE void __reset_vcount(void) {
    REG_VCOUNT = (REG_VCOUNT & 0xFE00) | (SCANLINES - 2);
}
#define __set_next_vcount_interrupt(x) SetYtrigger(x)
#define SCANLINE_IRQ_BIT DISP_YTRIGGER_IRQ
#define REG_WAITCNT REG_EXMEMCNT // Wait state Control
#define SRAM_READING_VALID_WAITCYCLES 3
#define SRAM_ACCESS_CYCLES (2 * 18)
#define NON_SRAM_MASK 0xFFFC
#define BASE_WAITCNT_VAL 0x0014
#define OVRAM_START ((uintptr_t)SPRITE_GFX)
#define OVRAM_START_SUB ((uintptr_t)SPRITE_GFX_SUB)
#define OBJ_ON DISPLAY_SPR_ACTIVE
#define OBJ_1D_MAP (1<<6)
#define TILE_1D_MAP (1<<4)
#define ACTIVATE_SCREEN_HW (1<<16)
#define EWRAM 0x2000000
#define EWRAM_SIZE 0x00400000
#define VRAM_0_SUB BG_GFX_SUB
#define ARM9_CLOCK_SPEED 67108864
#define ARM7_CLOCK_SPEED 33554432
#define ARM7_GBA_CLOCK_SPEED 16777216
#define CLOCK_SPEED ARM9_CLOCK_SPEED
#define SAME_ON_BOTH_SCREENS 1
#define CONSOLE_LETTER 'D'

// Define OBJATTR struct back
typedef struct {
    u16 attr0;
    u16 attr1;
    u16 attr2;
    u16 dummy;
} ALIGN(4) OBJATTR;

// Define SWIs back
#define Div(x, y) swiDivide(x, y)
#define DivMod(x, y) swiRemainder(x, y)
#define Sqrt(x) swiSqrt(x)
#define LZ77UnCompWram(x, y) swi_LZ77UnCompWrite8bit(x, y)
#define CpuFastSet(x, y, z) swiFastCopy(x, y, z)
#define VBlankIntrWait() swiWaitForVBlank()
#define Halt() CP15_WaitForInterrupt()
#define DIV_SWI_VAL "0x09"

#endif

#define VBLANK_SCANLINES SCREEN_HEIGHT

#endif
