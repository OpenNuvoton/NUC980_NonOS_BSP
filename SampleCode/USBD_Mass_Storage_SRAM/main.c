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
#include "nuc980.h"
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

    USBD_Open(&gsInfo, MSC_ClassRequest, NULL);

    /* enable CPSR I bit */
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_UDC);

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
    }

    while(1)
    {
        if (g_u8MscStart)
            MSC_ProcessCmd();
    }
}



/*** (C) COPYRIGHT 2018 Nuvoton Technology Corp. ***/

