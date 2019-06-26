/****************************************************************************
 * @file     main.c
 * @version  V1.00
 * @brief    Transmit and receive UART data in UART IrDA mode.
 *
 * @copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 *
 ******************************************************************************/
#include <stdio.h>
#include "nuc980.h"
#include "sys.h"
#include "uart.h"

#define RXBUFSIZE 1024

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
uint8_t g_u8SendData[12] = {0};
uint8_t g_u8RecData[RXBUFSIZE]  = {0};

volatile uint32_t g_u32comRbytes = 0;
volatile uint32_t g_u32comRhead  = 0;
volatile uint32_t g_u32comRtail  = 0;
volatile int32_t g_bWait         = TRUE;
volatile int32_t g_i32pointer = 0;

/*---------------------------------------------------------------------------------------------------------*/
/* Define functions prototype                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main(void);
//void UART_TEST_HANDLE(void);
void IrDA_FunctionTxTest(void);
void IrDA_FunctionRxTest(void);
void IrDA_FunctionTest(void);

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

void UART1_Init()
{
    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | (0x1 << 17)); // Enable UART1 engine clock

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART1, 57600);
}

/*---------------------------------------------------------------------------------------------------------*/
/* MAIN function                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/

int32_t main(void)
{
    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);

    /* Init UART0 for printf */
    UART_Init();

    /* Select UART1 Multi-Function */
    outpw(REG_SYS_GPC_MFPL, (inpw(REG_SYS_GPC_MFPL) & 0x000fffff) | 0x77700000); // UART1_RX:PC6, UART1_TX:PC5, UART1_RTS:PC7
    outpw(REG_SYS_GPC_MFPH, (inpw(REG_SYS_GPC_MFPH) & 0xfffffff0) | 0x00000007); // UART1_CTS:PC8

    /* Init UART1 */
    UART1_Init();

    /*---------------------------------------------------------------------------------------------------------*/
    /* SAMPLE CODE                                                                                             */
    /*---------------------------------------------------------------------------------------------------------*/

    printf("+------------------------+\n");
    printf("| IrDA function test |\n");
    printf("+------------------------+\n");

    IrDA_FunctionTest();

    while(1);
}

/*---------------------------------------------------------------------------------------------------------*/
/* ISR to handle UART Channel 0 interrupt event                                                            */
/*---------------------------------------------------------------------------------------------------------*/
void UART1_IRQHandler(void)
{

}

/*---------------------------------------------------------------------------------------------------------*/
/*  IrDA Function Test                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
void IrDA_FunctionTest()
{
    uint8_t u8item;

    printf("+-------------------------------------------------------------+\n");
    printf("|     Pin Configure                                           |\n");
    printf("+-------------------------------------------------------------+\n");
    printf("|  ______                                      _______        |\n");
    printf("| |      |                                    |       |       |\n");
    printf("| |Master|-------- TXD0  <====>  RXD0 ------- |Slave  |       |\n");
    printf("| |      |                                    |       |       |\n");
    printf("| |______|                                    |_______|       |\n");
    printf("|                                                             |\n");
    printf("+-------------------------------------------------------------+\n");

    printf("\n\n");
    printf("+-------------------------------------------------------------+\n");
    printf("|     IrDA Function Test                                      |\n");
    printf("+-------------------------------------------------------------+\n");
    printf("|  Description :                                              |\n");
    printf("|    The sample code needs two boards. One is Master and      |\n");
    printf("|    the other is slave.  Master will send data based on      |\n");
    printf("|    terminal input and Slave will printf received data on    |\n");
    printf("|    terminal screen.                                         |\n");
    printf("|  Please select Master or Slave test                         |\n");
    printf("|  [0] Master    [1] Slave                                    |\n");
    printf("+-------------------------------------------------------------+\n\n");
    u8item = getchar();

    if(u8item=='0')
        IrDA_FunctionTxTest();
    else
        IrDA_FunctionRxTest();

    printf("\nIrDA Sample Code End.\n");

}

/*---------------------------------------------------------------------------------------------------------*/
/*  IrDA Function Transmit Test                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
void IrDA_FunctionTxTest()
{
    uint8_t u8OutChar;

    printf("\n\n");
    printf("+-----------------------------------------------------------+\n");
    printf("|     IrDA Function Tx Mode Test                            |\n");
    printf("+-----------------------------------------------------------+\n");
    printf("| 1). Input char by UART terminal.                         |\n");
    printf("| 2). UART will send a char according to step 1.           |\n");
    printf("| 3). Return step 1. (Press '0' to exit)                    |\n");
    printf("+-----------------------------------------------------------+\n\n");

    printf("\nIRDA Sample Code Start. \n");

    /* Select IrDA Tx mode and set baud rate */
    UART_SelectIrDAMode(UART1, 57600, TRUE); // TRUE is TX mode

    /* Wait Terminal input to send data to UART TX pin */
    do
    {
        u8OutChar = getchar();
        printf("   Input: %c , Send %c out\n",u8OutChar,u8OutChar);
        UART_WRITE(UART1,u8OutChar);
    }
    while(u8OutChar !='0');

}

/*---------------------------------------------------------------------------------------------------------*/
/*  IrDA Function Receive Test                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
void IrDA_FunctionRxTest()
{
    uint8_t u8InChar=0xFF;

    printf("\n\n");
    printf("+-----------------------------------------------------------+\n");
    printf("|     IrDA Function Rx Mode Test                            |\n");
    printf("+-----------------------------------------------------------+\n");
    printf("| 1). Polling RDA_Flag to check data input though UART     |\n");
    printf("| 2). If received data is '0', the program will exit.       |\n");
    printf("|     Otherwise, print received data on terminal            |\n");
    printf("+-----------------------------------------------------------+\n\n");

    /* Select IrDA Rx mode and set baud rate */
    UART_SelectIrDAMode(UART1, 57600, FALSE); // FALSE is RX mode

    printf("Waiting...\n");

    /* Use polling method to wait master data */
    do
    {
        if( UART_IS_RX_READY(UART1))
        {
            u8InChar = UART_READ(UART1);
            printf("   Input: %c \n",u8InChar);
        }
    }
    while(u8InChar !='0');

}
