/**************************************************************************//**
 * @file     etimer.h
 * @brief    ETIMER driver header file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __ETIMER_H__
#define __ETIMER_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "nuc980.h"

/** @addtogroup Standard_Driver Standard Driver
  @{
*/

/** @addtogroup ETIMER_Driver ETIMER Driver
  @{
*/

/** @addtogroup ETIMER_EXPORTED_CONSTANTS ETIMER Exported Constants
  @{
*/

#define ETIMER_ONESHOT_MODE                      (0UL)          /*!< Timer working in one shot mode   */
#define ETIMER_PERIODIC_MODE                     (1UL << 4)     /*!< Timer working in periodic mode   */
#define ETIMER_TOGGLE_MODE                       (2UL << 4)     /*!< Timer working in toggle mode     */
#define ETIMER_CONTINUOUS_MODE                   (3UL << 4)     /*!< Timer working in continuous mode */

#define ETIMER_CAPTURE_FREE_COUNTING_MODE        (0UL)          /*!< Free counting mode    */
#define ETIMER_CAPTURE_TRIGGER_COUNTING_MODE     (1UL << 20)    /*!< Trigger counting mode */
#define ETIMER_CAPTURE_COUNTER_RESET_MODE        (1UL << 17)    /*!< Counter reset mode    */

#define ETIMER_CAPTURE_FALLING_EDGE              (0UL)          /*!< Falling edge trigger timer capture */
#define ETIMER_CAPTURE_RISING_EDGE               (1UL << 18)    /*!< Rising edge trigger timer capture  */
#define ETIMER_CAPTURE_FALLING_THEN_RISING_EDGE  (2UL << 18)    /*!< Falling edge then rising edge trigger timer capture */
#define ETIMER_CAPTURE_RISING_THEN_FALLING_EDGE  (3UL << 18)    /*!< Rising edge then falling edge trigger timer capture */

#define TIMER_TIMEOUT_TRIGGER                    (0UL)          /*!< Timer timeout trigger other modules */
#define TIMER_CAPTURE_TRIGGER                    (1UL << 11)    /*!< Timer capture trigger other modules */

#define TIMER_COUNTER_RISING_EDGE                (1UL << 13)    /*!< Counter increase on rising edge  */
#define TIMER_COUNTER_FALLING_EDGE               (0UL)          /*!< Counter increase on falling edge */

/*@}*/ /* end of group ETIMER_EXPORTED_CONSTANTS */

/** @addtogroup ETIMER_EXPORTED_FUNCTIONS ETIMER Exported Functions
  @{
*/

/**
  * @brief This macro is used to set new Timer compared value
  * @param[in] timer ETIMER number. Range from 0 ~ 5
  * @param[in] u32Value  Timer compare value. Valid values are between 2 to 0xFFFFFF
  * @return None
  * \hideinitializer
  */
#define ETIMER_SET_CMP_VALUE(timer, u32Value) \
    do{\
        if((timer) == 0) {\
            outpw(REG_ETMR0_CMPR, u32Value);\
        } else if((timer) == 1) {\
            outpw(REG_ETMR1_CMPR, u32Value);\
        } else if((timer) == 2) {\
            outpw(REG_ETMR2_CMPR, u32Value);\
        } else if((timer) == 3) {\
            outpw(REG_ETMR3_CMPR, u32Value);\
        } else if((timer) == 4) {\
            outpw(REG_ETMR4_CMPR, u32Value);\
        } else {\
            outpw(REG_ETMR5_CMPR, u32Value);\
        }\
    }while(0)

/**
  * @brief This macro is used to set new Timer prescale value
  * @param[in] timer ETIMER number. Range from 0 ~ 5
  * @param[in] u32Value  Timer prescale value. Valid values are between 0 to 0xFF
  * @return None
  * @note Clock input is divided by (prescale + 1) before it is fed into timer
  * \hideinitializer
  */
#define ETIMER_SET_PRESCALE_VALUE(timer, u32Value) \
    do{\
        if((timer) == 0) {\
            outpw(REG_ETMR0_PRECNT, u32Value);\
        } else if((timer) == 1) {\
            outpw(REG_ETMR1_PRECNT, u32Value);\
        } else if((timer) == 2) {\
            outpw(REG_ETMR2_PRECNT, u32Value);\
        } else if((timer) == 3) {\
            outpw(REG_ETMR3_PRECNT, u32Value);\
        } else if((timer) == 4) {\
            outpw(REG_ETMR4_PRECNT, u32Value);\
        } else {\
            outpw(REG_ETMR5_PRECNT, u32Value);\
        }\
    }while(0)

/**
  * @brief This macro is used to check if specify Timer is inactive or active
  * @param[in] timer ETIMER number. Range from 0 ~ 5
  * @return timer is activate or inactivate
  * @retval 0 Timer 24-bit up counter is inactive
  * @retval 1 Timer 24-bit up counter is active
  * \hideinitializer
  */
static __inline int ETIMER_Is_Active(UINT timer)
{
    int reg;

    if(timer == 0)
    {
        reg = inpw(REG_ETMR0_CTL);
    }
    else if(timer == 1)
    {
        reg = inpw(REG_ETMR1_CTL);
    }
    else if(timer == 2)
    {
        reg = inpw(REG_ETMR2_CTL);
    }
    else if(timer == 3)
    {
        reg = inpw(REG_ETMR3_CTL);
    }
    else if(timer == 4)
    {
        reg = inpw(REG_ETMR4_CTL);
    }
    else
    {
        reg = inpw(REG_ETMR5_CTL);
    }
    return reg & 0x80 ? 1 : 0;
}

/**
  * @brief This function is used to start Timer counting
  * @param[in] timer ETIMER number. Range from 0 ~ 5
  * @return None
  */
static __inline void ETIMER_Start(UINT timer)
{

    if(timer == 0)
    {
        outpw(REG_ETMR0_CTL, inpw(REG_ETMR0_CTL) | 1);
    }
    else if(timer == 1)
    {
        outpw(REG_ETMR1_CTL, inpw(REG_ETMR1_CTL) | 1);
    }
    else if(timer == 2)
    {
        outpw(REG_ETMR2_CTL, inpw(REG_ETMR2_CTL) | 1);
    }
    else if(timer == 3)
    {
        outpw(REG_ETMR3_CTL, inpw(REG_ETMR3_CTL) | 1);
    }
    else if(timer == 4)
    {
        outpw(REG_ETMR4_CTL, inpw(REG_ETMR4_CTL) | 1);
    }
    else
    {
        outpw(REG_ETMR5_CTL, inpw(REG_ETMR5_CTL) | 1);
    }
}

/**
  * @brief This function is used to stop Timer counting
  * @param[in] timer ETIMER number. Range from 0 ~ 5
  * @return None
  */
static __inline void ETIMER_Stop(UINT timer)
{
    if(timer == 0)
    {
        outpw(REG_ETMR0_CTL, inpw(REG_ETMR0_CTL) & ~1);
    }
    else if(timer == 1)
    {
        outpw(REG_ETMR1_CTL, inpw(REG_ETMR1_CTL) & ~1);
    }
    else if(timer == 2)
    {
        outpw(REG_ETMR2_CTL, inpw(REG_ETMR2_CTL) & ~1);
    }
    else if(timer == 3)
    {
        outpw(REG_ETMR3_CTL, inpw(REG_ETMR3_CTL) & ~1);
    }
    else if(timer == 4)
    {
        outpw(REG_ETMR4_CTL, inpw(REG_ETMR4_CTL) & ~1);
    }
    else
    {
        outpw(REG_ETMR5_CTL, inpw(REG_ETMR5_CTL) & ~1);
    }
}

/**
  * @brief This function is used to enable the Timer wake-up function
  * @param[in] timer ETIMER number. Range from 0 ~ 5
  * @return None
  * @note  To wake the system from power down mode, timer clock source must be ether LXT or LIRC
  */
static __inline void ETIMER_EnableWakeup(UINT timer)
{
    if(timer == 0)
    {
        outpw(REG_ETMR0_CTL, inpw(REG_ETMR0_CTL) | 4);
    }
    else if(timer == 1)
    {
        outpw(REG_ETMR1_CTL, inpw(REG_ETMR1_CTL) | 4);
    }
    else if(timer == 2)
    {
        outpw(REG_ETMR2_CTL, inpw(REG_ETMR2_CTL) | 4);
    }
    else if(timer == 3)
    {
        outpw(REG_ETMR3_CTL, inpw(REG_ETMR3_CTL) | 4);
    }
    else if(timer == 4)
    {
        outpw(REG_ETMR4_CTL, inpw(REG_ETMR4_CTL) | 4);
    }
    else
    {
        outpw(REG_ETMR5_CTL, inpw(REG_ETMR5_CTL) | 4);
    }
}

/**
  * @brief This function is used to disable the Timer wake-up function
  * @param[in] timer ETIMER number. Range from 0 ~ 5
  * @return None
  */
static __inline void ETIMER_DisableWakeup(UINT timer)
{
    if(timer == 0)
    {
        outpw(REG_ETMR0_CTL, inpw(REG_ETMR0_CTL) & ~4);
    }
    else if(timer == 1)
    {
        outpw(REG_ETMR1_CTL, inpw(REG_ETMR1_CTL) & ~4);
    }
    else if(timer == 2)
    {
        outpw(REG_ETMR2_CTL, inpw(REG_ETMR2_CTL) & ~4);
    }
    else if(timer == 3)
    {
        outpw(REG_ETMR3_CTL, inpw(REG_ETMR3_CTL) & ~4);
    }
    else if(timer == 4)
    {
        outpw(REG_ETMR4_CTL, inpw(REG_ETMR4_CTL) & ~4);
    }
    else
    {
        outpw(REG_ETMR5_CTL, inpw(REG_ETMR5_CTL) & ~4);
    }
}


/**
  * @brief This function is used to enable the capture pin detection de-bounce function.
  * @param[in] timer ETIMER number. Range from 0 ~ 5
  * @return None
  */
static __inline void ETIMER_EnableCaptureDebounce(UINT timer)
{
    if(timer == 0)
    {
        outpw(REG_ETMR0_CTL, inpw(REG_ETMR0_CTL) | 0x400000);
    }
    else if(timer == 1)
    {
        outpw(REG_ETMR1_CTL, inpw(REG_ETMR1_CTL) | 0x400000);
    }
    else if(timer == 2)
    {
        outpw(REG_ETMR2_CTL, inpw(REG_ETMR2_CTL) | 0x400000);
    }
    else if(timer == 3)
    {
        outpw(REG_ETMR3_CTL, inpw(REG_ETMR3_CTL) | 0x400000);
    }
    else if(timer == 4)
    {
        outpw(REG_ETMR4_CTL, inpw(REG_ETMR4_CTL) | 0x400000);
    }
    else
    {
        outpw(REG_ETMR5_CTL, inpw(REG_ETMR5_CTL) | 0x400000);
    }
}

/**
  * @brief This function is used to disable the capture pin detection de-bounce function.
  * @param[in] timer ETIMER number. Range from 0 ~ 5
  * @return None
  */
static __inline void ETIMER_DisableCaptureDebounce(UINT timer)
{
    if(timer == 0)
    {
        outpw(REG_ETMR0_CTL, inpw(REG_ETMR0_CTL) & ~0x400000);
    }
    else if(timer == 1)
    {
        outpw(REG_ETMR1_CTL, inpw(REG_ETMR1_CTL) & ~0x400000);
    }
    else if(timer == 2)
    {
        outpw(REG_ETMR2_CTL, inpw(REG_ETMR2_CTL) & ~0x400000);
    }
    else if(timer == 3)
    {
        outpw(REG_ETMR3_CTL, inpw(REG_ETMR3_CTL) & ~0x400000);
    }
    else if(timer == 4)
    {
        outpw(REG_ETMR4_CTL, inpw(REG_ETMR4_CTL) & ~0x400000);
    }
    else
    {
        outpw(REG_ETMR5_CTL, inpw(REG_ETMR5_CTL) & ~0x400000);
    }
}


/**
  * @brief This function is used to enable the Timer time-out interrupt function.
  * @param[in] timer ETIMER number. Range from 0 ~ 5
  * @return None
  */
static __inline void ETIMER_EnableInt(UINT timer)
{
    if(timer == 0)
    {
        outpw(REG_ETMR0_IER, inpw(REG_ETMR0_IER) | 1);
    }
    else if(timer == 1)
    {
        outpw(REG_ETMR1_IER, inpw(REG_ETMR1_IER) | 1);
    }
    else if(timer == 2)
    {
        outpw(REG_ETMR2_IER, inpw(REG_ETMR2_IER) | 1);
    }
    else if(timer == 3)
    {
        outpw(REG_ETMR3_IER, inpw(REG_ETMR3_IER) | 1);
    }
    else if(timer == 4)
    {
        outpw(REG_ETMR4_IER, inpw(REG_ETMR4_IER) | 1);
    }
    else
    {
        outpw(REG_ETMR5_IER, inpw(REG_ETMR5_IER) | 1);
    }
}

/**
  * @brief This function is used to disable the Timer time-out interrupt function.
  * @param[in] timer ETIMER number. Range from 0 ~ 5
  * @return None
  */
static __inline void ETIMER_DisableInt(UINT timer)
{
    if(timer == 0)
    {
        outpw(REG_ETMR0_IER, inpw(REG_ETMR0_IER) & ~1);
    }
    else if(timer == 1)
    {
        outpw(REG_ETMR1_IER, inpw(REG_ETMR1_IER) & ~1);
    }
    else if(timer == 2)
    {
        outpw(REG_ETMR2_IER, inpw(REG_ETMR2_IER) & ~1);
    }
    else if(timer == 3)
    {
        outpw(REG_ETMR3_IER, inpw(REG_ETMR3_IER) & ~1);
    }
    else if(timer == 4)
    {
        outpw(REG_ETMR4_IER, inpw(REG_ETMR4_IER) & ~1);
    }
    else
    {
        outpw(REG_ETMR5_IER, inpw(REG_ETMR5_IER) & ~1);
    }
}

/**
  * @brief This function is used to enable the Timer capture trigger interrupt function.
  * @param[in] timer ETIMER number. Range from 0 ~ 5
  * @return None
  */
static __inline void ETIMER_EnableCaptureInt(UINT timer)
{
    if(timer == 0)
    {
        outpw(REG_ETMR0_IER, inpw(REG_ETMR0_IER) | 2);
    }
    else if(timer == 1)
    {
        outpw(REG_ETMR1_IER, inpw(REG_ETMR1_IER) | 2);
    }
    else if(timer == 2)
    {
        outpw(REG_ETMR2_IER, inpw(REG_ETMR2_IER) | 2);
    }
    else if(timer == 3)
    {
        outpw(REG_ETMR3_IER, inpw(REG_ETMR3_IER) | 2);
    }
    else if(timer == 4)
    {
        outpw(REG_ETMR4_IER, inpw(REG_ETMR4_IER) | 2);
    }
    else
    {
        outpw(REG_ETMR5_IER, inpw(REG_ETMR5_IER) | 2);
    }
}

/**
  * @brief This function is used to disable the Timer capture trigger interrupt function.
  * @param[in] timer ETIMER number. Range from 0 ~ 5
  * @return None
  */
static __inline void ETIMER_DisableCaptureInt(UINT timer)
{
    if(timer == 0)
    {
        outpw(REG_ETMR0_IER, inpw(REG_ETMR0_IER) & ~2);
    }
    else if(timer == 1)
    {
        outpw(REG_ETMR1_IER, inpw(REG_ETMR1_IER) & ~2);
    }
    else if(timer == 2)
    {
        outpw(REG_ETMR2_IER, inpw(REG_ETMR2_IER) & ~2);
    }
    else if(timer == 3)
    {
        outpw(REG_ETMR3_IER, inpw(REG_ETMR3_IER) & ~2);
    }
    else if(timer == 4)
    {
        outpw(REG_ETMR4_IER, inpw(REG_ETMR4_IER) & ~2);
    }
    else
    {
        outpw(REG_ETMR5_IER, inpw(REG_ETMR5_IER) & ~2);
    }
}

/**
  * @brief This function indicates Timer time-out interrupt occurred or not.
  * @param[in] timer ETIMER number. Range from 0 ~ 5
  * @return Timer time-out interrupt occurred or not
  * @retval 0 Timer time-out interrupt did not occur
  * @retval 1 Timer time-out interrupt occurred
  */
static __inline UINT ETIMER_GetIntFlag(UINT timer)
{
    int reg;

    if(timer == 0)
    {
        reg = inpw(REG_ETMR0_ISR);
    }
    else if(timer == 1)
    {
        reg = inpw(REG_ETMR1_ISR);
    }
    else if(timer == 2)
    {
        reg = inpw(REG_ETMR2_ISR);
    }
    else if(timer == 3)
    {
        reg = inpw(REG_ETMR3_ISR);
    }
    else if(timer == 4)
    {
        reg = inpw(REG_ETMR4_ISR);
    }
    else
    {
        reg = inpw(REG_ETMR5_ISR);
    }
    return reg & 1;
}

/**
  * @brief This function clears the Timer time-out interrupt flag.
  * @param[in] timer ETIMER number. Range from 0 ~ 5
  * @return None
  */
static __inline void ETIMER_ClearIntFlag(UINT timer)
{
    if(timer == 0)
    {
        outpw(REG_ETMR0_ISR, 1);
    }
    else if(timer == 1)
    {
        outpw(REG_ETMR1_ISR, 1);
    }
    else if(timer == 2)
    {
        outpw(REG_ETMR2_ISR, 1);
    }
    else if(timer == 3)
    {
        outpw(REG_ETMR3_ISR, 1);
    }
    else if(timer == 4)
    {
        outpw(REG_ETMR4_ISR, 1);
    }
    else
    {
        outpw(REG_ETMR5_ISR, 1);
    }
}

/**
  * @brief This function indicates Timer capture interrupt occurred or not.
  * @param[in] timer ETIMER number. Range from 0 ~ 5
  * @return Timer capture interrupt occurred or not
  * @retval 0 Timer capture interrupt did not occur
  * @retval 1 Timer capture interrupt occurred
  */
static __inline UINT ETIMER_GetCaptureIntFlag(UINT timer)
{
    int reg;

    if(timer == 0)
    {
        reg = inpw(REG_ETMR0_ISR);
    }
    else if(timer == 1)
    {
        reg = inpw(REG_ETMR1_ISR);
    }
    else if(timer == 2)
    {
        reg = inpw(REG_ETMR2_ISR);
    }
    else if(timer == 3)
    {
        reg = inpw(REG_ETMR3_ISR);
    }
    else if(timer == 4)
    {
        reg = inpw(REG_ETMR4_ISR);
    }
    else
    {
        reg = inpw(REG_ETMR5_ISR);
    }
    return (reg & 2) >> 1;
}

/**
  * @brief This function clears the Timer capture interrupt flag.
  * @param[in] timer ETIMER number. Range from 0 ~ 5
  * @return None
  */
static __inline void ETIMER_ClearCaptureIntFlag(UINT timer)
{
    if(timer == 0)
    {
        outpw(REG_ETMR0_ISR, 2);
    }
    else if(timer == 1)
    {
        outpw(REG_ETMR1_ISR, 2);
    }
    else if(timer == 2)
    {
        outpw(REG_ETMR2_ISR, 2);
    }
    else if(timer == 3)
    {
        outpw(REG_ETMR3_ISR, 2);
    }
    else if(timer == 4)
    {
        outpw(REG_ETMR4_ISR, 2);
    }
    else
    {
        outpw(REG_ETMR5_ISR, 2);
    }
}

/**
  * @brief This function indicates Timer has waked up system or not.
  * @param[in] timer ETIMER number. Range from 0 ~ 5
  * @return Timer has waked up system or not
  * @retval 0 Timer did not wake up system
  * @retval 1 Timer wake up system
  */
static __inline UINT ETIMER_GetWakeupFlag(UINT timer)
{
    int reg;

    if(timer == 0)
    {
        reg = inpw(REG_ETMR0_ISR);
    }
    else if(timer == 1)
    {
        reg = inpw(REG_ETMR1_ISR);
    }
    else if(timer == 2)
    {
        reg = inpw(REG_ETMR2_ISR);
    }
    else if(timer == 3)
    {
        reg = inpw(REG_ETMR3_ISR);
    }
    else if(timer == 4)
    {
        reg = inpw(REG_ETMR4_ISR);
    }
    else
    {
        reg = inpw(REG_ETMR5_ISR);
    }
    return (reg & 0x10) >> 4;
}

/**
  * @brief This function clears the Timer wakeup interrupt flag.
  * @param[in] timer ETIMER number. Range from 0 ~ 5
  * @return None
  */
static __inline void ETIMER_ClearWakeupFlag(UINT timer)
{
    if(timer == 0)
    {
        outpw(REG_ETMR0_ISR, 0x10);
    }
    else if(timer == 1)
    {
        outpw(REG_ETMR1_ISR, 0x10);
    }
    else if(timer == 2)
    {
        outpw(REG_ETMR2_ISR, 0x10);
    }
    else if(timer == 3)
    {
        outpw(REG_ETMR3_ISR, 0x10);
    }
    else if(timer == 4)
    {
        outpw(REG_ETMR4_ISR, 0x10);
    }
    else
    {
        outpw(REG_ETMR5_ISR, 0x10);
    }
}

/**
  * @brief This function gets the Timer capture data.
  * @param[in] timer ETIMER number. Range from 0 ~ 5
  * @return Timer capture data value
  */
static __inline UINT ETIMER_GetCaptureData(UINT timer)
{

    if(timer == 0)
    {
        return inpw(REG_ETMR0_TCAP);
    }
    else if(timer == 1)
    {
        return inpw(REG_ETMR1_TCAP);
    }
    else if(timer == 2)
    {
        return inpw(REG_ETMR2_TCAP);
    }
    else if(timer == 3)
    {
        return inpw(REG_ETMR3_TCAP);
    }
    else if(timer == 4)
    {
        return inpw(REG_ETMR4_TCAP);
    }
    else
    {
        return inpw(REG_ETMR5_TCAP);
    }
}

/**
  * @brief This function reports the current timer counter value.
  * @param[in] timer ETIMER number. Range from 0 ~ 5
  * @return Timer counter value
  */
static __inline UINT ETIMER_GetCounter(UINT timer)
{
    if(timer == 0)
    {
        return inpw(REG_ETMR0_DR);
    }
    else if(timer == 1)
    {
        return inpw(REG_ETMR1_DR);
    }
    else if(timer == 2)
    {
        return inpw(REG_ETMR2_DR);
    }
    else if(timer == 3)
    {
        return inpw(REG_ETMR3_DR);
    }
    else if(timer == 4)
    {
        return inpw(REG_ETMR4_DR);
    }
    else
    {
        return inpw(REG_ETMR5_DR);
    }
}

UINT ETIMER_Open(UINT timer, UINT u32Mode, UINT u32Freq);
void ETIMER_Close(UINT timer);
void ETIMER_Delay(UINT timer, UINT u32Usec);
void ETIMER_EnableCapture(UINT timer, UINT u32CapMode, UINT u32Edge);
void ETIMER_DisableCapture(UINT timer);
void ETIMER_EnableEventCounter(UINT timer, uint32_t u32Edge);
void ETIMER_DisableEventCounter(UINT timer);

/*@}*/ /* end of group ETIMER_EXPORTED_FUNCTIONS */

/*@}*/ /* end of group ETIMER_Driver */

/*@}*/ /* end of group Standard_Driver */

#ifdef __cplusplus
}
#endif

#endif //__ETIMER_H__
