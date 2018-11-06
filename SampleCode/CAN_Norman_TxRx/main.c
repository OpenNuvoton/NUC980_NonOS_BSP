/**************************************************************************//**
 * @file     main.c
 * @brief    CAN Sample Code
 *
 * @note
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include "nuc980.h"
#include "sys.h"
#include "uart.h"
#include "can.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
STR_CANMSG_T rrMsg;

void CAN_ShowMsg(STR_CANMSG_T* Msg);


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
/* ISR to handle CAN interrupt event                                                            */
/*---------------------------------------------------------------------------------------------------------*/
void CAN_MsgInterrupt(CAN_T *tCAN, uint32_t u32IIDR)
{
    if(u32IIDR==1)
    {
        printf("Msg-0 INT and Callback\n");
        CAN_Receive(tCAN, 0,&rrMsg);
        CAN_ShowMsg(&rrMsg);
    }
    if(u32IIDR==5+1)
    {
        printf("Msg-5 INT and Callback \n");
        CAN_Receive(tCAN, 5,&rrMsg);
        CAN_ShowMsg(&rrMsg);
    }
    if(u32IIDR==31+1)
    {
        printf("Msg-31 INT and Callback \n");
        CAN_Receive(tCAN, 31,&rrMsg);
        CAN_ShowMsg(&rrMsg);
    }
}


/**
  * @brief  CAN0_IRQ Handler.
  * @param  None.
  * @return None.
  */
void CAN0_IRQHandler(void)
{
    uint32_t u8IIDRstatus;

    u8IIDRstatus = CAN0->IIDR;

    if(u8IIDRstatus == 0x00008000)        /* Check Status Interrupt Flag (Error status Int and Status change Int) */
    {
        /**************************/
        /* Status Change interrupt*/
        /**************************/
        if(CAN0->STATUS & CAN_STATUS_RXOK_Msk)
        {
            CAN0->STATUS &= ~CAN_STATUS_RXOK_Msk;   /* Clear Rx Ok status*/

            printf("RX OK INT\n") ;
        }

        if(CAN0->STATUS & CAN_STATUS_TXOK_Msk)
        {
            CAN0->STATUS &= ~CAN_STATUS_TXOK_Msk;    /* Clear Tx Ok status*/

            printf("TX OK INT\n") ;
        }

        /**************************/
        /* Error Status interrupt */
        /**************************/
        if(CAN0->STATUS & CAN_STATUS_EWARN_Msk)
        {
            printf("EWARN INT\n") ;
        }

        if(CAN0->STATUS & CAN_STATUS_BOFF_Msk)
        {
            printf("BOFF INT\n") ;
        }
    }
    else if (u8IIDRstatus!=0)
    {
        printf("=> Interrupt Pointer = %d\n",CAN0->IIDR -1);

        CAN_MsgInterrupt(CAN0, u8IIDRstatus);

        CAN_CLR_INT_PENDING_BIT(CAN0, ((CAN0->IIDR) -1));      /* Clear Interrupt Pending */

    }
    else if(CAN0->WU_STATUS == 1)
    {
        printf("Wake up\n");

        CAN0->WU_STATUS = 0;                       /* Write '0' to clear */
    }

}

/**
  * @brief  CAN1_IRQ Handler.
  * @param  None.
  * @return None.
  */
void CAN1_IRQHandler(void)
{
    uint32_t u8IIDRstatus;

    u8IIDRstatus = CAN1->IIDR;

    if(u8IIDRstatus == 0x00008000)        /* Check Status Interrupt Flag (Error status Int and Status change Int) */
    {
        /**************************/
        /* Status Change interrupt*/
        /**************************/
        if(CAN1->STATUS & CAN_STATUS_RXOK_Msk)
        {
            CAN1->STATUS &= ~CAN_STATUS_RXOK_Msk;   /* Clear Rx Ok status*/

            printf("RX OK INT\n") ;
        }

        if(CAN1->STATUS & CAN_STATUS_TXOK_Msk)
        {
            CAN1->STATUS &= ~CAN_STATUS_TXOK_Msk;    /* Clear Tx Ok status*/

            printf("TX OK INT\n") ;
        }

        /**************************/
        /* Error Status interrupt */
        /**************************/
        if(CAN1->STATUS & CAN_STATUS_EWARN_Msk)
        {
            printf("EWARN INT\n") ;
        }

        if(CAN1->STATUS & CAN_STATUS_BOFF_Msk)
        {
            printf("BOFF INT\n") ;
        }
    }
    else if (u8IIDRstatus!=0)
    {
        //printf("=> Interrupt Pointer = %d\n",CAN1->IIDR -1);
        printf("=> Interrupt Pointer = %d\n",u8IIDRstatus -1);

        CAN_MsgInterrupt(CAN1, u8IIDRstatus);

        //CAN_CLR_INT_PENDING_BIT(CAN1, ((CAN1->IIDR) -1));      /* Clear Interrupt Pending */
        CAN_CLR_INT_PENDING_BIT(CAN1, (u8IIDRstatus -1));      /* Clear Interrupt Pending */

    }
    else if(CAN1->WU_STATUS == 1)
    {
        printf("Wake up\n");

        CAN1->WU_STATUS = 0;                       /* Write '0' to clear */
    }

}

/**
  * @brief      Init CAN driver
  */

void CAN_Init(CAN_T  *tCAN)
{
    // Enable IP clock
    if(tCAN == CAN0)
    {
        outp32(REG_CLK_PCLKEN1, inp32(REG_CLK_PCLKEN1) | (0x1 << 8));
    }
    else if(tCAN == CAN1)
    {
        outp32(REG_CLK_PCLKEN1, inp32(REG_CLK_PCLKEN1) | (0x1 << 9));
    }
}

/**
  * @brief      Disable CAN
  * @details    Reset and clear all CAN control and disable CAN IP
  */

void CAN_STOP(CAN_T  *tCAN)
{
    /* Disable CANx Clock and Reset it */
    if(tCAN == CAN0)
    {
        outp32(REG_SYS_APBIPRST1, inp32(REG_SYS_APBIPRST1) |  (0x1 << 8));
        outp32(REG_SYS_APBIPRST1, inp32(REG_SYS_APBIPRST1) &~ (0x1 << 8));

        outp32(REG_CLK_PCLKEN1, inp32(REG_CLK_PCLKEN1) &~ (0x1 << 8));
    }
    else if(tCAN == CAN1)
    {
        outp32(REG_SYS_APBIPRST1, inp32(REG_SYS_APBIPRST1) |  (0x1 << 9));
        outp32(REG_SYS_APBIPRST1, inp32(REG_SYS_APBIPRST1) &~ (0x1 << 9));

        outp32(REG_CLK_PCLKEN1, inp32(REG_CLK_PCLKEN1) &~ (0x1 << 9));
    }
}

/*----------------------------------------------------------------------------*/
/*  Some description about how to create test environment                     */
/*----------------------------------------------------------------------------*/
void Note_Configure()
{
    printf("\n\n");
    printf("+------------------------------------------------------------------------+\n");
    printf("|  About CAN sample code configure                                       |\n");
    printf("+------------------------------------------------------------------------+\n");
    printf("|   The sample code provide a simple sample code for you study CAN       |\n");
    printf("|   Before execute it, please check description as below                 |\n");
    printf("|                                                                        |\n");
    printf("|   1.CAN_TX and CAN_RX should be connected to your CAN transceiver      |\n");
    printf("|   2.Using two module board and connect to the same CAN BUS             |\n");
    printf("|   3.Check the terminal resistor of bus is connected                    |\n");
    printf("|   4.Using UART0 as print message port                                  |\n");
    printf("|                                                                        |\n");
    printf("|  |--------|       |-----------| CANBUS  |-----------|       |--------| |\n");
    printf("|  |        |------>|           |<------->|           |<------|        | |\n");
    printf("|  |        |CAN_TX |   CAN     |  CAN_H  |   CAN     |CAN_TX |        | |\n");
    printf("|  |  M480  |       |Transceiver|         |Transceiver|       |  M480  | |\n");
    printf("|  |        |<------|           |<------->|           |------>|        | |\n");
    printf("|  |        |CAN_RX |           |  CAN_L  |           |CAN_RX |        | |\n");
    printf("|  |--------|       |-----------|         |-----------|       |--------| |\n");
    printf("|   |                                                           |        |\n");
    printf("|   |                                                           |        |\n");
    printf("|   V                                                           V        |\n");
    printf("| UART0                                                         UART0    |\n");
    printf("|(print message)                                          (print message)|\n");
    printf("+------------------------------------------------------------------------+\n");
}

/*----------------------------------------------------------------------------*/
/*  Test Function                                                             */
/*----------------------------------------------------------------------------*/
void CAN_ShowMsg(STR_CANMSG_T* Msg)
{
    uint8_t i;
    printf("Read ID=%8X, Type=%s, DLC=%d,Data=",Msg->Id,Msg->IdType?"EXT":"STD",Msg->DLC);
    for(i=0; i<Msg->DLC; i++)
        printf("%02X,",Msg->Data[i]);
    printf("\n\n");
}

/*----------------------------------------------------------------------------*/
/*  Send Tx Msg by Normal Mode Function (With Message RAM)                    */
/*----------------------------------------------------------------------------*/
void Test_NormalMode_Tx(CAN_T *tCAN)
{
    STR_CANMSG_T tMsg;
    uint32_t i;

    /* Send a 11-bits message */
    tMsg.FrameType= CAN_DATA_FRAME;
    tMsg.IdType   = CAN_STD_ID;
    tMsg.Id       = 0x7FF;
    tMsg.DLC      = 2;
    tMsg.Data[0]  = 7;
    tMsg.Data[1]  = 0xFF;

    if(CAN_Transmit(tCAN, MSG(0),&tMsg) == FALSE)   // Configure Msg RAM and send the Msg in the RAM
    {
        printf("Set Tx Msg Object failed\n");
        return;
    }

    printf("MSG(0).Send STD_ID:0x7FF, Data[07,FF]done\n");

    /* Send a 29-bits message */
    tMsg.FrameType= CAN_DATA_FRAME;
    tMsg.IdType   = CAN_EXT_ID;
    tMsg.Id       = 0x12345;
    tMsg.DLC      = 3;
    tMsg.Data[0]  = 1;
    tMsg.Data[1]  = 0x23;
    tMsg.Data[2]  = 0x45;

    if(CAN_Transmit(tCAN, MSG(1),&tMsg) == FALSE)
    {
        printf("Set Tx Msg Object failed\n");
        return;
    }

    printf("MSG(1).Send EXT:0x12345 ,Data[01,23,45]done\n");

    /* Send a data message */
    tMsg.FrameType= CAN_DATA_FRAME;
    tMsg.IdType   = CAN_EXT_ID;
    tMsg.Id       = 0x7FF01;
    tMsg.DLC      = 4;
    tMsg.Data[0]  = 0xA1;
    tMsg.Data[1]  = 0xB2;
    tMsg.Data[2]  = 0xC3;
    tMsg.Data[3]  = 0xD4;

    if(CAN_Transmit(tCAN, MSG(3),&tMsg) == FALSE)
    {
        printf("Set Tx Msg Object failed\n");
        return;
    }

    printf("MSG(3).Send EXT:0x7FF01 ,Data[A1,B2,C3,D4]done\n");

    for(i=0; i < 10000; i++);

    printf("Trasmit Done!\nCheck the receive host received data\n\n");

}

/*----------------------------------------------------------------------------*/
/*  Receive Rx Msg by Normal Mode Function (With Message RAM)                    */
/*----------------------------------------------------------------------------*/
void Test_NormalMode_SetRxMsg(CAN_T *tCAN)
{
    if(CAN_SetRxMsg(tCAN, MSG(0),CAN_STD_ID, 0x7FF) == FALSE)
    {
        printf("Set Rx Msg Object failed\n");
        return;
    }

    if(CAN_SetRxMsg(tCAN, MSG(5),CAN_EXT_ID, 0x12345) == FALSE)
    {
        printf("Set Rx Msg Object failed\n");
        return;
    }

    if(CAN_SetRxMsg(tCAN, MSG(31),CAN_EXT_ID, 0x7FF01) == FALSE)
    {
        printf("Set Rx Msg Object failed\n");
        return;
    }

}

void Test_NormalMode_WaitRxMsg(CAN_T *tCAN)
{
    /*Choose one mode to test*/
#if 1
    /* Polling Mode */
    while(1)
    {
        while(tCAN->IIDR ==0);            /* Wait IDR is changed */
        CAN_Receive(tCAN, tCAN->IIDR -1, &rrMsg);
        CAN_ShowMsg(&rrMsg);
    }
#else
    /* INT Mode */
    CAN_EnableInt(tCAN, CAN_CON_IE_Msk);

    if(tCAN == CAN0)
    {
        sysInstallISR(IRQ_LEVEL_1, IRQ_CAN0, (PVOID)CAN0_IRQHandler);
        sysSetLocalInterrupt(ENABLE_IRQ);
        sysEnableInterrupt(IRQ_CAN0);
    }
    else if(tCAN == CAN1)
    {
        sysInstallISR(IRQ_LEVEL_1, IRQ_CAN1, (PVOID)CAN1_IRQHandler);
        sysSetLocalInterrupt(ENABLE_IRQ);
        sysEnableInterrupt(IRQ_CAN1);
    }

    printf("Enter any key to exit\n");
    getchar();
#endif
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

    /* Select CAN Multi-Function */
    outpw(REG_SYS_GPD_MFPL, (inpw(REG_SYS_GPD_MFPL) & 0x00ffffff) | 0x44000000); // CAN0_RX:PD6, CAN0_TX:PD7
    outpw(REG_SYS_GPD_MFPH, (inpw(REG_SYS_GPD_MFPH) & 0x00ffffff) | 0x44000000); // CAN1_RX:PD14, CAN1_TX:PD15

    /* CAN Init */
    CAN_Init(CAN0);
    CAN_Init(CAN1);

    Note_Configure();

    CAN_Open(CAN0,  500000, CAN_NORMAL_MODE);
    CAN_Open(CAN1,  500000, CAN_NORMAL_MODE);

    printf("\n");
    printf("+------------------------------------------------------------------ +\n");
    printf("|  Nuvoton CAN BUS DRIVER DEMO                                      |\n");
    printf("+-------------------------------------------------------------------+\n");
    printf("|  Transmit/Receive a message by normal mode                        |\n");
    printf("+-------------------------------------------------------------------+\n");

    printf("Press any key to continue ...\n\n");
    getchar();

    Test_NormalMode_SetRxMsg(CAN1);

    Test_NormalMode_Tx(CAN0);

    Test_NormalMode_WaitRxMsg(CAN1);

    while(1);
}



