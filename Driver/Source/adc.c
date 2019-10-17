/**************************************************************************//**
* @file     adc.c
* @version  V1.00
* $Revision: 8 $
* $Date: 15/10/06 9:08a $
* @brief    NUC980 ADC driver source file
*
* @note
* Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include "nuc980.h"
#include "sys.h"
#include "adc.h"

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
    outpw(REG_CLK_PCLKEN1, inpw(REG_CLK_PCLKEN1) | (1<<24));

    /* Reset the ADC IP */
    outpw(REG_SYS_APBIPRST1, (1<<24));
    outpw(REG_SYS_APBIPRST1, 0);

    /* ADC Engine Clock is set to freq Khz */
    if(freqKhz>4000) freqKhz=4000;
    if(freqKhz<1000) freqKhz=1000;
    div=12000/freqKhz;
    outpw(REG_CLK_DIVCTL7, inpw(REG_CLK_DIVCTL7) & ~((0x3<<19)|(0x7<<16)|(0xFFul<<24)));
    outpw(REG_CLK_DIVCTL7, (0<<19)|(0<<16)|((div-1)<<24));

    /* Enable ADC Power */
    outpw(REG_ADC_CTL, ADC_CTL_ADEN);

    /* Enable ADC to high speed mode */
    outpw(REG_ADC_CONF, inpw(REG_ADC_CONF)|ADC_CONF_HSPEED);

    /* Set interrupt */
    sysInstallISR(IRQ_LEVEL_7, IRQ_ADC, (PVOID)adcISR);
    sysSetLocalInterrupt(ENABLE_IRQ);                            /* enable CPSR I bit */
    sysEnableInterrupt(IRQ_ADC);

    /* Init the FIFO buffer */
    adcHandler.voltage_battery_callback=NULL;
    adcHandler.voltage_battery_userData=NULL;

    adcHandler.normal_callback=NULL;
    adcHandler.normal_userData=NULL;

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
    outpw(REG_CLK_PCLKEN1, inpw(REG_CLK_PCLKEN1) & ~(1<<24));

    return Successful;
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
 * | :------------------- | :-------------------------------  | :-------------------------  |
 * |\ref START_MST        | NULL                              | NULL                        |
 * |\ref VBPOWER_ON       | NULL                              | NULL                        |
 * |\ref VBPOWER_OFF      | NULL                              | NULL                        |
 * |\ref NAC_ON           | Callback function                 | UserData                    |
 * |\ref NAC_OFF          | NULL                              | NULL                        |
 */
INT adcIoctl(ADC_CMD cmd, INT32 arg1, INT32 arg2)
{
    UINT32 reg;
    switch(cmd)
    {
    case START_MST:             //Menu Start Conversion
    {
        mst_complete=0;
        reg = inpw(REG_ADC_IER);
        reg = reg | ADC_IER_MIEN;
        outpw(REG_ADC_IER, reg);
        reg = inpw(REG_ADC_CTL);
        reg = reg | ADC_CTL_MST;
        outpw(REG_ADC_CTL, reg);
        while(!mst_complete);
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
        if (normal_callback != NULL )
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
        adcHandler.normal_callback = NULL;
        adcHandler.normal_userData = NULL;
    }
    break;
    default:
        return ADC_ERR_CMD;
    }
    return Successful;
}

/// @cond HIDDEN_SYMBOLS
#if 0
#define DbgPrintf sysprintf
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
    unsigned int isr,conf;
    conf=inpw(REG_ADC_CONF);
    isr=inpw(REG_ADC_ISR);
    //sysprintf("ADC_IRQHandler Interrupt(0x%08x): ",isr);
    DbgPrintf("REG_ADC_CTL=0x%08x\n",inpw(REG_ADC_CTL));
    DbgPrintf("REG_ADC_IER=0x%08x\n",inpw(REG_ADC_IER));
    DbgPrintf("REG_ADC_ISR=0x%08x\n",inpw(REG_ADC_ISR));

    if((isr & ADC_ISR_NACF) && (conf & ADC_CONF_NACEN))
    {
        outpw(REG_ADC_ISR,ADC_ISR_NACF);
        if(adcHandler.normal_callback!=NULL)
            adcHandler.normal_callback(inpw(REG_ADC_DATA), adcHandler.normal_userData);
        DbgPrintf("normal AD conversion complete\n");
    }

    if(isr & ADC_ISR_MF)
    {
        outpw(REG_ADC_ISR,ADC_ISR_MF);
        mst_complete=1;
        DbgPrintf("menu complete\n");
    }

}

/**
 * @brief       The ChangeChannel function of ADC device library
 *
 * @param       channel    Select ADC input channel. From 0 ~ 8. 
 *
 * @retval      <0         Wrong argument
 * @retval      0          Success
 *
 * @details     This function is used to set channel for normal mode.
 */
INT adcChangeChannel(int channel)
{
    UINT32 reg;
    if ((channel>>ADC_CONF_CHSEL_Pos) < 0 || (channel>>ADC_CONF_CHSEL_Pos) > 8)
    {
        return ADC_ERR_ARGS;
    }
    reg = inpw(REG_ADC_CONF);
    reg = (reg & ~ADC_CONF_CHSEL_Msk) | channel;
    outpw(REG_ADC_CONF,reg);
    return Successful;
}

/*@}*/ /* end of group ADC_EXPORTED_FUNCTIONS */

/*@}*/ /* end of group ADC_Driver */

/*@}*/ /* end of group Device_Driver */

/*** (C) COPYRIGHT 2018 Nuvoton Technology Corp. ***/
