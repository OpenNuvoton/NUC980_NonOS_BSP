/**************************************************************************//**
 * @file     main.c
 * @version  V3.00
 * @brief    Use RTC alarm interrupt event to wake up system.
 * @note
 * Copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include "nuc980.h"
#include "sys.h"
#include "uart.h"
#include "rtc.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Global Interface Variables Declarations                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
volatile uint8_t g_u8IsRTCAlarmINT = 0;

#if defined ( __GNUC__ ) && !(__CC_ARM)
void __wfi(void)
{
    asm volatile(
    "MCR p15, 0, r1, c7, c0, 4  "
    );
}
#else
__asm void __wfi()
{
    MCR p15, 0, r1, c7, c0, 4
    BX  lr
}
#endif

/**
 * @brief       IRQ Handler for RTC Interrupt
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The RTC_IRQHandler is default IRQ of RTC, declared in startup_M2351.s.
 */
void RTC_IRQHandler(void)
{
    /* To check if RTC alarm interrupt occurred */
    if(RTC_GET_ALARM_INT_FLAG() == 1)
    {
        /* Clear RTC alarm interrupt flag */
        RTC_WaitAccessEnable();
        RTC_CLEAR_ALARM_INT_FLAG();
        RTC_Check();

        g_u8IsRTCAlarmINT++;
    }
}

void UART0_Init(void)
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
/*  MAIN function                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
int main(void)
{
    uint32_t i;
    S_RTC_TIME_DATA_T sWriteRTC, sReadRTC;

    //sysDisableCache();
    //sysFlushCache(I_D_CACHE);
    //sysEnableCache(CACHE_WRITE_BACK);
    UART0_Init();

    // Enable IP clock
    outpw(REG_CLK_PCLKEN0, (inpw(REG_CLK_PCLKEN0) | (1 << 2)));

    printf("+-------------------------------------+\n");
    printf("|    RTC Alarm Wake-up Sample Code    |\n");
    printf("+-------------------------------------+\n\n");

    sysInstallISR(IRQ_LEVEL_1, IRQ_RTC, (PVOID)RTC_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_RTC);

    /* Open RTC */
    sWriteRTC.u32Year       = 2017;
    sWriteRTC.u32Month      = 3;
    sWriteRTC.u32Day        = 15;
    sWriteRTC.u32DayOfWeek  = RTC_SUNDAY;
    sWriteRTC.u32Hour       = 23;
    sWriteRTC.u32Minute     = 59;
    sWriteRTC.u32Second     = 40;
    sWriteRTC.u32TimeScale  = RTC_CLOCK_24;
    RTC_Open(&sWriteRTC);

    /* Set RTC alarm date/time */
    sWriteRTC.u32Year       = 2017;
    sWriteRTC.u32Month      = 3;
    sWriteRTC.u32Day        = 15;
    sWriteRTC.u32DayOfWeek  = RTC_SUNDAY;
    sWriteRTC.u32Hour       = 23;
    sWriteRTC.u32Minute     = 59;
    sWriteRTC.u32Second     = 55;
    RTC_SetAlarmDateAndTime(&sWriteRTC);

    /* Enable RTC alarm interrupt and wake-up function will be enabled also */
    RTC_EnableInt(RTC_INTEN_ALMIEN_Msk | RTC_INTEN_WAKEUPIEN_Msk);
    outpw(REG_SYS_WKUPSER1, inpw(REG_SYS_WKUPSER1) | (0x1 << 7));

    printf("# Set RTC current date/time: 2017/03/15 23:59:40.\n");
    printf("# Set RTC alarm date/time:   2017/03/15 23:59:55.\n");
    printf("# Wait system waken-up by RTC alarm interrupt event.\n");

    getchar();

    g_u8IsRTCAlarmINT = 0;

    /* System enter to Power-down */
    /* To program PWRCTL register, it needs to disable register protection first. */
    // Unlock Register
    outpw(0xB00001FC, 0x59);
    outpw(0xB00001FC, 0x16);
    outpw(0xB00001FC, 0x88);
    while(!(inpw(0xB00001FC) & 0x1));

    printf("\nSystem enter to power-down mode ...\n");
    /* To check if all the debug messages are finished */
    while(!(inpw(REG_UART0_FSR) & (1<<28)));

    //enable NUC980 to enter power down mode
    i = *(volatile unsigned int *)(0xB0000200);
    i = i & (0xFFFFFFFE);
    *(volatile unsigned int *)(0xB0000200)=i;

    __wfi();

    while(g_u8IsRTCAlarmINT == 0);

    /* Read current RTC date/time */
    RTC_GetDateAndTime(&sReadRTC);
    printf("System has been waken-up and current date/time is:\n");
    printf("    %d/%02d/%02d %02d:%02d:%02d\n",
           sReadRTC.u32Year, sReadRTC.u32Month, sReadRTC.u32Day, sReadRTC.u32Hour, sReadRTC.u32Minute, sReadRTC.u32Second);

    outpw(REG_SYS_WKUPSSR1, inpw(REG_SYS_WKUPSSR1));

    printf("# Set next RTC alarm date/time: 2017/03/16 00:00:05.\n");
    printf("# Wait system waken-up by RTC alarm interrupt event.\n");
    RTC_SetAlarmDate(2017, 3, 16, RTC_MONDAY);
    RTC_SetAlarmTime(0, 0, 5, RTC_CLOCK_24, 0);

    g_u8IsRTCAlarmINT = 0;

    printf("\n ");

    /* System enter to Power-down */
    /* To program PWRCTL register, it needs to disable register protection first. */
    printf("\nSystem enter to power-down mode ...\n");
    /* To check if all the debug messages are finished */
    while(!(inpw(REG_UART0_FSR) & (1<<28)));

    //enable NUC980 to enter power down mode
    i = *(volatile unsigned int *)(0xB0000200);
    i = i & (0xFFFFFFFE);
    *(volatile unsigned int *)(0xB0000200)=i;

    __wfi();

    while(g_u8IsRTCAlarmINT == 0);

    /* Read current RTC date/time */
    RTC_GetDateAndTime(&sReadRTC);
    printf("System has been waken-up and current date/time is:\n");
    printf("    %d/%02d/%02d %02d:%02d:%02d\n",
           sReadRTC.u32Year, sReadRTC.u32Month, sReadRTC.u32Day, sReadRTC.u32Hour, sReadRTC.u32Minute, sReadRTC.u32Second);

    while(1);
}

/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/
