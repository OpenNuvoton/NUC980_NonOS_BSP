/**************************************************************************//**
 * @file     retarget.c
 * @brief    NUC980 retarget code
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include <stdio.h>
#include <stdio.h>
#include <rt_misc.h>
#include "nuc980.h"
#include "sys.h"


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

int fflush (FILE *f)  {
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

extern unsigned int Image$$ARM_LIB_HEAP$$ZI$$Base;
extern unsigned int Image$$ARM_LIB_HEAP$$ZI$$Limit;
__value_in_regs struct R0_R3
{
    unsigned heap_base, stack_base, heap_limit, stack_limit;
}
__user_setup_stackheap(unsigned int R0, unsigned int SP, unsigned int R2, unsigned int SL)

{
    struct R0_R3 config;

    config.heap_base = (unsigned int)&Image$$ARM_LIB_HEAP$$ZI$$Base;
    config.heap_limit =(unsigned int)&Image$$ARM_LIB_HEAP$$ZI$$Limit;
    config.stack_base = SP;

    return config;
}
/// @endcond HIDDEN_SYMBOLS

