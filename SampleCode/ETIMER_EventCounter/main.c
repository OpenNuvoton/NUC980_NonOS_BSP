/**************************************************************************//**
* @file     main.c
* @brief    NUC980 ETIMER Sample Code
*
* @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
******************************************************************************/
#include <stdio.h>
#include "nuc980.h"
#include "sys.h"
#include "gpio.h"
#include "etimer.h"

void ETMR0_IRQHandler(void)
{
    printf("Count 1000 falling events! Test complete.\n");
    // clear timer interrupt flag
    ETIMER_ClearIntFlag(0);
}
/*-----------------------------------------------------------------------------*/
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
int main(void)
{
    uint32_t i;
    UART_Init();
    printf("\nThis sample code use TM0_CNT(PA.0) to count PB.3 input event\n");
    printf("Please connect PA.0 to PB.3, press any key to continue\n");
    getchar();
    outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | (1 << 11)); // Enable GPIO engine clock

    /* Configure PB.3 as Output mode */
    GPIO_SetMode(PB, BIT3, GPIO_MODE_OUTPUT);
    PB3 = 1;

    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | (1 << 8)); // Enable ETIMER0 engine clock
    outpw(REG_SYS_GPA_MFPL, (inpw(REG_SYS_GPA_MFPL) & ~(0xF << 0)) | (0x6 << 0)); // Enable ETIMER0 event counting pin @ PA0

    // Give a dummy target frequency here. Will over write prescale and compare value with macro
    ETIMER_Open(0, ETIMER_ONESHOT_MODE, 100);

    // Update prescale and compare value to what we need in event counter mode.
    outpw(REG_ETMR0_PRECNT, 0);
    outpw(REG_ETMR0_CMPR, 1000);
    // Counter increase on falling edge
    ETIMER_EnableEventCounter(0, TIMER_COUNTER_FALLING_EDGE);

    // Enable timer interrupt
    ETIMER_EnableInt(0);
    sysInstallISR(IRQ_LEVEL_1, IRQ_TIMER0, (PVOID)ETMR0_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_TIMER0);

    // Start Timer 0
    ETIMER_Start(0);

    for(i = 0; i < 1000; i++)
    {
        PB3 = 0; // low
        PB3 = 1;  // high
    }

    while(1);
}
