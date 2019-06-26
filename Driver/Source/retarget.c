/**************************************************************************//**
 * @file     retarget.c
 * @brief    NUC980 retarget code
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include <stdio.h>
#include <stdio.h>
#if defined (__CC_ARM)
#include <rt_misc.h>
#endif
#include "nuc980.h"
#include "sys.h"

#if defined (__CC_ARM)

#pragma import(__use_no_semihosting_swi)
/// @cond HIDDEN_SYMBOLS
int sendchar(int ch)
{
    while ((inpw(REG_UART0_FSR) & (1<<23))); //waits for TX_FULL bit is clear
    outpw(REG_UART0_THR, ch);
    if(ch == '\n')
    {
        while((inpw(REG_UART0_FSR) & (1<<23))); //waits for TX_FULL bit is clear
        outpw(REG_UART0_THR, '\r');
    }
    return (ch);
}

static void flush_uart(void)
{
    while(inpw(REG_UART0_FSR) & (1<<23));  //waits for TX_FULL bit is clear
}

/**
 * @brief    Check any char input from UART
 *
 * @param    None
 *
 * @retval   1: No any char input
 * @retval   0: Have some char input
 *
 * @details  Check UART RSR RX EMPTY or not to determine if any char input from UART
 */

int kbhit(void)
{
    return !((inpw(REG_UART0_FSR) & (1 << 14)) == 0);
}

int recvchar(void)
{
    while(1)
    {
        if((inpw(REG_UART0_FSR) & (1 << 14)) == 0)  // waits RX not empty
        {
            return inpw(REG_UART0_RBR);
        }
    }
}

struct __FILE
{
    int handle; /* Add whatever you need here */
};

FILE __stdout;
FILE __stdin;
FILE __stderr;

int fflush(FILE *f)
{
    flush_uart();
    return 0;
}

int fputc(int ch, FILE *f)
{
    return (sendchar(ch));
}

int fgetc(FILE *stream)
{
    return (recvchar());
}

int fclose(FILE* f) {
  return (0);
}

int fseek (FILE *f, long nPos, int nMode)  {
  return (0);
}

int ferror(FILE *f)
{
    /* Your implementation of ferror */
    return EOF;
}

void _ttywrch(int ch)
{
    sendchar(ch);
}

void _sys_exit(int return_code)
{
label:
    goto label;  /* No where to go, endless loop */
}

#pragma import(__use_two_region_memory)
extern unsigned int Image$$ARM_LIB_HEAP$$ZI$$Base;
extern unsigned int Image$$ARM_LIB_HEAP$$ZI$$Limit;

__value_in_regs struct __initial_stackheap __user_initial_stackheap(unsigned int R0, unsigned int SP, unsigned int R2, unsigned int SL)

{
    struct __initial_stackheap config;

    config.heap_base = (unsigned int)&Image$$ARM_LIB_HEAP$$ZI$$Base;
    config.heap_limit =(unsigned int)&Image$$ARM_LIB_HEAP$$ZI$$Limit;
    config.stack_base = SP;
    config.heap_limit = SP - 0x10000;
    return config;
}
/// @endcond HIDDEN_SYMBOLS

#else  // expect defined(__GNUC__)

#include "uart.h"

/// @cond HIDDEN_SYMBOLS

int kbhit(void)
{
    return !((inpw(REG_UART0_FSR) & (1 << 14)) == 0);
}

int _write (int fd, char *ptr, int len)
{
    int i = len;

    while(i--)
    {
        while ((inpw(REG_UART0_FSR) & (1<<23))); //waits for TX_FULL bit is clear

        outpw(REG_UART0_THR, *ptr);

        if(*ptr == '\n')
        {
            while ((inpw(REG_UART0_FSR) & (1<<23)));
            outpw(REG_UART0_THR, '\r');
        }

        ptr++;

    }
    return len;
}

int _read (int fd, char *ptr, int len)
{
    while( (inpw(REG_UART0_FSR) & (1 << 14)) != 0);  // waits RX not empty
    *ptr = inpw(REG_UART0_RBR);

    return 1;
}

void C_SWI_Handler( int swi_num, int *regs )
{
    switch( swi_num )
    {
    case 0:
        regs[0] = regs[0] * regs[1];
        break;
    case 1:
        regs[0] = regs[0] + regs[1];
        break;
    case 2:
        regs[0] = (regs[0] * regs[1]) + (regs[2] * regs[3]);
        break;
    case 3:
    {
        int w, x, y, z;

        w = regs[0];
        x = regs[1];
        y = regs[2];
        z = regs[3];

        regs[0] = w + x + y + z;
        regs[1] = w - x - y - z;
        regs[2] = w * x * y * z;
        regs[3] =(w + x) * (y - z);
    }
    break;
    }
}

/// @endcond HIDDEN_SYMBOLS
#endif






