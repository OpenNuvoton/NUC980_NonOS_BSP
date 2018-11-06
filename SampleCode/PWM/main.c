/**************************************************************************//**
 * @file     main.c
 * @version  V1.00
 * $Date: 15/05/08 7:09p $
 * @brief    NUC980 PWM Sample Code
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include "nuc980.h"
#include "sys.h"
#include "pwm.h"


void show_menu(void);
INT PWM_Timer(INT timer_num);
INT PWM_TimerDZ(INT dz_num);

extern int recvchar(void);

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

int main (void)
{
    INT item;

    // Disable all interrupts.
    sysSetGlobalInterrupt(DISABLE_ALL_INTERRUPTS);

    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    UART_Init();

    outpw(REG_CLK_PCLKEN1, inpw(REG_CLK_PCLKEN1) | (1 << 26)); // Enable PWM0 engine clock

    while(1)
    {
        show_menu();
        item = recvchar();// Get user key
        switch(item)
        {
        case '0':
            PWM_Timer(PWM0_TIMER0);
            break;
        case '1':
            PWM_Timer(PWM0_TIMER1);
            break;
        case '2':
            PWM_Timer(PWM0_TIMER2);
            break;
        case '3':
            PWM_Timer(PWM0_TIMER3);
            break;
        case '4':
            PWM_TimerDZ(PWM0_TIMER0);
            break;
        case '5':
            PWM_TimerDZ(PWM0_TIMER2);
            break;
        default:
            break;
        }
    }

}

void show_menu(void)
{
    printf("\n");
    printf("+-------------------------------------------------------+\n");
    printf("| Nuvoton PWM0 Demo Code                                 |\n");
    printf("+-------------------------------------------------------+\n");
    printf("| PWM0 Timer0 test (PF5)                          - [0] |\n");
    printf("| PWM0 Timer1 test (PF6)                          - [1] |\n");
    printf("| PWM0 Timer2 test (PF7)                          - [2] |\n");
    printf("| PWM0 Timer3 test (PF8)                          - [3] |\n");
    printf("| PWM0 Dead zone 0 test                           - [4] |\n");
    printf("| PWM0 Dead zone 1 test                           - [5] |\n");
    printf("+-------------------------------------------------------+\n");
    printf("Please Select :\n");
}

INT PWM_Timer(INT timer_num)
{
    typePWMVALUE pwmvalue;
    typePWMSTATUS PWMSTATUS;
    INT nLoop=0;
    INT nStatus=0;
    INT nInterruptInterval=0;
    PWMSTATUS.PDR=0;
    PWMSTATUS.InterruptFlag=FALSE;

    pwmInit();
    pwmOpen(timer_num);

    // Change PWM Timer setting
    pwmIoctl(timer_num, SET_CSR, 0, CSRD16);
    pwmIoctl(timer_num, SET_CP, 0, 249);
    pwmIoctl(timer_num, SET_DZI, 0, 0);
    pwmIoctl(timer_num, SET_INVERTER, 0, PWM_INVOFF);
    pwmIoctl(timer_num, SET_MODE, 0, PWM_TOGGLE);
    pwmIoctl(timer_num, DISABLE_DZ_GENERATOR, 0, 0);
    if(timer_num == PWM0_TIMER0)
        pwmIoctl(timer_num, ENABLE_PWMGPIOOUTPUT, PWM0_TIMER0, PWM00_GPF5);
    else if(timer_num == PWM0_TIMER1)
        pwmIoctl(timer_num, ENABLE_PWMGPIOOUTPUT, PWM0_TIMER1, PWM01_GPF6);
    else if(timer_num == PWM0_TIMER2)
        pwmIoctl(timer_num, ENABLE_PWMGPIOOUTPUT, PWM0_TIMER2, PWM02_GPF7);
    else
        pwmIoctl(timer_num, ENABLE_PWMGPIOOUTPUT, PWM0_TIMER3, PWM03_GPF8);

    pwmvalue.field.cnr=59999;
    pwmvalue.field.cmr=4999;
    pwmWrite(timer_num, (PUCHAR)(&pwmvalue), sizeof(pwmvalue));

    printf("PWM Timer%d one shot mode test\nPWM. Timer interrupt will occure soon.", timer_num);

    //Start PWM Timer
    pwmIoctl(timer_num, START_PWMTIMER, 0, 0);

    while(1)
    {
        nLoop++;
        if(nLoop%100000 == 0)
        {
            printf(".");
        }
        nStatus=pwmRead(timer_num, (PUCHAR)&PWMSTATUS, sizeof(PWMSTATUS));
        if(nStatus != Successful)
        {
            printf("PWM read error, ERR CODE:%d",nStatus);
            pwmClose(timer_num);
            return Fail;
        }
        if(PWMSTATUS.InterruptFlag==TRUE)
        {
            printf("\n\nPWM Timer interrupt occurred!\n\n");
            break;
        }
    }

    // Change PWM Timer setting
    pwmIoctl(timer_num, SET_CSR, 0, CSRD16);
    pwmIoctl(timer_num, SET_CP, 0, 255);
    pwmIoctl(timer_num, SET_DZI, 0, 0);
    pwmIoctl(timer_num, SET_INVERTER, 0, PWM_INVOFF);
    pwmIoctl(timer_num, SET_MODE, 0, PWM_TOGGLE);
    pwmIoctl(timer_num, DISABLE_DZ_GENERATOR, 0, 0);
    //pwmIoctl(timer_num, ENABLE_PWMGPIOOUTPUT, 0, 0);


    nInterruptInterval=30000;
    pwmvalue.field.cnr=nInterruptInterval;
    pwmvalue.field.cmr=4999;
    pwmWrite(timer_num, (PUCHAR)(&pwmvalue), sizeof(pwmvalue));

    printf("PWM Timer%d toggle mode test\nPWM Timer interrupt interval will decrease gradually\n", timer_num);

    //Start PWM Timer
    pwmIoctl(timer_num, START_PWMTIMER, 0, 0);
    nLoop=0;
    while(1)
    {
        nStatus=pwmRead(timer_num, (PUCHAR)&PWMSTATUS, sizeof(PWMSTATUS));
        if(nStatus != Successful)
        {
            printf("PWM read error, ERR CODE:%d",nStatus);
            pwmClose(timer_num);
            return Fail;
        }
        if(PWMSTATUS.InterruptFlag==TRUE)
        {
            printf("PWM Timer interrupt [%d], CNR:%d\n",nLoop,nInterruptInterval);
            nInterruptInterval/=2;
            pwmvalue.field.cnr=nInterruptInterval;
            pwmvalue.field.cmr=4999;
            pwmWrite(timer_num, (PUCHAR)(&pwmvalue), sizeof(pwmvalue));
            nLoop++;
            if(nLoop==10)
            {
                break;
            }
        }
    }
    pwmIoctl(timer_num, STOP_PWMTIMER, 0, 0);
    pwmClose(timer_num);
    printf("\nPWM Timer %d test finish\nPress any key to continue....", timer_num);
    recvchar();
    return Successful;
}

INT PWM_TimerDZ(INT timer_num)
{
    typePWMSTATUS PWMSTATUS0;
    typePWMSTATUS PWMSTATUS1;

    PWMSTATUS0.PDR=0;
    PWMSTATUS0.InterruptFlag=FALSE;
    PWMSTATUS1.PDR=0;
    PWMSTATUS1.InterruptFlag=FALSE;

    pwmInit();
    pwmOpen(timer_num);
    pwmOpen(timer_num+1);
    if(timer_num == PWM0_TIMER0)
    {
        pwmIoctl(timer_num, ENABLE_PWMGPIOOUTPUT, PWM0_TIMER0, PWM00_GPF5);
        pwmIoctl(timer_num + 1, ENABLE_PWMGPIOOUTPUT, PWM0_TIMER1, PWM01_GPF6);
    }
    else
    {
        pwmIoctl(timer_num, ENABLE_PWMGPIOOUTPUT, PWM0_TIMER2, PWM02_GPF7);
        pwmIoctl(timer_num + 1, ENABLE_PWMGPIOOUTPUT, PWM0_TIMER3, PWM03_GPF8);
    }

    pwmIoctl(timer_num, SET_DZI, 0, 0x50);
    pwmIoctl(timer_num, ENABLE_DZ_GENERATOR, 0, 0);

    pwmIoctl(timer_num, START_PWMTIMER, 0, 0);
    pwmIoctl(timer_num+1, START_PWMTIMER, 0, 0);

    printf("Hit any key to quit\n");
    recvchar();
    pwmIoctl(timer_num, STOP_PWMTIMER, 0, 0);
    pwmIoctl(timer_num+1, STOP_PWMTIMER, 0, 0);
    pwmClose(timer_num);
    pwmClose(timer_num+1);

    return Successful;
}
