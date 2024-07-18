/******************************************************************************
 * @file     SDGlue.c
 * @brief    SD glue functions for FATFS
 *
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nuc980.h"
#include "sys.h"
#include "sdh.h"
#include "ff.h"
#include "diskio.h"

extern int sd0_ok;
extern int sd1_ok;

FATFS  _FatfsVolSd0;
FATFS  _FatfsVolSd1;

static TCHAR  _Path[3] = { '0', ':', 0 };

void SDH_Open_Disk(SDH_T *sdh, uint32_t u32CardDetSrc)
{
    SDH_Open(sdh, u32CardDetSrc);
    if (SDH_Probe(sdh))
    {
        printf("SD initial fail!!\n");
        return;
    }

    _Path[1] = ':';
    _Path[2] = 0;
    if (sdh == SDH0)
    {
        _Path[0] = '0';
        f_mount(&_FatfsVolSd0, _Path, 1);
    }
    else
    {
        _Path[0] = '1';
        f_mount(&_FatfsVolSd1, _Path, 1);
    }

}

void SDH_Close_Disk(SDH_T *sdh)
{
    if (sdh == SDH0)
    {
        _Path[0]='0';
        memset(&SD0, 0, sizeof(SDH_INFO_T));
        f_mount(NULL, _Path, 1);
        memset(&_FatfsVolSd0, 0, sizeof(FATFS));
    }
    else
    {
        _Path[0]='1';
        memset(&SD1, 0, sizeof(SDH_INFO_T));
        f_mount(NULL, _Path, 1);
        memset(&_FatfsVolSd1, 0, sizeof(FATFS));
    }
}
