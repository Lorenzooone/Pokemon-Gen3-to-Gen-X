#ifndef OPTIMIZED_SWI__
#define OPTIMIZED_SWI__

#include "base_include.h"
#include "useful_qualifiers.h"

static s32 SWI_DivMod(const uint32_t, const uint32_t);
static s32 SWI_Div(const uint32_t, const uint32_t);
static s32 SWI_DivDivMod(const uint32_t, const uint32_t, int* mod);

ALWAYS_INLINE MAX_OPTIMIZE s32 SWI_DivMod(const uint32_t dividend, const uint32_t divisor)
{
    register uint32_t divid_ asm("r0") = (uint32_t)dividend;
    register uint32_t divis_ asm("r1") = (uint32_t)divisor;
    register uint32_t abs_divis_ asm("r3");

    asm volatile(
        "swi "DIV_SWI_VAL";" : "=r"(divid_), "=r"(divis_), "=r"(abs_divis_) :
        "r"(divid_), "r"(divis_), "r"(abs_divis_) :
    );
    
    return divis_;
}

ALWAYS_INLINE MAX_OPTIMIZE s32 SWI_Div(const uint32_t dividend, const uint32_t divisor)
{
    register uint32_t divid_ asm("r0") = (uint32_t)dividend;
    register uint32_t divis_ asm("r1") = (uint32_t)divisor;
    register uint32_t abs_divis_ asm("r3");

    asm volatile(
        "swi "DIV_SWI_VAL";" : "=r"(divid_), "=r"(divis_), "=r"(abs_divis_) :
        "r"(divid_), "r"(divis_), "r"(abs_divis_) :
    );
    
    return divid_;
}

ALWAYS_INLINE MAX_OPTIMIZE s32 SWI_DivDivMod(const uint32_t dividend, const uint32_t divisor, int* mod)
{
    register uint32_t divid_ asm("r0") = (uint32_t)dividend;
    register uint32_t divis_ asm("r1") = (uint32_t)divisor;
    register uint32_t abs_divis_ asm("r3");

    asm volatile(
        "swi "DIV_SWI_VAL";" : "=r"(divid_), "=r"(divis_), "=r"(abs_divis_) :
        "r"(divid_), "r"(divis_), "r"(abs_divis_) :
    );
    
    *mod = divis_;
    return divid_;
}

#endif
