/**************************************************************************//**
 * @file     main.c
 * @version  V1.10
 * $Revision: 10 $
 * $Date: 15/11/19 10:11a $
 * @brief    Shows how to use NUC980 Crypto RSA engine to sign and verify
 *           signatures. This sample use MbedTLS library to generate RSA
 *           private key and public key.
 *
 * @note
 * Copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include "mbedtls/platform.h"

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include "mbedtls/memory_buffer_alloc.h"
#endif

#include <stdint.h>
#include <string.h>

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include "mbedtls/memory_buffer_alloc.h"
#endif

#include <stdio.h>
#include <string.h>
#include "nuc980.h"
#include "sys.h"
#include "etimer.h"
#include "crypto.h"

#include "mbedtls/rsa.h"
#include "mbedtls/rsa_internal.h"
#include "mbedtls/md2.h"
#include "mbedtls/md4.h"
#include "mbedtls/md5.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"


#define RSA_BIT_LEN      2048

extern void init_adc_init(void);
void adc_trng_gen_key(char *key, int key_len);


static volatile int  g_PRNG_done;
volatile int  g_Crypto_Int_done = 0;

void CRYPTO_IRQHandler()
{
    if (PRNG_GET_INT_FLAG(CRPT))
    {
        g_PRNG_done = 1;
        PRNG_CLR_INT_FLAG(CRPT);
    }
    if (SHA_GET_INT_FLAG(CRPT))
    {
        g_Crypto_Int_done = 1;
        SHA_CLR_INT_FLAG(CRPT);
    }
}

volatile int  _timer_tick;

void ETMR0_IRQHandler(void)
{
    _timer_tick ++;
    // clear timer interrupt flag
    ETIMER_ClearIntFlag(0);
}

int  get_ticks(void)
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

int myrand(void *unused, unsigned char *buff, size_t len)
{
    int         i;
    uint32_t    au32PrngData[8];
    unsigned char  *p = buff;

    //printf("myrand: len=%d\n", len);
    for (i=len; i>0; i-=8, p+=8)
    {
        g_PRNG_done = 0;
        PRNG_Start(CRPT);
        while (!g_PRNG_done);
        PRNG_Read(CRPT, au32PrngData);

        if (i < 8)
        {
            memcpy(p, (unsigned char *)au32PrngData, i);
            break;
        }
        else
            memcpy(p, (unsigned char *)au32PrngData, 8);
    }
#if 0
    printf("Random number: ");
    for (i=0; i<len; i++)
        printf("%02x", buff[i]);
    printf("\n");
#endif
    return 0;
}

int  Generate_RSA_Key(int keysize, int (*f_rng)(void *, unsigned char *, size_t),
                      char *N, char *E, char *d)
{
    char     str_buff[RSA_KBUF_HLEN];
    size_t   wlen;

    mbedtls_rsa_context rsa;

    mbedtls_rsa_init( &rsa, MBEDTLS_RSA_PKCS_V15, 0 );

    mbedtls_rsa_gen_key( &rsa, f_rng, NULL, keysize, 65537 );

    memset(str_buff, 0, sizeof(str_buff));
    mbedtls_mpi_write_string(&rsa.P, 16, str_buff, RSA_KBUF_HLEN, &wlen);
    printf("p = %s\n", str_buff);
    mbedtls_mpi_write_string(&rsa.Q, 16, str_buff, RSA_KBUF_HLEN, &wlen);
    printf("q = %s\n", str_buff);
    mbedtls_mpi_write_string(&rsa.N, 16, N, RSA_KBUF_HLEN, &wlen);
    mbedtls_mpi_write_string(&rsa.E, 16, E, RSA_KBUF_HLEN, &wlen);
    mbedtls_mpi_write_string(&rsa.D, 16, d, RSA_KBUF_HLEN, &wlen);

    mbedtls_rsa_free( &rsa );
    return 0;
}


int32_t main (void)
{
    char    N[RSA_KBUF_HLEN];
    char    E[RSA_KBUF_HLEN];
    char    d[RSA_KBUF_HLEN];
    char    C[RSA_KBUF_HLEN];
    char    Msg[RSA_KBUF_HLEN] = "70992c9d95a4908d2a94b3ab9fa1cd643f120e326f9d7808af50cac42c4b0b4eeb7f0d4df303a568fbfb82b0f58300d25357645721bb71861caf81b27a56082c80a146499fb4eab5bde4493f5d00f1a437bbc360dfcd8056fe6be10e608adb30b6c2f7652428b8d32d362945982a46585d2102ef7995a8ba6e8ad8fd16bd7ae8f53c3d7fcfba290b57ce7f8f09c828d6f2d3ce56f131bd9461e5667e5b73edac77f504dac4f202a9570eb4515b2bf516407db831518db8a2083ec701e8fd387c430bb1a72deca5b49d429cf9deb09cc4518dc5f57c089aa2d3420e567e732102c2c92b88a07c69d70917140ab3823c63f312d3f11fa87ba29da3c7224b4fb4bc";
    char    Sign[RSA_KBUF_HLEN];
    uint32_t     seed;

    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    UART_Init();

    Start_ETIMER0();

    outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | (1<<23));   /* Enable Crypto clock */

    printf("\n\n+---------------------------------------------+\n");
    printf("|   Crypto RSA sample                         |\n");
    printf("+---------------------------------------------+\n");

    sysInstallISR(IRQ_LEVEL_1, IRQ_CRYPTO, (PVOID)CRYPTO_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_CRYPTO);

    init_adc_init();

    PRNG_ENABLE_INT(CRPT);

    adc_trng_gen_key((char *)&seed, 32);

    PRNG_Open(CRPT, PRNG_KEY_SIZE_64, 1, seed);

    RSA_claim_bit_length(RSA_BIT_LEN);

    Generate_RSA_Key(RSA_BIT_LEN, myrand, N, E, d);

    printf("Private key (N,d) -\n");
    printf("    N = %s\n", N);
    printf("    d = %s\n", d);
    printf("Public key (N,e) -\n");
    printf("    E = %s\n", E);

    RSA_Calculate_C(RSA_BIT_LEN, N, C);
    printf("    C = %s\n", C);

    /*
     *  RSA sign
     */
    RSA_GenerateSignature(CRPT, RSA_BIT_LEN, N, d, C, Msg, Sign);
    printf("RSA sign: %s\n", Sign);

    /*
     *  RSA verify
     */
    if (RSA_VerifySignature(CRPT, RSA_BIT_LEN, N, E, C, Sign, Msg) == 0)
        printf("RSA signature verify OK.\n");
    else
    {
        printf("RSA signature verify failed!!\n");
        while (1);
    }
    printf("\nDone.\n");
    while (1);
}



