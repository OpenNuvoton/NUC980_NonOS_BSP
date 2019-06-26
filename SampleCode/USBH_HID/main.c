/**************************************************************************//**
 * @file     main.c
 * @version  V1.00
 * $Revision: 1 $
 * $Date: 15/05/18 4:02p $
 * @brief    This sample shows how to manage several USB HID class devices.
 *
 * @note
 * Copyright (C) 2015 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <string.h>

#include "nuc980.h"
#include "sys.h"
#include "etimer.h"
#include "usbh_lib.h"
#include "usbh_hid.h"


uint32_t  g_buff_pool[1024] __attribute__((aligned(32)));


volatile uint32_t  _timer_tick;

void ETMR0_IRQHandler(void)
{
    _timer_tick ++;
    // clear timer interrupt flag
    ETIMER_ClearIntFlag(0);
}

uint32_t  get_ticks(void)
{
    return _timer_tick;
}

void Start_ETIMER0(void)
{
    // Enable ETIMER0 engine clock
    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | (1 << 8));

    // Set timer frequency to 100 HZ
    ETIMER_Open(0, ETIMER_PERIODIC_MODE, 100);

    // Enable timer interrupt
    ETIMER_EnableInt(0);
    sysInstallISR(IRQ_LEVEL_1, IRQ_TIMER0, (PVOID)ETMR0_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_TIMER0);

    _timer_tick = 0;

    // Start Timer 0
    ETIMER_Start(0);
}

void delay_us(int usec)
{
    volatile int  loop = 300 * usec;
    while (loop > 0) loop--;
}


void  dump_buff_hex(uint8_t *pucBuff, int nBytes)
{
    int     nIdx, i;

    nIdx = 0;
    while (nBytes > 0)
    {
        printf("0x%04X  ", nIdx);
        for (i = 0; (i < 16) && (nBytes > 0); i++)
        {
            printf("%02x ", pucBuff[nIdx + i]);
            nBytes--;
        }
        nIdx += 16;
        printf("\n");
    }
    printf("\n");
}


void  int_read_callback(HID_DEV_T *hdev, uint16_t ep_addr, int status, uint8_t *rdata, uint32_t data_len)
{
    /*
     *  USB host HID driver notify user the transfer status via <status> parameter. If the
     *  If <status> is 0, the USB transfer is fine. If <status> is not zero, this interrupt in
     *  transfer failed and HID driver will stop this pipe. It can be caused by USB transfer error
     *  or device disconnected.
     */
    if (status < 0)
    {
        printf("Interrupt in transfer failed! status: %d\n", status);
        return;
    }
    printf("Device [0x%x,0x%x] ep 0x%x, %d bytes received =>\n",
           hdev->idVendor, hdev->idProduct, ep_addr, data_len);
    dump_buff_hex(rdata, data_len);
}

#if 0
static uint8_t  _write_data_buff[4];

void  int_write_callback(HID_DEV_T *hdev, uint16_t ep_addr, uint8_t **wbuff, uint32_t *buff_size)
{
    printf("INT-out pipe request to write data.\n");

    *wbuff = &_write_data_buff[0];
    *buff_size = 4;
}
#endif

int  init_hid_device(HID_DEV_T *hdev)
{
    uint8_t   *data_buff;
    int       i, ret;

    data_buff = (uint8_t *)((uint32_t)g_buff_pool | 0x80000000);   // get non-cachable buffer address

    printf("\n\n==================================\n");
    printf("  Init HID device : 0x%x\n", (int)hdev);
    printf("  VID: 0x%x, PID: 0x%x\n\n", hdev->idVendor, hdev->idProduct);

    ret = usbh_hid_get_report_descriptor(hdev, data_buff, 1024);
    if (ret > 0)
    {
        printf("\nDump report descriptor =>\n");
        dump_buff_hex(data_buff, ret);
    }

    /*
     *  Example: GET_PROTOCOL request.
     */
    ret = usbh_hid_get_protocol(hdev, data_buff);
    printf("[GET_PROTOCOL] ret = %d, protocol = %d\n", ret, data_buff[0]);

    /*
     *  Example: SET_PROTOCOL request.
     */
    ret = usbh_hid_set_protocol(hdev, data_buff[0]);
    printf("[SET_PROTOCOL] ret = %d, protocol = %d\n", ret, data_buff[0]);

    /*
     *  Example: GET_REPORT request on report ID 0x1, report type FEATURE.
     */
    ret = usbh_hid_get_report(hdev, RT_FEATURE, 0x1, data_buff, 64);
    if (ret > 0)
    {
        printf("[GET_REPORT] Data => ");
        for (i = 0; i < ret; i++)
            printf("%02x ", data_buff[i]);
        printf("\n");
    }

    printf("\nUSBH_HidStartIntReadPipe...\n");
    ret = usbh_hid_start_int_read(hdev, 0, int_read_callback);
    if (ret != HID_RET_OK)
        printf("usbh_hid_start_int_read failed!\n");
    else
        printf("Interrupt in transfer started...\n");

    //if (usbh_hid_start_int_write(hdev, 0, int_write_callback) == HID_RET_OK)
    //{
    //  printf("Interrupt out transfer started...\n");
    //}

    return 0;
}

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

/*----------------------------------------------------------------------------
  MAIN function
 *----------------------------------------------------------------------------*/
int32_t main(void)
{
    HID_DEV_T    *hdev;

    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    UART_Init();

    SYS_UnlockReg();
    outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | (1<<18));      /* enable USB Host clock   */
    outpw(REG_SYS_MISCFCR, (inpw(REG_SYS_MISCFCR) | (1<<11)));  /* set USRHDSEN as 1; USB host/device role selection decided by USBID (SYS_PWRON[16]) */
    outpw(REG_SYS_PWRON, (inpw(REG_SYS_PWRON) | (1<<16)));      /* set USB port 0 used for Host */

    // set PE.12  for USBH_PWREN
    outpw(REG_SYS_GPE_MFPH, (inpw(REG_SYS_GPE_MFPH) & ~0x000f0000) | 0x00010000);

    // set PE.10  for USB_OVC
    outpw(REG_SYS_GPE_MFPH, (inpw(REG_SYS_GPE_MFPH) & ~0x00000f00) | 0x00000100);

    printf("\n\n");
    printf("+--------------------------------------------+\n");
    printf("|                                            |\n");
    printf("|     USB Host HID class sample program      |\n");
    printf("|                                            |\n");
    printf("+--------------------------------------------+\n");

    Start_ETIMER0();

    usbh_core_init();
    usbh_hid_init();

    while (1)
    {
        if (usbh_pooling_hubs())             /* USB Host port detect polling and management */
        {
            printf("\n Has hub events.\n");
            hdev = usbh_hid_get_device_list();
            if (hdev == NULL)
                continue;

            while (hdev != NULL)
            {
                init_hid_device(hdev);

                if (hdev != NULL)
                    hdev = hdev->next;
            }
        }
    }
}


/*** (C) COPYRIGHT 2015 Nuvoton Technology Corp. ***/
