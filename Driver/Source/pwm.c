/**************************************************************************//**
 * @file     pwm.c
 * @version  V1.00
 * $Revision: 5 $
 * $Date: 15/06/11 7:32p $
 * @brief    NUC980 series PWM driver source file
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include "nuc980.h"
#include "sys.h"
#include "pwm.h"
/** @addtogroup Standard_Driver Standard Driver
  @{
*/

/** @addtogroup PWM_Driver PWM Driver
  @{
*/


/** @addtogroup PWM_EXPORTED_FUNCTIONS PWM Exported Functions
  @{
*/

//Internal function definition
/// @cond HIDDEN_SYMBOLS

void pwmISR(PVOID pvParam);


static INT pwmInitGPIO(const INT nTimerIdentity, const INT nValue);
static INT pwmInitTimer(const INT nTimerIdentity);
static INT pwmStartTimer(const INT nTimerIdentity);
static INT pwmStopTimer(const INT nTimerIdentity, const INT nMethod);
// Register operation
static INT pwmSetCP(const INT nTimerIdentity, const INT nValue);
static INT pwmSetDZI(const INT nTimerIdentity, const INT nValue);
static INT pwmSetCSR(const INT nTimerIdentity, const INT nValue);
static INT pwmSetDZGenerator(const INT nTimerIdentity, const INT nStatus);
static INT pwmSetTimerState(const INT nTimerIdentity, const INT nStatus);
static INT pwmSetInverter(const INT nTimerIdentity, const INT nStatus);
static INT pwmSetMode(const INT nTimerIdentity, const INT nStatus);
static INT pwmSetCNR(const INT nTimerIdentity, const INT nValue);
static INT pwmSetCMR(const INT nTimerIdentity, const INT nValue);
static UINT pwmGetPDR(const INT nTimerIdentity);
static INT pwmSetPIER(const INT nTimerIdentity, const INT value);
static INT pwmCleanPIIR(const INT nTimerIdentity);

//Global variable
static BOOL bPWMIRQFlag=FALSE;  //IRQ enable flag, set after PWM IRQ enable
static BOOL bPWMTimerOpenStatus[PWM_TIMER_NUM]; //timer flag which set after open(for disable IRQ decision)
static BOOL bPWMTimerStartStatus[PWM_TIMER_NUM]; //timer flag which set after Start count(to avoid incorrectly stop procedure)
static BOOL bPWMTimerMode[PWM_TIMER_NUM]; //PWM timer toggle/one shot mode
static BOOL volatile bPWMIntFlag[PWM_TIMER_NUM]; //interrupt flag which set by ISR
/// @endcond /* HIDDEN_SYMBOLS */


/**
  * @brief The init function of PWM device driver
  */
INT pwmInit(void)
{
    UINT temp;
    // Enable PWM clock
    temp = inpw(REG_CLK_PCLKEN1);
    temp = temp | 0x4000000;
    outpw(REG_CLK_PCLKEN1, temp);

    sysInstallISR(IRQ_LEVEL_1, IRQ_PWM0, (PVOID)pwmISR);
    sysSetLocalInterrupt(ENABLE_IRQ);  // Enable CPSR I bit

    return 0;
}

/**
  * @brief The exit function of PWM device driver
  */
INT pwmExit(void)
{
    return 0;
}

/**
  * @brief The open function of PWM device driver
  * @param[in] nTimerIdentity PWM Timer channel identity
  * @retval Successful PWM successfully opened
  * @retval pwmTimerBusy PWM timer already open
  * @retval pwmInvalidTimerChannel PWM Timer channel number error
  */
INT pwmOpen(const INT nTimerIdentity)
{
    if(nTimerIdentity<PWM_TIMER_MIN || nTimerIdentity>PWM_TIMER_MAX)
    {
        return pwmInvalidTimerChannel;// nTimerIdentity value error
    }
    if(bPWMTimerOpenStatus[nTimerIdentity] == TRUE)
    {
        return pwmTimerBusy;
    }
    if(bPWMIRQFlag == FALSE)
    {

        sysEnableInterrupt(IRQ_PWM0);

        bPWMIRQFlag=TRUE;
    }
    bPWMTimerOpenStatus[nTimerIdentity]=TRUE;

    // Set PWM timer default value(CSR->PPR->PCR->CMR->CNR)
    pwmInitTimer(nTimerIdentity);

    //Enable PIER
    pwmSetPIER(nTimerIdentity, PWM_ENABLE);

    //Reset PIIR
    pwmCleanPIIR(nTimerIdentity);

    //Reset PWM timer start count flag
    bPWMTimerStartStatus[nTimerIdentity] = FALSE;

    return Successful;

}

/**
  * @brief The close function of PWM device driver
  * @param[in] nTimerIdentity PWM Timer channel identity
  * @retval Successful PWM successfully closed
  * @retval pwmTimerNotOpen PWM timer not open
  * @retval pwmInvalidTimerChannel PWM Timer channel number error
  */
INT pwmClose(const INT nTimerIdentity)
{
    INT nLoop;
    BOOL uAllTimerClose=TRUE;
    if( nTimerIdentity<PWM_TIMER_MIN || nTimerIdentity>PWM_TIMER_MAX )
    {
        return pwmInvalidTimerChannel;// nTimerIdentity value error
    }
    if(bPWMTimerOpenStatus[nTimerIdentity] == FALSE)
    {
        return pwmTimerNotOpen;
    }
    bPWMTimerOpenStatus[nTimerIdentity] = FALSE;
    //Check if all timer stop, IRQ can be disable
    for(nLoop=PWM_TIMER_MIN; nLoop<PWM_TIMER_NUM; nLoop++)
    {
        if(bPWMTimerOpenStatus[nLoop] == TRUE)
        {
            uAllTimerClose=FALSE;
        }
    }
    //All timer stop, disable IRQs
    if(uAllTimerClose == TRUE)
    {

        sysDisableInterrupt(IRQ_PWM0);
        bPWMIRQFlag=FALSE;
    }

    pwmSetPIER(nTimerIdentity, PWM_DISABLE);
    pwmCleanPIIR(nTimerIdentity);


    return Successful;

}

/**
  * @brief The read function of PWM device driver
  * @param[in] nTimerIdentity PWM Timer channel identity
  * @param[out] pucStatusValue The point of typePWMSTATUS
  * @param[in] uLength The length of typePWMSTATUS
  * @retval Successful Read PWM value successfully
  * @retval pwmTimerNotOpen PWM timer not open
  * @retval pwmInvalidTimerChannel PWM Timer channel number error
  * @retval pwmInvalidStructLength Struct length error(struct type error)
  */
INT pwmRead(const INT nTimerIdentity, PUCHAR pucStatusValue, const UINT uLength)
{
    if( nTimerIdentity<PWM_TIMER_MIN || nTimerIdentity>PWM_TIMER_MAX )
    {
        return pwmInvalidTimerChannel;// nTimerIdentity value error
    }
    if(bPWMTimerOpenStatus[nTimerIdentity] == FALSE)
    {
        return pwmTimerNotOpen;
    }
    if(uLength != sizeof(typePWMSTATUS))
    {
        return pwmInvalidStructLength;// Struct length error(struct type error)
    }
    if( sizeof(*((typePWMSTATUS *)pucStatusValue)) != sizeof(typePWMSTATUS) )
    {
        return pwmInvalidStructLength;// Struct length error(struct type error)
    }
    ((typePWMSTATUS *)pucStatusValue)->PDR=pwmGetPDR(nTimerIdentity);
    if(bPWMIntFlag[nTimerIdentity] == TRUE)
    {
        bPWMIntFlag[nTimerIdentity]=FALSE;
        ((typePWMSTATUS *)pucStatusValue)->InterruptFlag=TRUE;
    }
    else
    {
        ((typePWMSTATUS *)pucStatusValue)->InterruptFlag=FALSE;
    }

    return Successful;

}

/**
  * @brief The write function of PWM device driver
  * @param[in] nTimerIdentity PWM Timer channel identity
  * @param[in] pucCNRCMRValue The value of CNR and CMR
  * @param[in] uLength For future usage
  * @retval Successful Write PWM setting successfully
  * @retval pwmTimerNotOpen PWM timer not open
  * @retval pwmInvalidTimerChannel PWM Timer channel number error
  */
INT pwmWrite(const INT nTimerIdentity, PUCHAR pucCNRCMRValue, const UINT uLength)
{
    typePWMVALUE pwmvalue;
    INT nStatus;
    if( nTimerIdentity<PWM_TIMER_MIN || nTimerIdentity>PWM_TIMER_MAX )
    {
        return pwmInvalidTimerChannel;// nTimerIdentity value error
    }
    if(bPWMTimerOpenStatus[nTimerIdentity] == FALSE)
    {
        return pwmTimerNotOpen;
    }
    if(uLength != sizeof(typePWMVALUE))
    {
        return pwmInvalidStructLength;// Struct length error(struct type error)
    }
    pwmvalue.value=((typePWMVALUE *)pucCNRCMRValue)->value;
    nStatus=pwmSetCNR(nTimerIdentity, pwmvalue.field.cnr);

    if(nStatus != Successful)
    {
        return nStatus;
    }
    nStatus=pwmSetCMR(nTimerIdentity, pwmvalue.field.cmr);

    if(nStatus != Successful)
    {
        return nStatus;
    }
    return Successful;

}

/**
  * @brief The ioctl function of PWM device driver
  * @param[in] nTimerIdentity PWM Timer channel identity
  * @param[in] uCommand Ioctl command which indicates different operation
  * @param[in] uIndication Not use in PWM
  * @param[in] uValue The value which use with uCommand
  * @retval Successful PWM ioctl execute successfully
  * @retval pwmTimerNotOpen PWM timer not open
  * @retval pwmInvalidTimerChannel PWM Timer channel number error
  * @retval pwmInvalidIoctlCommand Ioctl command error
  * @retval Others Error according to different uCommand
  */
INT pwmIoctl(const INT nTimerIdentity, const UINT uCommand, const UINT uIndication, UINT uValue)
{
    INT nStatus;
    if(nTimerIdentity<PWM_TIMER_MIN || nTimerIdentity>PWM_TIMER_MAX)
    {
        return pwmInvalidTimerChannel;// nTimerIdentity value error
    }
    if(bPWMTimerOpenStatus[nTimerIdentity] == FALSE)
    {
        return pwmTimerNotOpen;
    }
    switch(uCommand)
    {
    case START_PWMTIMER:
    {
        nStatus=pwmStartTimer(nTimerIdentity);
        break;
    }
    case STOP_PWMTIMER:
    {
        // default stop method is 2
        nStatus=pwmStopTimer(nTimerIdentity, PWM_STOP_METHOD2);
        break;
    }
    case SET_CSR:
    {
        nStatus=pwmSetCSR(nTimerIdentity, uValue);
        break;
    }
    case SET_CP:
    {
        nStatus=pwmSetCP(nTimerIdentity, uValue);
        break;
    }
    case SET_DZI:
    {
        nStatus=pwmSetDZI(nTimerIdentity, uValue);
        break;
    }
    case SET_INVERTER:
    {
        nStatus=pwmSetInverter(nTimerIdentity, uValue);
        break;
    }
    case SET_MODE:
    {
        nStatus=pwmSetMode(nTimerIdentity, uValue);
        break;
    }
    case ENABLE_DZ_GENERATOR:
    {
        nStatus=pwmSetDZGenerator(nTimerIdentity, PWM_ENABLE);
        break;
    }
    case DISABLE_DZ_GENERATOR:
    {
        nStatus=pwmSetDZGenerator(nTimerIdentity, PWM_DISABLE);
        break;
    }
    case ENABLE_PWMGPIOOUTPUT:
    {
        nStatus=pwmInitGPIO(nTimerIdentity, uValue);
        break;
    }
    default:
    {
        return pwmInvalidIoctlCommand;
    }
    }
    return nStatus;
}


/// @cond HIDDEN_SYMBOLS

/**
  * @brief The interrupt service routines of PWM
  * @param[in] pvParam IRQ Parameter(not use in PWM)
  */
VOID pwmISR(PVOID pvParam)
{
    INT i;

    UINT32 uRegisterValue = 0;
    uRegisterValue = inpw(REG_PWM0_PIIR);// Get PIIR value
    for(i = 0; i < PWM_TIMER_NUM ; i++)
    {
        if(uRegisterValue & (1 << i))
        {
            bPWMIntFlag[i] = 1;
            outpw(REG_PWM0_PIIR, (1 << i));
        }
    }
}

/**
  * @brief This function set corresponding GPIO as PWM function according to the
  *        parameter nTimerIdentity
  * @param[in] nTimerIdentity Timer channel number
  * @retval Successful PWM init GPIO successfully
  * @retval pwmInvalidTimerChannel PWM Timer channel number error
  * @retval pwmInvalidPin PWM output pin setting error
  */
static INT pwmInitGPIO(const INT nTimerIdentity, const INT nValue)
{
    UINT temp=0;

    if(nTimerIdentity<PWM_TIMER_MIN || nTimerIdentity>PWM_TIMER_MAX)
    {
        return pwmInvalidTimerChannel;// Timer_num value error
    }

    if(nTimerIdentity == PWM0_TIMER0)
    {
        if(nValue == PWM00_GPF5)
        {
            temp = inpw(REG_SYS_GPF_MFPL);
            temp = (temp & ~0x00F00000) | 0x00400000;
            outpw(REG_SYS_GPF_MFPL, temp);
        }
        else if(nValue == PWM00_GPG0)
        {
            temp = inpw(REG_SYS_GPG_MFPL);
            temp = (temp & ~0xF) | 0x6;
            outpw(REG_SYS_GPG_MFPL, temp);
        }
        else if(nValue == PWM00_GPD12)
        {
            temp = inpw(REG_SYS_GPD_MFPH);
            temp = (temp & ~0x000F0000) | 0x60000;
            outpw(REG_SYS_GPD_MFPH, temp);
        }
        else if(nValue == PWM00_GPG10)
        {
            temp = inpw(REG_SYS_GPG_MFPH);
            temp = (temp & ~0x00000F00) | 0x700;
            outpw(REG_SYS_GPG_MFPH, temp);
        }
        else
            return pwmInvalidPin;
    }
    else if(nTimerIdentity == PWM0_TIMER1)
    {
        if(nValue == PWM01_GPF6)
        {
            temp = inpw(REG_SYS_GPF_MFPL);
            temp = (temp & ~0x0F000000) | 0x04000000;
            outpw(REG_SYS_GPF_MFPL, temp);
        }
        else if(nValue == PWM01_GPG1)
        {
            temp = inpw(REG_SYS_GPG_MFPL);
            temp = (temp & ~0xF0) | 0x60;
            outpw(REG_SYS_GPG_MFPL, temp);
        }
        else if(nValue == PWM01_GPD13)
        {
            temp = inpw(REG_SYS_GPD_MFPH);
            temp = (temp & ~0x00F00000) | 0x600000;
            outpw(REG_SYS_GPD_MFPH, temp);
        }
        else if(nValue == PWM01_GPA15)
        {
            temp = inpw(REG_SYS_GPA_MFPH);
            temp = (temp & ~0xF0000000) | 0x70000000;
            outpw(REG_SYS_GPA_MFPH, temp);
        }
        else
            return pwmInvalidPin;
    }
    else if(nTimerIdentity == PWM0_TIMER2)
    {
        if(nValue == PWM02_GPF7)
        {
            temp = inpw(REG_SYS_GPF_MFPL);
            temp = (temp & ~0xF0000000) | 0x40000000;
            outpw(REG_SYS_GPF_MFPL, temp);
        }
        else if(nValue == PWM02_GPG2)
        {
            temp = inpw(REG_SYS_GPG_MFPL);
            temp = (temp & ~0xF00) | 0x600;
            outpw(REG_SYS_GPG_MFPL, temp);
        }
        else if(nValue == PWM02_GPD14)
        {
            temp = inpw(REG_SYS_GPD_MFPH);
            temp = (temp & ~0x0F000000) | 0x6000000;
            outpw(REG_SYS_GPD_MFPH, temp);
        }
        else if(nValue == PWM02_GPA14)
        {
            temp = inpw(REG_SYS_GPA_MFPH);
            temp = (temp & ~0x0F000000) | 0x07000000;
            outpw(REG_SYS_GPA_MFPH, temp);
        }
        else if(nValue == PWM02_GPB13)
        {
            temp = inpw(REG_SYS_GPB_MFPH);
            temp = (temp & ~0x00F00000) | 0x0040000;
            outpw(REG_SYS_GPB_MFPH, temp);
        }
        else
            return pwmInvalidPin;
    }
    else if(nTimerIdentity == PWM0_TIMER3)
    {
        if(nValue == PWM03_GPF8)
        {
            temp = inpw(REG_SYS_GPF_MFPH);
            temp = (temp & ~0xF) | 0x4;
            outpw(REG_SYS_GPF_MFPH, temp);
        }
        else if(nValue == PWM03_GPG3)
        {
            temp = inpw(REG_SYS_GPG_MFPL);
            temp = (temp & ~0xF000) | 0x6000;
            outpw(REG_SYS_GPG_MFPL, temp);
        }
        else if(nValue == PWM03_GPD15)
        {
            temp = inpw(REG_SYS_GPD_MFPH);
            temp = (temp & ~0xF0000000) | 0x60000000;
            outpw(REG_SYS_GPD_MFPH, temp);
        }
        else if(nValue == PWM03_GPA13)
        {
            temp = inpw(REG_SYS_GPA_MFPH);
            temp = (temp & ~0x00F00000) | 0x00700000;
            outpw(REG_SYS_GPA_MFPH, temp);
        }
        else
            return pwmInvalidPin;
    }
    else if(nTimerIdentity == PWM1_TIMER0)
    {
        if(nValue == PWM10_GPB12)
        {
            temp = inpw(REG_SYS_GPB_MFPH);
            temp = (temp & ~0x000F0000) | 0x00020000;
            outpw(REG_SYS_GPB_MFPH, temp);
        }
        else if(nValue == PWM10_GPG6)
        {
            temp = inpw(REG_SYS_GPG_MFPL);
            temp = (temp & ~0x0F000000) | 0x06000000;
            outpw(REG_SYS_GPG_MFPL, temp);
        }
        else if(nValue == PWM10_GPG11)
        {
            temp = inpw(REG_SYS_GPG_MFPH);
            temp = (temp & ~0x0000F000) | 0x00006000;
            outpw(REG_SYS_GPG_MFPH, temp);
        }
        else if(nValue == PWM10_GPF9)
        {
            temp = inpw(REG_SYS_GPF_MFPH);
            temp = (temp & ~0x000000F0) | 0x000000040;
            outpw(REG_SYS_GPF_MFPH, temp);
        }
        else
            return pwmInvalidPin;
    }
    else if(nTimerIdentity == PWM1_TIMER1)
    {
        if(nValue == PWM11_GPB11)
        {
            temp = inpw(REG_SYS_GPB_MFPH);
            temp = (temp & ~0x0000F000) | 0x00002000;
            outpw(REG_SYS_GPB_MFPH, temp);
        }
        else if(nValue == PWM11_GPG7)
        {
            temp = inpw(REG_SYS_GPG_MFPL);
            temp = (temp & ~0xF0000000) | 0x60000000;
            outpw(REG_SYS_GPG_MFPL, temp);
        }
        else if(nValue == PWM11_GPG12)
        {
            temp = inpw(REG_SYS_GPG_MFPH);
            temp = (temp & ~0x000F0000) | 0x00060000;
            outpw(REG_SYS_GPG_MFPH, temp);
        }
        else if(nValue == PWM11_GPF10)
        {
            temp = inpw(REG_SYS_GPF_MFPH);
            temp = (temp & ~0x00000F00) | 0x000000400;
            outpw(REG_SYS_GPF_MFPH, temp);
        }
        else
            return pwmInvalidPin;
    }
    else if(nTimerIdentity == PWM1_TIMER2)
    {
        if(nValue == PWM12_GPB10)
        {
            temp = inpw(REG_SYS_GPB_MFPH);
            temp = (temp & ~0x00000F00) | 0x00000200;
            outpw(REG_SYS_GPB_MFPH, temp);
        }
        else if(nValue == PWM12_GPG8)
        {
            temp = inpw(REG_SYS_GPG_MFPH);
            temp = (temp & ~0xF) | 0x6;
            outpw(REG_SYS_GPG_MFPL, temp);
        }
        else if(nValue == PWM12_GPG13)
        {
            temp = inpw(REG_SYS_GPG_MFPH);
            temp = (temp & ~0x00F00000) | 0x00600000;
            outpw(REG_SYS_GPG_MFPH, temp);
        }
        else if(nValue == PWM12_GPE10)
        {
            temp = inpw(REG_SYS_GPE_MFPH);
            temp = (temp & ~0x00000F00) | 0x000000400;
            outpw(REG_SYS_GPE_MFPH, temp);
        }
        else
            return pwmInvalidPin;
    }
    else if(nTimerIdentity == PWM1_TIMER3)
    {
        if(nValue == PWM13_GPB9)
        {
            temp = inpw(REG_SYS_GPB_MFPH);
            temp = (temp & ~0x000000F0) | 0x00000020;
            outpw(REG_SYS_GPB_MFPH, temp);
        }
        else if(nValue == PWM13_GPG9)
        {
            temp = inpw(REG_SYS_GPG_MFPH);
            temp = (temp & ~0xF0) | 0x60;
            outpw(REG_SYS_GPG_MFPL, temp);
        }
        else if(nValue == PWM13_GPG14)
        {
            temp = inpw(REG_SYS_GPG_MFPH);
            temp = (temp & ~0x0F000000) | 0x06000000;
            outpw(REG_SYS_GPG_MFPH, temp);
        }
        else if(nValue == PWM13_GPE12)
        {
            temp = inpw(REG_SYS_GPE_MFPH);
            temp = (temp & ~0x000F0000) | 0x00040000;
            outpw(REG_SYS_GPE_MFPH, temp);
        }
        else
            return pwmInvalidPin;
    }

    return Successful;
}


/**
  * @brief This function initiates PWM timer n and set the default setting to CSR,
  *        PPR, PCR, CNR, CMR
  * @param[in] nTimerIdentity Timer channel number
  * @retval Successful PWM init timer successfully
  * @retval pwmInvalidTimerChannel PWM Timer channel number error
  */
static INT pwmInitTimer(const INT nTimerIdentity)
{
    typePPR PWMPPR;
    INT nStatus;
    if(nTimerIdentity<PWM_TIMER_MIN || nTimerIdentity>PWM_TIMER_MAX)
    {
        return pwmInvalidTimerChannel;// nTimerIdentity value error
    }

    //Set CSR
    nStatus=pwmSetCSR(nTimerIdentity, DEFAULT_CSR);

    if(nStatus != Successful)
    {
        return nStatus;
    }

    //Set PPR
    PWMPPR.value=(UINT)inpw(REG_PWM0_PPR);
    switch(nTimerIdentity)
    {
    case PWM0_TIMER0:
    {
        if(PWMPPR.field.cp0 == 0)
        {
            pwmSetCP(nTimerIdentity, DEFAULT_CP);
        }
        break;
    }
    case PWM0_TIMER1:
    {
        if(PWMPPR.field.cp0 == 0)
        {
            pwmSetCP(nTimerIdentity, DEFAULT_CP);
        }
        break;
    }
    case PWM0_TIMER2:
    {
        if(PWMPPR.field.cp1 == 0)
        {
            pwmSetCP(nTimerIdentity, DEFAULT_CP);
        }
        break;
    }
    case PWM0_TIMER3:
    {
        if(PWMPPR.field.cp1 == 0)
        {
            pwmSetCP(nTimerIdentity, DEFAULT_CP);
        }
        break;
    }
    case PWM1_TIMER0:
    {
        if(PWMPPR.field.cp0 == 0)
        {
            pwmSetCP(nTimerIdentity, DEFAULT_CP);
        }
        break;
    }
    case PWM1_TIMER1:
    {
        if(PWMPPR.field.cp0 == 0)
        {
            pwmSetCP(nTimerIdentity, DEFAULT_CP);
        }
        break;
    }
    case PWM1_TIMER2:
    {
        if(PWMPPR.field.cp1 == 0)
        {
            pwmSetCP(nTimerIdentity, DEFAULT_CP);
        }
        break;
    }
    case PWM1_TIMER3:
    {
        if(PWMPPR.field.cp1 == 0)
        {
            pwmSetCP(nTimerIdentity, DEFAULT_CP);
        }
        break;
    }
    }

    //Set PCR
    nStatus=pwmSetMode(nTimerIdentity, DEFAULT_MODE);

    if(nStatus != Successful)
    {
        return nStatus;
    }
    bPWMTimerMode[nTimerIdentity] = DEFAULT_MODE;

    //Set CMR
    nStatus=pwmSetCMR(nTimerIdentity, DEFAULT_CMR);

    if(nStatus != Successful)
    {
        return nStatus;
    }

    //Set CNR
    nStatus=pwmSetCNR(nTimerIdentity, DEFAULT_CNR);

    if(nStatus != Successful)
    {
        return nStatus;
    }

    return Successful;

}


/**
  * @brief This function starts PWM timer according to the parameter
  * @param[in] nTimerIdentity Timer channel number
  * @retval Successful PWM start timer successfully
  * @retval pwmInvalidTimerChannel PWM Timer channel number error
  */
static INT pwmStartTimer(const INT nTimerIdentity)
{
    if(nTimerIdentity<PWM_TIMER_MIN || nTimerIdentity>PWM_TIMER_MAX)
    {
        return pwmInvalidTimerChannel;// Timer_num value error
    }
    pwmSetTimerState(nTimerIdentity, PWM_ENABLE);
    if(bPWMTimerMode[nTimerIdentity] == PWM_TOGGLE)
    {
        bPWMTimerStartStatus[nTimerIdentity] = TRUE;
    }

    return Successful;
}

/**
  * @brief This function stops PWM timer n using method 1, 2, or 3 according to the
  *        parameter nTimerIdentity and nStatus
  * @param[in] nTimerIdentity Timer channel number
  * @param[in] nMethod Stop PWM timer method
  * @retval Successful PWM stop timer successfully
  * @retval pwmInvalidTimerChannel PWM Timer channel number error
  * @retval pwmInvalidStopMethod Stop method error
  */
static INT pwmStopTimer(const INT nTimerIdentity, INT nMethod)
{
    typeCNR PWMCNR;
    if(nTimerIdentity<PWM_TIMER_MIN || nTimerIdentity>PWM_TIMER_MAX)
    {
        // Timer_num value error
        return pwmInvalidTimerChannel;
    }
    //Can't stop before open PWM timer
    if(bPWMTimerOpenStatus[nTimerIdentity] == FALSE)
    {
        return Successful;
    }
    // one shot mode didn't need stop procedure
    if(bPWMTimerMode[nTimerIdentity] == PWM_ONESHOT)
    {
        return Successful;
    }
    // Timer stop already, no need to stop again
    if(bPWMTimerStartStatus[nTimerIdentity] == FALSE)
    {
        return Successful;
    }

    // Set CNR as 0
    PWMCNR.field.cnr=0;
    outpw(REG_PWM0_CNR0+(PWM_OFFSET*nTimerIdentity), PWMCNR.value);

    switch(nMethod)
    {
    case PWM_STOP_METHOD1:
    {
        while(1)
        {
            if(pwmGetPDR(nTimerIdentity) == 0)   // Wait PDR reach to 0
            {
                pwmSetTimerState(nTimerIdentity, PWM_DISABLE);// Disable pwm timer
                bPWMIntFlag[nTimerIdentity]=FALSE;
                bPWMTimerStartStatus[nTimerIdentity] = FALSE;
                break;
            }
        }
        break;
    }
    case PWM_STOP_METHOD2:
    {
        while(1)
        {
            if(bPWMIntFlag[nTimerIdentity] == TRUE)   // Wait interrupt happen
            {
                pwmSetTimerState(nTimerIdentity, PWM_DISABLE);// Disable pwm timer
                bPWMIntFlag[nTimerIdentity] = FALSE;
                bPWMTimerStartStatus[nTimerIdentity] = FALSE;
                break;
            }
        }
        break;
    }
    /*case PWM_STOP_METHOD3:
    {
        pwmSetPCRState(nTimerIdentity, PWM_DISABLE);// Disable pwm timer
        bPWMIntFlag[nTimerIdentity] = FALSE;
        bPWMTimerStartStatus[nTimerIdentity] = FALSE;
        break;
    }*/
    default:
    {
        return pwmInvalidStopMethod;// Stop method value error
    }
    }

    return Successful;
}

/**
  * @brief This function set CPn value according to the parameter nTimerIdentity and nValue
  * @param[in] nTimerIdentity Timer channel number
  * @param[in] nValue The value which want to set in CSRn
  * @retval Successful Set CPn successfully
  * @retval pwmInvalidTimerChannel PWM Timer channel number error
  * @retval pwmInvalidCPValue PWM_PPR CPn value out of range
  */
static INT pwmSetCP(const INT nTimerIdentity, const INT nValue)
{
    typePPR PWMPPR;
    if(nTimerIdentity<PWM_TIMER_MIN || nTimerIdentity>PWM_TIMER_MAX)
    {
        return pwmInvalidTimerChannel;// Timer_num value error
    }
    if(nValue<CP_MIN || nValue>CP_MAX)
    {
        return pwmInvalidCPValue;// CP value error
    }

    if (nTimerIdentity < PWM1_TIMER0)
    {
        PWMPPR.value=(UINT)inpw(REG_PWM0_PPR);
        switch(nTimerIdentity)
        {
        case PWM0_TIMER0:
        {
            PWMPPR.field.cp0=nValue;
            break;
        }
        case PWM0_TIMER1:
        {
            PWMPPR.field.cp0=nValue;
            break;
        }
        case PWM0_TIMER2:
        {
            PWMPPR.field.cp1=nValue;
            break;
        }
        case PWM0_TIMER3:
        {
            PWMPPR.field.cp1=nValue;
            break;
        }
        }
        outpw(REG_PWM0_PPR, PWMPPR.value);
    }
    else
    {
        PWMPPR.value=(UINT)inpw(REG_PWM1_PPR);
        switch(nTimerIdentity)
        {
        case PWM1_TIMER0:
        {
            PWMPPR.field.cp0=nValue;
            break;
        }
        case PWM1_TIMER1:
        {
            PWMPPR.field.cp0=nValue;
            break;
        }
        case PWM1_TIMER2:
        {
            PWMPPR.field.cp1=nValue;
            break;
        }
        case PWM1_TIMER3:
        {
            PWMPPR.field.cp1=nValue;
            break;
        }
        }
        outpw(REG_PWM1_PPR, PWMPPR.value);
    }

    return Successful;
}

/**
  * @brief This function set DZIn value according to the parameter nTimerIdentity and nValue
  * @param[in] nTimerIdentity Timer channel number
  * @param[in] nValue The value which want to set in DZIn
  * @retval Successful Set DZIn successfully
  * @retval pwmInvalidTimerChannel PWM Timer channel number error
  * @retval pwmInvalidDZIValue PWM_PPR DZIn value out of range
  */
static INT pwmSetDZI(const INT nTimerIdentity, const INT nValue)
{
    typePPR PWMPPR;
    if(nTimerIdentity<PWM_TIMER_MIN || nTimerIdentity>PWM_TIMER_MAX)
    {
        return pwmInvalidTimerChannel;// Timer_num value error
    }
    if(nValue<DZI_MIN || nValue>DZI_MAX)
    {
        return pwmInvalidDZIValue;// CSR value error
    }

    if (nTimerIdentity<PWM0_TIMER0)
    {
        PWMPPR.value=(UINT)inpw(REG_PWM0_PPR);
        switch(nTimerIdentity)
        {
        case PWM0_TIMER0:
        {
            PWMPPR.field.dzi0=nValue;
            break;
        }
        case PWM0_TIMER1:
        {
            PWMPPR.field.dzi0=nValue;
            break;
        }
        case PWM0_TIMER2:
        {
            PWMPPR.field.dzi1=nValue;
            break;
        }
        case PWM0_TIMER3:
        {
            PWMPPR.field.dzi1=nValue;
            break;
        }
        }
        outpw(REG_PWM0_PPR, PWMPPR.value);
    }
    else
    {
        PWMPPR.value=(UINT)inpw(REG_PWM1_PPR);
        switch(nTimerIdentity)
        {
        case PWM1_TIMER0:
        {
            PWMPPR.field.dzi0=nValue;
            break;
        }
        case PWM1_TIMER1:
        {
            PWMPPR.field.dzi0=nValue;
            break;
        }
        case PWM1_TIMER2:
        {
            PWMPPR.field.dzi1=nValue;
            break;
        }
        case PWM1_TIMER3:
        {
            PWMPPR.field.dzi1=nValue;
            break;
        }
        }
        outpw(REG_PWM1_PPR, PWMPPR.value);
    }

    return Successful;
}

/**
  * @brief This function set CSRn value according to the parameter nTimerIdentity and nValue
  * @param[in] nTimerIdentity Timer channel number
  * @param[in] nValue The value which want to set in CSRn
  * @retval Successful Set CSRn successfully
  * @retval pwmInvalidTimerChannel PWM Timer channel number error
  */
static INT pwmSetCSR(const INT nTimerIdentity, const INT nValue)
{
    typeCSR PWMCSR;
    if(nTimerIdentity<PWM_TIMER_MIN || nTimerIdentity>PWM_TIMER_MAX)
    {
        return pwmInvalidTimerChannel;// Timer_num value error
    }
    if(nValue<CSR_MIN || nValue>CSR_MAX)
    {
        return pwmInvalidCSRValue;// CSR value error
    }

    if(nTimerIdentity<PWM1_TIMER0)
    {
        PWMCSR.value=(UINT)inpw(REG_PWM0_CSR);
        switch(nTimerIdentity)
        {
        case PWM0_TIMER0:
        {
            PWMCSR.field.csr0=nValue;
            break;
        }
        case PWM0_TIMER1:
        {
            PWMCSR.field.csr1=nValue;
            break;
        }
        case PWM0_TIMER2:
        {
            PWMCSR.field.csr2=nValue;
            break;
        }
        case PWM0_TIMER3:
        {
            PWMCSR.field.csr3=nValue;
            break;
        }
        }
        outpw(REG_PWM0_CSR, PWMCSR.value);
    }
    else
    {
        PWMCSR.value=(UINT)inpw(REG_PWM1_CSR);
        switch(nTimerIdentity)
        {
        case PWM1_TIMER0:
        {
            PWMCSR.field.csr0=nValue;
            break;
        }
        case PWM1_TIMER1:
        {
            PWMCSR.field.csr1=nValue;
            break;
        }
        case PWM1_TIMER2:
        {
            PWMCSR.field.csr2=nValue;
            break;
        }
        case PWM1_TIMER3:
        {
            PWMCSR.field.csr3=nValue;
            break;
        }
        }
        outpw(REG_PWM1_CSR, PWMCSR.value);
    }
    return Successful;
}

/**
  * @brief This function enable/disable PWM channel n dead zone function according to the
  *        parameter nTimerIdentity and nStatus
  * @param[in] nTimerIdentity Timer channel number
  * @param[in] nStatus PWMDZG_ENABLE/PWMDZG_DISABLE
  * @retval Successful Set dead zone successfully
  * @retval pwmInvalidTimerChannel PWM Timer channel number error
  * @retval pwmInvalidDZGStatus PWM Dead-Zone Generator enable/disable status error
  */
static INT pwmSetDZGenerator(const INT nTimerIdentity, INT nStatus)
{
    typePCR PWMPCR;
    if(nTimerIdentity<PWM_TIMER_MIN || nTimerIdentity>PWM_TIMER_MAX)
    {
        return pwmInvalidTimerChannel;// Timer_num value error
    }
    if(nStatus != PWMDZG_ENABLE && nStatus != PWMDZG_DISABLE)
    {
        return pwmInvalidDZGStatus;// PCR inverter value error
    }
    if (nTimerIdentity<PWM1_TIMER0)
    {
        PWMPCR.value=(UINT)inpw(REG_PWM0_PCR);
        switch(nTimerIdentity)
        {
        case PWM0_TIMER0:
        {
            PWMPCR.field.grpup0_dzen=nStatus;
            break;
        }
        case PWM0_TIMER1:
        {
            PWMPCR.field.grpup0_dzen=nStatus;
            break;
        }
        case PWM0_TIMER2:
        {
            PWMPCR.field.grpup1_dzen=nStatus;
            break;
        }
        case PWM0_TIMER3:
        {
            PWMPCR.field.grpup1_dzen=nStatus;
            break;
        }
        }
        outpw(REG_PWM0_PCR, PWMPCR.value);
    }
    else
    {
        PWMPCR.value=(UINT)inpw(REG_PWM1_PCR);
        switch(nTimerIdentity)
        {
        case PWM1_TIMER0:
        {
            PWMPCR.field.grpup0_dzen=nStatus;
            break;
        }
        case PWM1_TIMER1:
        {
            PWMPCR.field.grpup0_dzen=nStatus;
            break;
        }
        case PWM1_TIMER2:
        {
            PWMPCR.field.grpup1_dzen=nStatus;
            break;
        }
        case PWM1_TIMER3:
        {
            PWMPCR.field.grpup1_dzen=nStatus;
            break;
        }
        }
        outpw(REG_PWM1_PCR, PWMPCR.value);
    }
    return Successful;
}

/**
  * @brief This function set PWM channel n enable/disable according to the
  *        parameter nTimerIdentity and nStatus
  * @param[in] nTimerIdentity Timer channel number
  * @param[in] nStatus PWM_ENABLE/PWMDISABLE
  * @retval Successful Set channel enable/disable successfully
  * @retval pwmInvalidTimerChannel PWM Timer channel number error
  */
static INT pwmSetTimerState(const INT nTimerIdentity, INT nStatus)
{
    typePCR PWMPCR;
    if(nTimerIdentity<PWM_TIMER_MIN || nTimerIdentity>PWM_TIMER_MAX)
    {
        return pwmInvalidTimerChannel;// Timer_num value error
    }
    if(nStatus != PWM_ENABLE && nStatus != PWM_DISABLE)
    {
        return pwmInvalidTimerStatus;
    }
    if (nTimerIdentity<PWM1_TIMER0)
    {
        PWMPCR.value=(UINT)inpw(REG_PWM0_PCR);
        switch(nTimerIdentity)
        {
        case PWM0_TIMER0:
        {
            PWMPCR.field.ch0_en=nStatus;
            break;
        }
        case PWM0_TIMER1:
        {
            PWMPCR.field.ch1_en=nStatus;
            break;
        }
        case PWM0_TIMER2:
        {
            PWMPCR.field.ch2_en=nStatus;
            break;
        }
        case PWM0_TIMER3:
        {
            PWMPCR.field.ch3_en=nStatus;
            break;
        }
        }
        outpw(REG_PWM0_PCR, PWMPCR.value);
    }
    else
    {
        PWMPCR.value=(UINT)inpw(REG_PWM1_PCR);
        switch(nTimerIdentity)
        {
        case PWM1_TIMER0:
        {
            PWMPCR.field.ch0_en=nStatus;
            break;
        }
        case PWM1_TIMER1:
        {
            PWMPCR.field.ch1_en=nStatus;
            break;
        }
        case PWM1_TIMER2:
        {
            PWMPCR.field.ch2_en=nStatus;
            break;
        }
        case PWM1_TIMER3:
        {
            PWMPCR.field.ch3_en=nStatus;
            break;
        }
        }
        outpw(REG_PWM1_PCR, PWMPCR.value);
    }

    return Successful;
}


/**
  * @brief This function set PWM channel n inverter on/off according to the
  *        parameter nTimerIdentity and nStatus
  * @param[in] nTimerIdentity Timer channel number
  * @param[in] nStatus PWM_ENABLE/PWM_DISABLE
  * @retval Successful Set inverter successfully
  * @retval pwmInvalidTimerChannel PWM Timer channel number error
  * @retval pwmInvalidInverterValue Inverter value error
  */
static INT pwmSetInverter(const INT nTimerIdentity, INT nStatus)
{
    typePCR PWMPCR;
    if(nTimerIdentity<PWM_TIMER_MIN || nTimerIdentity>PWM_TIMER_MAX)
    {
        return pwmInvalidTimerChannel;// Timer_num value error
    }
    if(nStatus != PWM_INVON && nStatus != PWM_INVOFF)
    {
        return pwmInvalidInverterValue;// PCR inverter value error
    }
    if (nTimerIdentity<PWM1_TIMER0)
    {
        PWMPCR.value=(UINT)inpw(REG_PWM0_PCR);
        switch(nTimerIdentity)
        {
        case PWM0_TIMER0:
        {
            PWMPCR.field.ch0_inverter=nStatus;
            break;
        }
        case PWM0_TIMER1:
        {
            PWMPCR.field.ch1_inverter=nStatus;
            break;
        }
        case PWM0_TIMER2:
        {
            PWMPCR.field.ch2_inverter=nStatus;
            break;
        }
        case PWM0_TIMER3:
        {
            PWMPCR.field.ch3_inverter=nStatus;
            break;
        }
        }
        outpw(REG_PWM0_PCR, PWMPCR.value);
    }
    else
    {
        PWMPCR.value=(UINT)inpw(REG_PWM1_PCR);
        switch(nTimerIdentity)
        {
        case PWM1_TIMER0:
        {
            PWMPCR.field.ch0_inverter=nStatus;
            break;
        }
        case PWM1_TIMER1:
        {
            PWMPCR.field.ch1_inverter=nStatus;
            break;
        }
        case PWM1_TIMER2:
        {
            PWMPCR.field.ch2_inverter=nStatus;
            break;
        }
        case PWM1_TIMER3:
        {
            PWMPCR.field.ch3_inverter=nStatus;
            break;
        }
        }
        outpw(REG_PWM1_PCR, PWMPCR.value);
    }
    return Successful;
}

/**
  * @brief This function set PWM channel n toggle/one shot mode according to the
  *        parameter nTimerIdentity and nStatus
  * @param[in] nTimerIdentity Timer channel number
  * @param[in] nStatus PWM_TOGGLE/PWM_ONESHOT
  * @retval Successful Set operation mode successfully
  * @retval pwmInvalidTimerChannel PWM Timer channel number error
  * @retval pwmInvalidModeStatus Operating mode error
  */
static INT pwmSetMode(const INT nTimerIdentity, INT nStatus)
{
    typePCR PWMPCR;
    if(nTimerIdentity<PWM_TIMER_MIN || nTimerIdentity>PWM_TIMER_MAX)
    {
        return pwmInvalidTimerChannel;// Timer_num value error
    }
    if(nStatus != PWM_TOGGLE && nStatus != PWM_ONESHOT)
    {
        return pwmInvalidModeStatus;// PCR inverter value error
    }
    if (nTimerIdentity<PWM1_TIMER0)
    {
        PWMPCR.value=(UINT)inpw(REG_PWM0_PCR);
        switch(nTimerIdentity)
        {
        case PWM0_TIMER0:
        {
            PWMPCR.field.ch0_mode=nStatus;
            break;
        }
        case PWM0_TIMER1:
        {
            PWMPCR.field.ch1_mode=nStatus;
            break;
        }
        case PWM0_TIMER2:
        {
            PWMPCR.field.ch2_mode=nStatus;
            break;
        }
        case PWM0_TIMER3:
        {
            PWMPCR.field.ch3_mode=nStatus;
            break;
        }
        }
        outpw(REG_PWM0_PCR, PWMPCR.value);
    }
    else
    {
        PWMPCR.value=(UINT)inpw(REG_PWM1_PCR);
        switch(nTimerIdentity)
        {
        case PWM1_TIMER0:
        {
            PWMPCR.field.ch0_mode=nStatus;
            break;
        }
        case PWM1_TIMER1:
        {
            PWMPCR.field.ch1_mode=nStatus;
            break;
        }
        case PWM1_TIMER2:
        {
            PWMPCR.field.ch2_mode=nStatus;
            break;
        }
        case PWM1_TIMER3:
        {
            PWMPCR.field.ch3_mode=nStatus;
            break;
        }
        }
        outpw(REG_PWM1_PCR, PWMPCR.value);
    }
    bPWMTimerMode[nTimerIdentity] = nStatus;

    return Successful;
}


/**
  * @brief This function set PWM_CNRn value according to the parameter nTimerIdentity and nValue
  * @param[in] nTimerIdentity Timer channel number
  * @param[in] nValue CNR value
  * @retval Successful Set CNR successfully
  * @retval pwmInvalidTimerChannel PWM Timer channel number error
  * @retval pwmInvalidCNRValue Invalid CNR value
  */
static INT pwmSetCNR(const INT nTimerIdentity, INT nValue)
{
    typeCNR PWMCNR;
    if(nTimerIdentity<PWM_TIMER_MIN || nTimerIdentity>PWM_TIMER_MAX)
    {
        return pwmInvalidTimerChannel;// Timer_num value error
    }
    if(nValue < CNR_MIN || nValue > CNR_MAX)
    {
        return pwmInvalidCNRValue;// PCR inverter value error
    }
    PWMCNR.field.cnr=nValue;
    if (nTimerIdentity<PWM1_TIMER0)
        outpw(REG_PWM0_CNR0+(PWM_OFFSET*nTimerIdentity), PWMCNR.value);
    else
        outpw(REG_PWM0_CNR1+(PWM_OFFSET*(nTimerIdentity - PWM1_TIMER0)), PWMCNR.value);
    return Successful;
}

/**
  * @brief This function set PWM_CMRn value according to the parameter nTimerIdentity and nValue
  * @param[in] nTimerIdentity Timer channel number
  * @param[in] nValue CMR value
  * @retval Successful Set CMR successfully
  * @retval pwmInvalidTimerChannel PWM Timer channel number error
  * @retval pwmInvalidCMRValue Invalid CMR value
  */
static INT pwmSetCMR(const INT nTimerIdentity, INT nValue)
{
    typeCMR PWMCMR;
    if(nTimerIdentity<PWM_TIMER_MIN || nTimerIdentity>PWM_TIMER_MAX)
    {
        return pwmInvalidTimerChannel;// Timer_num value error
    }
    if(nValue < CMR_MIN || nValue > CMR_MAX)
    {
        return pwmInvalidCMRValue;// CMR value error
    }
    PWMCMR.field.cmr=nValue;
    if (nTimerIdentity<PWM1_TIMER0)
        outpw(REG_PWM0_CMR0+(PWM_OFFSET*nTimerIdentity), PWMCMR.value);
    else
        outpw(REG_PWM0_CMR1+(PWM_OFFSET*(nTimerIdentity - PWM1_TIMER0)), PWMCMR.value);
    return Successful;
}

/**
  * @brief This function return the PDR value of PWM timer n
  * @param[in] nTimerIdentity Timer channel number
  * @retval pwmInvalidTimerChannel PWM Timer channel number error
  * @retval Others Current PDR value
  */
static UINT pwmGetPDR(const INT nTimerIdentity)
{
    if(nTimerIdentity<PWM_TIMER_MIN || nTimerIdentity>PWM_TIMER_MAX)
    {
        return pwmInvalidTimerChannel;// Timer_num value error
    }
    else
    {
        if (nTimerIdentity<PWM1_TIMER0)
            return (UINT)inpw(REG_PWM0_PDR0+(PWM_OFFSET*nTimerIdentity));// Return PDR value
        else
            return (UINT)inpw(REG_PWM1_PDR0+(PWM_OFFSET*nTimerIdentity));// Return PDR value
    }
}

/**
  * @brief This function set the PIERn bit of PWM timer n as 1 or 0 according to the
  *        parameter nTimerIdentity and nValue
  * @param[in] nTimerIdentity Timer channel number
  * @param[in] nValue PWM_ENABLE/PWM_DISABLE
  * @retval Successful Set PIER successfully
  * @retval pwmInvalidTimerChannel PWM Timer channel number error
  */
static INT pwmSetPIER(const INT nTimerIdentity, INT nValue)
{
    UINT uRegisterValue=0;;
    if(nTimerIdentity<PWM_TIMER_MIN || nTimerIdentity>PWM_TIMER_MAX)
    {
        return pwmInvalidTimerChannel;// Timer_num value error
    }
    else
    {
        uRegisterValue=(UINT)inpw(REG_PWM0_PIER);
        if(nValue==PWM_ENABLE)
        {
            uRegisterValue=uRegisterValue|(1<<nTimerIdentity);// Set PIER
        }
        else
        {
            uRegisterValue=uRegisterValue&(0<<nTimerIdentity);// Clear PIER
        }
        if (nTimerIdentity<PWM1_TIMER0)
            outpw(REG_PWM0_PIER, uRegisterValue);// Write value to PIER
        else
            outpw(REG_PWM1_PIER, uRegisterValue);// Write value to PIER
        return Successful;
    }
}


/**
  * @brief This function clear PIIRn bit according to the parameter nTimerIdentity
  * @param[in] nTimerIdentity Timer channel number
  * @retval Successful Clear PIIR successfully
  * @retval pwmInvalidTimerChannel PWM Timer channel number error
  */
static INT pwmCleanPIIR(const INT nTimerIdentity)
{
    UINT uRegisterValue=0;
    if(nTimerIdentity<PWM_TIMER_MIN || nTimerIdentity>PWM_TIMER_MAX)
    {
        return pwmInvalidTimerChannel;// nTimerIdentity value error
    }
    if (nTimerIdentity<PWM1_TIMER0)
    {
        uRegisterValue=(UINT)inpw(REG_PWM0_PIIR);
        uRegisterValue=uRegisterValue&~(1<<nTimerIdentity);
        outpw(REG_PWM0_PIIR,uRegisterValue);
    }
    else
    {
        uRegisterValue=(UINT)inpw(REG_PWM1_PIIR);
        uRegisterValue=uRegisterValue&~(1<<nTimerIdentity);
        outpw(REG_PWM1_PIIR,uRegisterValue);
    }

    return Successful;
}

/// @endcond /* HIDDEN_SYMBOLS */

/*@}*/ /* end of group PWM_EXPORTED_FUNCTIONS */

/*@}*/ /* end of group PWM_Driver */

/*@}*/ /* end of group Standard_Driver */

/*** (C) COPYRIGHT 2018 Nuvoton Technology Corp. ***/
