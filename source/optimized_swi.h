#ifndef OPTIMIZED_SWI__
#define OPTIMIZED_SWI__

static __attribute__((optimize(3), always_inline)) s32 SWI_DivMod(const uint32_t dividend, const uint32_t divisor)
{
    register uint32_t divid_ asm("r0") = (uint32_t)dividend;
    register uint32_t divis_ asm("r1") = (uint32_t)divisor;
    register uint32_t abs_divis_ asm("r3");

    asm volatile(
        "swi 0x06;" : "=r"(divid_), "=r"(divis_), "=r"(abs_divis_) :
        "r"(divid_), "r"(divis_) :
    );
    
    return divis_;
}

static __attribute__((optimize(3), always_inline)) s32 SWI_Div(const uint32_t dividend, const uint32_t divisor)
{
    register uint32_t divid_ asm("r0") = (uint32_t)dividend;
    register uint32_t divis_ asm("r1") = (uint32_t)divisor;
    register uint32_t abs_divis_ asm("r3");

    asm volatile(
        "swi 0x06;" : "=r"(divid_), "=r"(divis_), "=r"(abs_divis_) :
        "r"(divid_), "r"(divis_) :
    );
    
    return divid_;
}

static __attribute__((optimize(3), always_inline)) s32 SWI_DivDivMod(const uint32_t dividend, const uint32_t divisor, uint32_t* mod)
{
    register uint32_t divid_ asm("r0") = (uint32_t)dividend;
    register uint32_t divis_ asm("r1") = (uint32_t)divisor;
    register uint32_t abs_divis_ asm("r3");

    asm volatile(
        "swi 0x06;" : "=r"(divid_), "=r"(divis_), "=r"(abs_divis_) :
        "r"(divid_), "r"(divis_) :
    );
    
    *mod = divis_;
    return divid_;
}

#endif