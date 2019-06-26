/**************************************************************************//**
 * @file     sys_arch.c
 * @brief    LwIP system architecture file
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include "nuc980.h"
#include "sys.h"

extern uint32_t sysTick;
UINT32 sys_now(void)
{
    return sysTick * 10;
}

int sys_arch_protect(void)
{
    int _old, _new;
#if defined ( __GNUC__ ) && !(__CC_ARM)

    asm volatile (" mrs %0, cpsr" : "=r" (_old) :  );
    _new = _old | 0x80;
    asm volatile (" msr cpsr, %0":  :"r" (_new));

#else
    __asm
    {
        MRS    _old, CPSR
        ORR    _new, _old, DISABLE_FIQ_IRQ
        MSR    CPSR_c, _new
    }
#endif
    return(_old);
}

void sys_arch_unprotect(int pval)
{
#if defined ( __GNUC__ ) && !(__CC_ARM)

    asm volatile (" msr cpsr, %0":  :"r" (pval));

#else
    __asm
    {
        MSR    CPSR_c, pval
    }
#endif
    return;
}
