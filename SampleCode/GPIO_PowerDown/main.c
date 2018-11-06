/**************************************************************************//**
 * @file     main.c
 * @brief    Show how to wake up system from Power-down mode by GPIO interrupt.
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include "nuc980.h"
#include "sys.h"
#include "gpio.h"
#include <stdio.h>

/*---------------------------------------------------------------------------------------------------------*/
/*  Function for System Entry to Power Down Mode                                                           */
/*---------------------------------------------------------------------------------------------------------*/
__asm void __wfi()
{
    MCR p15, 0, r1, c7, c0, 4
    BX            lr
}
void PowerDownFunction(void)
{
    uint32_t i;
    while (!(inpw(REG_UART0_FSR) & (1<<22))); //waits for TX_EMPTY
    i = *(volatile unsigned int *)(0xB0000200);
    i = i & (0xFFFFFFFE);
    *(volatile unsigned int *)(0xB0000200)=i;
    __wfi();
}

/**
 * @brief       GPIO PB IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The PB default IRQ, declared in startup_M480.s.
 */
void GPB_IRQHandler(void)
{
    /* To check if PB.3 interrupt occurred */
    if(GPIO_GET_INT_FLAG(PB, BIT3))
    {
        GPIO_CLR_INT_FLAG(PB, BIT3);
        printf("PB.3 INT occurred.\n");
    }
    else
    {
        /* Un-expected interrupt. Just clear all PB interrupts */
        PB->INTSRC = PB->INTSRC;
        printf("Un-expected interrupts.\n");
    }
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

/*---------------------------------------------------------------------------------------------------------*/
/* MAIN function                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
int main(void)
{
    outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN)|(1<<11)); //Enable GPIO engine
    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    UART_Init();;

    printf("+-------------------------------------------------------+\n");
    printf("|    GPIO Power-Down and Wake-up by PB.3 Sample Code    |\n");
    printf("+-------------------------------------------------------+\n\n");

    sysInstallISR(IRQ_LEVEL_1, IRQ_GPB, (PVOID)GPB_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_GPB);

    /* Configure PB.3 as Input mode and enable interrupt by rising edge trigger */
    GPIO_SetMode(PB, BIT3, GPIO_MODE_INPUT);
    GPIO_EnableInt(PB, 3, GPIO_INT_RISING);

    /* Enable interrupt de-bounce function and select de-bounce sampling cycle time is 1024 clocks of LIRC clock */
    GPIO_SET_DEBOUNCE_TIME(GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_1024);
    GPIO_ENABLE_DEBOUNCE(PB, BIT3);

    /* Enable GPIO wake up */
    outpw(REG_SYS_WKUPSER0, inpw(REG_SYS_WKUPSER0)|(1<<3));

    /* Waiting for PB.3 rising-edge interrupt event */
    while(1)
    {
        printf("Enter to Power-Down ......\n");

        /* Enter to Power-down mode */
        PowerDownFunction();

        printf("System waken-up done. %x",inpw(REG_SYS_WKUPSSR0));

        /* Clear GPIO wakeup flag */
        outpw(REG_SYS_WKUPSSR0, inpw(REG_SYS_WKUPSSR0)|(1<<3));

        printf(", %x\n\n",inpw(REG_SYS_WKUPSSR0));
    }

}
