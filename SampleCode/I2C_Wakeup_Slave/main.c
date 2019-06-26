/**************************************************************************//**
 * @file     main.c
 * @brief    I2C Wakeup Slave Sample Code
 *
 * @note
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include "nuc980.h"
#include "sys.h"
#include "i2c.h"

uint32_t slave_buff_addr;
uint8_t g_au8SlvData[256];
uint8_t g_au8SlvRxData[3];
uint8_t g_u8SlvI2CWK;
/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
uint8_t g_u8DeviceAddr;
uint8_t g_u8SlvDataLen;

typedef void (*I2C_FUNC)(uint32_t u32Status);

static I2C_FUNC s_I2C0HandlerFn = NULL;

#if defined ( __GNUC__ ) && !(__CC_ARM)
void __wfi(void)
{
    asm volatile(
    "MCR p15, 0, r1, c7, c0, 4  "
    );
}
#else
__asm void __wfi()
{
    MCR p15, 0, r1, c7, c0, 4
    BX  lr
}
#endif

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
/*  I2C0 IRQ Handler                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
void I2C0_IRQHandler(void)
{
    uint32_t u32Status;

    /* Check I2C Wake-up interrupt flag set or not */
    if(I2C_GET_WAKEUP_FLAG(I2C0))
    {
        /* Clear I2C Wake-up interrupt flag */
        I2C_CLEAR_WAKEUP_FLAG(I2C0);
        g_u8SlvI2CWK = 1;

        return;
    }

    u32Status = I2C_GET_STATUS(I2C0);

    if(I2C_GET_TIMEOUT_FLAG(I2C0))
    {
        /* Clear I2C0 Timeout Flag */
        I2C_ClearTimeoutFlag(I2C0);
    }
    else
    {
        if(s_I2C0HandlerFn != NULL)
            s_I2C0HandlerFn(u32Status);
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/*  Function for System Entry to Power Down Mode                                                           */
/*---------------------------------------------------------------------------------------------------------*/
void PowerDownFunction(void)
{
    uint32_t i;

    /* To check if all the debug messages are finished */
    while(!(inpw(REG_UART0_FSR) & (1<<28)));

    //enable NUC980 to enter power down mode
    i = *(volatile unsigned int *)(0xB0000200);
    i = i & (0xFFFFFFFE);
    *(volatile unsigned int *)(0xB0000200)=i;

    __wfi();
}

/*---------------------------------------------------------------------------------------------------------*/
/*  I2C Slave Transmit/Receive Callback Function                                                           */
/*---------------------------------------------------------------------------------------------------------*/
void I2C_SlaveTRx(uint32_t u32Status)
{
    if(u32Status == 0x60)                       /* Own SLA+W has been receive; ACK has been return */
    {
        g_u8SlvDataLen = 0;
        I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI_AA);
    }
    else if(u32Status == 0x80)                 /* Previously address with own SLA address
                                                   Data has been received; ACK has been returned*/
    {
        g_au8SlvRxData[g_u8SlvDataLen] = (unsigned char)I2C_GET_DATA(I2C0);
        g_u8SlvDataLen++;

        if(g_u8SlvDataLen == 2)
        {
            slave_buff_addr = (g_au8SlvRxData[0] << 8) + g_au8SlvRxData[1];
        }
        if(g_u8SlvDataLen == 3)
        {
            g_au8SlvData[slave_buff_addr] = g_au8SlvRxData[2];
            g_u8SlvDataLen = 0;
        }
        I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI_AA);
    }
    else if(u32Status == 0xA8)                  /* Own SLA+R has been receive; ACK has been return */
    {
        I2C_SET_DATA(I2C0, g_au8SlvData[slave_buff_addr]);
        slave_buff_addr++;
        I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI_AA);
    }
    else if(u32Status == 0xC0)                 /* Data byte or last data in I2CDAT has been transmitted
                                                   Not ACK has been received */
    {
        I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI_AA);
    }
    else if(u32Status == 0x88)                 /* Previously addressed with own SLA address; NOT ACK has
                                                   been returned */
    {
        g_u8SlvDataLen = 0;
        I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI_AA);
    }
    else if(u32Status == 0xA0)                 /* A STOP or repeated START has been received while still
                                                   addressed as Slave/Receiver*/
    {
        g_u8SlvDataLen = 0;
        I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI_AA);
    }
    else
    {
        /* TO DO */
        printf("Status 0x%x is NOT processed\n", u32Status);
    }
}

void I2C0_Init(void)
{
    outpw(REG_CLK_PCLKEN1, inpw(REG_CLK_PCLKEN1) | (0x1 << 0)); // Enable I2C0 engine clock

    /* Open I2C module and set bus clock */
    I2C_Open(I2C0, 100000);

    /* Get I2C0 Bus Clock */
    printf("I2C clock %d Hz\n", I2C_GetBusClockFreq(I2C0));

    /* SDA:GPA0, SCL:GPA1 */
    outpw(REG_SYS_GPA_MFPL, (inpw(REG_SYS_GPA_MFPL) & 0xffffff00) | 0x33);  // I2C0 multi-function

    /* Set I2C 4 Slave Addresses */
    I2C_SetSlaveAddr(I2C0, 0, 0x15, 0);   /* Slave Address : 0x15 */
    I2C_SetSlaveAddr(I2C0, 1, 0x35, 0);   /* Slave Address : 0x35 */
    I2C_SetSlaveAddr(I2C0, 2, 0x55, 0);   /* Slave Address : 0x55 */
    I2C_SetSlaveAddr(I2C0, 3, 0x75, 0);   /* Slave Address : 0x75 */

    /* Set I2C 4 Slave Addresses Mask */
    I2C_SetSlaveAddrMask(I2C0, 0, 0x01);
    I2C_SetSlaveAddrMask(I2C0, 1, 0x04);
    I2C_SetSlaveAddrMask(I2C0, 2, 0x01);
    I2C_SetSlaveAddrMask(I2C0, 3, 0x04);

    /* Enable I2C interrupt */
    I2C_EnableInt(I2C0);
    sysInstallISR(IRQ_LEVEL_1, IRQ_I2C0, (PVOID)I2C0_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_I2C0);
}

void I2C0_Close(void)
{
    /* Disable I2C0 interrupt */
    I2C_DisableInt(I2C0);

    /* Disable I2C0 */
    I2C_Close(I2C0);
}

/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main(void)
{
    uint32_t i;

    // Unlock Register
    outpw(0xB00001FC, 0x59);
    outpw(0xB00001FC, 0x16);
    outpw(0xB00001FC, 0x88);
    while(!(inpw(0xB00001FC) & 0x1));

    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    UART_Init();

    /*
        This sample code is I2C SLAVE mode and it simulates EEPROM function
    */
    printf("\n");
    printf("+----------------------------------------------------------------------+\n");
    printf("| I2C Driver Sample Code (Slave) for wake-up & access Slave test       |\n");
    printf("|                                                                      |\n");
    printf("| I2C Master (I2C0) <---> I2C Slave(I2C0)                              |\n");
    printf("+----------------------------------------------------------------------+\n");

    printf("Configure I2C0 as a slave.\n");
    printf("The I/O connection for I2C0:\n");

    /* Init I2C0 */
    I2C0_Init();

    for(i = 0; i < 0x100; i++)
    {
        g_au8SlvData[i] = 0;
    }

    /* I2C function to Transmit/Receive data as slave */
    s_I2C0HandlerFn = I2C_SlaveTRx;

    /* Set I2C0 enter Not Address SLAVE mode */
    I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI_AA);

    /* Select power wake-up Source*/
    outpw(REG_SYS_WKUPSER1, inpw(REG_SYS_WKUPSER1) | (0x1 << 0));

    /* Enable I2C wake-up */
    I2C_EnableWakeup(I2C0);
    g_u8SlvI2CWK = 0;

    printf("\n");
    printf("\n");
    printf("CHIP enter power down status.\n");

    if(((I2C0->CTL0)&I2C_CTL0_SI_Msk) != 0)
    {
        I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI);
    }
    /* Enter to Power-down mode */
    PowerDownFunction();

    /* Waiting for syteem wake-up and I2C wake-up finish*/
    while(g_u8SlvI2CWK == 0);

    /* Waitinn for I2C response ACK finish */
    while(!(I2C0->WKSTS & I2C_WKSTS_WKAKDONE_Msk));

    /* Clear Wakeup done flag, I2C will release bus */
    I2C0->WKSTS = I2C_WKSTS_WKAKDONE_Msk;

    /* Wake-up Interrupt Message */
    printf("Power-down Wake-up Source 0x%x\n", inpw(REG_SYS_WKUPSSR1));
    printf("I2C0 WAKE INT 0x%x\n", I2C0->WKSTS);

    printf("\n");
    printf("Slave wake-up from power down status.\n");

    printf("\n");
    printf("Slave Waiting for receiving data.\n");

    while(1);
}







