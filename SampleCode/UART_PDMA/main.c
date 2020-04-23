/****************************************************************************
 * @file     main.c
 * @version  V1.00
 * @brief    Demonstrate UART transmit and receive function with PDMA
 *
 * @copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 *
 ******************************************************************************/
#include <stdio.h>
#include "nuc980.h"
#include "sys.h"
#include "uart.h"
#include "pdma.h"

#define ENABLE_PDMA_INTERRUPT 1
#define PDMA_TEST_LENGTH 100
#define PDMA_TIME 0x5555

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#if defined ( __GNUC__ ) && !(__CC_ARM)
__attribute__((aligned(32))) static uint8_t g_u8Tx_Buffer[PDMA_TEST_LENGTH];
__attribute__((aligned(32))) static uint8_t g_u8Rx_Buffer[PDMA_TEST_LENGTH];
#else
__align(32) static uint8_t g_u8Tx_Buffer[PDMA_TEST_LENGTH];
__align(32) static uint8_t g_u8Rx_Buffer[PDMA_TEST_LENGTH];
#endif
static uint8_t *g_u8Tx_Buffer_point;
static uint8_t *g_u8Rx_Buffer_point;
volatile uint32_t u32IsTestOver = 0;

/*---------------------------------------------------------------------------------------------------------*/
/* Define functions prototype                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA_IRQHandler(void);
void UART_PDMATest(void);

void UART0_Init()
{
    /* enable UART0 clock */
    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | 0x10000);

    /* GPF11, GPF12 */
    outpw(REG_SYS_GPF_MFPH, (inpw(REG_SYS_GPF_MFPH) & 0xfff00fff) | 0x11000);   // UART0 multi-function

    /* UART0 line configuration for (115200,n,8,1) */
    outpw(REG_UART0_LCR, inpw(REG_UART0_LCR) | 0x07);
    outpw(REG_UART0_BAUD, 0x30000066); /* 12MHz reference clock input, 115200 */
}

void UART1_Init()
{
    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | (0x1 << 17)); // Enable UART1 engine clock

    /* Configure UART1 and set UART1 baud rate */
    UART_Open(UART1, 115200);
}

void PDMA_Init(void)
{
    /* Open PDMA Channel */
    PDMA_Open(PDMA0,1 << 0); // Channel 0 for UART1 TX
    PDMA_Open(PDMA0,1 << 1); // Channel 1 for UART1 RX
    // Select basic mode
    PDMA_SetTransferMode(PDMA0,0, PDMA_UART1_TX, 0, 0);
    PDMA_SetTransferMode(PDMA0,1, PDMA_UART1_RX, 0, 0);
    // Set data width and transfer count
    PDMA_SetTransferCnt(PDMA0,0, PDMA_WIDTH_8, PDMA_TEST_LENGTH);
    PDMA_SetTransferCnt(PDMA0,1, PDMA_WIDTH_8, PDMA_TEST_LENGTH);
    //Set PDMA Transfer Address
    PDMA_SetTransferAddr(PDMA0,0, ((uint32_t) (&g_u8Tx_Buffer[0])), PDMA_SAR_INC, UART1_BA, PDMA_DAR_FIX);
    PDMA_SetTransferAddr(PDMA0,1, UART1_BA, PDMA_SAR_FIX, ((uint32_t) (&g_u8Rx_Buffer[0])), PDMA_DAR_INC);
    //Select Single Request
    PDMA_SetBurstType(PDMA0,0, PDMA_REQ_SINGLE, 0);
    PDMA_SetBurstType(PDMA0,1, PDMA_REQ_SINGLE, 0);
    //Set timeout
    //PDMA_SetTimeOut(0, 0, 0x5555);
    //PDMA_SetTimeOut(1, 0, 0x5555);

#ifdef ENABLE_PDMA_INTERRUPT
    u32IsTestOver = 0;

    PDMA_EnableInt(PDMA0,0, 0);
    PDMA_EnableInt(PDMA0,1, 0);

    sysInstallISR(IRQ_LEVEL_1, IRQ_PDMA0, (PVOID)PDMA_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_PDMA0);
#endif
}

/*---------------------------------------------------------------------------------------------------------*/
/* MAIN function                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/

int main(void)
{
    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);

    g_u8Tx_Buffer_point = (uint8_t*)((uint32_t)g_u8Tx_Buffer | 0x80000000);
    g_u8Rx_Buffer_point = (uint8_t*)((uint32_t)g_u8Rx_Buffer | 0x80000000);

    outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | (0x1 << 12)); // Enable PDMA0 engine clock

    /* Init UART0 for printf */
    UART0_Init();

    /* Init UART1 */
    UART1_Init();

    /* Select UART1 Multi-Function */
    outpw(REG_SYS_GPC_MFPL, (inpw(REG_SYS_GPC_MFPL) & 0x000fffff) | 0x77700000); // UART1_RX:PC6, UART1_TX:PC5, UART1_RTS:PC7
    outpw(REG_SYS_GPC_MFPH, (inpw(REG_SYS_GPC_MFPH) & 0xfffffff0) | 0x00000007); // UART1_CTS:PC8

    /*---------------------------------------------------------------------------------------------------------*/
    /* SAMPLE CODE                                                                                             */
    /*---------------------------------------------------------------------------------------------------------*/
    UART_PDMATest();

    while(1);
}

/**
 * @brief       DMA IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The DMA default IRQ, declared in startup_M480.s.
 */
void PDMA_IRQHandler(void)
{
    uint32_t status = PDMA_GET_INT_STATUS(PDMA0);

    if (status & 0x1)   /* abort */
    {
        printf("target abort interrupt !!\n");
        if (PDMA_GET_ABORT_STS(PDMA0) & 0x4)
            u32IsTestOver = 2;
        PDMA_CLR_ABORT_FLAG(PDMA0,PDMA_GET_ABORT_STS(PDMA0));
    }
    else if (status & 0x2)     /* done */
    {
        if ( (PDMA_GET_TD_STS(PDMA0) & (1 << 0)) && (PDMA_GET_TD_STS(PDMA0) & (1 << 1)) )
        {
            u32IsTestOver = 1;
            PDMA_CLR_TD_FLAG(PDMA0,PDMA_GET_TD_STS(PDMA0));
        }
    }
    else if (status & 0x300)     /* channel 2 timeout */
    {
        printf("timeout interrupt !!\n");
        u32IsTestOver = 3;

        PDMA_SetTimeOut(PDMA0,0, 0, 0);
        PDMA_CLR_TMOUT_FLAG(PDMA0,0);
        PDMA_SetTimeOut(PDMA0,0, 1, PDMA_TIME);

        PDMA_SetTimeOut(PDMA0,1, 0, 0);
        PDMA_CLR_TMOUT_FLAG(PDMA0,1);
        PDMA_SetTimeOut(PDMA0,1, 1, PDMA_TIME);
    }
    else
        printf("unknown interrupt !!\n");
}


/*---------------------------------------------------------------------------------------------------------*/
/*  UART PDMA Test                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
void UART_PDMATest()
{
    uint32_t i;

    printf("+-----------------------------------------------------------+\n");
    printf("|  UART PDMA Test                                           |\n");
    printf("+-----------------------------------------------------------+\n");
    printf("|  Description :                                            |\n");
    printf("|    The sample code will demo UART1 PDMA function.         |\n");
    printf("|    Please connect UART1_TX and UART1_RX pin.              |\n");
    printf("+-----------------------------------------------------------+\n");
    printf("Please press any key to start test. \n\n");

    getchar();

    /*
        Using UAR1 external loop back.
        This code will send data from UART1_TX and receive data from UART1_RX.
    */

    for (i=0; i<PDMA_TEST_LENGTH; i++)
    {
        g_u8Tx_Buffer_point[i] = i;
        g_u8Rx_Buffer_point[i] = 0xff;
    }

    PDMA_CLR_TD_FLAG(PDMA0, PDMA_TDSTS_TDIF0_Msk|PDMA_TDSTS_TDIF1_Msk);

    while(1)
    {
        PDMA_Init();

        UART1->INTEN = UART_INTEN_RLSIEN_Msk; // Enable Receive Line interrupt

        UART1->INTEN |= UART_INTEN_TXPDMAEN_Msk | UART_INTEN_RXPDMAEN_Msk;

#ifdef ENABLE_PDMA_INTERRUPT
        while(u32IsTestOver == 0);

        if (u32IsTestOver == 1)
            printf("test done...\n");
        else if (u32IsTestOver == 2)
            printf("target abort...\n");
        else if (u32IsTestOver == 3)
            printf("timeout...\n");
#else
        while( (!(PDMA_GET_TD_STS(PDMA0) & (1 << 0))) || (!(PDMA_GET_TD_STS(PDMA0) & (1 << 1))) );

        PDMA_CLR_TD_FLAG(PDMA0,PDMA_GET_TD_STS(PDMA0));
#endif

        UART1->INTEN &= ~UART_INTEN_TXPDMAEN_Msk;
        UART1->INTEN &= ~UART_INTEN_RXPDMAEN_Msk;

        for (i=0; i<PDMA_TEST_LENGTH; i++)
        {
            if(g_u8Rx_Buffer_point[i] != i)
            {
                printf("\n Receive Data Compare Error !!");
                while(1);
            }

            g_u8Rx_Buffer_point[i] = 0xff;
        }

        printf("\nUART PDMA test Pass.\n");
    }

}

void UART1_IRQHandler(void)
{
    uint32_t u32DAT;
    uint32_t u32IntSts = UART1->INTSTS;

    if(u32IntSts & UART_INTSTS_HWRLSIF_Msk)
    {
        if(UART1->FIFOSTS & UART_FIFOSTS_BIF_Msk)
            printf("\n BIF \n");
        if(UART1->FIFOSTS & UART_FIFOSTS_FEF_Msk)
            printf("\n FEF \n");
        if(UART1->FIFOSTS & UART_FIFOSTS_PEF_Msk)
            printf("\n PEF \n");

        u32DAT = UART1->DAT; // read out data
        printf("\n Error Data is '0x%x' \n", u32DAT);
        UART1->FIFOSTS = (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk);
    }
}





