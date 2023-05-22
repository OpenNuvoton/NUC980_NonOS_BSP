/**************************************************************************//**
* @file     main.c
* @brief    NUC980 EBI_SRAM Sample Code. This sample requires an external
*           SRAM connected to EBI interface.
*
* @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
******************************************************************************/
#include <stdio.h>
#include "nuc980.h"
#include "sys.h"
#include "ebi.h"

#define CONFIG_EBI_16BITS

void EBI_FuncPinInit(void)
{
	/* EBI AD0~15 */
	SYS->GPC_MFPL |= SYS_GPC_MFPL_PC0MFP_EBI_DATA0;

	SYS->GPD_MFPH |= SYS_GPD_MFPH_PD12MFP_EBI_DATA1 | SYS_GPD_MFPH_PD13MFP_EBI_DATA2 |
					 SYS_GPD_MFPH_PD14MFP_EBI_DATA3 | SYS_GPD_MFPH_PD15MFP_EBI_DATA4;

	SYS->GPF_MFPL |= SYS_GPF_MFPL_PF0MFP_EBI_DATA5 | SYS_GPF_MFPL_PF1MFP_EBI_DATA6  |
					 SYS_GPF_MFPL_PF2MFP_EBI_DATA7 | SYS_GPF_MFPL_PF3MFP_EBI_DATA8  |
					 SYS_GPF_MFPL_PF4MFP_EBI_DATA9 | SYS_GPF_MFPL_PF5MFP_EBI_DATA10  |
					 SYS_GPF_MFPL_PF6MFP_EBI_DATA11 | SYS_GPF_MFPL_PF7MFP_EBI_DATA12;

	SYS->GPF_MFPH |= SYS_GPF_MFPH_PF8MFP_EBI_DATA13 | SYS_GPF_MFPH_PF9MFP_EBI_DATA14 | SYS_GPF_MFPH_PF10MFP_EBI_DATA15;

	/* EBI ADR0~19 */
	SYS->GPG_MFPL |= SYS_GPG_MFPL_PG0MFP_EBI_ADDR0 | SYS_GPG_MFPL_PG1MFP_EBI_ADDR1 |
					 SYS_GPG_MFPL_PG2MFP_EBI_ADDR2 | SYS_GPG_MFPL_PG3MFP_EBI_ADDR3 |
					 SYS_GPG_MFPL_PG6MFP_EBI_ADDR4 | SYS_GPG_MFPL_PG7MFP_EBI_ADDR5;

	SYS->GPG_MFPH |= SYS_GPG_MFPH_PG8MFP_EBI_ADDR6 | SYS_GPG_MFPH_PG9MFP_EBI_ADDR7;
	SYS->GPA_MFPH |= SYS_GPA_MFPH_PA12MFP_EBI_ADDR8 | SYS_GPA_MFPH_PA11MFP_EBI_ADDR9 |
					 SYS_GPA_MFPH_PA10MFP_EBI_ADDR10;

	SYS->GPB_MFPH |= SYS_GPB_MFPH_PB8MFP_EBI_ADDR11;
	SYS->GPB_MFPL |= SYS_GPB_MFPL_PB0MFP_EBI_ADDR12;
	SYS->GPA_MFPH |= SYS_GPA_MFPH_PA13MFP_EBI_ADDR13 | SYS_GPA_MFPH_PA14MFP_EBI_ADDR14;
	SYS->GPB_MFPL |= SYS_GPB_MFPL_PB7MFP_EBI_ADDR15 | SYS_GPB_MFPL_PB5MFP_EBI_ADDR16 |
					 SYS_GPB_MFPL_PB1MFP_EBI_ADDR17 | SYS_GPB_MFPL_PB3MFP_EBI_ADDR18;

	SYS->GPA_MFPH |= SYS_GPA_MFPH_PA15MFP_EBI_ADDR19;

	/* EBI RD and WR pins on PA.8 and PA.7 */
	SYS->GPA_MFPH |= SYS_GPA_MFPH_PA8MFP_EBI_nRE;
	SYS->GPA_MFPL |= SYS_GPA_MFPL_PA7MFP_EBI_nWE;

	/* EBI CS0 pin on PA.9 */
	SYS->GPA_MFPH |= SYS_GPA_MFPH_PA9MFP_EBI_nCS0;

}

/*-----------------------------------------------------------------------------*/
void UART_Init()
{
	/* enable UART0 clock */
	outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | 0x10000);

	/* PF11, PF12 */
	outpw(REG_SYS_GPF_MFPH, (inpw(REG_SYS_GPF_MFPH) & 0xfff00fff) | 0x11000);// UART0 multi-function

	/* UART0 line configuration for (115200,n,8,1) */
	outpw(REG_UART0_LCR, inpw(REG_UART0_LCR) | 0x07);
	outpw(REG_UART0_BAUD, 0x30000066); /* 12MHz reference clock input, 115200 */
}

int main(void)
{
	uint32_t u32Idx = 0, u32WriteData = 0x5A5A5A5A;
	uint32_t i;

	sysDisableCache();
	sysFlushCache(I_D_CACHE);
	sysEnableCache(CACHE_WRITE_BACK);

	UART_Init();

	/* Enable EBI/ GPIO clock */
	outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | 0xA00);
	/* Configure multi-function pins for EBI 16-bit application */
	EBI_FuncPinInit();

#ifdef CONFIG_EBI_16BITS
	/* CPU Write - EBI_BUSWIDTH_16BIT */
	/* Initialize EBI bank0 to access external SRAM */
	EBI_Open(EBI_BANK0, EBI_BUSWIDTH_16BIT, EBI_TIMING_FASTEST, 0, EBI_CS_ACTIVE_LOW);
	printf("    All 0x%02X access ... EBI->CTL0 = 0x%x\n", (uint8_t)u32WriteData, EBI->CTL0);
	for(i = 0; i < 128; i++)
	{
		u32Idx = 0;
		while(u32Idx < ((EBI_MAX_SIZE >> 1)/4))
		{
			EBI0_WRITE_DATA32(u32Idx, (uint32_t)(u32WriteData));
			EBI0_READ_DATA32(u32Idx);
			u32Idx+=4;
		}
	}
	printf("*** CPU Write - EBI_BUSWIDTH_16BIT done ***\n\n");
#else
	EBI_Open(EBI_BANK0, EBI_BUSWIDTH_8BIT, EBI_TIMING_FASTEST, 0, EBI_CS_ACTIVE_LOW);

	printf("    All 0x%02X access ... EBI->CTL0 = 0x%x\n", (uint8_t)u32WriteData, EBI->CTL0);

	for(i = 0; i < 128; i++)
	{
		u32Idx = 0;
		while(u32Idx < EBI_MAX_SIZE)
		{
			EBI0_WRITE_DATA8(u32Idx, (uint8_t)(u32WriteData));
			EBI0_READ_DATA8(u32Idx);
			u32Idx++;
		}
	}
	printf("*** CPU Write - EBI_BUSWIDTH_8BIT done ***\n\n");
#endif

	/* Disable EBI function */
	EBI_Close(EBI_BANK0);

	/* Disable EBI clock */
	outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) & ~(0x200));
	while(1);
}

