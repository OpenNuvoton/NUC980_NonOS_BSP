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
    __asm
    {
        MRS    _old, CPSR
        ORR    _new, _old, DISABLE_FIQ_IRQ
        MSR    CPSR_c, _new
    }
    return(_old);
}

void sys_arch_unprotect(int pval)
{
    __asm
    {
        MSR    CPSR_c, pval
    }

    return;
}


