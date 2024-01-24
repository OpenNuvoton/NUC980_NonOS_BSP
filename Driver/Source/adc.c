/**************************************************************************//**
* @file     adc.c
* @version  V1.00
* $Revision: 8 $
* $Date: 15/10/06 9:08a $
* @brief    NUC980 ADC driver source file
*
* @note
 * SPDX-License-Identifier: Apache-2.0
* Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include "nuc980.h"
#include "sys.h"
#include "adc.h"
#include "string.h"

/** @addtogroup Standard_Driver Standard Driver
  @{
*/

/** @addtogroup ADC_Driver ADC Driver
  @{
*/

/** @addtogroup ADC_EXPORTED_FUNCTIONS ADC Exported Functions
  @{
*/
/// @cond HIDDEN_SYMBOLS
#define ADC_FIFO_LENGTH 128
volatile int mst_complete;
typedef struct
{
    ADC_CALLBACK voltage_battery_callback;
    UINT32 voltage_battery_userData;

    ADC_CALLBACK normal_callback;
    UINT32 normal_userData;

    ADC_CALLBACK touch_callback;
    UINT32 touch_userData;

    ADC_CALLBACK touchz_callback;
    UINT32 touchz_userData;

    ADC_CALLBACK touch_wakeup_callback;
    UINT32 touch_wakeup_userData;

    ADC_CALLBACK pendown_callback;
    UINT32 pendown_userData;

    ADC_CALLBACK penup_callback;
    UINT32 penup_userData;

    INT16 fifoX[ADC_FIFO_LENGTH];
    INT32 fifoHeadX;
    INT32 fifoTailX;
    INT32 fifoLengthX;

    INT16 fifoY[ADC_FIFO_LENGTH];
    INT32 fifoHeadY;
    INT32 fifoTailY;
    INT32 fifoLengthY;

    INT16 fifoZ1[ADC_FIFO_LENGTH];
    INT32 fifoHeadZ1;
    INT32 fifoTailZ1;
    INT32 fifoLengthZ1;

    INT16 fifoZ2[ADC_FIFO_LENGTH];
    INT32 fifoHeadZ2;
    INT32 fifoTailZ2;
    INT32 fifoLengthZ2;
} ADC_HANDLE;
/// @endcond HIDDEN_SYMBOLS

static ADC_HANDLE adcHandler;

void adcISR(void);


/**
 * @brief       Open ADC Function.
 *
 * @retval      <0              Fail
 * @retval      0               Success
 *
 * @details     This function is used to open adc function.
 */
INT adcOpen(void)
{
    return adcOpen2(4000);
}

/**
 * @brief       Open ADC2 Function.
 *
 * @param[in]   freqKhz  The ADC engine clock. It should be 1000Khz~4000Khz
 *
 * @retval      <0              Fail
 * @retval      0               Success
 *
 * @details     This function is used to open adc function.
 */
INT adcOpen2(uint32_t freqKhz)
{
    uint32_t div;
    /* Enable ADC engine clock */
    outpw(REG_CLK_PCLKEN1, inpw(REG_CLK_PCLKEN1) | (1 << 24));

    /* Reset the ADC IP */
    outpw(REG_SYS_APBIPRST1, (1 << 24));
    outpw(REG_SYS_APBIPRST1, 0);

    /* ADC Engine Clock is set to freq Khz */
    if (freqKhz > 4000) freqKhz = 4000;
    if (freqKhz < 1000) freqKhz = 1000;
    div = 12000 / freqKhz;
    outpw(REG_CLK_DIVCTL7, inpw(REG_CLK_DIVCTL7) & ~((0x3 << 19) | (0x7 << 16) | (0xFFul << 24)));
    outpw(REG_CLK_DIVCTL7, (0 << 19) | (0 << 16) | ((div - 1) << 24));

    /* Enable ADC Power */
    outpw(REG_ADC_CTL, ADC_CTL_ADEN);

    /* Enable ADC to high speed mode */
    outpw(REG_ADC_CONF, inpw(REG_ADC_CONF) | ADC_CONF_HSPEED);

    /* Set interrupt */
    sysInstallISR(IRQ_LEVEL_7, IRQ_ADC, (PVOID)adcISR);
    sysSetLocalInterrupt(ENABLE_IRQ);                            /* enable CPSR I bit */
    sysEnableInterrupt(IRQ_ADC);

    /* Init the FIFO buffer */
    memset(&adcHandler, 0, sizeof(ADC_HANDLE));

    return Successful;
}

/**
 * @brief       Close ADC Function.
 *
 * @retval      <0              Fail
 * @retval      0               Success
 *
 * @details     This function is used to close adc function.
 */
int adcClose(void)
{
    /* Disable interrupt */
    sysDisableInterrupt(IRQ_ADC);
    sysSetLocalInterrupt(DISABLE_IRQ);   /* Disable CPSR I bit */

    /* Disable ADC Power */
    outpw(REG_ADC_CTL, inpw(REG_ADC_CTL) & ~ADC_CTL_ADEN);

    /* Disable ADC engine clock */
    outpw(REG_CLK_PCLKEN1, inpw(REG_CLK_PCLKEN1) & ~(1 << 24));

    return Successful;
}

/**
 * @brief       The read touch xy data of ADC.
 *
 * @param[out]  bufX     Data buffer for x-position data
 * @param[out]  bufY     Data buffer for y-position data
 * @param[in]   dataCnt  The length of ADC x- and y-position data from FIFO
 *
 * @return      Data count actually
 *
 * @details     This function is used to read touch x-position data and touch y-position data .
 */
INT adcReadXY(INT16 *bufX, INT16 *bufY, int dataCnt)
{
    INT32 i;
    volatile INT16 *fifoX, *fifoY;
    volatile INT32 headX, headY;
    volatile INT32 fifoLengthX, readLengthX;
    volatile INT32 fifoLengthY, readLengthY;

    fifoX = adcHandler.fifoX;
    fifoY = adcHandler.fifoY;
    headX = adcHandler.fifoHeadX;
    headY = adcHandler.fifoHeadY;
    fifoLengthX = adcHandler.fifoLengthX;
    fifoLengthY = adcHandler.fifoLengthY;

    readLengthX = 0;
    readLengthY = 0;

    for (i = 0; i < dataCnt; i++)
    {
        if (fifoLengthX > readLengthX)
        {
            bufX[i] = fifoX[headX];
            readLengthX++;
            headX++;
            if (headX >= ADC_FIFO_LENGTH)
            {
                headX = 0;
            }
        }
        else
        {
            /* FIFO is empty, just return */
            break;
        }
    }

    for (i = 0; i < dataCnt; i++)
    {
        if (fifoLengthY > readLengthY)
        {
            bufY[i] = fifoY[headY];
            readLengthY++;
            headY++;
            if (headY >= ADC_FIFO_LENGTH)
            {
                headY = 0;
            }
        }
        else
        {
            /* FIFO is empty, just return */
            break;
        }
    }

    /* Update FIFO information */
    adcHandler.fifoHeadX = headX;
    adcHandler.fifoLengthX -= readLengthX;
    adcHandler.fifoHeadY = headY;
    adcHandler.fifoLengthY -= readLengthY;
    return i;
}

/**
 * @brief       The read touch z data of ADC.
 *
 * @param[out]  bufZ1    Data buffer for pressure measure Z1 data
 * @param[out]  bufZ2    Data buffer for pressure measure Z2 data
 * @param[in]   dataCnt  The length of ADC x- and y-position data from FIFO
 *
 * @return      Data count actually
 *
 * @details     This function is used to read touch pressure measure Z1 data and touch pressure measure Z2 data .
 */
int adcReadZ(short *bufZ1, short *bufZ2, int dataCnt)
{
    INT32 i;
    volatile INT16 *fifoZ1, *fifoZ2;
    volatile INT32 headZ1, headZ2;
    volatile INT32 fifoLengthZ1, readLengthZ1;
    volatile INT32 fifoLengthZ2, readLengthZ2;

    fifoZ1 = adcHandler.fifoZ1;
    fifoZ2 = adcHandler.fifoZ2;
    headZ1 = adcHandler.fifoHeadZ1;
    headZ2 = adcHandler.fifoHeadZ2;
    fifoLengthZ1 = adcHandler.fifoLengthZ1;
    fifoLengthZ2 = adcHandler.fifoLengthZ2;

    readLengthZ1 = 0;
    readLengthZ2 = 0;

    for (i = 0; i < dataCnt; i++)
    {
        if (fifoLengthZ1 > readLengthZ1)
        {
            bufZ1[i] = fifoZ1[headZ1];
            readLengthZ1++;
            headZ1++;
            if (headZ1 >= ADC_FIFO_LENGTH)
            {
                headZ1 = 0;
            }
        }
        else
        {
            /* FIFO is empty, just return */
            break;
        }
    }

    for (i = 0; i < dataCnt; i++)
    {
        if (fifoLengthZ2 > readLengthZ2)
        {
            bufZ2[i] = fifoZ2[headZ2];
            readLengthZ2++;
            headZ2++;
            if (headZ2 >= ADC_FIFO_LENGTH)
            {
                headZ2 = 0;
            }
        }
        else
        {
            /* FIFO is empty, just return */
            break;
        }
    }

    /* Update FIFO information */
    adcHandler.fifoHeadZ1 = headZ1;
    adcHandler.fifoLengthZ1 -= readLengthZ1;
    adcHandler.fifoHeadZ2 = headZ2;
    adcHandler.fifoLengthZ2 -= readLengthZ2;
    return i;
}

/**
 * @brief       The ioctl function of ADC device library.
 *
 * @param[in]   cmd   The command of adcIoctl function
 * @param[in]   arg1  The first argument of adcIoctl function
 * @param[in]   arg2  The second argument of adcIoctl function
 *
 * @retval      <0              Wrong command of adcIoctl
 * @retval      0               Success
 *
 * @details     This function is used to ioctl of ADC device library.
 *              Valid parameter combinations listed in following table:
 * |cmd                   |arg1                               |arg2                         |
 * | :---------------------| :-------------------------------  | :-------------------------  |
 * |\ref START_MST        | NULL                              | NULL                        |
 * |\ref START_MST_POLLING | NULL                              | NULL                        |
 * |\ref VBPOWER_ON       | NULL                              | NULL                        |
 * |\ref VBPOWER_OFF      | NULL                              | NULL                        |
 * |\ref PEPOWER_ON       | NULL                              | NULL                        |
 * |\ref PEPOWER_OFF      | NULL                              | NULL                        |
 * |\ref PEDEF_ON         | Callback function                 | UserData                    |
 * |\ref PEDEF_OFF        | NULL                              | NULL                        |
 * |\ref T_ON             | Callback function                 | UserData                    |
 * |\ref T_OFF            | NULL                              | NULL                        |
 * |\ref Z_ON             | Callback function                 | UserData                    |
 * |\ref Z_OFF            | NULL                              | NULL                        |
 * |\ref NAC_ON           | Callback function                 | UserData                    |
 * |\ref NAC_OFF          | NULL                              | NULL                        |
 */
INT adcIoctl(ADC_CMD cmd, INT32 arg1, INT32 arg2)
{
    UINT32 reg;
    switch (cmd)
    {
    case START_MST:             //Menu Start Conversion
    {
        mst_complete = 0;
        reg = inpw(REG_ADC_IER);
        reg = reg | ADC_IER_MIEN;
        outpw(REG_ADC_IER, reg);
        reg = inpw(REG_ADC_CTL);
        reg = reg | ADC_CTL_MST;
        outpw(REG_ADC_CTL, reg);
        while (!mst_complete);
    }
    break;
    case VBPOWER_ON:           //Enable ADC Internal Bandgap Power
    {
        reg = inpw(REG_ADC_CTL);
        reg = reg | ADC_CTL_VBGEN;
        outpw(REG_ADC_CTL, reg);
    }
    break;
    case VBPOWER_OFF:          //Disable ADC Internal Bandgap Power
    {
        reg = inpw(REG_ADC_CTL);
        reg = reg & ~ADC_CTL_VBGEN;
        outpw(REG_ADC_CTL, reg);
    }
    break;
    case NAC_ON: //Enable Normal AD Conversion
    {
        ADC_CALLBACK normal_callback;
        reg = inpw(REG_ADC_CONF);
        reg = reg | ADC_CONF_NACEN | ADC_CONF_REFSEL_AVDD33;
        outpw(REG_ADC_CONF, reg);
        normal_callback = (ADC_CALLBACK) arg1;
        if (normal_callback != NULL)
        {
            adcHandler.normal_callback = normal_callback;
            adcHandler.normal_userData = (UINT32)arg2;
        }
    }
    break;
    case NAC_OFF: //Disable Normal AD Conversion
    {
        reg = inpw(REG_ADC_CONF);
        reg = reg & ~ADC_CONF_NACEN;
        outpw(REG_ADC_CONF, reg);
        adcHandler.normal_callback = (ADC_CALLBACK)NULL;
        adcHandler.normal_userData = (UINT32)NULL;
    }
    break;
    case START_MST_POLLING:             //Menu Start Conversion
    {
        reg = inpw(REG_ADC_IER);
        reg = reg & ~ADC_IER_MIEN;
        outpw(REG_ADC_IER, reg);
        reg = inpw(REG_ADC_CTL);
        reg = reg | ADC_CTL_MST;
        outpw(REG_ADC_CTL, reg);
        while ((inpw(REG_ADC_ISR)&ADC_ISR_MF) == 0);
        adcISR();
    }
    break;
    case PEPOWER_ON:          //Enable Pen Power
    {
        UINT32 treg;
        UINT32 delay;
        treg = inpw(REG_ADC_IER);
        outpw(REG_ADC_IER, treg & ~(ADC_IER_PEDEIEN | ADC_IER_PEUEIEN));

        reg = inpw(REG_ADC_CTL);
        reg = reg | ADC_CTL_PEDEEN;
        outpw(REG_ADC_CTL, reg);

        do
        {
            reg = (ADC_ISR_PEDEF | ADC_ISR_PEUEF);
            outpw(REG_ADC_ISR, reg);
            for (delay = 0; delay < 10000; delay++)
            {
#if defined ( __GNUC__ ) && !(__CC_ARM)
                asm("nop");
#else
                __nop();
#endif
            }
        }
        while (inpw(REG_ADC_ISR) & (ADC_ISR_PEDEF | ADC_ISR_PEUEF));

        outpw(REG_ADC_IER, treg);
    }
    break;
    case PEPOWER_OFF:         //Disable Pen Power
    {
        reg = inpw(REG_ADC_CTL);
        reg = reg & ~ADC_CTL_PEDEEN;
        outpw(REG_ADC_CTL, reg);
    }
    break;
    case PEDEF_ON:          //Enable Pen Down Event
    {
        ADC_CALLBACK pendown_callback;
        reg = inpw(REG_ADC_IER);
        reg = reg | ADC_IER_PEDEIEN;
        outpw(REG_ADC_IER, reg);
        pendown_callback = (ADC_CALLBACK) arg1;
        if (pendown_callback != NULL)
        {
            adcHandler.pendown_callback = pendown_callback;
            adcHandler.pendown_userData = (UINT32)arg2;
        }
    }
    break;
    case PEDEF_OFF:         //Disable Pen Down Event
    {
        reg = inpw(REG_ADC_IER);
        reg = reg & ~ADC_IER_PEDEIEN;
        outpw(REG_ADC_IER, reg);
        adcHandler.pendown_callback = (ADC_CALLBACK)NULL;
        adcHandler.pendown_userData = (UINT32)NULL;
    }
    break;

    case T_ON:   //Enable Touch detection function
    {
        ADC_CALLBACK touch_callback;
        reg = inpw(REG_ADC_CONF);
        reg = reg | ADC_CONF_TEN;
        outpw(REG_ADC_CONF, reg);
        touch_callback = (ADC_CALLBACK) arg1;
        if (touch_callback != NULL)
        {
            adcHandler.touch_callback = touch_callback;
            adcHandler.touch_userData = (UINT32)arg2;
        }
        /* Flush the FIFO */
        adcHandler.fifoHeadX = 0;
        adcHandler.fifoTailX = 0;
        adcHandler.fifoLengthX = 0;
        adcHandler.fifoHeadY = 0;
        adcHandler.fifoTailY = 0;
        adcHandler.fifoLengthY = 0;
    }
    break;
    case T_OFF:   //Disable Touch detection function
    {
        reg = inpw(REG_ADC_CONF);
        reg = reg & ~ADC_CONF_TEN;
        outpw(REG_ADC_CONF, reg);
        adcHandler.touch_callback = (ADC_CALLBACK)NULL;
        adcHandler.touch_userData = (UINT32)NULL;
    }
    break;
    case Z_ON:   //Enable Press measure function
    {
        ADC_CALLBACK touchz_callback;
        reg = inpw(REG_ADC_CONF);
        reg = reg | ADC_CONF_ZEN;
        outpw(REG_ADC_CONF, reg);
        touchz_callback = (ADC_CALLBACK) arg1;
        if (touchz_callback != NULL)
        {
            adcHandler.touchz_callback = touchz_callback;
            adcHandler.touchz_userData = (UINT32)arg2;
        }
        /* Flush the FIFO */
        adcHandler.fifoHeadZ1 = 0;
        adcHandler.fifoTailZ1 = 0;
        adcHandler.fifoLengthZ1 = 0;
        adcHandler.fifoHeadZ2 = 0;
        adcHandler.fifoTailZ2 = 0;
        adcHandler.fifoLengthZ2 = 0;
    }
    break;
    case Z_OFF:   //Disable Press measure function
    {
        reg = inpw(REG_ADC_CONF);
        reg = reg & ~ADC_CONF_ZEN;
        outpw(REG_ADC_CONF, reg);
    }
    break;
    default:
        return ADC_ERR_CMD;
    }
    return Successful;
}

/// @cond HIDDEN_SYMBOLS
#if 0
    #define DbgPrintf printf
#else
    #define DbgPrintf(...)
#endif
/// @endcond HIDDEN_SYMBOLS
/**
 * @brief       The interrupt service routines of ADC
 * @return      None
 * @details     This function is used to ADC interrupt handler.
 */
void adcISR(void)
{
    unsigned int isr, ier, conf;
    conf = inpw(REG_ADC_CONF);
    isr = inpw(REG_ADC_ISR);
    ier = inpw(REG_ADC_IER);

    //DbgPrintf("ADC_IRQHandler Interrupt(0x%08x): ",isr);
    DbgPrintf("REG_ADC_CTL=0x%08x\n", inpw(REG_ADC_CTL));
    DbgPrintf("REG_ADC_IER=0x%08x\n", inpw(REG_ADC_IER));
    DbgPrintf("REG_ADC_ISR=0x%08x\n", inpw(REG_ADC_ISR));

    if ((isr & ADC_ISR_NACF) && (conf & ADC_CONF_NACEN))
    {
        outpw(REG_ADC_ISR, ADC_ISR_NACF);
        if (adcHandler.normal_callback != NULL)
            adcHandler.normal_callback(inpw(REG_ADC_DATA), adcHandler.normal_userData);
        DbgPrintf("normal AD conversion complete\n");
    }

    if (isr & ADC_ISR_MF)
    {
        outpw(REG_ADC_ISR, ADC_ISR_MF);
        mst_complete = 1;
        DbgPrintf("menu complete\n");
    }

    if (isr & ADC_ISR_TF)
    {
        unsigned int value;
        INT32 tailX, lengthX;
        INT32 tailY, lengthY;
        INT16 *fifoX, *fifoY;
        tailX   = adcHandler.fifoTailX;
        lengthX = adcHandler.fifoLengthX;
        tailY   = adcHandler.fifoTailY;
        lengthY = adcHandler.fifoLengthY;
        fifoX  = adcHandler.fifoX;
        fifoY  = adcHandler.fifoY;
        outpw(REG_ADC_ISR, ADC_ISR_TF); //Clear TF flags
        value = inpw(REG_ADC_XYDATA);
        if (adcHandler.touch_callback != NULL)
            adcHandler.touch_callback(value, adcHandler.touch_userData);

        if ((lengthX < ADC_FIFO_LENGTH) && (lengthY < ADC_FIFO_LENGTH))
        {
            fifoX[tailX] = value & 0xFFF;
            lengthX++;
            tailX++;
            if (tailX == ADC_FIFO_LENGTH) tailX = 0;

            fifoY[tailY] = (value >> 16) & 0xFFF;
            lengthY++;
            tailY++;
            if (tailY == ADC_FIFO_LENGTH) tailY = 0;
        }
        /* Update FIFO status */
        adcHandler.fifoTailX = tailX;
        adcHandler.fifoLengthX = lengthX;
        adcHandler.fifoTailY = tailY;
        adcHandler.fifoLengthY = lengthY;
        DbgPrintf("touch detect complete\n");
    }

    if ((isr & ADC_ISR_ZF) && (conf & ADC_CONF_ZEN))
    {
        unsigned int value;
        volatile INT32 tailZ1, lengthZ1;
        volatile INT32 tailZ2, lengthZ2;
        volatile INT16 *fifoZ1, *fifoZ2;
        tailZ1   = adcHandler.fifoTailZ1;
        lengthZ1 = adcHandler.fifoLengthZ1;
        tailZ2   = adcHandler.fifoTailZ2;
        lengthZ2 = adcHandler.fifoLengthZ2;

        fifoZ1  = adcHandler.fifoZ1;
        fifoZ2  = adcHandler.fifoZ2;
        outpw(REG_ADC_ISR, ADC_ISR_ZF); //clear TF flags
        value = inpw(REG_ADC_ZDATA);
        if (adcHandler.touchz_callback != NULL)
            adcHandler.touchz_callback(value, adcHandler.touchz_userData);
        if ((lengthZ1 < ADC_FIFO_LENGTH) && (lengthZ2 < ADC_FIFO_LENGTH))
        {
            fifoZ1[tailZ1] = value & 0xFFF;
            lengthZ1++;
            tailZ1++;
            if (tailZ1 == ADC_FIFO_LENGTH) tailZ1 = 0;

            fifoZ2[tailZ2] = (value >> 16) & 0xFFF;
            lengthZ2++;
            tailZ2++;
            if (tailZ2 == ADC_FIFO_LENGTH) tailZ2 = 0;
        }
        /* Update FIFO status */
        adcHandler.fifoTailZ1 = tailZ1;
        adcHandler.fifoLengthZ1 = lengthZ1;
        adcHandler.fifoTailZ2 = tailZ2;
        adcHandler.fifoLengthZ2 = lengthZ2;
        DbgPrintf("z conversion complete\n");
    }

    if ((isr & ADC_ISR_PEUEF) && (ier & ADC_IER_PEUEIEN))
    {
        outpw(REG_ADC_ISR, ADC_ISR_PEUEF | ADC_ISR_PEDEF);
        DbgPrintf("menu pen up complete\n");
    }
    else if ((isr & ADC_ISR_PEDEF) && (isr & ADC_IER_PEDEIEN))
    {
        if (adcHandler.pendown_callback != NULL)
            adcHandler.pendown_callback(isr, adcHandler.pendown_userData);
        outpw(REG_ADC_ISR, ADC_ISR_PEUEF | ADC_ISR_PEDEF);
        DbgPrintf("pen down complete\n");
    }
}

/**
 * @brief       The ChangeChannel function of ADC device library
 *
 * @param       channel    Select ADC input for normal mode.Including:
 *                         - \ref AIN0
 *                         - \ref AIN1
 *                         - \ref AIN2
 *                         - \ref AIN3
 *                         - \ref AIN4
 *                         - \ref AIN5
 *                         - \ref AIN6
 *                         - \ref AIN7
 *
 * @retval      <0         Wrong argument
 * @retval      0          Success
 *
 * @details     This function is used to set channel for normal mode.
 */
INT adcChangeChannel(int channel)
{
    UINT32 reg;
    if ((channel >> ADC_CONF_CHSEL_Pos) < 0 || (channel >> ADC_CONF_CHSEL_Pos) > 8)
    {
        return ADC_ERR_ARGS;
    }
    reg = inpw(REG_ADC_CONF);
    reg = (reg & ~ADC_CONF_CHSEL_Msk) | channel;
    outpw(REG_ADC_CONF, reg);
    return Successful;
}

/*@}*/ /* end of group ADC_EXPORTED_FUNCTIONS */

/*@}*/ /* end of group ADC_Driver */

/*@}*/ /* end of group Device_Driver */

/*** (C) COPYRIGHT 2018 Nuvoton Technology Corp. ***/
