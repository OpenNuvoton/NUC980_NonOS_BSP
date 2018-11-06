/**************************************************************************//**
 * @file     main.c
 * @version  V1.00
 * @brief    Show Crypto IP SHA function
 *
 *
 * @copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nuc980.h"
#include "sys.h"
#include "crypto.h"


extern void open_test_vector(void);
extern int  get_next_pattern(void);

extern uint8_t  *_au8ShaData;
extern uint8_t  _au8ShaDigest[64];
extern int      _i32DataLen;

static int  _i32DigestLength = 0;

static volatile int g_SHA_done;


void CRYPTO_IRQHandler()
{
    if (SHA_GET_INT_FLAG(CRPT))
    {
        g_SHA_done = 1;
        SHA_CLR_INT_FLAG(CRPT);
    }
}


int  do_compare(uint8_t *output, uint8_t *expect, int cmp_len)
{
    int   i;

    if (memcmp(expect, output, cmp_len))
    {
        printf("\nMismatch!! - %d\n", cmp_len);
        for (i = 0; i < cmp_len; i++)
            printf("0x%02x    0x%02x\n", expect[i], output[i]);
        return -1;
    }
    return 0;
}


int  run_sha()
{
    uint32_t  au32OutputDigest[8];

    SHA_Open(CRPT, SHA_MODE_SHA1, SHA_IN_SWAP, 0);

    SHA_SetDMATransfer(CRPT, (uint32_t)&_au8ShaData[0],  _i32DataLen/8);

    printf("Key len= %d bits\n", _i32DataLen);

    g_SHA_done = 0;
    SHA_Start(CRPT, CRYPTO_DMA_ONE_SHOT);
    while (!g_SHA_done) ;

    SHA_Read(CRPT, au32OutputDigest);

    /*--------------------------------------------*/
    /*  Compare                                   */
    /*--------------------------------------------*/
    if (do_compare((uint8_t *)&au32OutputDigest[0], &_au8ShaDigest[0], _i32DigestLength) < 0)
    {
        printf("Compare error!\n");
        while (1);
    }
    return 0;
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

    outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | (1<<23));   /* Enable Crypto clock */

    printf("+-----------------------------------+\n");
    printf("|  M480 Crypto SHA Sample Demo      |\n");
    printf("+-----------------------------------+\n");

    sysInstallISR(IRQ_LEVEL_1, IRQ_CRYPTO, (PVOID)CRYPTO_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_CRYPTO);

    SHA_ENABLE_INT(CRPT);

    open_test_vector();

    while (1)
    {
        if (get_next_pattern() < 0)
            break;

        run_sha();
    }

    printf("SHA test done.\n");

    while (1);
}
