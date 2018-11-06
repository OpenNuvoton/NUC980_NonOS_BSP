/**************************************************************************//**
 * @file     main.c
 * @version  V1.00
 * @brief    Simulate an USB mouse and draws circle on the screen
 *
 * @copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include "NUC980.h"
#include "sys.h"
#include "usbd.h"
#include "hid_mouse.h"


extern void USBD_IRQHandler(void);

/*--------------------------------------------------------------------------*/
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


int32_t main (void)
{
    UART_Init();
    printf("\n");
    printf("=======================\n");
    printf("     USB HID Mouse     \n");
    printf("=======================\n");

    sysInstallISR(IRQ_LEVEL_1, IRQ_UDC, (PVOID)USBD_IRQHandler);
    /* enable CPSR I bit */
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_UDC);

    USBD_Open(&gsInfo, HID_ClassRequest, NULL);

    /* Endpoint configuration */
    HID_Init();

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
        HID_UpdateMouseData();
    }
}



/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/

