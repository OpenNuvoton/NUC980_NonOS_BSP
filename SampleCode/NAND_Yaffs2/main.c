/******************************************************************************
 * @file     main.c
 * @version  V1.00
 * $Revision: 1 $
 * $Date: 15/05/28 5:19p $
 * @brief    Access a NAND flash formatted in YAFFS2 file system
 *
 * @note
 * Copyright (C) 2015 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <string.h>
#include "stdio.h"

#include "nuc980.h"
#include "sys.h"
#include "etimer.h"
#include "yaffs_glue.h"


/*******************************************************************************/
volatile uint32_t _timer_tick;

void ETMR0_IRQHandler(void)
{
    _timer_tick ++;
    // clear timer interrupt flag
    ETIMER_ClearIntFlag(0);
}

uint32_t get_ticks(void)
{
    return _timer_tick;
}

void Start_ETIMER0(void)
{
    // Enable ETIMER0 engine clock
    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | (1 << 8));

    // Set timer frequency to 1000 HZ
    ETIMER_Open(0, ETIMER_PERIODIC_MODE, 1000);

    // Enable timer interrupt
    ETIMER_EnableInt(0);
    sysInstallISR(IRQ_LEVEL_1, IRQ_TIMER0, (PVOID)ETMR0_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_TIMER0);

    _timer_tick = 0;

    // Start Timer 0
    ETIMER_Start(0);
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

/*******************************************************************************/
extern void nand_init(void);

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/

void FMI_Init(void)
{
    /* enable FMI NAND */
    outpw(REG_CLK_HCLKEN, (inpw(REG_CLK_HCLKEN) | 0x300000));

    outpw(REG_SYS_GPC_MFPL, 0x33333330);
    outpw(REG_SYS_GPC_MFPH, 0x33333333);
}

static char CommandLine[256];

/*----------------------------------------------*/
/* Get a line from the input                    */
/*----------------------------------------------*/
void get_line (char *buff, int len)
{
    char c;
    int idx = 0;

    for (;;)
    {
        c = getchar();
        putchar(c);
        if (c == '\r') break;
        if ((c == '\b') && idx) idx--;
        if ((c >= ' ') && (idx < len - 1)) buff[idx++] = c;
    }
    buff[idx] = 0;

    putchar('\n');
}



int main(void)
{
    char *ptr;
    char mtpoint[] = "user";
    int volatile i;
    unsigned int volatile btime, etime;

    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    UART_Init();
    Start_ETIMER0();
    printf("\n");
    printf("==========================================\n");
    printf("          FMI NAND YAFFS2                 \n");
    printf("==========================================\n");

    FMI_Init();
    nand_init();
    cmd_yaffs_devconfig(mtpoint, 0, 0xb0, 0x3ff);
    cmd_yaffs_dev_ls();
    cmd_yaffs_mount(mtpoint);
    cmd_yaffs_dev_ls();
    printf("\n");

    for (;;)
    {

        printf(">");
        ptr = CommandLine;
        get_line(ptr, sizeof(CommandLine));
        switch (*ptr++)
        {

        case 'q' :  /* Exit program */
            cmd_yaffs_umount(mtpoint);
            printf("Program terminated!\n");
            return 0;

        case 'l' :  /* ls */
            if (*ptr++ == 's')
            {
                while (*ptr == ' ') ptr++;
                cmd_yaffs_ls(ptr, 1);
            }
            break;

        case 'w' :  /* wr */
            if (*ptr++ == 'r')
            {
                while (*ptr == ' ') ptr++;
                btime = get_ticks();
                cmd_yaffs_write_file(ptr, 0x55, 1*1024);    /* write 0x55 into file 10 times */
                etime = get_ticks() - btime;
                printf("write %d MB/sec\n", 1*1024*100/etime);
            }
            break;

        case 'r' :
            if (*ptr == 'd')    /* rd */
            {
                ptr++;
                while (*ptr == ' ') ptr++;
                printf("Reading file %s ...\n\n", ptr);
                btime = get_ticks();
                cmd_yaffs_read_file(ptr);
                etime = get_ticks() - btime;
                printf("read %d MB/sec\n", 1*1024*100/etime);
                printf("\ndone.\n");
            }
            else if (*ptr == 'm')    /* rm */
            {
                ptr++;
                if (*ptr == 'd')
                {
                    while(*ptr != ' ')
                        i = *ptr++;
                    ptr++;
                    printf("Remove dir %s ...\n\n", ptr);
                    cmd_yaffs_rmdir(ptr);
                }
                else
                {
                    while (*ptr == ' ') ptr++;
                    printf("Remove file %s ...\n\n", ptr);
                    cmd_yaffs_rm(ptr);
                }
            }
            break;

        case 'm' :  /* mkdir */
            if (*ptr == 'k')
            {
                ptr++;
                if (*ptr == 'd')
                {
                    while(*ptr != ' ')
                        i = *ptr++;
                    ptr++;
                    cmd_yaffs_mkdir(ptr);
                }
            }
            break;

        case '?':       /* Show usage */
            printf("ls    <path>     - Show a directory. ex: ls user/test ('user' is mount point).\n");
            printf("rd    <file name> - Read a file. ex: rd user/test.bin ('user' is mount point).\n");
            printf("wr    <file name> - Write a file. ex: wr user/test.bin ('user' is mount point).\n");
            printf("rm    <file name> - Delete a file. ex: rm user/test.bin ('user' is mount point).\n");
            printf("mkdir <dir name> - Create a directory. ex: mkdir user/test ('user' is mount point).\n");
            printf("rmdir <dir name> - Create a directory. ex: mkdir user/test ('user' is mount point).\n");
            printf("\n");
        }
    }
}



/*** (C) COPYRIGHT 2015 Nuvoton Technology Corp. ***/
