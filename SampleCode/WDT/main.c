/**************************************************************************//**
 * @file     main.c
 * @brief    WDT Sample Code
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include "nuc980.h"
#include "sys.h"
#include "wdt.h"


void WDT_IRQHandler(void)
{
    // Reload WWDT counter and clear WWDT interrupt flag
    WDT_RESET_COUNTER();
    WDT_CLEAR_TIMEOUT_INT_FLAG();
    printf("Reset WDT counter\n");

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

    printf("\nThis sample code demonstrate reset WDT function\n");

    // Disable write protect mode to control WDT register
    SYS_UnlockReg();

    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | 1); // Enable WDT engine clock

    sysInstallISR(IRQ_LEVEL_1, IRQ_WDT, (PVOID)WDT_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_WDT);

    /* Configure WDT settings and start WDT counting, Set WDT time out interval
       to 2^14 Twdt = 0.7 sec. Where Twdt = 12MHZ / 512 */
    WDT_Open(WDT_TIMEOUT_2POW14, WDT_RESET_DELAY_18CLK, TRUE, TRUE);

    /* Enable WDT interrupt function */
    WDT_EnableInt();

    while(1);
}
