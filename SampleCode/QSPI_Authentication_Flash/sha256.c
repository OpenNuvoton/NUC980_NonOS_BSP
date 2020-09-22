/**************************************************************************//**
* @file     sha256.c
* @brief    HMAC SHA256 library
*
* SPDX-License-Identifier: Apache-2.0
* @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
******************************************************************************/

#include <string.h>
#include <stdint.h>
#include "nuc980.h"
#include "sys.h"
#include "crypto.h"

#include "sha256.h"

static volatile uint32_t g_u32HMACDone = 0;

static uint8_t g_au8HMACMsgPool[1024] __attribute__((aligned(32)));
static uint8_t *g_pu8HMACMsg;

void CRYPTO_IRQHandler(void)
{
    if (SHA_GET_INT_FLAG(CRPT))
    {
        g_u32HMACDone = 1;
        SHA_CLR_INT_FLAG(CRPT);
    }
}

void CRYPTO_SHA_Initialize(void)
{
    /* Enable Crypto clock */
    outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | (1 << 23));

    sysInstallISR(IRQ_LEVEL_1, IRQ_CRYPTO, (PVOID)CRYPTO_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_CRYPTO);

    SHA_ENABLE_INT(CRPT);

    /* Non-cacheable buffer pointer */
    g_pu8HMACMsg = (uint8_t *)((uint32_t)g_au8HMACMsgPool | 0x80000000);
}

void HMAC_SHA256(uint8_t *key, uint32_t key_len, uint8_t *text, uint32_t text_len, uint8_t *digest)
{
    uint32_t au32OutputDigest[8];

    /* Copy key to HMAC Msg buffer */
    memcpy((void *)&g_pu8HMACMsg[0], (void *)key, key_len);

    /* Copy text to remaining HMAC Msg buffer */
    memcpy((void *)&g_pu8HMACMsg[(key_len + 3) & 0xfffffffc], (void *)text, text_len);

    SHA_Open(CRPT, SHA_MODE_SHA256, SHA_IN_OUT_SWAP, key_len);

    SHA_SetDMATransfer(CRPT, (uint32_t)&g_pu8HMACMsg[0], text_len + ((key_len + 3) & 0xfffffffc));

    g_u32HMACDone = 0;

    /* Start HMAC_SHA256 calculation. */
    SHA_Start(CRPT, CRYPTO_DMA_ONE_SHOT);

    /* Wait it done. */
    while (!g_u32HMACDone);

    /* Read out digest */
    SHA_Read(CRPT, au32OutputDigest);

    /* Copy digest to HMAC Msg buffer */
    memcpy((void *)&digest[0], (void *)&au32OutputDigest[0], sizeof(au32OutputDigest));
}
