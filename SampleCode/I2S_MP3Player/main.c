/**************************************************************************//**
 * @file     main.c
 * @version  V1.00
 * @brief    MP3 player sample plays MP3 files stored on SD memory card
 *
 * @copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 *
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "nuc980.h"
#include "sys.h"
#include "i2s.h"
#include "i2c.h"
#include "config.h"
#include "sdh.h"
#include "ff.h"
#include "diskio.h"

FATFS FatFs[_VOLUMES];               /* File system object for logical drive */

uint8_t bAudioPlaying = 0;
extern uint32_t volatile sd_init_ok;
extern volatile uint8_t aPCMBuffer_Full[2];
volatile uint8_t u8PCMBuffer_Playing=0;

/***********************************************/
/*---------------------------------------------------------*/
/* User Provided RTC Function for FatFs module             */
/*---------------------------------------------------------*/
/* This is a real time clock service to be called from     */
/* FatFs module. Any valid time must be returned even if   */
/* the system does not support an RTC.                     */
/* This function is not required in read-only cfg.         */

unsigned long get_fattime (void)
{
    unsigned long tmr;

    tmr=0x00000;

    return tmr;
}

unsigned int volatile gCardInit = 0;
void SDH_IRQHandler(void)
{
    unsigned int volatile isr;
    unsigned int volatile ier;

    // FMI data abort interrupt
    if (SDH1->GINTSTS & SDH_GINTSTS_DTAIF_Msk)
    {
        /* ResetAllEngine() */
        SDH1->GCTL |= SDH_GCTL_GCTLRST_Msk;
    }

    //----- SD interrupt status
    isr = SDH1->INTSTS;
    if (isr & SDH_INTSTS_BLKDIF_Msk)
    {
        // block down
        g_u8SDDataReadyFlag = TRUE;
        SDH1->INTSTS = SDH_INTSTS_BLKDIF_Msk;
    }

    if (isr & SDH_INTSTS_CDIF_Msk)   // card detect
    {

        //----- SD interrupt status
        // it is work to delay 50 times for SDH_CLK = 200KHz
        {
            int volatile i;         // delay 30 fail, 50 OK
            for (i=0; i<0x500; i++);  // delay to make sure got updated value from REG_SDISR.
            isr = SDH1->INTSTS;
        }

        if (isr & SDH_INTSTS_CDSTS_Msk)
        {
            printf("\n***** card remove !\n");
            gCardInit = 0;
            SDH_Close_Disk(SDH1);
        }
        else
        {
            printf("***** card insert !\n");
            gCardInit = 1;
        }
        SDH1->INTSTS = SDH_INTSTS_CDIF_Msk;
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
        SDH1->INTSTS = SDH_INTSTS_CRCIF_Msk;      // clear interrupt flag
    }

    if (isr & SDH_INTSTS_DITOIF_Msk)
    {
        printf("***** ISR: data in timeout !\n");
        SDH1->INTSTS |= SDH_INTSTS_DITOIF_Msk;
    }

    // Response in timeout interrupt
    if (isr & SDH_INTSTS_RTOIF_Msk)
    {
        printf("***** ISR: response in timeout !\n");
        SDH1->INTSTS |= SDH_INTSTS_RTOIF_Msk;
    }
}

void Delay(int count)
{
	volatile uint32_t i;
	for (i = 0; i < count ; i++);
}

void play_callback(void)
{
    if (aPCMBuffer_Full[u8PCMBuffer_Playing^1] != 1)
        printf("underflow!!\n");
    aPCMBuffer_Full[u8PCMBuffer_Playing] = 0;       //set empty flag
    u8PCMBuffer_Playing ^= 1;
}

/*---------------------------------------------------------------------------------------------------------*/
/*  Write 9-bit data to 7-bit address register of NAU8822 with I2C0                                        */
/*---------------------------------------------------------------------------------------------------------*/
void I2C_WriteNAU8822(uint8_t u8addr, uint16_t u16data)
{
    //printf("%d = 0x%x\n", u8addr, u16data);
    I2C_START(I2C0);
    I2C_WAIT_READY(I2C0);

    I2C_SET_DATA(I2C0, 0x1A<<1);
    I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI);
    I2C_WAIT_READY(I2C0);

    I2C_SET_DATA(I2C0, (uint8_t)((u8addr << 1) | (u16data >> 8)));
    I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI);
    I2C_WAIT_READY(I2C0);

    I2C_SET_DATA(I2C0, (uint8_t)(u16data & 0x00FF));
    I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI);
    I2C_WAIT_READY(I2C0);

    I2C_STOP(I2C0);
}

/*---------------------------------------------------------------------------------------------------------*/
/*  NAU8822 Settings with I2C interface                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
void NAU8822_Setup()
{
    printf("\nConfigure NAU8822 ...");

    I2C_WriteNAU8822(0,  0x000);   /* Reset all registers */
    Delay(0x200);

    //input source is MIC
    I2C_WriteNAU8822(1,  0x03F);
    I2C_WriteNAU8822(2,  0x1BF);   /* Enable L/R Headphone, ADC Mix/Boost, ADC */
    I2C_WriteNAU8822(3,  0x07F);   /* Enable L/R main mixer, DAC */
    I2C_WriteNAU8822(4,  0x010);   /* 16-bit word length, I2S format, Stereo */
    I2C_WriteNAU8822(5,  0x000);   /* Companding control and loop back mode (all disable) */
    I2C_WriteNAU8822(10, 0x008);   /* DAC soft mute is disabled, DAC oversampling rate is 128x */
    I2C_WriteNAU8822(14, 0x108);   /* ADC HP filter is disabled, ADC oversampling rate is 128x */
    I2C_WriteNAU8822(15, 0x1EF);   /* ADC left digital volume control */
    I2C_WriteNAU8822(16, 0x1EF);   /* ADC right digital volume control */
    I2C_WriteNAU8822(44, 0x033);   /* LMICN/LMICP is connected to PGA */
    I2C_WriteNAU8822(50, 0x001);   /* Left DAC connected to LMIX */
    I2C_WriteNAU8822(51, 0x001);   /* Right DAC connected to RMIX */

    printf("[OK]\n");
}

void SYS_Init(void)
{
    /* enable SDH */
    outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | 0x40000000);

    /* select multi-function-pin */
    /* SD Port 0 -> PF0~6 */
    outpw(REG_SYS_GPF_MFPL, (inpw(REG_SYS_GPF_MFPL)&0x0FFFFFFF) | 0x02222222);
    //SD_Drv = 0;

    /* Configure multi function pins to I2S A2~A6 */
    outpw(REG_SYS_GPA_MFPL, (inpw(REG_SYS_GPA_MFPL) & ~0x0FFFFF00) | 0x02222200);
    /* Configure multi function pins to I2C0 */
	outpw(REG_SYS_GPG_MFPL, (inpw(REG_SYS_GPG_MFPL) & ~0xffff) | 0x88);
}

extern void MP3Player(void);

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

void I2C0_Init(void)
{
    outpw(REG_CLK_PCLKEN1, inpw(REG_CLK_PCLKEN1) | (0x1 << 0)); // Enable I2C0 engine clock

    /* Open I2C0 and set clock to 100k */
    I2C_Open(I2C0, 100000);

    /* Get I2C0 Bus Clock */
    //printf("I2C clock %d Hz\n", I2C_GetBusClockFreq(I2C0));

    /* SDA:GPA0, SCL:GPA1 */
    outpw(REG_SYS_GPA_MFPL, (inpw(REG_SYS_GPA_MFPL) & 0xffffff00) | 0x33);  // I2C0 multi-function
}

int32_t main(void)
{
    TCHAR sd_path[] = { '0', ':', 0 };    /* SD drive started from 0 */

	sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    UART_Init();

    printf("\n");
    printf("+-----------------------------------------------------------------------+\n");
    printf("|                   MP3 Player Sample with NAU8822 Codec                |\n");
    printf("+-----------------------------------------------------------------------+\n");
    printf(" Please put MP3 files on SD card \n");

    SYS_Init();

    sysInstallISR(IRQ_LEVEL_1, IRQ_SDH, (PVOID)SDH_IRQHandler);
    /* enable CPSR I bit */
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_SDH);

    SDH_Open_Disk(SDH1, CardDetect_From_GPIO);
    f_chdrive(sd_path);          /* set default path */

    I2C0_Init();

    // Initialize I2S interface
    i2sInit();
    if(i2sOpen() != 0)
        return 0;

    // Select I2S function
    i2sIoctl(I2S_SELECT_BLOCK, I2S_BLOCK_I2S, 0);
    // Select 16-bit data width
    i2sIoctl(I2S_SELECT_BIT, I2S_BIT_WIDTH_16, 0);
    
    // Set DMA interrupt selection to half of DMA buffer
    i2sIoctl(I2S_SET_PLAY_DMA_INT_SEL, I2S_DMA_INT_HALF, 0);
    
    // Set to stereo 
    i2sIoctl(I2S_SET_CHANNEL, I2S_PLAY, I2S_CHANNEL_P_I2S_TWO);
    
    // Select I2S format
    i2sIoctl(I2S_SET_I2S_FORMAT, I2S_FORMAT_I2S, 0);

    // Set as master
    i2sIoctl(I2S_SET_MODE, I2S_MODE_MASTER, 0);

    // Set play and record call-back functions
    i2sIoctl(I2S_SET_I2S_CALLBACKFUN, I2S_PLAY, (uint32_t)&play_callback);

    // Configure NAU8822 audio codec
    NAU8822_Setup();

    while(1)
    {
        /* play mp3 */
        if (SDH_CardDetection(SDH1))
            MP3Player();
    }
}	

/* config play sampling rate */
void i2sConfigSampleRate(unsigned int u32SampleRate)
{
    printf("Configure Sampling Rate to %d\n", u32SampleRate);
    if((u32SampleRate % 8) == 0)
    {
        //12.288MHz ==> APLL=98.4MHz / 4 = 24.6MHz
        //APLL is 98.4MHz
        outpw(REG_CLK_APLLCON, 0xC0008028);
        // Select APLL as I2S source and divider is (3+1)
        outpw(REG_CLK_DIVCTL1, (inpw(REG_CLK_DIVCTL1) & ~0x001f0000) | (0x2 << 19) | (0x3 << 24));
        // Set data width is 16-bit, stereo
        i2sSetSampleRate(24600000, u32SampleRate, 16, 2);
    }
    else
    {
        //11.2896Mhz ==> APLL=90.4 MHz / 8 = 11.2875 MHz
        //APLL is 90.4 MHz
        outpw(REG_CLK_APLLCON, 0xC0008170);
        // Select APLL as I2S source and divider is (7+1)
        outpw(REG_CLK_DIVCTL1, (inpw(REG_CLK_DIVCTL1) & ~0x001f0000) | (0x2 << 19) | (0x7 << 24));
        // Set data width is 16-bit, stereo
        i2sSetSampleRate(11287500, u32SampleRate, 16, 2);
    }
}
