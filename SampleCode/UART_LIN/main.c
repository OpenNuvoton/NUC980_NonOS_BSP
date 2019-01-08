/**************************************************************************//**
 * @file     main.c
 * @version  V3.00
 * @brief
 *           Transmit LIN header and response.
 *
 * @copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 *
 ******************************************************************************/
#include <stdio.h>
#include "nuc980.h"
#include "sys.h"
#include "uart.h"

#define LIN_ID      0x30

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
volatile int32_t g_i32pointer;
volatile int32_t g_i32RxCounter;
uint8_t g_u8SendData[12];
int8_t g_u8ReceiveData[9];

/*---------------------------------------------------------------------------------------------------------*/
/* Define functions prototype                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main(void);
void UART_FunctionTest(void);
void LIN_Tx_FunctionTest(void);
void LIN_Rx_FunctionTest(void);

void UART1_IRQHandler(void)
{
    volatile uint32_t u32IntSts = UART1->INTSTS;

    if(u32IntSts & UART_INTSTS_LININT_Msk)
    {
        if(UART1->LINSTS & UART_LINSTS_SLVHDETF_Msk)
        {
            // Clear LIN slave header detection flag
            UART1->LINSTS = UART_LINSTS_SLVHDETF_Msk;
            printf("\n LIN Slave Header detected ");
        }

        if(UART1->LINSTS & (UART_LINSTS_SLVHEF_Msk | UART_LINSTS_SLVIDPEF_Msk | UART_LINSTS_BITEF_Msk))
        {
            // Clear LIN error flag
            UART1->LINSTS = (UART_LINSTS_SLVHEF_Msk | UART_LINSTS_SLVIDPEF_Msk | UART_LINSTS_BITEF_Msk);
            printf("\n LIN error detected ");
        }
    }

    if(u32IntSts & UART_INTSTS_RDAINT_Msk)
    {
        g_u8ReceiveData[g_i32RxCounter] = UART1->DAT;
        g_i32RxCounter++;
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


void UART1_Init(void)
{
    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | (0x1 << 17)); // Enable UART1 engine clock

    /* Configure UART1 and set UART1 baud rate */
    UART_Open(UART1, 115200);
}

/*---------------------------------------------------------------------------------------------------------*/
/* UART Test Sample                                                                                        */
/* Test Item                                                                                               */
/* It sends the received data to HyperTerminal.                                                            */
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main(void)
{
    /* Init UART0 for printf and test */
    UART_Init();

    /* Select UART1 Multi-Function */
    outpw(REG_SYS_GPC_MFPL, (inpw(REG_SYS_GPC_MFPL) & 0x000fffff) | 0x77700000); // UART1_RX:PC6, UART1_TX:PC5, UART1_RTS:PC7
    outpw(REG_SYS_GPC_MFPH, (inpw(REG_SYS_GPC_MFPH) & 0xfffffff0) | 0x00000007); // UART1_CTS:PC8

    UART1_Init();

    /*---------------------------------------------------------------------------------------------------------*/
    /* SAMPLE CODE                                                                                             */
    /*---------------------------------------------------------------------------------------------------------*/

    printf("\nUART Sample Program\n");

    /* UART sample function */
    UART_FunctionTest();

    while(1);

}

/*---------------------------------------------------------------------------------------------------------*/
/* Compute Parity Value                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
uint8_t GetParityValue(uint32_t u32id)
{
    uint32_t u32Res = 0, ID[6], p_Bit[2], mask = 0;

    for(mask = 0; mask < 6; mask++)
        ID[mask] = (u32id & (1 << mask)) >> mask;

    p_Bit[0] = (ID[0] + ID[1] + ID[2] + ID[4]) % 2;
    p_Bit[1] = (!((ID[1] + ID[3] + ID[4] + ID[5]) % 2));

    u32Res = u32id + (p_Bit[0] << 6) + (p_Bit[1] << 7);
    return u32Res;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Compute CheckSum Value                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
uint8_t ComputeChksumValue(uint8_t *pu8Buf, uint32_t u32ByteCnt)
{
    uint32_t i, CheckSum = 0;

    for(i = 0 ; i < u32ByteCnt; i++)
    {
        CheckSum += pu8Buf[i];
        if(CheckSum >= 256)
            CheckSum -= 255;
    }
    return (uint8_t)(255 - CheckSum);
}



/*---------------------------------------------------------------------------------------------------------*/
/*  UART Function Test                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
void TestItem(void)
{
    printf("+--------------------------------------------------------------------------------+\n");
    printf("|  UART LIN Demo Function Select                                                 |\n");
    printf("+--------------------------------------------------------------------------------+\n");
    printf("|  1 : Demo LIN Tx Function                                                      |\n");
    printf("|  2 : Demo LIN Rx Function                                                      |\n");
    printf("+--------------------------------------------------------------------------------+\n");
    printf("| Quit                                                                   - [ESC] |\n");
    printf("+--------------------------------------------------------------------------------+\n\n");
    printf("Please Select key (1~2): ");
}

void UART_FunctionTest(void)
{
    uint32_t u32Item;

    do
    {
        TestItem();
        u32Item = getchar();
        printf("%c\n", u32Item);

        switch (u32Item)
        {
            case '1':
                LIN_Tx_FunctionTest();
                break;

            case '2':
                LIN_Rx_FunctionTest();
                break;

            default:
                break;
        }
    }while (u32Item != 27);

    printf("\nUART Demo Program End\n");

}

void LIN_Tx_FunctionTest()
{
    //The sample code will send a LIN header with ID is 0x30 and response field.
    //The response field with 8 data bytes and checksum without including ID.
    //Measurement the UART1 Tx pin to check it.

    uint8_t au8TestPattern[9] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x0}; // 8 data byte + 1 byte checksum

    printf("+--------------------------------------------------------------------------------+\n");
    printf("|  UART LIN Tx Function Test                                                        |\n");
    printf("+--------------------------------------------------------------------------------+\n");
    printf("|  Description :                                                                 |\n");
    printf("|    The sample code will send a LIN header with ID is 0x30 and response field   |\n");
    printf("+--------------------------------------------------------------------------------+\n");

    /* Send break+sync+ID */
    g_i32pointer = 0 ;

    /* Set UART Configuration, LIN Max Speed is 20K */
    UART_SetLineConfig(UART1, 9600, UART_WORD_LEN_8, UART_PARITY_NONE, UART_STOP_BIT_1);

    /* Switch back to LIN Function */
    UART1->FUNCSEL = UART_FUNCSEL_LIN;

    // Send Header
    UART1->LINCTL = UART_LINCTL_PID(LIN_ID) | UART_LINCTL_HSEL_BREAK_SYNC_ID |
                    UART_LINCTL_BSL(1) | UART_LINCTL_BRKFL(12) | UART_LINCTL_IDPEN_Msk;
    /* LIN TX Send Header Enable */
    UART1->LINCTL |= UART_LINCTL_SENDH_Msk;
    /* Wait until break field, sync field and ID field transfer completed */
    while((UART1->LINCTL & UART_LINCTL_SENDH_Msk) == UART_LINCTL_SENDH_Msk);

    /* Compute checksum without ID and fill checksum value to  au8TestPattern[8] */
    au8TestPattern[8] = ComputeChksumValue(&au8TestPattern[0], 8);
    UART_Write(UART1, &au8TestPattern[0], 9);

    printf("\n UART LIN Tx Function Demo End !!\n");
}

void LIN_Rx_FunctionTest(void)
{
    //The sample code will detect LIN header(break+sync+ID) and receive response field.
    //The response field with 8 data bytes and checksum without including ID.

    printf("+--------------------------------------------------------------------------------+\n");
    printf("|  UART LIN Rx Function Test                                                     |\n");
    printf("+--------------------------------------------------------------------------------+\n");
    printf("|  Description :                                                                 |\n");
    printf("|    The sample code will receive a LIN response field data                      |\n");
    printf("+--------------------------------------------------------------------------------+\n");

    g_i32RxCounter = 0;

    /* Set UART Configuration, LIN Max Speed is 20K */
    UART_SetLineConfig(UART1, 9600, UART_WORD_LEN_8, UART_PARITY_NONE, UART_STOP_BIT_1);

    /* Switch back to LIN Function */
    UART1->FUNCSEL = UART_FUNCSEL_LIN;

    /* Enable RDA\Time-out\LIN Interrupt  */
    sysInstallISR(IRQ_LEVEL_1, IRQ_UART1, (PVOID)UART1_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_UART1);
    UART_ENABLE_INT(UART1, (UART_INTEN_RDAIEN_Msk | UART_INTEN_RXTOIEN_Msk | UART_INTEN_LINIEN_Msk));

    // Receive Header: break+sync+ID
    UART1->LINCTL = UART_LINCTL_PID(LIN_ID) | UART_LINCTL_HSEL_BREAK_SYNC_ID | UART_LINCTL_SLVHDEN_Msk | 
                    UART_LINCTL_IDPEN_Msk | UART_LINCTL_MUTE_Msk | UART_LINCTL_SLVEN_Msk;

    while(g_i32RxCounter < 9);

    printf("\n Receive Data:\n [ 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x 0x%x ] \n",
            g_u8ReceiveData[0], g_u8ReceiveData[1], g_u8ReceiveData[2], g_u8ReceiveData[3],
            g_u8ReceiveData[4], g_u8ReceiveData[5], g_u8ReceiveData[6], g_u8ReceiveData[7],
            g_u8ReceiveData[8]);

    sysDisableInterrupt(IRQ_UART1);

    printf("\n UART LIN Rx Function Demo End !!\n");
}

