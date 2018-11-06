/******************************************************************************
 * @file     main.c
 * @brief    Demonstrate how to implement a USB virtual com port device.
 * @version  V1.00
 * $Date: 15/05/11 10:06a $
 *
 * @note
 * Copyright (C) 2015 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include "NUC980.h"
#include "sys.h"
#include "usbd.h"
#include "vcom_serial.h"

extern void USBD_IRQHandler(void);

/*--------------------------------------------------------------------------*/
STR_VCOM_LINE_CODING gLineCoding = {115200, 0, 0, 8};   /* Baud rate : 115200    */
/* Stop bit     */
/* parity       */
/* data bits    */
uint16_t gCtrlSignal = 0;     /* BIT0: DTR(Data Terminal Ready) , BIT1: RTS(Request To Send) */

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
/* UART0 */
#ifdef __ICCARM__
#pragma data_alignment=4
uint8_t gUsbRxBuf[64] = {0};
#else
__align(4) uint8_t gUsbRxBuf[64] = {0};
#endif

uint32_t gu32RxSize = 0;
uint32_t gu32TxSize = 0;

volatile int8_t gi8BulkInReady = 0;
volatile int8_t gi8BulkOutReady = 0;

void VCOM_TransferData(void)
{
    int32_t i;

    /* Process the Bulk out data when bulk out data is ready. */
    if (gi8BulkOutReady)
    {
        for (i=0; i<gu32RxSize; i++)
            USBD->EP[EPA].ep.EPDAT_BYTE = gUsbRxBuf[i];
        gi8BulkOutReady = 0; /* Clear bulk out ready flag */
        USBD->EP[EPA].EPRSPCTL = USB_EP_RSPCTL_SHORTTXEN;    // packet end
        USBD->EP[EPA].EPTXCNT = gu32RxSize;
        USBD_ENABLE_EP_INT(EPA, USBD_EPINTEN_INTKIEN_Msk);
        while(1)
        {
            if (gi8BulkInReady)
            {
                gi8BulkInReady = 0;
                break;
            }
        }
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
    UART_Init();
    printf("\n");
    printf("=========================\n");
    printf("     NUC980 USB VCOM     \n");
    printf("=========================\n");

    sysInstallISR(IRQ_LEVEL_1, IRQ_UDC, (PVOID)USBD_IRQHandler);
    /* enable CPSR I bit */
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_UDC);

    USBD_Open(&gsInfo, VCOM_ClassRequest, NULL);

    /* Endpoint configuration */
    VCOM_Init();

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
        VCOM_TransferData();
    }
}



/*** (C) COPYRIGHT 2015 Nuvoton Technology Corp. ***/

