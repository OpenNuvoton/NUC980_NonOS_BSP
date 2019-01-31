/**************************************************************************//**
 * @file     main.c
 * @version  V1.00
 * $Date: 15/05/11 10:06a $
 * @brief    Use internal SRAM as back end storage media to simulate a
 *           30 KB USB pen drive
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <string.h>
#include "NUC980.h"
#include "sys.h"
#include "usbd.h"
#include "sdh.h"
#include "massstorage.h"

uint8_t volatile g_u8SdInitFlag = 0;
/*--------------------------------------------------------------------------*/
extern uint8_t volatile g_u8MscStart;
extern void USBD_IRQHandler(void);


void FMI_IRQHandler(void)
{
    unsigned int volatile isr;
    unsigned int volatile ier;

    // FMI data abort interrupt
    if (SDH0->GINTSTS & SDH_GINTSTS_DTAIF_Msk)
    {
        /* ResetAllEngine() */
        SDH0->GCTL |= SDH_GCTL_GCTLRST_Msk;
    }

    //----- SD interrupt status
    isr = SDH0->INTSTS;
    if (isr & SDH_INTSTS_BLKDIF_Msk)
    {
        // block down
        g_u8SDDataReadyFlag = TRUE;
        SDH0->INTSTS = SDH_INTSTS_BLKDIF_Msk;
    }

    if (isr & SDH_INTSTS_CDIF_Msk)   // card detect
    {
        //----- SD interrupt status
        // it is work to delay 50 times for SD_CLK = 200KHz
        {
            int volatile i;         // delay 30 fail, 50 OK
            for (i=0; i<0x500; i++);  // delay to make sure got updated value from REG_SDISR.
            isr = SDH0->INTSTS;
        }

        if (isr & SDH_INTSTS_CDSTS_Msk)
        {
            printf("\n***** card remove !\n");
            SD0.IsCardInsert = FALSE;   // SDISR_CD_Card = 1 means card remove for GPIO mode
            memset(&SD0, 0, sizeof(SDH_INFO_T));
        }
        else
        {
            printf("***** card insert !\n");
            SDH_Open(SDH0, CardDetect_From_GPIO);
            if (SDH_Probe(SDH0))
            {
                g_u8SdInitFlag = 0;
                printf("SD initial fail!!\n");
            }
            else
                g_u8SdInitFlag = 1;
        }

        SDH0->INTSTS = SDH_INTSTS_CDIF_Msk;
    }

    // CRC error interrupt
    if (isr & SDH_INTSTS_CRCIF_Msk)
    {
        if (!(isr & SDH_INTSTS_CRC16_Msk))
        {
            //printf("***** ISR sdioIntHandler(): CRC_16 error !\n");
            // handle CRC error
        }
        else if (!(isr & SDH_INTSTS_CRC7_Msk))
        {
            if (!g_u8R3Flag)
            {
                //printf("***** ISR sdioIntHandler(): CRC_7 error !\n");
                // handle CRC error
            }
        }
        SDH0->INTSTS = SDH_INTSTS_CRCIF_Msk;      // clear interrupt flag
    }

    if (isr & SDH_INTSTS_DITOIF_Msk)
    {
        printf("***** ISR: data in timeout !\n");
        SDH0->INTSTS |= SDH_INTSTS_DITOIF_Msk;
    }

    // Response in timeout interrupt
    if (isr & SDH_INTSTS_RTOIF_Msk)
    {
        printf("***** ISR: response in timeout !\n");
        SDH0->INTSTS |= SDH_INTSTS_RTOIF_Msk;
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


/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main (void)
{
    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    UART_Init();
    printf("\n\n\n\n");
    printf("==========================\n");
    printf("     USB Mass Storage     \n");
    printf("==========================\n");

    sysInstallISR(IRQ_LEVEL_1, IRQ_UDC, (PVOID)USBD_IRQHandler);
    sysEnableInterrupt(IRQ_UDC);

    /* initial SD card */
    /* FMI-SD0 */
    outpw(REG_CLK_HCLKEN, (inpw(REG_CLK_HCLKEN) | 0x700000)); /* enable FMI, NAND, SD clock */
    /* Set GPC for FMI-SD */
    outpw(REG_SYS_GPC_MFPL, 0x66600000);
    outpw(REG_SYS_GPC_MFPH, 0x00060666);

    sysInstallISR(IRQ_LEVEL_1, IRQ_FMI, (PVOID)FMI_IRQHandler);
    sysEnableInterrupt(IRQ_FMI);
    sysSetLocalInterrupt(ENABLE_IRQ);

    SDH_Open(SDH0, CardDetect_From_GPIO);
    if (SDH_Probe(SDH0))
    {
        g_u8SdInitFlag = 0;
        printf("SD initial fail!!\n");
    }
    else
        g_u8SdInitFlag = 1;

    USBD_Open(&gsInfo, MSC_ClassRequest, NULL);

    /* Endpoint configuration */
    MSC_Init();

    /* Start transaction */
    while(1)
    {
        if (USBD_IS_ATTACHED())
        {
            USBD_Start();
            break;
        }
    }

    while(1)
    {
        if (g_usbd_Configured)
            MSC_ProcessCmd();
    }
}



/*** (C) COPYRIGHT 2018 Nuvoton Technology Corp. ***/

