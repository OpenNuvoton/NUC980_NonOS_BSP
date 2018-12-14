/**************************************************************************//**
 * @file     main.c
 * @version  V1.00
 * $Date: 15/05/11 10:06a $
 * @brief    Use internal SRAM as back end storage media to simulate a
 *           30 KB USB pen drive
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include "NUC980.h"
#include "sys.h"
#include "usbd.h"
#include "massstorage.h"


/*--------------------------------------------------------------------------*/
extern uint8_t volatile g_u8MscStart;
extern void USBD_IRQHandler(void);

void UART_Init()
{
    /* enable UART0 clock */
    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | 0x10000);

    /* GPF11, GPF12 */
    outpw(REG_SYS_GPF_MFPH, (inpw(REG_SYS_GPF_MFPH) & 0xfff00fff) | 0x11000);   // UART0 multi-function

    /* UART0 line configuration for (115200,n,8,1) */
    outpw(REG_UART0_LCR, inpw(REG_UART0_LCR) | 0x07);
    outpw(REG_UART0_BAUD, 0x30000066); /* 12MHz reference clock input, 115200 */
}

__asm void __wfi()
{
    MCR p15, 0, r1, c7, c0, 4
    BX  lr
}

void power_down()
{
    *(volatile unsigned int *)(0xB0000200) &= 0xFFFFFFFE;
    *(unsigned int volatile *)(0xB00001FC) = 0;
    __wfi();
}


/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main (void)
{
    unsigned int volatile i;

    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    UART_Init();
    printf("\n");
    printf("==========================\n");
    printf("     USB Mass Storage     \n");
    printf("==========================\n");

    outpw(REG_SYS_WKUPSER1, inpw(REG_SYS_WKUPSER1)|0x80000);
    sysInstallISR(IRQ_LEVEL_1, IRQ_UDC, (PVOID)USBD_IRQHandler);
    /* enable CPSR I bit */
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_UDC);

    USBD_Open(&gsInfo, MSC_ClassRequest, NULL);

    /* Endpoint configuration */
    MSC_Init();

    /* Start transaction */
    while(1)
    {
        if (USBD_IS_ATTACHED())
        {
            USBD_Start();
            break;
        }
        else
        {
            outpw(REG_USBD_PHYCTL, (inpw(REG_USBD_PHYCTL) & ~0x200) | 0x1000000);
            printf("power down 0x%x\n", inpw(REG_USBD_PHYCTL));
            /* enter power down */
            outpw(0xB00001FC, 0x59);    /* unlock */
            outpw(0xB00001FC, 0x16);
            outpw(0xB00001FC, 0x88);

            //enable NUC980 to enter power down mode
            while (!(inpw(REG_UART0_FSR) & (1<<22)));   // wait until TX empty
            power_down();
            printf("wakeup 0x%x\n", inpw(REG_USBD_PHYCTL));
        }
    }

    while(1)
    {
        if (g_u8MscStart)
            MSC_ProcessCmd();
    }
}



/*** (C) COPYRIGHT 2018 Nuvoton Technology Corp. ***/

