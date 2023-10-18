/**************************************************************************//**
* @file     adc.h
* @version  V1.00
* $Revision: 6 $
* $Date: 15/10/05 7:00p $
* @brief    NUC980 ADC driver header file
*
* @note
 * SPDX-License-Identifier: Apache-2.0
* Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#ifndef __ADC_H__
#define __ADC_H__

#ifdef __cplusplus
extern "C"
{
#endif


/** @addtogroup Standard_Driver Standard Driver
  @{
*/

/** @addtogroup ADC_Driver ADC Driver
  @{
*/

/** @addtogroup ADC_EXPORTED_CONSTANTS ADC Exported Constants
  @{
*/

#define ADC_ERR_ARGS            1   /*!< The arguments is wrong */
#define ADC_ERR_CMD             2   /*!< The command is wrong */

/// @cond HIDDEN_SYMBOLS
typedef INT32(*ADC_CALLBACK)(UINT32 status, UINT32 userData);
/// @endcond HIDDEN_SYMBOLS
/*---------------------------------------------------------------------------------------------------------*/
/* ADC_CTL constant definitions                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
#define ADC_CTL_ADEN            0x00000001  /*!< ADC Power Control */
#define ADC_CTL_VBGEN           0x00000002  /*!< ADC Internal Bandgap Power Control */
#define ADC_CTL_MST             0x00000100  /*!< Menu Start Conversion */
#define ADC_CTL_PEDEEN          0x00000200  /*!< Pen Down Event Enable */

/*---------------------------------------------------------------------------------------------------------*/
/* ADC_CONF constant definitions                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
#define ADC_CONF_TEN            0x00000001  /*!< Touch Enable */
#define ADC_CONF_ZEN            0x00000002  /*!< Press Enable */
#define ADC_CONF_NACEN          0x00000004  /*!< Normal AD Conversion Enable */
#define ADC_CONF_SELFTEN        0x00000400  /*!< Selft Test Enable */
#define ADC_CONF_HSPEED         (1<<22)     /*!< High Speed Enable */

#define ADC_CONF_CHSEL_Pos      12                           /*!< Channel Selection Position */
#define ADC_CONF_CHSEL_Msk      (0xF<<ADC_CONF_CHSEL_Pos)    /*!< Channel Selection Mask */
#define ADC_CONF_CHSEL_A0       (0<<ADC_CONF_CHSEL_Pos)      /*!< ADC input channel select A0 */
#define ADC_CONF_CHSEL_A1       (1<<ADC_CONF_CHSEL_Pos)      /*!< ADC input channel select A1 */
#define ADC_CONF_CHSEL_A2       (2<<ADC_CONF_CHSEL_Pos)      /*!< ADC input channel select A2 */
#define ADC_CONF_CHSEL_A3       (3<<ADC_CONF_CHSEL_Pos)      /*!< ADC input channel select A3 */
#define ADC_CONF_CHSEL_YM       (4<<ADC_CONF_CHSEL_Pos)      /*!< ADC input channel select YM */
#define ADC_CONF_CHSEL_YP       (5<<ADC_CONF_CHSEL_Pos)      /*!< ADC input channel select YP */
#define ADC_CONF_CHSEL_XM       (6<<ADC_CONF_CHSEL_Pos)      /*!< ADC input channel select XM */
#define ADC_CONF_CHSEL_XP       (7<<ADC_CONF_CHSEL_Pos)      /*!< ADC input channel select XP */

#define ADC_CONF_REFSEL_Pos      6                           /*!< Reference Selection Position */
#define ADC_CONF_REFSEL_Msk     (3<<ADC_CONF_REFSEL_Pos)     /*!< Reference Selection Mask */
#define ADC_CONF_REFSEL_VREF    (0<<ADC_CONF_REFSEL_Pos)     /*!< ADC reference select VREF input or 2.5v buffer output */
#define ADC_CONF_REFSEL_YMYP    (1<<ADC_CONF_REFSEL_Pos)     /*!< ADC reference select YM vs YP */
#define ADC_CONF_REFSEL_XMXP    (2<<ADC_CONF_REFSEL_Pos)     /*!< ADC reference select XM vs XP */
#define ADC_CONF_REFSEL_AVDD33  (3<<ADC_CONF_REFSEL_Pos)     /*!< ADC reference select AGND33 vs AVDD33 */

/*---------------------------------------------------------------------------------------------------------*/
/* ADC_IER constant definitions                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
#define ADC_IER_MIEN            0x00000001  /*!< Menu Interrupt Enable */
#define ADC_IER_PEDEIEN         0x00000004  /*!< Pen Down Even Interrupt Enable */
#define ADC_IER_PEUEIEN         0x00000040  /*!< Pen Up Event Interrupt Enable */

/*---------------------------------------------------------------------------------------------------------*/
/* ADC_ISR constant definitions                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
#define ADC_ISR_MF              0x00000001  /*!< Menu Complete Flag */
#define ADC_ISR_TF              0x00000100  /*!< Touch Conversion Finish */
#define ADC_ISR_ZF              0x00000200  /*!< Press Conversion Finish */
#define ADC_ISR_PEDEF           0x00000004  /*!< Pen Down Event Flag */
#define ADC_ISR_PEUEF           0x00000010  /*!< Pen Up Event Flag */
#define ADC_ISR_NACF            0x00000400  /*!< Normal AD Conversion Finish */

/** \brief  Structure type of ADC_CHAN
 */
typedef enum
{
    AIN0  = ADC_CONF_CHSEL_A0,     /*!< ADC input channel select \ref ADC_CONF_CHSEL_A0 */
    AIN1  = ADC_CONF_CHSEL_A1,     /*!< ADC input channel select \ref ADC_CONF_CHSEL_A1 */
    AIN2  = ADC_CONF_CHSEL_A2,     /*!< ADC input channel select \ref ADC_CONF_CHSEL_A2 */
    AIN3  = ADC_CONF_CHSEL_A3,     /*!< ADC input channel select \ref ADC_CONF_CHSEL_A3 */
    AIN4  = ADC_CONF_CHSEL_YM,     /*!< ADC input channel select \ref ADC_CONF_CHSEL_YM */
    AIN5  = ADC_CONF_CHSEL_XP,     /*!< ADC input channel select \ref ADC_CONF_CHSEL_XP */
    AIN6  = ADC_CONF_CHSEL_XM,     /*!< ADC input channel select \ref ADC_CONF_CHSEL_XM */
    AIN7  = ADC_CONF_CHSEL_XP      /*!< ADC input channel select \ref ADC_CONF_CHSEL_XP */
} ADC_CHAN;

/** \brief  Structure type of ADC_CMD
 */
typedef enum
{
    START_MST,                  /*!<Menu Start Conversion with interrupt */
    START_MST_POLLING,          /*!<Menu Start Conversion with polling */
    VBPOWER_ON,                 /*!<Enable ADC Internal Bandgap Power */
    VBPOWER_OFF,                /*!<Disable ADC Internal Bandgap Power */

    PEPOWER_ON,                 /*!<Enable Pen Down Power ,It can control pen down event */
    PEPOWER_OFF,                /*!<Disable Pen Power */
    PEDEF_ON,                   /*!<Enable Pen Down Event Flag */
    PEDEF_OFF,                  /*!<Disable Pen Down Event Flag */

    T_ON,                       /*!<Enable Touch detection function */
    T_OFF,                      /*!<Disable Touch detection function */
    Z_ON,                       /*!<Enable Press measure function */
    Z_OFF,                      /*!<Disable Press measure function */

    NAC_ON,                     /*!<Enable Normal AD Conversion */
    NAC_OFF,                    /*!<Disable Normal AD Conversion */
} ADC_CMD;

/*@}*/ /* end of group ADC_EXPORTED_CONSTANTS */

/** @addtogroup ADC_EXPORTED_FUNCTIONS ADC Exported Functions
  @{
*/

int adcOpen(void);
int adcOpen2(uint32_t freq);
int adcClose(void);
int adcReadXY(short *bufX, short *bufY, int dataCnt);
int adcReadZ(short *bufZ1, short *bufZ2, int dataCnt);
int adcIoctl(ADC_CMD cmd, int arg1, int arg2);
int adcChangeChannel(int channel);

/*@}*/ /* end of group ADC_EXPORTED_FUNCTIONS */

/*@}*/ /* end of group ADC_Driver */

/*@}*/ /* end of group Standard_Driver */

#ifdef __cplusplus
}
#endif

#endif //__ADC_H__
