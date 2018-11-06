/**************************************************************************//**
 * @file     main.c
 * @brief    Demonstrate smartcard UART mode
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include "nuc980.h"
#include "sys.h"
#include "scuart.h"

char au8TxBuf[] = "Hello World!";


/**
  * @brief  The interrupt services routine of smartcard port 0
  * @param  None
  * @retval None
  */
void SC0_IRQHandler(void)
{
    // Print SCUART received data to UART port
    // Data length here is short, so we're not care about UART FIFO over flow.
    printf("%c", SCUART_READ(0));

    // RDA is the only interrupt enabled in this sample, this status bit
    // automatically cleared after Rx FIFO empty. So no need to clear interrupt
    // status here.

    return;
}

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

    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    UART_Init();

    outpw(REG_CLK_PCLKEN1, inpw(REG_CLK_PCLKEN1) | (1 << 12)); // Enable SC0 engine clock
    outpw(REG_SYS_GPC_MFPH, (inpw(REG_SYS_GPC_MFPH) & ~(0xFF0000)) | 0x440000); // Enable SCUART Tx/Rx pin


    printf("This sample code demos smartcard interface UART mode\n");
    printf("Please connect SC0 CLK pin(PC.12) with SC0 I/O pin(PC.13)\n");
    printf("Hit any key to continue\n");
    getchar();

    // Open smartcard interface 0 in UART mode.
    SCUART_Open(0, 115200);
    // Enable receive interrupt
    SCUART_ENABLE_INT(0, SC_INTEN_RDAIEN_Msk);

    sysInstallISR(IRQ_LEVEL_1, IRQ_SMC0, (PVOID)SC0_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_SMC0);


    SCUART_Write(0, au8TxBuf, sizeof(au8TxBuf));

    while(1);
}



