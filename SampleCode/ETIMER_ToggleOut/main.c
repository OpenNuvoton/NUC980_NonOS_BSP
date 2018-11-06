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

    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | (1 << 8)); // Enable ETIMER0 engine clock
    outpw(REG_SYS_GPB_MFPL, (inpw(REG_SYS_GPB_MFPL) & ~(0xF << 12)) | (0x5 << 12)); // Enable ETIMER0 toggle out pin @ PB3

    printf("\nThis sample code use timer 0 to generate 500Hz toggle output to PB.3 pin\n");

    /* To generate 500HZ toggle output, timer frequency must set to 1000Hz.
       Because toggle output state change on every timer timeout event */
    ETIMER_Open(0, ETIMER_TOGGLE_MODE, 1000);
    ETIMER_Start(0);

    while(1);

}
