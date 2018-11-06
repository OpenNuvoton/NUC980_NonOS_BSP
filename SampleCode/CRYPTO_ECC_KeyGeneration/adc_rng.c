/**************************************************************************//**
 * @file     adc_rng.c
 * @version  V1.10
 * $Revision: 10 $
 * $Date: 15/11/19 10:11a $
 * @brief    Use M480 ADC to generate true random numbers.
 *
 * @note
 * Copyright (C) 2017 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "nuc980.h"
#include "adc.h"

#define SNUM        32       /* recorded number of lastest samples */


static uint32_t   adc_val[SNUM];
static uint32_t   val_sum;
static int        oldest;


volatile int normal_complete=0;
static int  xseed = 0;


int NormalConvCallback(UINT32 status, UINT32 userData)
{
    normal_complete=1;
    return 0;
}

int get_adc_val()
{
    adcIoctl(NAC_ON, (UINT32)NormalConvCallback,0); //Enable Normal AD Conversion
    adcChangeChannel(1 << ADC_CONF_CHSEL_Pos);
    normal_complete = 0;
    adcIoctl(START_MST, 0, 0);
    while (normal_complete == 0);
    return inpw(REG_ADC_DATA) + xseed++;
}

int  adc_trng_gen_bit()
{
    uint32_t   new_val, average;
    int        ret_val;

    new_val = get_adc_val();

    average = (val_sum / SNUM);   /* sum divided by 32 */

    if (average >= new_val)
        ret_val = 1;
    else
        ret_val = 0;

    //printf("%d - sum = 0x%x, avg = 0x%x, new = 0x%x\n", oldest, val_sum, average, new_val);

    /* kick-off the oldest one and insert the new one */
    val_sum -= adc_val[oldest];
    val_sum += new_val;
    adc_val[oldest] = new_val;
    oldest = (oldest + 1) % SNUM;

    return ret_val;
}


uint32_t  adc_trng_gen_rnd()
{
    int       i;
    uint32_t  val32;

    val32 = 0;
    for (i = 31; i >= 0; i--)
        val32 |= (adc_trng_gen_bit() << i);

    return val32;
}

void adc_trng_gen_key(char *key, int key_len)
{
    int    bcnt = (key_len+3)/4;
    char   c;
    int    i, j;

    memset(key, 0, bcnt);

    for (i = bcnt-1; i >= 0; i--)        /* hex loop */
    {
        c = 0;
        for (j = 0; j < 4; j++)
        {
            c = (c << 1) | adc_trng_gen_bit();
        }

        if (c < 10)
            key[i] = c + '0';
        else
            key[i] = c + 'a' - 10;
    }
}


void  init_adc_init()
{
    int    i;

    adcOpen();

    val_sum = 0;
    for (i = 0; i < SNUM; i++)
    {
        adc_val[i] = get_adc_val();
        // printf("int adc val = 0x%x\n", adc_val[i]);
        val_sum += adc_val[i];
    }
    oldest = 0;

    adc_trng_gen_rnd();    // drop the first 32-bits
}





