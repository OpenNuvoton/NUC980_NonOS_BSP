/**************************************************************************//**
 * @file     main.c
 * @version  V1.10
 * $Revision: 7 $
 * $Date: 14/05/30 5:58p $
 * @brief    Generate random numbers using Crypto IP PRNG
 *
 * @note
 * Copyright (C) 2013 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "nuc980.h"
#include "sys.h"
#include "crypto.h"


#define GENERATE_COUNT      10


static volatile int  g_PRNG_done;

void CRYPTO_IRQHandler()
{
    if (PRNG_GET_INT_FLAG(CRPT))
    {
        g_PRNG_done = 1;
        PRNG_CLR_INT_FLAG(CRPT);
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

int32_t main (void)
{
    uint32_t   i, u32KeySize;
    uint32_t   au32PrngData[8];

    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    UART_Init();

    outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | (1<<23));   /* Enable Crypto clock */

    printf("+-----------------------------------+\n");
    printf("|  M480 Crypto PRNG Sample Demo     |\n");
    printf("+-----------------------------------+\n");

    sysInstallISR(IRQ_LEVEL_1, IRQ_CRYPTO, (PVOID)CRYPTO_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_CRYPTO);

    PRNG_ENABLE_INT(CRPT);

    for (u32KeySize = PRNG_KEY_SIZE_64; u32KeySize <= PRNG_KEY_SIZE_256; u32KeySize++)
    {
        printf("\n\nPRNG Key size = %s\n\n",(u32KeySize == PRNG_KEY_SIZE_64) ? "64" :
               (u32KeySize == PRNG_KEY_SIZE_128) ? "128" :
               (u32KeySize == PRNG_KEY_SIZE_192) ? "192" :
               (u32KeySize == PRNG_KEY_SIZE_256) ? "256" : "unknown");

        //PRNG_Open(CRPT, u32KeySize, 0, 0);        // start PRNG with default seed
        PRNG_Open(CRPT, u32KeySize, 1, 0x55);     // start PRNG with seed 0x55

        for (i = 0; i < GENERATE_COUNT; i++)
        {
            g_PRNG_done = 0;
            PRNG_Start(CRPT);
            while (!g_PRNG_done);

            memset(au32PrngData, 0, sizeof(au32PrngData));
            PRNG_Read(CRPT, au32PrngData);

            printf("PRNG DATA ==>\n");
            printf("    0x%08x  0x%08x  0x%08x  0x%08x\n", au32PrngData[0], au32PrngData[1], au32PrngData[2], au32PrngData[3]);
            printf("    0x%08x  0x%08x  0x%08x  0x%08x\n", au32PrngData[4], au32PrngData[5], au32PrngData[6], au32PrngData[7]);
        }
    }

    printf("\nAll done.\n");

    while (1);
}



