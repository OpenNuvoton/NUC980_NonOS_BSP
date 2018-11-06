/**************************************************************************//**
 * @file     main.c
 * @version  V1.10
 * $Revision: 10 $
 * $Date: 15/11/19 10:11a $
 * @brief    Show Crypto IP ECC P-192 ECDSA signature generation function.
 *
 * @note
 * Copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "nuc980.h"
#include "sys.h"
#include "crypto.h"


char sha_msg[] = "608079423f12421de616b7493ebe551cf4d65b92";      /* SHA-1 hash                                    */
char d[] = "e14f37b3d1374ff8b03f41b9b3fdd2f0ebccf275d660d7f3";    /* private key                                   */
char k[] = "cb0abc7043a10783684556fb12c4154d57bc31a289685f25";    /* random integer k form [1, n-1]                */
char R[] = "6994d962bdd0d793ffddf855ec5bf2f91a9698b46258a63e";    /* Expected answer: R of (R,S) digital signature */
char S[] = "02ba6465a234903744ab02bc8521405b73cf5fc00e1a9f41";    /* Expected answer: S of (R,S) digital signature */

char sig_R[168], sig_S[168];             /* temporary buffer used to keep digital signature (R,S) pair */



void CRYPTO_IRQHandler()
{
    ECC_Complete(CRPT);
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
    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    UART_Init();

    outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | (1<<23));   /* Enable Crypto clock */

    printf("+---------------------------------------------+\n");
    printf("|   Crypto ECC Public Key Generation Demo     |\n");
    printf("+---------------------------------------------+\n");

    sysInstallISR(IRQ_LEVEL_1, IRQ_CRYPTO, (PVOID)CRYPTO_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_CRYPTO);

    ECC_ENABLE_INT(CRPT);

    if (ECC_GenerateSignature(CRPT, CURVE_P_192, sha_msg, d, k, sig_R, sig_S) < 0)
    {
        printf("ECC signature generation failed!!\n");
        while (1);
    }

    if (memcmp(sig_R, R, sizeof(R)))
    {
        printf("Signature R [%s] is not matched with expected [%s]!\n", sig_R, R);
        while (1);
    }

    if (memcmp(sig_S, S, sizeof(S)))
    {
        printf("Signature S [%s] is not matched with expected [%s]!\n", sig_S, S);
        while (1);
    }

    printf("ECC digital signature compared OK.\n");

    while (1);
}



