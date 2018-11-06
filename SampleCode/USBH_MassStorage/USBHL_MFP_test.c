/******************************************************************************
 * @file     crypto_test.c
 * @version  V1.00
 * $Revision: 4 $
 * $Date: 13/07/26 6:17p $
 * @brief    Crypto test program for NUC970 series MCU
 *
 * @note
 * Copyright (C) 2014 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nuc980.h"
#include "sys.h"


typedef struct pin_cfg_t
{
    char         name[8];
    char         fpga_pin[12];
    uint32_t     reg;
    uint32_t     mask;
    uint32_t     mfp;
}  PIN_CFG_T;


PIN_CFG_T  _USBHL0_DP[] =
{
    { "PB.4 ", "CON31.73", REG_SYS_GPB_MFPL, 0x000F0000, 0x00040000 },
    { "PB.5 ", "CON31.75", REG_SYS_GPB_MFPL, 0x00F00000, 0x00400000 },
    { "PB.10", "CON35.7 ", REG_SYS_GPB_MFPH, 0x00000F00, 0x00000400 },
    { "PD.15", "CON35.28", REG_SYS_GPD_MFPH, 0xF0000000, 0x50000000 },
    { "N/A  ", "N/A     ", 0x00000000, 0x00000000, 0x00000000 },
};

PIN_CFG_T  _USBHL0_DM[] =
{
    { "PB.6 ", "CON31.98", REG_SYS_GPB_MFPL, 0x0F000000, 0x04000000 },
    { "PB.7 ", "CON31.99", REG_SYS_GPB_MFPL, 0xF0000000, 0x40000000 },
    { "PB.9 ", "CON35.6 ", REG_SYS_GPB_MFPH, 0x000000F0, 0x00000040 },
    { "PD.14", "CON35.26", REG_SYS_GPD_MFPH, 0x0F000000, 0x05000000 },
    { "N/A  ", "N/A     ", 0x00000000, 0x00000000, 0x00000000 },
};

PIN_CFG_T  _USBHL1_DP[] =
{
    { "PF.1 ", "CON37.47", REG_SYS_GPF_MFPL, 0x000000F0, 0x00000060 },
    { "PE.1 ", "CON35.68", REG_SYS_GPE_MFPL, 0x000000F0, 0x00000060 },
    { "N/A  ", "N/A     ", 0x00000000, 0x00000000, 0x00000000 },
};

PIN_CFG_T  _USBHL1_DM[] =
{
    { "PF.0 ", "CON37.48", REG_SYS_GPF_MFPL, 0x0000000F, 0x00000006 },
    { "PE.0 ", "CON35.67", REG_SYS_GPE_MFPL, 0x0000000F, 0x00000006 },
    { "N/A  ", "N/A     ", 0x00000000, 0x00000000, 0x00000000 },
};

PIN_CFG_T  _USBHL2_DP[] =
{
    { "PF.3 ", "CON37.45", REG_SYS_GPF_MFPL, 0x0000F000, 0x00006000 },
    { "PE.3 ", "CON35.70", REG_SYS_GPE_MFPL, 0x0000F000, 0x00006000 },
    { "N/A  ", "N/A     ", 0x00000000, 0x00000000, 0x00000000 },
};

PIN_CFG_T  _USBHL2_DM[] =
{
    { "PF.2 ", "CON37.46", REG_SYS_GPF_MFPL, 0x00000F00, 0x00000600 },
    { "PE.2 ", "CON35.69", REG_SYS_GPE_MFPL, 0x00000F00, 0x00000600 },
    { "N/A  ", "N/A     ", 0x00000000, 0x00000000, 0x00000000 },
};

PIN_CFG_T  _USBHL3_DP[] =
{
    { "PF.5 ", "CON37.43", REG_SYS_GPF_MFPL, 0x00F00000, 0x00600000 },
    { "PE.5 ", "CON35.72", REG_SYS_GPE_MFPL, 0x00F00000, 0x00600000 },
    { "N/A  ", "N/A     ", 0x00000000, 0x00000000, 0x00000000 },
};

PIN_CFG_T  _USBHL3_DM[] =
{
    { "PF.4 ", "CON37.44", REG_SYS_GPF_MFPL, 0x000F0000, 0x00060000 },
    { "PE.4 ", "CON35.71", REG_SYS_GPE_MFPL, 0x000F0000, 0x00060000 },
    { "N/A  ", "N/A     ", 0x00000000, 0x00000000, 0x00000000 },
};

PIN_CFG_T  _USBHL4_DP[] =
{
    { "PE.7 ", "CON35.74", REG_SYS_GPE_MFPL, 0xF0000000, 0x60000000 },
    { "PG.10", "CON31.68", REG_SYS_GPG_MFPH, 0x00000F00, 0x00000400 },
    { "PB.13", "CON35.10", REG_SYS_GPB_MFPH, 0x00F00000, 0x00600000 },
    { "PF.7 ", "CON35.29", REG_SYS_GPF_MFPL, 0xF0000000, 0x60000000 },
    { "N/A  ", "N/A     ", 0x00000000, 0x00000000, 0x00000000 },
};

PIN_CFG_T  _USBHL4_DM[] =
{
    { "PE.6 ", "CON37.73", REG_SYS_GPE_MFPL, 0x0F000000, 0x06000000 },
    { "PA.15", "CON31.67", REG_SYS_GPA_MFPH, 0xF0000000, 0x40000000 },
    { "PF.6 ", "CON37.42", REG_SYS_GPF_MFPL, 0x0F000000, 0x06000000 },
    { "N/A  ", "N/A     ", 0x00000000, 0x00000000, 0x00000000 },
};

PIN_CFG_T  _USBHL5_DP[] =
{
    { "PF.9 ", "CON35.31", REG_SYS_GPF_MFPH, 0x000000F0, 0x00000060 },
    { "PE.9 ", "CON35.76", REG_SYS_GPE_MFPH, 0x000000F0, 0x00000060 },
    { "PA.14", "CON31.66", REG_SYS_GPA_MFPH, 0x0F000000, 0x04000000 },
    { "PB.12", "CON35.9 ", REG_SYS_GPB_MFPH, 0x000F0000, 0x00040000 },
    { "N/A  ", "N/A     ", 0x00000000, 0x00000000, 0x00000000 },
};

PIN_CFG_T  _USBHL5_DM[] =
{
    { "PF.8 ", "CON35.30", REG_SYS_GPF_MFPH, 0x0000000F, 0x00000006 },
    { "PE.8 ", "CON35.75", REG_SYS_GPE_MFPH, 0x0000000F, 0x00000006 },
    { "PA.13", "CON31.65", REG_SYS_GPA_MFPH, 0x00F00000, 0x00400000 },
    { "PB.11", "CON35.8 ", REG_SYS_GPB_MFPH, 0x0000F000, 0x00004000 },
    { "N/A  ", "N/A     ", 0x00000000, 0x00000000, 0x00000000 },
};


char * get_current_mfp_name(PIN_CFG_T pf[])
{
    int   i;

    for (i = 0; pf[i].reg != 0; i++)
    {
        if ((inpw(pf[i].reg) & pf[i].mask) == pf[i].mfp)
            return pf[i].name;
    }
    return pf[i].name;;
}

void  clear_ubhl_mfp(PIN_CFG_T pf[])
{
    int    i;

    for (i = 0; pf[i].reg != 0; i++)
    {
        /* If the multi-function pin is for USBHL, clear it */
        if ((inpw(pf[i].reg) & pf[i].mask) == pf[i].mfp)
            outpw(pf[i].reg, (inpw(pf[i].reg) & ~(pf[i].mask)));
    }
}

void  select_usbhl_mfp(PIN_CFG_T pf[], int lite_no, int is_DP)
{
    int   ch, i;

    printf("+-----------------------------+\n");
    printf("|  USBHL%d D%c pin select       +\n", lite_no, (is_DP ? '+' : '-'));
    printf("+-----------------------------+\n");
    for (i = 0; pf[i].reg != 0; i++)
    {
        printf("|  [%d] %s (%s)       |\n", i+1, pf[i].name, pf[i].fpga_pin);
    }
    printf("+-----------------------------+\n");
    printf("Please select one of them: ");
    ch = getchar() - '0';

    for (i = 0; pf[i].reg != 0; i++)
    {
        if (i+1 == ch)
        {
            clear_ubhl_mfp(pf);
            outpw(pf[i].reg, ((inpw(pf[i].reg) & ~(pf[i].mask)) | pf[i].mfp));
            break;
        }
    }
    printf("\n");
}

void  USBHL_select_MFP()
{
    int   item;

    while (1)
    {
        printf("\n\n\n");
        printf("+--------------------------------------------------+\n");
        printf("|  Select NUC980 USBH Lite multi-function pins     +\n");
        printf("+--------------------------------------------------+\n");
        printf("| [1] USBHL0 D+ (%s)                            |\n", get_current_mfp_name(_USBHL0_DP));
        printf("| [2] USBHL0 D- (%s)                            |\n", get_current_mfp_name(_USBHL0_DM));
        printf("| [3] USBHL1 D+ (%s)                            |\n", get_current_mfp_name(_USBHL1_DP));
        printf("| [4] USBHL1 D- (%s)                            |\n", get_current_mfp_name(_USBHL1_DM));
        printf("| [5] USBHL2 D+ (%s)                            |\n", get_current_mfp_name(_USBHL2_DP));
        printf("| [6] USBHL2 D- (%s)                            |\n", get_current_mfp_name(_USBHL2_DM));
        printf("| [7] USBHL3 D+ (%s)                            |\n", get_current_mfp_name(_USBHL3_DP));
        printf("| [8] USBHL3 D- (%s)                            |\n", get_current_mfp_name(_USBHL3_DM));
        printf("| [9] USBHL4 D+ (%s)                            |\n", get_current_mfp_name(_USBHL4_DP));
        printf("| [a] USBHL4 D- (%s)                            |\n", get_current_mfp_name(_USBHL4_DM));
        printf("| [b] USBHL5 D+ (%s)                            |\n", get_current_mfp_name(_USBHL5_DP));
        printf("| [c] USBHL5 D- (%s)                            |\n", get_current_mfp_name(_USBHL5_DM));
        printf("| [Q] exit                                         |\n");
        printf("+--------------------------------------------------+\n");

        item = getchar();

        switch (item)
        {
        case '1':
            select_usbhl_mfp(_USBHL0_DP, 0, 1);
            break;

        case '2':
            select_usbhl_mfp(_USBHL0_DM, 0, 0);
            break;

        case '3':
            select_usbhl_mfp(_USBHL1_DP, 1, 1);
            break;

        case '4':
            select_usbhl_mfp(_USBHL1_DM, 1, 0);
            break;

        case '5':
            select_usbhl_mfp(_USBHL2_DP, 2, 1);
            break;

        case '6':
            select_usbhl_mfp(_USBHL2_DM, 2, 0);
            break;

        case '7':
            select_usbhl_mfp(_USBHL3_DP, 3, 1);
            break;

        case '8':
            select_usbhl_mfp(_USBHL3_DM, 3, 0);
            break;

        case '9':
            select_usbhl_mfp(_USBHL4_DP, 4, 1);
            break;

        case 'A':
        case 'a':
            select_usbhl_mfp(_USBHL4_DM, 4, 0);
            break;

        case 'B':
        case 'b':
            select_usbhl_mfp(_USBHL5_DP, 5, 1);
            break;

        case 'C':
        case 'c':
            select_usbhl_mfp(_USBHL5_DM, 5, 0);
            break;

        case 'Q':
        case 'q':
            return;
        }
    }
}



