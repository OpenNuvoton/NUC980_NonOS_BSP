/**************************************************************************//**
 * @file     main.c
 * @brief    Use packet format (all the luma and chroma data interleaved) to
 *           store captured image from NT99141 sensor to DDR.
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include "nuc980.h"
#include "sensor.h"
#include "sys.h"
#include "cap.h"
#include <stdio.h>

#define SENSOR_NT99141  0x1
#define SENSOR_NT99050  0x2
#define SENSOR_GC0308   0x3

/*------------------------------------------------------------------------------------------*/
/* To run CAP_InterruptHandler, when CAP frame end interrupt                                */
/*------------------------------------------------------------------------------------------*/
volatile uint32_t u32FramePass = 0;
void CAP_InterruptHandler(void)
{
    u32FramePass++;
}

/*------------------------------------------------------------------------------------------*/
/*  CAP_IRQHandler                                                                          */
/*------------------------------------------------------------------------------------------*/
void CAP1_IRQHandler(void)
{
    uint32_t u32CapInt;
    u32CapInt = CAP1->INT;
    if( (u32CapInt & (CAP_INT_VIEN_Msk | CAP_INT_VINTF_Msk )) == (CAP_INT_VIEN_Msk | CAP_INT_VINTF_Msk))
    {
        CAP_InterruptHandler();
        CAP1->INT |= CAP_INT_VINTF_Msk;        /* Clear Frame end interrupt */
        u32EscapeFrame = u32EscapeFrame+1;
    }

    if((u32CapInt & (CAP_INT_ADDRMIEN_Msk|CAP_INT_ADDRMINTF_Msk)) == (CAP_INT_ADDRMIEN_Msk|CAP_INT_ADDRMINTF_Msk))
    {
        CAP1->INT |= CAP_INT_ADDRMINTF_Msk; /* Clear Address match interrupt */
    }

    if ((u32CapInt & (CAP_INT_MEIEN_Msk|CAP_INT_MEINTF_Msk)) == (CAP_INT_MEIEN_Msk|CAP_INT_MEINTF_Msk))
    {
        CAP1->INT |= CAP_INT_MEINTF_Msk;    /* Clear Memory error interrupt */
    }

    if ((u32CapInt & (CAP_INT_MDIEN_Msk|CAP_INT_MDINTF_Msk)) == (CAP_INT_MDIEN_Msk|CAP_INT_MDINTF_Msk))
    {
        CAP1->INT |= CAP_INT_MDINTF_Msk;    /* Clear Motion Detection interrupt */
    }
    CAP1->CTL = CAP1->CTL | CAP_CTL_UPDATE;
}

void CAP_SetFreq(CAP_T *CAP,uint32_t u32ModFreqKHz,uint32_t u32SensorFreq)
{
    int32_t i32Div;

    i32Div = (u32ModFreqKHz/u32SensorFreq)-1;
    if(CAP==CAP0)
    {

        /* Enable CAP0 Clock */
        outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | 1<<26);

        /* Enable Sensor Clock */
        outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | 1<<27);

        /* Specified sensor clock */
        if(i32Div>0xF) i32Div=0xf;
        outpw(REG_CLK_DIVCTL3, (inpw(REG_CLK_DIVCTL3) & ~(0xF<<24) ) | i32Div<<24 );
        outpw(REG_CLK_DIVCTL3, (inpw(REG_CLK_DIVCTL3) & ~(0x7<<16) ) | (0<<16) );

        /*
        PC.3    CAP0_CLKO
        PC.4    CAP0_PCLK
        PC.5    CAP0_HSYNC
        PC.6    CAP0_VSYNC
        PC.7    CAP0_FIELD
        PC.8    CAP0_DATA0
        PC.9    CAP0_DATA1
        PC.10   CAP0_DATA2
        PC.11   CAP0_DATA3
        PC.12   CAP0_DATA4
        PC.13   CAP0_DATA5
        PC.14   CAP0_DATA6
        PC.15   CAP0_DATA7
        */
        *((volatile unsigned int *)(SYS_BA+0x080)) |= 0x22222000;
        *((volatile unsigned int *)(SYS_BA+0x084)) |= 0x22222222;
    }
    else
    {
        /* Enable CAP1 Clock */
        outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | 1ul<<31);

        /* Enable Sensor Clock */
        outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | 1<<27);

        /* Specified sensor clock */
        if(i32Div>0xF) i32Div=0xf;
        outpw(REG_CLK_DIVCTL2, (inpw(REG_CLK_DIVCTL2) & ~(0xF<<24) ) | i32Div<<24 );
        outpw(REG_CLK_DIVCTL2, (inpw(REG_CLK_DIVCTL2) & ~(0x7<<16) ) | (0<<16) );

        /*
        PE.0  CAP1_HSYNC
        PE.1  CAP1_VSYNC
        PE.2  CAP1_DATA0
        PE.3  CAP1_DATA1
        PE.4  CAP1_DATA2
        PE.5  CAP1_DATA3
        PE.6  CAP1_DATA4
        PE.7  CAP1_DATA5
        PE.8  CAP1_DATA6
        PE.9  CAP1_DATA7
        PE.10 CAP1_FIELD
        PE.12 CAP1_CLKO
        PF.10 CAP1_PCLK
        */
        *((volatile unsigned int *)(SYS_BA+0x090)) |= 0x77777777;
        *((volatile unsigned int *)(SYS_BA+0x094)) |= 0x00070777;
        *((volatile unsigned int *)(SYS_BA+0x09C)) |= 0x00000700;
    }
}


#define SENSOR_IN_WIDTH         640
#define SENSOR_IN_HEIGHT        480
#define SYSTEM_WIDTH            160
#define SYSTEM_HEIGHT           120
#if defined ( __GNUC__ ) && !(__CC_ARM)
__attribute__((aligned(32))) uint8_t u8FrameBuffer[SYSTEM_WIDTH*SYSTEM_HEIGHT*2];
#else
__align(32) uint8_t u8FrameBuffer[SYSTEM_WIDTH*SYSTEM_HEIGHT*2];
#endif

void PacketFormatDownScale(uint32_t SensorId)
{
    uint32_t u32Frame;

    /* Enable External CAP Interrupt */
    CAP_EnableInt(CAP1,CAP_INT_VIEN_Msk);

    /* Set Vsync polarity, Hsync polarity, pixel clock polarity, Sensor Format and Order */
    if(SensorId==SENSOR_GC0308)
        CAP_Open(CAP1,GC0308SensorPolarity | GC0308DataFormatAndOrder, CAP_CTL_PKTEN );
    else
        CAP_Open(CAP1,NT99XXXSensorPolarity | NT99XXXDataFormatAndOrder, CAP_CTL_PKTEN );

    /* Set Cropping Window Vertical/Horizontal Starting Address and Cropping Window Size */
    CAP_SetCroppingWindow(CAP1,0,0,SENSOR_IN_HEIGHT,SENSOR_IN_WIDTH);

    /* Set System Memory Packet Base Address Register */
    CAP_SetPacketBuf(CAP1,(uint32_t)u8FrameBuffer);

    /* Set Packet Scaling Vertical/Horizontal Factor Register */
    CAP_SetPacketScaling(CAP1,SYSTEM_HEIGHT,SENSOR_IN_HEIGHT,SYSTEM_WIDTH,SENSOR_IN_WIDTH);

    /* Set Packet Frame Output Pixel Stride Width */
    CAP_SetPacketStride(CAP1,SYSTEM_WIDTH);

    /* Start Image Capture Interface */
    CAP_Start(CAP1);

    u32Frame=u32FramePass;
    while(1)
    {
        if(u32Frame!=u32FramePass)
        {
            u32Frame=u32FramePass;
            printf("Get frame %3d\n",u32Frame);
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

extern int InitNT99141_VGA_YUV422(void);
extern int InitNT99050_VGA_YUV422(void);
extern int InitGC0308_VGA_YUV422(void);
/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main (void)
{
    uint32_t sensor_id;
    uint32_t u32Item;
    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    UART_Init();
    printf("\nThis sample code demonstrate CAP packet down scale function\n");
    sysInstallISR(IRQ_LEVEL_1, IRQ_CAP1, (PVOID)CAP1_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_CAP1);

    /* Init Engine clock and Sensor clock */
    CAP_SetFreq(CAP1,48000000,24000000);

    do
    {
        printf("======================================================\n");
        printf(" CAP library demo code                                \n");
        printf(" [1] NT99141 VGA                                      \n");
        printf(" [2] NT99050 VGA                                      \n");
        printf(" [3] GC0308 VGA                                       \n");
        printf("======================================================\n");
        u32Item = getchar();
        switch(u32Item)
        {
        case '1':
            /* Initialize NT99141 sensor and set NT99141 output YUV422 format  */
            sensor_id=SENSOR_NT99141;
            if(InitNT99141_VGA_YUV422()==FALSE)
                printf("Initialize NT99141 sensor failed\n");
            break;
        case '2':
            /* Initialize NT99050 sensor and set NT99050 output YUV422 format  */
            sensor_id=SENSOR_NT99050;
            if(InitNT99050_VGA_YUV422()==FALSE)
                printf("Initialize NT99050 sensor failed\n");
            break;
        case '3':
            /* Initialize GC0308 sensor and set GC0308 output YUV422 format  */
            sensor_id=SENSOR_GC0308;
            if(InitGC0308_VGA_YUV422()==FALSE)
                printf("Initialize GC0308 sensor failed\n");
            break;
        default:
            break;
        }
        /* Using Picket format to Image down scale */
        PacketFormatDownScale(sensor_id);
    }
    while((u32Item!= 'q') || (u32Item!= 'Q'));

    while(1);
}
