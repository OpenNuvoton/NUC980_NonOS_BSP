/**************************************************************************//**
* @file     main.c
* @brief    NUC980 ETIMER Sample Code
*
* @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
******************************************************************************/
#include <stdio.h>
#include "nuc980.h"
#include "sys.h"
#include "etimer.h"

void ETMR0_IRQHandler(void)
{
    // printf takes long time and affect the freq. calculation, we only print out once a while
    static int cnt = 0;
    static UINT t0, t1;

    if(cnt == 0)
    {
        t0 = ETIMER_GetCaptureData(0);
        cnt++;
    }
    else if(cnt == 1)
    {
        t1 = ETIMER_GetCaptureData(0);
        cnt++;
        if(t0 > t1)
        {
            // over run, drop this data and do nothing
        }
        else
        {
            printf("Input frequency is %dHz\n", 12000000 / (t1 - t0));
        }
    }
    else
    {
        cnt = 0;
    }

    ETIMER_ClearCaptureIntFlag(0);
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
    UART_Init();

    // Enable ETIMER0 engine clock
    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | (1 << 8));
    // Enable ETIMER0 Capture pin @ PB1
    outpw(REG_SYS_GPB_MFPL, (inpw(REG_SYS_GPB_MFPL) & ~(0x000000F0)) | 0x00000050);

    printf("\nThis sample code demonstrate timer free counting mode.\n");
    printf("Please connect input source with Timer 0 capture pin PB.1, press any key to continue\n");
    getchar();

    // Give a dummy target frequency here. Will over write capture resolution with macro
    ETIMER_Open(0, ETIMER_PERIODIC_MODE, 1000000);

    // Update prescale for better resolution.
    ETIMER_SET_PRESCALE_VALUE(0, 0);

    // Set compare value as large as possible, so don't need to worry about counter overrun too frequently.
    ETIMER_SET_CMP_VALUE(0, 0xFFFFFF);

    // Configure Timer 0 free counting mode, capture TDR value on rising edge
    ETIMER_EnableCapture(0, ETIMER_CAPTURE_FREE_COUNTING_MODE, ETIMER_CAPTURE_RISING_EDGE);

    // Start Timer 0
    ETIMER_Start(0);

    // Enable timer interrupt
    ETIMER_EnableCaptureInt(0);

    sysInstallISR(IRQ_LEVEL_1, IRQ_TIMER0, (PVOID)ETMR0_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_TIMER0);

    while(1);
}
