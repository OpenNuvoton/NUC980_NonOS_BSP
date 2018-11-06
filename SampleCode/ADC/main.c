/**************************************************************************//**
 * @file     main.c
 * @version  V1.00
 * $Date: 15/05/07 6:35p $
 * @brief    NUC970 Driver Sample Code
 *
 * @note
 * Copyright (C) 2015 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nuc980.h"
#include "sys.h"
#include "adc.h"

extern int recvchar(void);
/*-----------------------------------------------------------------------------*/
volatile int normal_complete=0;
INT32 NormalConvCallback(UINT32 status, UINT32 userData)
{
    /*  The status content that contains normal data.
     */
    normal_complete=1;
    printf("\r normal data=0x%3x",status);
    return 0;
}
void normal_demo()
{
    char c;
    int val;
    printf("Select channel 0 ~ 8 (0~7 for external channel, 8 for Vref)\n");
    c = recvchar();
    val = c - '0';
    adcIoctl(NAC_ON,(UINT32)NormalConvCallback,0); //Enable Normal AD Conversion
    adcChangeChannel(val << ADC_CONF_CHSEL_Pos);
    do
    {
        adcIoctl(START_MST,0,0);
    }
    while(1);
}
/*-----------------------------------------------------------------------------*/
/*! Unlock protected register */
#define UNLOCKREG(x)  do{outpw(REG_SYS_REGWPCTL,0x59); outpw(REG_SYS_REGWPCTL,0x16); outpw(REG_SYS_REGWPCTL,0x88);}while(inpw(REG_SYS_REGWPCTL) == 0x00)
/*! Lock protected register */
#define LOCKREG(x)  do{outpw(REG_SYS_REGWPCTL,0x00);}while(0)

/*-----------------------------------------------------------------------------*/
void UART_Init()
{
    /* enable UART0 clock */
    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | 0x10000);

    /* GPF11, GPF12 */
    outpw(REG_SYS_GPF_MFPH, (inpw(REG_SYS_GPF_MFPH) & 0xfff00fff) | 0x11000);  // UART0 multi-function

    /* UART0 line configuration for (115200,n,8,1) */
    outpw(REG_UART0_LCR, inpw(REG_UART0_LCR) | 0x07);
    outpw(REG_UART0_BAUD, 0x30000066); /* 12MHz reference clock input, 115200 */
}

int main(void)
{
    *((volatile unsigned int *)REG_AIC_INTDIS0)=0xFFFFFFFF;  // disable all interrupt channel
    *((volatile unsigned int *)REG_AIC_INTDIS1)=0xFFFFFFFF;  // disable all interrupt channel
    *(volatile unsigned int *)(CLK_BA+0x18) |= (1<<16); /* Enable UART0 */
    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);

    UART_Init();

    printf("+-------------------------------------------------+\n");
    printf("|                 ADC Sample Code                 |\n");
    printf("+-------------------------------------------------+\n\n");
    adcOpen();
    normal_demo();
    while(1);
}
