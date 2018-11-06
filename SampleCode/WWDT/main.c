/**************************************************************************//**
 * @file     main.c
 * @brief    WWDT Sample Code
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include "nuc980.h"
#include "sys.h"
#include "wwdt.h"

void WWDT_IRQHandler(void)
{

    // Reload WWDT counter and clear WWDT interrupt flag
    WWDT_RELOAD_COUNTER();
    WWDT_CLEAR_INT_FLAG();
    printf("WWDT counter reload\n");

}

void UART_Init(void)
{
    /* enable UART0 clock */
    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | 0x10000);

    /* GPF11, GPF12 */
    outpw(REG_SYS_GPF_MFPH, (inpw(REG_SYS_GPF_MFPH) & 0xfff00fff) | 0x11000);   // UART0 multi-function

    /* UART0 line configuration for (115200,n,8,1) */
    outpw(REG_UART0_LCR, inpw(REG_UART0_LCR) | 0x07);
    outpw(REG_UART0_BAUD, 0x30000066); /* 12MHz reference clock input, 115200 */
}


int main(void)
{
    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    UART_Init();

    printf("\nThis sample code demonstrate WWDT reload function\n");


    sysInstallISR(IRQ_LEVEL_1, IRQ_WWDT, (PVOID)WWDT_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_WWDT);

    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | 2); // Enable WWDT engine clock

    // WWDT timeout every 2048 * 64 WWDT clock, compare interrupt trigger at 2048 * 32 WWDT clock. About every 0.7 sec
    // enable WWDT counter compare interrupt. Default WWDT clock source is 12MHz / 128.
    WWDT_Open(WWDT_PRESCALER_2048, 0x20, TRUE);

    while(1);
}
