/*************************************************************************//**
 * @file     main.c
 * @brief    Demonstrate how to read phone book information in the SIM card.
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include "nuc980.h"
#include "sys.h"
#include "sc.h"
#include "sclib.h"

/* The definition of commands used in this sample code and directory structures could
   be found in GSM 11.11 which is free for download from Internet. */

// Select File
const uint8_t au8SelectMF[] = {0xA0, 0xA4, 0x00, 0x00, 0x02, 0x3F, 0x00};
const uint8_t au8SelectDF_TELECOM[] = {0xA0, 0xA4, 0x00, 0x00, 0x02, 0x7F, 0x10};
const uint8_t au8SelectEF_ADN[] = {0xA0, 0xA4, 0x00, 0x00, 0x02, 0x6F, 0x3A};
//Get Response
uint8_t au8GetResp[] = {0xA0, 0xC0, 0x00, 0x00, 0x00};
//Read Record
uint8_t au8ReadRec[] = {0xA0, 0xB2, 0x01, 0x04, 0x00};
//Verify CHV, CHV = Card Holder Verification information
uint8_t au8VerifyChv[] = {0xA0, 0x20, 0x00, 0x01, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

uint8_t buf[300];
uint32_t len;

/**
  * @brief  The interrupt services routine of smartcard port 1
  * @param  None
  * @return None
  */
void SC1_IRQHandler(void)
{
    // Please don't remove any of the function calls below

    // No need to check CD event for SIM card.
    //if(SCLIB_CheckCDEvent(1))
    //    return; // Card insert/remove event occurred, no need to check other event...
    SCLIB_CheckTimeOutEvent(1);
    SCLIB_CheckTxRxEvent(1);
    SCLIB_CheckErrorEvent(1);

    return;
}

/**
  * @brief  Ask user to input PIN from console
  * @param  None
  * @return None
  * @details Valid input characters (0~9) are echo to console and store in command buffer.
  *         Backspace key can delete previous input digit, ESC key delete all input digits.
  *         Valid PIN length is between 4~8 digits. If PIN length is shorter than 8
  *         digits, an Enter key can terminate the input procedure.
  */
void get_pin(void)
{
    int i = 0;
    char c = 0;

    printf("Please input PIN number:");
    while(i < 8)
    {
        c = getchar();
        if(c >= 0x30 && c <= 0x39)      // Valid input characters (0~9)
        {
            au8VerifyChv[5 + i] = c;
            printf("%c", c);
            i++;
        }
        else if(c == 0x7F)    // DEL (Back space)
        {
            i--;
            printf("%c", c);
        }
        else if(c == 0x0D)     // Enter
        {
            if(i >= 4)  //Min CHV length is 4 digits
                break;
        }
        else if(c == 0x1B)    //ESC
        {
            printf("\nPlease input PIN number:");
            i = 0;  // retry
        }
        else
        {
            continue;
        }

    }

    // Fill remaining digits with 0xFF
    for(; i < 8; i++)
    {
        au8VerifyChv[5 + i] = 0xFF;
    }

    printf("\n");

    return;
}

/**
  * @brief  Send verify command to verify CHV1
  * @param  Remaining retry count, valid values are between 3~1
  * @return Unlock SIM card success or not
  * @retval 0 Unlock success
  * @retval -1 Unlock failed
  */
int unlock_sim(uint32_t u32RetryCnt)
{
    while(u32RetryCnt > 0)
    {

        get_pin(); // Ask user input PIN

        if(SCLIB_StartTransmission(1, au8VerifyChv, 13, buf, &len) != SCLIB_SUCCESS)
        {
            printf("Command Verify CHV failed\n");
            break;
        }
        if(buf[0] == 0x90 || buf[1] == 0x00)
        {
            printf("Pass\n");
            return 0;
        }
        else
        {
            u32RetryCnt--;
            printf("Failed, remaining retry count: %d\n", u32RetryCnt);
        }
    }

    printf("Oops, SIM card locked\n");

    return -1;
}

/**
  * @brief  Read phone book and print on console
  * @param  Phone book record number
  * @return None
  */
void read_phoneBook(uint32_t cnt)
{

    int i, j, k;

    for(i = 1; i < cnt + 1; i++)
    {
        au8ReadRec[2] = (uint8_t)i;
        if(SCLIB_StartTransmission(1, au8ReadRec, 5, buf, &len) != SCLIB_SUCCESS)
        {
            printf("Command Read Record failed\n");
            break;
        }
        if(buf[0] == 0xFF) // This is an empty entry
            continue;
        printf("\n======== %d ========", i);
        printf("\nName: ");
        for(j = 0; buf[j] != 0xFF; j++)
        {
            printf("%c", buf[j]);
        }
        while(buf[j] == 0xFF)   // Skip reset of the Alpha Identifier bytes
            j++;

        printf("\nNumber: ");
        j += 2; // Skip Length of BCD and TNO/NPI
        for(k = 0; k < 10; k++)
        {
            if((buf[j + k] & 0xf) != 0xF)
                printf("%c", (buf[j + k] & 0xf) + 0x30);
            else
                break;

            if((buf[j + k] >> 4) != 0xF)
                printf("%c", (buf[j + k] >> 4) + 0x30);
            else
                break;
        }
    }
    printf("\n");
    return;
}

void UART_Init()
{
    /* enable UART0 clock */
    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | 0x10000);

    /* GPF11, GPF12 */
    outpw(REG_SYS_GPF_MFPH, (inpw(REG_SYS_GPF_MFPH) & 0xfff00fff) | 0x11000); // UART0 multi-function

    /* UART0 line configuration for (115200,n,8,1) */
    outpw(REG_UART0_LCR, inpw(REG_UART0_LCR) | 0x07);
    outpw(REG_UART0_BAUD, 0x30000066); /* 12MHz reference clock input, 115200 */
}
#include "gpio.h"
int main(void)
{
    int retval;
    int retry = 0, cnt, chv1_disbled = 0;

    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    UART_Init();

    // enable smartcard 1 clock and multi function pin
    outpw(REG_CLK_PCLKEN1, inpw(REG_CLK_PCLKEN1) | 0x00002000);
    outpw(REG_CLK_DIVCTL6, inpw(REG_CLK_DIVCTL6) | 0x20000000);

    // Select port F for smartcard interface
    outpw(REG_SYS_GPF_MFPL, (inpw(REG_SYS_GPF_MFPL) & ~(0xFFFFF)) | 0x44444);
    printf("\nThis sample code reads phone book from SIM card\n");

    // Open smartcard interface 1. CD pin state ignore and PWR pin high raise VCC pin to card
    SC_Open(1, SC_PIN_STATE_IGNORE, SC_PIN_STATE_HIGH);
    sysInstallISR(IRQ_LEVEL_1, IRQ_SMC1, (PVOID)SC1_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_SMC1);

    // Ignore CD pin for SIM card
    //while(SC_IsCardInserted(1) == FALSE);
    // Activate slot 1
    retval = SCLIB_Activate(1, FALSE);

    if(retval != SCLIB_SUCCESS)
    {
        printf("SIM card activate failed\n");
        goto exit;
    }

    if(SCLIB_StartTransmission(1, (uint8_t *)au8SelectMF, 7, buf, &len) != SCLIB_SUCCESS)
    {
        printf("Command Select MF failed\n");
        goto exit;
    }


    if(len == 2 && buf[0] == 0x9F )    // response data length
    {
        au8GetResp[4] = buf[1];
        if(SCLIB_StartTransmission(1, au8GetResp, 5, buf, &len) != SCLIB_SUCCESS)
        {
            printf("Command Get response failed\n");
            goto exit;
        }
    }
    else
    {
        printf("Unknown response\n");
        goto exit;
    }

    if(buf[len - 2] != 0x90 || buf[len - 1] != 0x00)
    {
        printf("Cannot select MF\n");
        goto exit;
    }

    // Check if SIM is locked
    if(buf[18] & 0x80)
    {
        if((retry = (buf[18] & 0xF)) == 0)
        {
            printf("SIM locked, and unlock retry count exceed\n");
            goto exit;
        }
    }

    if(buf[13] & 0x80)
    {
        printf("CHV1 disabled\n");
        chv1_disbled = 1;
    }

    if(SCLIB_StartTransmission(1, (uint8_t *)au8SelectDF_TELECOM, 7, buf, &len) != SCLIB_SUCCESS)
    {
        printf("Command Select DF failed\n");
        goto exit;
    }

    if(SCLIB_StartTransmission(1, (uint8_t *)au8SelectEF_ADN, 7, buf, &len) != SCLIB_SUCCESS)
    {
        printf("Command Select EF failed\n");
        goto exit;
    }

    if(len == 2 && buf[0] == 0x9F )    // response data length
    {
        au8GetResp[4] = buf[1];
        if(SCLIB_StartTransmission(1, au8GetResp, 5, buf, &len) != SCLIB_SUCCESS)
        {
            printf("Command Get response failed\n");
            goto exit;
        }
    }
    else
    {
        printf("Unknown response\n");
        goto exit;
    }

    au8ReadRec[4] = buf[14]; // Phone book record length
    cnt = ((buf[2] << 8) + buf[3]) / buf[14];   // Phone book record number

    if(((buf[8] & 0x10) == 0x10) && (chv1_disbled == 0))    //Protect by CHV1 ?
    {
        if(unlock_sim(retry) < 0)
        {
            printf("Unlock SIM card failed\n");
            goto exit;
        }
    }

    read_phoneBook(cnt);
    printf("Done\n");
exit:
    while(1);
}


