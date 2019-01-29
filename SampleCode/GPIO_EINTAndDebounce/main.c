/**************************************************************************//**
 * @file     main.c
 * @brief    Show the usage of GPIO external interrupt function and de-bounce function.
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include "nuc980.h"
#include "sys.h"
#include "gpio.h"
#include <stdio.h>

/**
 * @brief       External INT0 IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The External INT0 default IRQ, declared in startup_M480.s.
 */
void EINT0_IRQHandler(void)
{

    /* To check if PA.6 external interrupt occurred */
    if(GPIO_GET_INT_FLAG(PA, BIT0))
    {
        GPIO_CLR_INT_FLAG(PA, BIT0);
        printf("PA.0 EINT0 occurred.\n");
    }

    /* To check if PA.13 external interrupt occurred */
    if(GPIO_GET_INT_FLAG(PA, BIT13))
    {
        GPIO_CLR_INT_FLAG(PA, BIT13);
        printf("PA.13 EINT0 occurred.\n");
    }

}

/**
 * @brief       External INT1 IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The External INT1 default IRQ, declared in startup_M480.s.
 */
void EINT1_IRQHandler(void)
{

    /* To check if PA.1 external interrupt occurred */
    if(GPIO_GET_INT_FLAG(PA, BIT1))
    {
        GPIO_CLR_INT_FLAG(PA, BIT1);
        printf("PA.1 EINT1 occurred.\n");
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
    UART_Init();

    printf("+------------------------------------------------------------+\n");
    printf("|    GPIO EINT0/EINT1 Interrupt and De-bounce Sample Code    |\n");
    printf("+------------------------------------------------------------+\n\n");

    /*-----------------------------------------------------------------------------------------------------*/
    /* GPIO External Interrupt Function Test                                                               */
    /*-----------------------------------------------------------------------------------------------------*/
    printf("EINT0(PA.0 and PA.13) and EINT1(PA.1) are used to test interrupt\n");

    sysInstallISR(IRQ_LEVEL_1, IRQ_EXTI0, (PVOID)EINT0_IRQHandler);
    sysInstallISR(IRQ_LEVEL_1, IRQ_EXTI1, (PVOID)EINT1_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_EXTI0);
    sysEnableInterrupt(IRQ_EXTI1);

    /* Configure PA.0 as EINT0 pin and enable interrupt by falling edge trigger */
    GPIO_MFP_PA0MFP_EINT0;
    GPIO_SetMode(PA, BIT0, GPIO_MODE_INPUT);
    GPIO_EnableInt(PA, 0, GPIO_INT_FALLING);

    /* Configure PA.13 as EINT0 pin and enable interrupt by rising edge trigger */
    GPIO_MFP_PA13MFP_EINT0;
    GPIO_SetMode(PA, BIT13, GPIO_MODE_INPUT);
    GPIO_EnableInt(PA, 13, GPIO_INT_RISING);

    /* Configure PA.1 as EINT1 pin and enable interrupt by falling and rising edge trigger */
    GPIO_MFP_PA1MFP_EINT1;
    GPIO_SetMode(PA, BIT1, GPIO_MODE_INPUT);
    GPIO_EnableInt(PA, 1, GPIO_INT_BOTH_EDGE);

    /* Enable interrupt de-bounce function and select de-bounce sampling cycle time is 1024 clocks of LIRC clock */
    GPIO_SET_DEBOUNCE_TIME(GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_1024);
    GPIO_ENABLE_DEBOUNCE(PA, BIT0);
    GPIO_ENABLE_DEBOUNCE(PA, BIT13);
    GPIO_ENABLE_DEBOUNCE(PA, BIT1);

    /* Waiting for interrupts */
    while(1);
}
