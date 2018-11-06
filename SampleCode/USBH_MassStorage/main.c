/**************************************************************************//**
 * @file     main.c
 * @version  V1.00
 * $Revision: 2 $
 * $Date: 15/05/18 3:59p $
 * @brief    This sample shows how to access USB mass stroage disk.
 *
 * @note
 * Copyright (C) 2015 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <string.h>

#include "nuc980.h"
#include "sys.h"
#include "etimer.h"
#include "config.h"
#include "usbh_lib.h"
#include "ff.h"
#include "diskio.h"


#define BUFF_SIZE       (64*1024)

static UINT blen = BUFF_SIZE;
DWORD acc_size;                         /* Work register for fs command */
WORD acc_files, acc_dirs;
FILINFO Finfo;

char Line[256];                         /* Console input buffer */
#if _USE_LFN
char Lfname[512];
#endif

__align(32) BYTE Buff_Pool[BUFF_SIZE*2] ;       /* Working buffer */

BYTE  *Buff, *Buff2;

static FIL file1, file2;        /* File objects */

volatile uint32_t  _timer_tick;

extern int kbhit(void);
extern void  USBHL_select_MFP(void);


void ETMR0_IRQHandler(void)
{
    _timer_tick ++;
    // clear timer interrupt flag
    ETIMER_ClearIntFlag(0);
}

uint32_t  get_ticks(void)
{
    return _timer_tick;
}

void Start_ETIMER0(void)
{
    // Enable ETIMER0 engine clock
    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | (1 << 8));

    // Set timer frequency to 100 HZ
    ETIMER_Open(0, ETIMER_PERIODIC_MODE, 100);

    // Enable timer interrupt
    ETIMER_EnableInt(0);
    sysInstallISR(IRQ_LEVEL_1, IRQ_TIMER0, (PVOID)ETMR0_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_TIMER0);

    _timer_tick = 0;

    // Start Timer 0
    ETIMER_Start(0);
}

void delay_us(int usec)
{
    volatile int  loop = 20 * usec;
    while (loop > 0) loop--;
}


void timer_init()
{
    printf("timer_init() To do...\n");
}

uint32_t get_timer_value()
{
    printf("get_timer_value() To do...\n");
    return 1;
}


void  dump_buff_hex(uint8_t *pucBuff, int nBytes)
{
    int     nIdx, i;

    nIdx = 0;
    while (nBytes > 0)
    {
        printf("0x%04X  ", nIdx);
        for (i = 0; i < 16; i++)
            printf("%02x ", pucBuff[nIdx + i]);
        printf("  ");
        for (i = 0; i < 16; i++)
        {
            if ((pucBuff[nIdx + i] >= 0x20) && (pucBuff[nIdx + i] < 127))
                printf("%c", pucBuff[nIdx + i]);
            else
                printf(".");
            nBytes--;
        }
        nIdx += 16;
        printf("\n");
    }
    printf("\n");
}


/*--------------------------------------------------------------------------*/
/* Monitor                                                                  */

/*----------------------------------------------*/
/* Get a value of the string                    */
/*----------------------------------------------*/
/*  "123 -5   0x3ff 0b1111 0377  w "
        ^                           1st call returns 123 and next ptr
           ^                        2nd call returns -5 and next ptr
                   ^                3rd call returns 1023 and next ptr
                          ^         4th call returns 15 and next ptr
                               ^    5th call returns 255 and next ptr
                                  ^ 6th call fails and returns 0
*/

int xatoi (         /* 0:Failed, 1:Successful */
    TCHAR **str,    /* Pointer to pointer to the string */
    long *res       /* Pointer to a variable to store the value */
)
{
    unsigned long val;
    unsigned char r, s = 0;
    TCHAR c;


    *res = 0;
    while ((c = **str) == ' ') (*str)++;    /* Skip leading spaces */

    if (c == '-')       /* negative? */
    {
        s = 1;
        c = *(++(*str));
    }

    if (c == '0')
    {
        c = *(++(*str));
        switch (c)
        {
        case 'x':       /* hexadecimal */
            r = 16;
            c = *(++(*str));
            break;
        case 'b':       /* binary */
            r = 2;
            c = *(++(*str));
            break;
        default:
            if (c <= ' ') return 1; /* single zero */
            if (c < '0' || c > '9') return 0;   /* invalid char */
            r = 8;      /* octal */
        }
    }
    else
    {
        if (c < '0' || c > '9') return 0;   /* EOL or invalid char */
        r = 10;         /* decimal */
    }

    val = 0;
    while (c > ' ')
    {
        if (c >= 'a') c -= 0x20;
        c -= '0';
        if (c >= 17)
        {
            c -= 7;
            if (c <= 9) return 0;   /* invalid char */
        }
        if (c >= r) return 0;       /* invalid char for current radix */
        val = val * r + c;
        c = *(++(*str));
    }
    if (s) val = 0 - val;           /* apply sign if needed */

    *res = val;
    return 1;
}


/*----------------------------------------------*/
/* Dump a block of byte array                   */

void put_dump (
    const unsigned char* buff,  /* Pointer to the byte array to be dumped */
    unsigned long addr,         /* Heading address value */
    int cnt                     /* Number of bytes to be dumped */
)
{
    int i;


    printf("%08x ", addr);

    for (i = 0; i < cnt; i++)
        printf(" %02x", buff[i]);

    printf(" ");
    for (i = 0; i < cnt; i++)
        putchar((TCHAR)((buff[i] >= ' ' && buff[i] <= '~') ? buff[i] : '.'));

    printf("\n");
}


/*--------------------------------------------------------------------------*/
/* Monitor                                                                  */
/*--------------------------------------------------------------------------*/

static
FRESULT scan_files (
    char* path      /* Pointer to the path name working buffer */
)
{
    DIR dirs;
    FRESULT res;
    BYTE i;
    char *fn;


    if ((res = f_opendir(&dirs, path)) == FR_OK)
    {
        i = strlen(path);
        while (((res = f_readdir(&dirs, &Finfo)) == FR_OK) && Finfo.fname[0])
        {
            if (_FS_RPATH && Finfo.fname[0] == '.') continue;
#if _USE_LFN
            fn = *Finfo.lfname ? Finfo.lfname : Finfo.fname;
#else
            fn = Finfo.fname;
#endif
            if (Finfo.fattrib & AM_DIR)
            {
                acc_dirs++;
                *(path+i) = '/';
                strcpy(path+i+1, fn);
                res = scan_files(path);
                *(path+i) = '\0';
                if (res != FR_OK) break;
            }
            else
            {
                /*              printf("%s/%s\n", path, fn); */
                acc_files++;
                acc_size += Finfo.fsize;
            }
        }
    }

    return res;
}



void put_rc (FRESULT rc)
{
    const TCHAR *p =
        _T("OK\0DISK_ERR\0INT_ERR\0NOT_READY\0NO_FILE\0NO_PATH\0INVALID_NAME\0")
        _T("DENIED\0EXIST\0INVALID_OBJECT\0WRITE_PROTECTED\0INVALID_DRIVE\0")
        _T("NOT_ENABLED\0NO_FILE_SYSTEM\0MKFS_ABORTED\0TIMEOUT\0LOCKED\0")
        _T("NOT_ENOUGH_CORE\0TOO_MANY_OPEN_FILES\0");
    //FRESULT i;
    uint32_t i;
    for (i = 0; (i != (UINT)rc) && *p; i++)
    {
        while(*p++) ;
    }
    printf(_T("rc=%d FR_%s\n"), (UINT)rc, p);
}

/*----------------------------------------------*/
/* Get a line from the input                    */
/*----------------------------------------------*/

void get_line (char *buff, int len)
{
    TCHAR c;
    int idx = 0;
//  DWORD dw;


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

/*---------------------------------------------------------*/
/* User Provided RTC Function for FatFs module             */
/*---------------------------------------------------------*/
/* This is a real time clock service to be called from     */
/* FatFs module. Any valid time must be returned even if   */
/* the system does not support an RTC.                     */
/* This function is not required in read-only cfg.         */

unsigned long get_fattime (void)
{
    unsigned long tmr;

    tmr=0x00000;

    return tmr;
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

__asm void __wfi(void)
{
    MCR p15, 0, r1, c7, c0, 4
    BX  lr
}

void power_down()
{
    *(volatile unsigned int *)(0xB0000200) &= 0xFFFFFFFE;
    *(unsigned int volatile *)(0xB00001FC) = 0;
    __wfi();
}


/*----------------------------------------------------------------------------
  MAIN function
 *----------------------------------------------------------------------------*/
int32_t main(void)
{
    char        *ptr, *ptr2;
    long        p1, p2, p3;
    BYTE        *buf;
    FATFS       *fs;              /* Pointer to file system object */
    TCHAR       usb_path[] = { '3', ':', 0 };    /* USB drive started from 3 */
    char        cmd_backup[128];
    FRESULT     res;

    DIR dir;                /* Directory object */
    UINT s1, s2, cnt;
    static const BYTE ft[] = {0, 12, 16, 32};
    DWORD ofs = 0, sect = 0;

    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    UART_Init();

    SYS_UnlockReg();
    outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | (1<<18));      /* enable USB Host clock   */
    outpw(REG_SYS_MISCFCR, (inpw(REG_SYS_MISCFCR) | (1<<11)));  /* set USRHDSEN as 1; USB host/device role selection decided by USBID (SYS_PWRON[16]) */
    outpw(REG_SYS_PWRON, (inpw(REG_SYS_PWRON) | (1<<16)));      /* set USB port 0 used for Host */

    /* Enable USBH Lite */
    //USBHL0_MFP_DP_B10;
    //USBHL0_MFP_DM_B9;
    //USBHL4_MFP_DP_B13;
    //USBHL4_MFP_DM_A15;
    //USBHL5_MFP_DP_B12;
    //USBHL5_MFP_DM_B11;

    /* set PE.12  for USBH_PWREN */
    outpw(REG_SYS_GPE_MFPH, (inpw(REG_SYS_GPE_MFPH) & ~0x000f0000) | 0x00010000);

    /* set PE.10  for USB_OVC    */
    outpw(REG_SYS_GPE_MFPH, (inpw(REG_SYS_GPE_MFPH) & ~0x00000f00) | 0x00000100);

    printf("\n\n");
    printf("+-----------------------------------------------+\n");
    printf("|                                               |\n");
    printf("|     USB Host Mass Storage sample program      |\n");
    printf("|                                               |\n");
    printf("+-----------------------------------------------+\n");

    Start_ETIMER0();

    Buff = (BYTE *)((UINT32)&Buff_Pool[0] | 0x80000000);   /* use non-cache buffer */
    Buff2 = (BYTE *)((UINT32)&Buff_Pool[BUFF_SIZE] | 0x80000000);   /* use non-cache buffer */

    usbh_core_init();
    usbh_umas_init();
    usbh_pooling_hubs();

    f_chdrive(usb_path);          /* set default path */

    for (;;)
    {

        printf("EHCI: 0x%x, 0x%x;  OHCI: 0x%x, 0x%x; LITE: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", HSUSBH->UPSCR[0], HSUSBH->UPSCR[1], USBH->HcRhPortStatus[0], USBH->HcRhPortStatus[1], USBH->HcRhPortStatus[2], USBH->HcRhPortStatus[3], USBH->HcRhPortStatus[4], USBH->HcRhPortStatus[5], USBH->HcRhPortStatus[6], USBH->HcRhPortStatus[7]);

        usbh_pooling_hubs();

        printf(_T(">"));
        ptr = Line;

        while (kbhit())
            usbh_pooling_hubs();

        get_line(ptr, sizeof(Line));

        //dump_buff_hex(ptr, 16);

        switch (*ptr++)
        {

        case 'q' :  /* Exit program */
            return 0;

        case '1':          /* Power down and wakeup by EHCI/OCHI connect/disconnect or OHCI remote wakeup */
            outpw(REG_CLK_PMCON, ((inpw(REG_CLK_PMCON) & 0x000000FF) | (0xff00)));  /* config wakeup pre-scaler */
            outpw(REG_CLK_PMCON, (inpw(REG_CLK_PMCON) & ~0x2));
            printf("REG_CLK_PMCON = 0x%x\n", inpw(REG_CLK_PMCON));
            outpw(REG_SYS_WKUPSER1, inpw(REG_SYS_WKUPSER1) | (1UL<<18));   /* USBH wakeup system enable */
            printf("\n\n");
            printf("USBH suspend....n");
            usbh_suspend();
            printf("EHCI: 0x%x, 0x%x; OHCI: 0x%x, 0x%x\n", HSUSBH->UPSCR[0], HSUSBH->UPSCR[1], USBH->HcRhPortStatus[0], USBH->HcRhPortStatus[1]);
            printf("Enter power down...\n");
            while (!(inpw(REG_UART0_FSR) & (1<<22)));   // wait until TX empty
            power_down();
            printf("Wakeup!!\n");
            usbh_resume();
            printf("USBH resume....n");
            printf("EHCI: 0x%x, 0x%x; OHCI: 0x%x, 0x%x\n", HSUSBH->UPSCR[0], HSUSBH->UPSCR[1], USBH->HcRhPortStatus[0], USBH->HcRhPortStatus[1]);
            printf("\n\n");
            break;

        case '3' :
            USBHL_select_MFP();
            break;

        case '4' :
        case '5' :
        case '6' :
        case '7' :
            ptr--;
            *(ptr+1) = ':';
            *(ptr+2) = 0;
            put_rc(f_chdrive((TCHAR *)ptr));
            break;

        case '#':
            for (p1 = 0; p1 < 10000000; p1 += 8)
            {
                res = (FRESULT)disk_read(3, Buff, p1, 8);
                printf("Read sector %d, rc=%d\n", p1, (WORD)res);
                if (res)
                {
                    break;
                }
            }
            break;

        case 'd' :
            switch (*ptr++)
            {
            case 'd' :  /* dd [<lba>] - Dump sector */
                if (!xatoi(&ptr, &p2)) p2 = sect;
                res = (FRESULT)disk_read(3, Buff, p2, 1);
                if (res)
                {
                    printf("rc=%d\n", (WORD)res);
                    break;
                }
                sect = p2 + 1;
                printf("Sector:%d\n", p2);
                for (buf=(unsigned char*)Buff, ofs = 0; ofs < 0x200; buf+=16, ofs+=16)
                    put_dump(buf, ofs, 16);
                break;

            }
            break;

        case 'b' :
            switch (*ptr++)
            {
            case 'd' :  /* bd <addr> - Dump R/W buffer */
                if (!xatoi(&ptr, &p1)) break;
                for (ptr=(char*)&Buff[p1], ofs = p1, cnt = 32; cnt; cnt--, ptr+=16, ofs+=16)
                    put_dump((BYTE*)ptr, ofs, 16);
                break;

            case 'e' :  /* be <addr> [<data>] ... - Edit R/W buffer */
                if (!xatoi(&ptr, &p1)) break;
                if (xatoi(&ptr, &p2))
                {
                    do
                    {
                        Buff[p1++] = (BYTE)p2;
                    }
                    while (xatoi(&ptr, &p2));
                    break;
                }
                for (;;)
                {
                    printf("%04X %02X-", (WORD)p1, Buff[p1]);
                    get_line(Line, sizeof(Line));
                    ptr = Line;
                    if (*ptr == '.') break;
                    if (*ptr < ' ')
                    {
                        p1++;
                        continue;
                    }
                    if (xatoi(&ptr, &p2))
                        Buff[p1++] = (BYTE)p2;
                    else
                        printf("???\n");
                }
                break;

            case 'r' :  /* br <sector> [<n>] - Read disk into R/W buffer */
                if (!xatoi(&ptr, &p2)) break;
                if (!xatoi(&ptr, &p3)) p3 = 1;
                printf("rc=%d\n", disk_read(0, Buff, p2, p3));
                break;

            case 'w' :  /* bw <sector> [<n>] - Write R/W buffer into disk */
                if (!xatoi(&ptr, &p2)) break;
                if (!xatoi(&ptr, &p3)) p3 = 1;
                printf("rc=%d\n", disk_write(0, Buff, p2, p3));
                break;

            case 'f' :  /* bf <n> - Fill working buffer */
                if (!xatoi(&ptr, &p1)) break;
                memset(Buff, (int)p1, BUFF_SIZE);
                break;

            }
            break;



        case 'f' :
            switch (*ptr++)
            {

            case 's' :  /* fs - Show logical drive status */
                res = f_getfree("", (DWORD*)&p2, &fs);
                if (res)
                {
                    put_rc(res);
                    break;
                }
                printf("FAT type = FAT%d\nBytes/Cluster = %d\nNumber of FATs = %d\n"
                       "Root DIR entries = %d\nSectors/FAT = %d\nNumber of clusters = %d\n"
                       "FAT start (lba) = %d\nDIR start (lba,clustor) = %d\nData start (lba) = %d\n\n...",
                       ft[fs->fs_type & 3], fs->csize * 512UL, fs->n_fats,
                       fs->n_rootdir, fs->fsize, fs->n_fatent - 2,
                       fs->fatbase, fs->dirbase, fs->database
                      );
                acc_size = acc_files = acc_dirs = 0;
#if _USE_LFN
                Finfo.lfname = Lfname;
                Finfo.lfsize = sizeof(Lfname);
#endif
                res = scan_files(ptr);
                if (res)
                {
                    put_rc(res);
                    break;
                }
                printf("\r%d files, %d bytes.\n%d folders.\n"
                       "%d KB total disk space.\n%d KB available.\n",
                       acc_files, acc_size, acc_dirs,
                       (fs->n_fatent - 2) * (fs->csize / 2), p2 * (fs->csize / 2)
                      );
                break;
            case 'l' :  /* fl [<path>] - Directory listing */
                while (*ptr == ' ') ptr++;
                res = f_opendir(&dir, ptr);
                if (res)
                {
                    put_rc(res);
                    break;
                }
                p1 = s1 = s2 = 0;
                for(;;)
                {
                    res = f_readdir(&dir, &Finfo);
                    if ((res != FR_OK) || !Finfo.fname[0]) break;
                    if (Finfo.fattrib & AM_DIR)
                    {
                        s2++;
                    }
                    else
                    {
                        s1++;
                        p1 += Finfo.fsize;
                    }
                    printf("%c%c%c%c%c %d/%02d/%02d %02d:%02d    %9d  %s",
                           (Finfo.fattrib & AM_DIR) ? 'D' : '-',
                           (Finfo.fattrib & AM_RDO) ? 'R' : '-',
                           (Finfo.fattrib & AM_HID) ? 'H' : '-',
                           (Finfo.fattrib & AM_SYS) ? 'S' : '-',
                           (Finfo.fattrib & AM_ARC) ? 'A' : '-',
                           (Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
                           (Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63, Finfo.fsize, Finfo.fname);
#if _USE_LFN
                    for (p2 = strlen(Finfo.fname); p2 < 14; p2++)
                        printf(" ");
                    printf("%s\n", Lfname);
#else
                    printf("\n");
#endif
                }
                printf("%4d File(s),%10d bytes total\n%4d Dir(s)", s1, p1, s2);
                if (f_getfree(ptr, (DWORD*)&p1, &fs) == FR_OK)
                    printf(", %10d bytes free\n", p1 * fs->csize * 512);
                break;


            case 'o' :  /* fo <mode> <file> - Open a file */
                if (!xatoi(&ptr, &p1)) break;
                while (*ptr == ' ') ptr++;
                put_rc(f_open(&file1, ptr, (BYTE)p1));
                break;

            case 'c' :  /* fc - Close a file */
                put_rc(f_close(&file1));
                break;

            case 'e' :  /* fe - Seek file pointer */
                if (!xatoi(&ptr, &p1)) break;
                res = f_lseek(&file1, p1);
                put_rc(res);
                if (res == FR_OK)
                    printf("fptr=%d(0x%lX)\n", file1.fptr, file1.fptr);
                break;

            case 'd' :  /* fd <len> - read and dump file from current fp */
                if (!xatoi(&ptr, &p1)) break;
                ofs = file1.fptr;
                while (p1)
                {
                    if ((UINT)p1 >= 16)
                    {
                        cnt = 16;
                        p1 -= 16;
                    }
                    else
                    {
                        cnt = p1;
                        p1 = 0;
                    }
                    res = f_read(&file1, Buff, cnt, &cnt);
                    if (res != FR_OK)
                    {
                        put_rc(res);
                        break;
                    }
                    if (!cnt) break;
                    put_dump(Buff, ofs, cnt);
                    ofs += 16;
                }
                break;

            case 'r' :  /* fr <len> - read file */
                if (!xatoi(&ptr, &p1)) break;
                p2 = 0;
                timer_init();
                while (p1)
                {
                    if ((UINT)p1 >= blen)
                    {
                        cnt = blen;
                        p1 -= blen;
                    }
                    else
                    {
                        cnt = p1;
                        p1 = 0;
                    }
                    res = f_read(&file1, Buff, cnt, &s2);
                    if (res != FR_OK)
                    {
                        put_rc(res);
                        break;
                    }
                    p2 += s2;
                    if (cnt != s2) break;
                }
                p1 = get_timer_value()/1000;
                if (p1)
                    printf("%d bytes read with %d kB/sec.\n", p2, ((p2 * 100) / p1)/1024);
                break;

            case 'w' :  /* fw <len> <val> - write file */
                if (!xatoi(&ptr, &p1) || !xatoi(&ptr, &p2)) break;
                memset(Buff, (BYTE)p2, blen);
                p2 = 0;
                timer_init();
                while (p1)
                {
                    if ((UINT)p1 >= blen)
                    {
                        cnt = blen;
                        p1 -= blen;
                    }
                    else
                    {
                        cnt = p1;
                        p1 = 0;
                    }
                    res = f_write(&file1, Buff, cnt, &s2);
                    if (res != FR_OK)
                    {
                        put_rc(res);
                        break;
                    }
                    p2 += s2;
                    if (cnt != s2) break;
                }
                p1 = get_timer_value()/1000;
                if (p1)
                    printf("%d bytes written with %d kB/sec.\n", p2, ((p2 * 100) / p1)/1024);
                break;

            case 'n' :  /* fn <old_name> <new_name> - Change file/dir name */
                while (*ptr == ' ') ptr++;
                ptr2 = strchr(ptr, ' ');
                if (!ptr2) break;
                *ptr2++ = 0;
                while (*ptr2 == ' ') ptr2++;
                put_rc(f_rename(ptr, ptr2));
                break;

            case 'u' :  /* fu <name> - Unlink a file or dir */
                while (*ptr == ' ') ptr++;
                put_rc(f_unlink(ptr));
                break;

            case 'v' :  /* fv - Truncate file */
                put_rc(f_truncate(&file1));
                break;

            case 'k' :  /* fk <name> - Create a directory */
                while (*ptr == ' ') ptr++;
                put_rc(f_mkdir(ptr));
                break;

            case 'a' :  /* fa <atrr> <mask> <name> - Change file/dir attribute */
                if (!xatoi(&ptr, &p1) || !xatoi(&ptr, &p2)) break;
                while (*ptr == ' ') ptr++;
                put_rc(f_chmod(ptr, p1, p2));
                break;

            case 't' :  /* ft <year> <month> <day> <hour> <min> <sec> <name> - Change timestamp */
                if (!xatoi(&ptr, &p1) || !xatoi(&ptr, &p2) || !xatoi(&ptr, &p3)) break;
                Finfo.fdate = (WORD)(((p1 - 1980) << 9) | ((p2 & 15) << 5) | (p3 & 31));
                if (!xatoi(&ptr, &p1) || !xatoi(&ptr, &p2) || !xatoi(&ptr, &p3)) break;
                Finfo.ftime = (WORD)(((p1 & 31) << 11) | ((p1 & 63) << 5) | ((p1 >> 1) & 31));
                put_rc(f_utime(ptr, &Finfo));
                break;

            case 'x' : /* fx <src_name> <dst_name> - Copy file */
                while (*ptr == ' ') ptr++;
                ptr2 = strchr(ptr, ' ');
                if (!ptr2) break;
                *ptr2++ = 0;
                while (*ptr2 == ' ') ptr2++;
                printf("Opening \"%s\"", ptr);
                res = f_open(&file1, ptr, FA_OPEN_EXISTING | FA_READ);
                printf("\n");
                if (res)
                {
                    put_rc(res);
                    break;
                }
                printf("Creating \"%s\"", ptr2);
                res = f_open(&file2, ptr2, FA_CREATE_ALWAYS | FA_WRITE);
                putchar('\n');
                if (res)
                {
                    put_rc(res);
                    f_close(&file1);
                    break;
                }
                printf("Copying...");
                p1 = 0;
                for (;;)
                {
                    res = f_read(&file1, Buff, BUFF_SIZE, &s1);
                    if (res || s1 == 0) break;   /* error or eof */
                    res = f_write(&file2, Buff, s1, &s2);
                    p1 += s2;
                    if (res || s2 < s1) break;   /* error or disk full */
                }
                printf("\n%d bytes copied.\n", p1);
                f_close(&file1);
                f_close(&file2);
                break;

            case 'y' : /* fz <src_name> <dst_name> - Compare file */
                strcpy(cmd_backup, ptr);
                while (kbhit())
                {
                    while (*ptr == ' ') ptr++;
                    ptr2 = strchr(ptr, ' ');
                    if (!ptr2) break;
                    *ptr2++ = 0;
                    while (*ptr2 == ' ') ptr2++;
                    printf("Opening \"%s\"", ptr);
                    res = f_open(&file1, ptr, FA_OPEN_EXISTING | FA_READ);
                    printf("\n");
                    if (res)
                    {
                        put_rc(res);
                        break;
                    }
                    printf("Opening \"%s\"", ptr2);
                    res = f_open(&file2, ptr2, FA_OPEN_EXISTING | FA_READ);
                    printf("\n");
                    if (res)
                    {
                        put_rc(res);
                        f_close(&file1);
                        break;
                    }
                    printf("Compare...");
                    p1 = 0;
                    for (;;)
                    {
                        res = f_read(&file1, Buff, BUFF_SIZE, &s1);
                        if (res || s1 == 0)
                        {
                            printf("\nRead file %s terminated. (%d)\n", ptr, res);
                            break;     /* error or eof */
                        }

                        res = f_read(&file2, Buff2, BUFF_SIZE, &s2);
                        if (res || s2 == 0)
                        {
                            printf("\nRead file %s terminated. (%d)\n", ptr2, res);
                            break;     /* error or eof */
                        }

                        p1 += s2;
                        if (res || s2 < s1) break;   /* error or disk full */

                        if (memcmp(Buff, Buff2, s1) != 0)
                        {
                            printf("Compre failed!! %d %d\n", s1,s2);
                            if (memcmp(Buff, Buff2, s1) != 0)
                                printf("Compre failed!! %d %d\n", s1,s2);

                            printf("Check ==>\n");
                            for (p1 = 0; p1 < s1; p1++)
                            {
                                if (Buff[p1] != Buff2[p1])
                                {
                                    printf("0x%08x:  0x%02x   0x%02x\n", p1, Buff[p1], Buff2[p1]);
                                }
                            }

                            printf("Check ==>\n");
                            for (p1 = 0; p1 < s1; p1++)
                            {
                                if (Buff[p1] != Buff2[p1])
                                {
                                    printf("0x%08x:  0x%02x   0x%02x\n", p1, Buff[p1], Buff2[p1]);
                                }
                            }
                            break;
                        }

                        if ((p1 % 0x10000) == 0)
                            printf("\n%d KB compared.", p1/1024);
                        printf(".");
                    }
                    if (s1 == 0)
                        printf("\nPASS. \n ");
                    f_close(&file1);
                    f_close(&file2);

                    strcpy(Line, cmd_backup);
                    ptr = Line;
                }
                break;

#if _FS_RPATH
            case 'g' :  /* fg <path> - Change current directory */
                while (*ptr == ' ') ptr++;
                put_rc(f_chdir(ptr));
                break;

            case 'j' :  /* fj <drive#> - Change current drive */
                while (*ptr == ' ') ptr++;
                dump_buff_hex((uint8_t *)&p1, 16);
                put_rc(f_chdrive((TCHAR *)ptr));
                break;
#endif
#if _USE_MKFS
            case 'm' :  /* fm <partition rule> <sect/clust> - Create file system */
                if (!xatoi(&ptr, &p2) || !xatoi(&ptr, &p3)) break;
                printf("The memory card will be formatted. Are you sure? (Y/n)=");
                get_line(ptr, sizeof(Line));
                if (*ptr == 'Y')
                    put_rc(f_mkfs(0, (BYTE)p2, (WORD)p3));
                break;
#endif
            case 'z' :  /* fz [<rw size>] - Change R/W length for fr/fw/fx command */
                if (xatoi(&ptr, &p1) && p1 >= 1 && (size_t)p1 <= BUFF_SIZE)
                    blen = p1;
                printf("blen=%d\n", blen);
                break;
            }
            break;
        case '?':       /* Show usage */
            printf(
                _T("n: - Change default drive (USB drive is 3~7)\n")
                _T("dd [<lba>] - Dump sector\n")
                //_T("ds <pd#> - Show disk status\n")
                _T("\n")
                _T("bd <ofs> - Dump working buffer\n")
                _T("be <ofs> [<data>] ... - Edit working buffer\n")
                _T("br <pd#> <sect> [<num>] - Read disk into working buffer\n")
                _T("bw <pd#> <sect> [<num>] - Write working buffer into disk\n")
                _T("bf <val> - Fill working buffer\n")
                _T("\n")
                _T("fs - Show volume status\n")
                _T("fl [<path>] - Show a directory\n")
                _T("fo <mode> <file> - Open a file\n")
                _T("fc - Close the file\n")
                _T("fe <ofs> - Move fp in normal seek\n")
                //_T("fE <ofs> - Move fp in fast seek or Create link table\n")
                _T("fd <len> - Read and dump the file\n")
                _T("fr <len> - Read the file\n")
                _T("fw <len> <val> - Write to the file\n")
                _T("fn <object name> <new name> - Rename an object\n")
                _T("fu <object name> - Unlink an object\n")
                _T("fv - Truncate the file at current fp\n")
                _T("fk <dir name> - Create a directory\n")
                _T("fa <atrr> <mask> <object name> - Change object attribute\n")
                _T("ft <year> <month> <day> <hour> <min> <sec> <object name> - Change timestamp of an object\n")
                _T("fx <src file> <dst file> - Copy a file\n")
                _T("fg <path> - Change current directory\n")
                _T("fj <ld#> - Change current drive. For example: <fj 4:>\n")
                _T("fm <ld#> <rule> <cluster size> - Create file system\n")
                _T("\n")
            );
            break;
        }
    }
    //while(1);
}


/*** (C) COPYRIGHT 2013 Nuvoton Technology Corp. ***/
