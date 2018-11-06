/**************************************************************************//**
 * @file     main.c
 * @version  V1.10
 * $Revision: 10 $
 * $Date: 15/11/19 10:11a $
 * @brief    Show Crypto IP ECC P-192 key generation function.
 *
 * @note
 * Copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "nuc980.h"
#include "sys.h"
#include "crypto.h"

#define KEY_LENGTH          192          /* Select ECC P-192 curve, 192-bits key length */

char d[580]  = "e5ce89a34adddf25ff3bf1ffe6803f57d0220de3118798ea";    /* private key */
char Qx[] = "8abf7b3ceb2b02438af19543d3e5b1d573fa9ac60085840f";    /* expected answer: public key 1 */
char Qy[] = "a87f80182dcd56a6a061f81f7da393e7cffd5e0738c6b245";    /* expected answer: public key 2 */

char key1[580], key2[580];               /* temporary buffer used to keep output public keys */

extern void init_adc_init(void);
void adc_trng_gen_key(char *key, int key_len);


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
    int  i;

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

    if (ECC_GeneratePublicKey(CRPT, CURVE_P_192, d, key1, key2) < 0)
    {
        printf("ECC key generation failed!!\n");
        while (1);
    }

    if (memcmp(Qx, key1, KEY_LENGTH/8))
    {
        printf("Public key 1 [%s] is not matched with expected [%s]!\n", key1, Qx);

        if (memcmp(Qx, key1, KEY_LENGTH/8) == 0)
            printf("PASS.\n");
        else
            printf("Error !!\n");

        for (i = 0; i < KEY_LENGTH/8; i++)
        {
            if (Qx[i] != key1[i])
                printf("\n%d - 0x%x 0x%x\n", i, Qx[i], key1[i]);
        }
        while (1);
    }

    if (memcmp(Qy, key2, KEY_LENGTH/8))
    {
        printf("Public key 2 [%s] is not matched with expected [%s]!\n", key2, Qy);
        while (1);
    }

    printf("ECC public key generation test vector compared passed.\n");

    printf("\n\nECC P-192 key pair generation =>\n");

    init_adc_init();
    memset(d, 0, sizeof(d));

    for (i = 0; i < 192; i++)
    {
        adc_trng_gen_key(d, 192-i);

        if (ECC_IsPrivateKeyValid(CRPT, CURVE_P_192, d))
        {
            break;
        }
    }

    printf("Select private key: [%s]\n", d);

    if (ECC_GeneratePublicKey(CRPT, CURVE_P_192, d, key1, key2) < 0)
    {
        printf("ECC key generation failed!!\n");
        while (1);
    }

    printf("Generated public key is:\n");
    printf("Qx: [%s]\n", key1);
    printf("Qy: [%s]\n", key2);


    printf("\n\nECC P-256 key pair generation =>\n");

    memset(d, 0, sizeof(d));
    for (i = 0; i < 256; i++)
    {
        adc_trng_gen_key(d, 256-i);

        if (ECC_IsPrivateKeyValid(CRPT, CURVE_P_256, d))
        {
            break;
        }
    }

    printf("Select private key: [%s]\n", d);

    if (ECC_GeneratePublicKey(CRPT, CURVE_P_256, d, key1, key2) < 0)
    {
        printf("ECC key generation failed!!\n");
        while (1);
    }

    printf("Generated public key is:\n");
    printf("Qx: [%s]\n", key1);
    printf("Qy: [%s]\n", key2);

    printf("\nDone.\n");

    while (1);
}



