/****************************************************************************
 * @file     main.c
 * @version  V1.00
 * @brief    Demonstrate RTC Spare Register read and write operations.
 *
 *
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 ******************************************************************************/
#include <stdio.h>
#include "nuc980.h"
#include "sys.h"
#include "uart.h"
#include "rtc.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/

#define RTC_TEST_DATA   0x55AA55AA

static void RTC_SpareRegister_Test(void)
{
    int32_t  ret;
    uint32_t write_val = RTC_TEST_DATA;
    uint32_t read_val  = 0;

	printf("RTC Spare Register R/W Test\n");

	/* Write Spare Register Test */
	ret = RTC_WriteSpareRegister(RTC_SPR0, write_val);
	if(ret != 1)
	{
		printf("[FAIL] RTC Write to SPR0 failed (Err: %d)\n", ret);
		while(1);
	}
	printf("[OK] Successfully wrote 0x%08X to SPR0\n", write_val);

	/* Read Spare Register Test */
	ret = RTC_ReadSpareRegister(RTC_SPR0, &read_val);
	if (ret != 1)
	{
		printf("[FAIL] RTC Read from SPR0 failed (Err: %d)\n", ret);
		while(1);
	}
	printf("[OK] Successfully read 0x%08X from SPR0\n", read_val);

	/* Data Verification */
	if (read_val == write_val)
	{
		printf("===> [SUCCESS] RTC Spare Register R/W Verification PASSED! \n\n");
		while(1);
	}
	else
	{
		printf("===> [ERROR] Data Mismatch! Expected: 0x%08X, Got: 0x%08X\n\n", write_val, read_val);
	}
}

void UART0_Init()
{
    /* enable UART0 clock */
    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | 0x10000);

    /* GPF11, GPF12 */
    outpw(REG_SYS_GPF_MFPH, (inpw(REG_SYS_GPF_MFPH) & 0xfff00fff) | 0x11000);   // UART0 multi-function

    /* UART0 line configuration for (115200,n,8,1) */
    outpw(REG_UART0_LCR, inpw(REG_UART0_LCR) | 0x07);
    outpw(REG_UART0_BAUD, 0x30000066); /* 12MHz reference clock input, 115200 */
}

int32_t main(void)
{
    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    UART0_Init();

    // Enable IP clock
    outpw(REG_CLK_PCLKEN0, (inpw(REG_CLK_PCLKEN0) | (1 << 2)));

	RTC_SpareRegister_Test();

    while(1);

}

/*** (C) COPYRIGHT 2026 Nuvoton Technology Corp. ***/

