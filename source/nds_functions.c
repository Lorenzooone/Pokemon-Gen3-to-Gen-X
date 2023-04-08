#include "base_include.h"
#include "useful_qualifiers.h"
#include "nds_functions.h"

#ifdef __NDS__
ARM_TARGET MAX_OPTIMIZE void Halt_ARM9() {
    register uint32_t destroyed asm("r0");
    asm volatile(
            "mov r0, #0; mcr p15, 0, r0, c7, c0, 4;" : "=r"(destroyed) :
            "r"(destroyed) :
        );
}
#endif
