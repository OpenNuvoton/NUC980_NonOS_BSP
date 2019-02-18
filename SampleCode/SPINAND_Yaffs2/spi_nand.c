/**************************************************************************//**
 * @file     spi_nand.c
 * @version  V1.00
 * $Revision: 1 $
 * $Date: 19/01/28 5:17p $
 * @brief    NuMicro ARM9 SPI NAND driver source file
 *
 * @note
 * Copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <common.h>
#include <nand.h>
#include <errno.h>
#include <linux/mtd/concat.h>
#include "yaffs_malloc.h"
#include "nuc980.h"
#include "qspi.h"
#include "gpio.h"
#include "nand.h"

#define Page_size 0x800			// 2048 bytes per page
#define Spare_size 0x40			// 64 bytes per page spare area
#define Pages_per_Block 0x40			// 64 pages	per block
#define Block_count 0x400		// 1024 blocks per 1G NAND
#define status_ok 0x00
#define ID_error 0x01
#define Read_error 0x02
#define Erase_error 0x03
#define Program_error 0x04
#define Bad_block_count_over 0x05

#ifndef CONFIG_SYS_NAND_BASE_LIST
#define CONFIG_SYS_NAND_BASE_LIST { CONFIG_SYS_NAND_BASE }
#endif

int32_t EnableHWECC(void);
int32_t DisableHWECC(void);
int32_t MarkBadBlock(int32_t page);
int32_t ReadPage(int32_t nPBlockAddr, int32_t nPageNo, uint8_t *buff);
int32_t WritePage(int32_t nPBlockAddr, int32_t nPageNo, uint8_t *buff);
int32_t IsDirtyPage(int32_t page);
int32_t IsValidBlock(int32_t page);
int32_t EraseBlock(int32_t page);
void spinand_init(void);

/** @addtogroup Standard_Driver Standard Driver
  @{
*/

/** @addtogroup NAND_Driver NAND Driver
  @{
*/

/** @addtogroup NAND_EXPORTED_CONSTANTS NAND Exported Constants
  @{
*/
/// @cond HIDDEN_SYMBOLS


#define DIRTY_FUNCTION

static void SPI_CS_LOW(void)
{
    // /CS: active
    QSPI_SET_SS_LOW(QSPI0);
}

static void SPI_CS_HIGH(void)
{
    // /CS: de-active
    QSPI_SET_SS_HIGH(QSPI0);
}

static uint8_t SPIin(uint8_t DI)
{
    QSPI_WRITE_TX(QSPI0, DI);
    while(QSPI_GET_RX_FIFO_EMPTY_FLAG(QSPI0));
    return (QSPI_READ_RX(QSPI0) & 0xff);
}

static void SPINAND_ReadyBusyCheck(void)
{
    uint8_t SR = 0xFF;
    while((SR & 0x1) != 0x00) {
        SPI_CS_LOW();
        SPIin(0x0F);
        SPIin(0xC0);
        SR = SPIin(0x00);
        SPI_CS_HIGH();
    }
    return;
}

static void SPINAND_Reset(void)
{
    SPI_CS_LOW();
    SPIin(0xFF);
    SPI_CS_HIGH();
    SPINAND_ReadyBusyCheck();
}

static uint32_t SPINAND_ReadID(void)
{
    uint32_t JEDECID = 0;
    SPI_CS_LOW();
    SPIin(0x9F);
    SPIin(0x00); // dummy
    JEDECID += SPIin(0x00);
    JEDECID <<= 8;
    JEDECID += SPIin(0x00);
    JEDECID <<= 8;
    JEDECID += SPIin(0x00);
    SPI_CS_HIGH();
    return JEDECID;
}

static void SPINAND_PageDataRead(uint8_t PA_H, uint8_t PA_L)
{
    SPI_CS_LOW();
    SPIin(0x13); //
    SPIin(0x00); // dummy
    SPIin(PA_H); // Page address
    SPIin(PA_L); // Page address
    SPI_CS_HIGH();
    SPINAND_ReadyBusyCheck(); // Need to wait for the data transfer.
    return;
}

#define PDMA_BA         0xB0008000
#define REG_PDMA_DSCT0_CTL      (PDMA_BA+0x000)
#define REG_PDMA_DSCT0_SA       (PDMA_BA+0x004)
#define REG_PDMA_DSCT0_DA       (PDMA_BA+0x008)
#define REG_PDMA_CHCTL          (PDMA_BA+0x400)
#define REG_PDMA_TDSTS          (PDMA_BA+0x424)
#define REG_PDMA_REQSEL0_3      (PDMA_BA+0x480)
#define TXCNT_Pos               (16)
#define TXWIDTH_32              (0x2000)
#define SRC_FIXED               (0x300)
#define TBINTDIS                (0x80)
#define TX_SINGLE               (0x4)
#define MODE_BASIC              (0x1)

static void SPINAND_NormalRead(uint8_t addh, uint8_t addl, uint8_t* buff, uint32_t len)
{
    uint32_t i = 0;
    SPI_CS_LOW();
    SPIin(0x03);
    SPIin(addh);
    SPIin(addl);
    SPIin(0x00); // dummy
    if (len > 100) {
        uint32_t count;
        QSPI0->CTL &= ~1; //Disable SPIEN
        while (QSPI0->STATUS & 0x8000) {} //SPIENSTS

        QSPI0->CTL &= ~0x1F00; //Data Width = 32 bit
        QSPI0->CTL &= ~0xF0; //Suspend interval = 0
        QSPI0->CTL |= 0x80000; //Byte reorder

        QSPI0->FIFOCTL |= 3; //TX/RX reset
        while (QSPI0->STATUS & 0x800000);

        QSPI0->CTL |= 0x1; //Enable SPIEN
        while (!(QSPI0->STATUS & 0x8000)) {} //SPIENSTS

        outpw(REG_CLK_HCLKEN, (inpw(REG_CLK_HCLKEN) | 0x1000)); /* enable PDMA0 clock */
        /* initial PDMA channel 0 */

        outpw(REG_PDMA_CHCTL, 0x1);
        outpw(REG_PDMA_REQSEL0_3,21);  /* QSPI0 RX */

        count = (len + 3) >> 2;
        /* set PDMA */
        outpw(REG_PDMA_DSCT0_SA, REG_QSPI0_RX);
        outpw(REG_PDMA_DSCT0_DA, (unsigned int) buff);
        outpw(REG_PDMA_DSCT0_CTL, ((count-1)<<TXCNT_Pos)|TXWIDTH_32|SRC_FIXED|TBINTDIS|TX_SINGLE|MODE_BASIC);
        QSPI0->PDMACTL |= 0x2;
        while(1) {
            if (inpw(REG_PDMA_TDSTS) & 0x1) {
                outpw(REG_PDMA_TDSTS, 0x1);
                break;
            }
        }
        QSPI0->CTL &= ~(0x80000); //Disable Byte reorder
        QSPI0->CTL = (QSPI0->CTL & ~0x1F00)| (0x800); //Data length 8 bit
        QSPI0->PDMACTL = 0; // disable RX
        outpw(REG_CLK_HCLKEN, (inpw(REG_CLK_HCLKEN) & ~0x1000)); /* disable PDMA0 clock */
    } else {
        for( i = 0; i < len; i++) {
            *(buff+i) = SPIin(0x00);
        }
    }
    SPI_CS_HIGH();
    return;
}

static uint8_t SPINAND_ReadStatusRegister(uint8_t sr_sel)
{
    uint8_t SR = 0;  // status register data
    switch(sr_sel) {
    case 0x01:
        SPI_CS_LOW();
        SPIin(0x0F);
        SPIin(0xA0); // SR1
        SR = SPIin(0x00);
        SPI_CS_HIGH();
        break;
    case 0x02:
        SPI_CS_LOW();
        SPIin(0x0F);
        SPIin(0xB0); // SR2
        SR = SPIin(0x00);
        SPI_CS_HIGH();
        break;
    case 0x03:
        SPI_CS_LOW();
        SPIin(0x0F);
        SPIin(0xC0); // SR3
        SR = SPIin(0x00);
        SPI_CS_HIGH();
        break;
    default:
        SR = 0xFF;
        break;
    }
    return SR;
}

static void SPINAND_WriteStatusRegister1(uint8_t SR1)
{
    SPI_CS_LOW();
    SPIin(0x1F);
    SPIin(0xA0);
    SPIin(SR1);
    SPI_CS_HIGH();
    SPINAND_ReadyBusyCheck();
    return;
}

static void SPINAND_WriteStatusRegister2(uint8_t SR2)
{
    SPI_CS_LOW();
    SPIin(0x1F);
    SPIin(0xB0);
    SPIin(SR2);
    SPI_CS_HIGH();
    SPINAND_ReadyBusyCheck();
    return;
}

static uint8_t SPINAND_CheckEmbeddedECCFlag(void)
{
    uint8_t SR;
    SR = SPINAND_ReadStatusRegister(3); 	// Read status register 3
    return (SR&0x30)>>4;							// Check ECC-1, ECC0 bit
}

int32_t EnableHWECC(void)
{
    uint8_t SR;
    SR = SPINAND_ReadStatusRegister(2); 	// Read status register 2
    SR |= 0x10;										// Enable ECC-E bit
    SPINAND_WriteStatusRegister2(SR);
    return 0;
}

int32_t DisableHWECC(void)
{
    uint8_t SR;
    SR = SPINAND_ReadStatusRegister(2); 	// Read status register 2
    SR &= 0xEF;									// Disable ECC-E bit
    SPINAND_WriteStatusRegister2(SR);
    return 0;
}

static void SPINAND_Unprotect(void)
{
    uint8_t SR;
    SR = SPINAND_ReadStatusRegister(1); 	// Read status register 2
    SR &= 0x83;										// Enable ECC-E bit
    SPINAND_WriteStatusRegister1(SR);
    return;
}

int32_t ReadPage(int32_t nPBlockAddr, int32_t nPageNo, uint8_t *buff)
{
    uint32_t page = nPBlockAddr * Pages_per_Block + nPageNo;
    uint8_t EPR_status;

    //printf("ReadPage : nPBlockAddr = %d, nPageNo = %d\n",nPBlockAddr, nPageNo);

    SPINAND_PageDataRead(page/0x100, page%0x100);		// Read verify
    SPINAND_NormalRead(0, 0, buff, Page_size);
    EPR_status = SPINAND_CheckEmbeddedECCFlag();
    if((EPR_status != 0x00) && (EPR_status != 0x01)) {
        printf("ECC status error [0x%x] while read Block[%d] page[%d(%d)]\n",EPR_status,nPBlockAddr,nPageNo,page);
        MarkBadBlock(nPBlockAddr);
        return Read_error;					// Check ECC status and return fail if (ECC-1, ECC0) != (0,0) or != (0,1)
    }

    return 0;
}

static void SPINAND_LoadPageProgramData(uint8_t addh, uint8_t addl, uint8_t* program_buffer, uint32_t count)
{
    uint32_t i = 0;

    SPI_CS_LOW();
    SPIin(0x06);
    SPI_CS_HIGH();

    SPI_CS_LOW();
    SPIin(0x02);
    SPIin(addh);
    SPIin(addl);
    for(i = 0; i < count; i++) {
        SPIin(*(program_buffer+i));
    }
    SPI_CS_HIGH();

    return;
}

static void SPINAND_ProgramExcute(uint8_t addh, uint8_t addl)
{

    SPI_CS_LOW();
    SPIin(0x10);
    SPIin(0x00); // dummy
    SPIin(addh);
    SPIin(addl);
    SPI_CS_HIGH();
    SPINAND_ReadyBusyCheck();

    return;
}

static uint8_t SPINAND_CheckProgramEraseFailFlag(void)
{
    uint8_t SR;
    SR = SPINAND_ReadStatusRegister(3); 	// Read status register 3
    return (SR&0x0C)>>2;							// Check P-Fail, E-Fail bit
}

int32_t WritePage(int32_t nPBlockAddr, int32_t nPageNo, uint8_t *buff)
{
    uint32_t page = nPBlockAddr * Pages_per_Block + nPageNo;

#ifdef DIRTY_FUNCTION
    buff[Page_size] = 0xFF; //BAD block marker
    buff[Page_size+1] = 0xFF; //BAD block marker
    buff[Page_size+2] = 0xFF; //User data II
    buff[Page_size+3] = 0xFF; //User data II
    buff[Page_size+4] = 0x12; //User data I : set to dirty page

    SPINAND_LoadPageProgramData(0, 0, buff, Page_size + 5);
#else
    SPINAND_LoadPageProgramData(0, 0, buff, Page_size);
#endif
    SPINAND_ProgramExcute(page/0x100, page%0x100);
    if(SPINAND_CheckProgramEraseFailFlag() != 0)
        return -1; // Program failed

    return 0;
}

static uint8_t SPINAND_CheckBadBlock(uint32_t page_address)
{
    uint8_t read_buf;

    SPINAND_PageDataRead(page_address/0x100, page_address%0x100);   // Read the first page of a block

    SPINAND_NormalRead(0x8, 0x0, &read_buf, 1);		// Read bad block mark at 0x800
    if(read_buf != 0xFF) {
        return 1;
    }
    SPINAND_PageDataRead((page_address+1)/0x100, (page_address+1)%0x100);   // Read the second page of a block

    SPINAND_NormalRead(0x8, 0x0, &read_buf, 1);	// Read bad block mark at 0x800
    if(read_buf != 0xFF) {
        return 1;
    }
    return 0;
}

int32_t MarkBadBlock(int32_t page)
{
    uint8_t bad_marker = 0x01; /* Non 0xFF means a bad block */

    /* Write a non 0xFF value to the first byte of spare area of page 0 */
    SPINAND_LoadPageProgramData(0x8, 0, &bad_marker, 1);
    SPINAND_ProgramExcute(page/0x100, page%0x100);
    if(SPINAND_CheckProgramEraseFailFlag() != 0)
        return -1; // Program failed

    return 0;
}

int32_t IsDirtyPage(int32_t page)
{
#ifdef DIRTY_FUNCTION
    //uint8_t c0, c1;
    uint8_t c0;

    //printf("IsDirtyPage:\n");

    SPINAND_PageDataRead(page/0x100, page%0x100);		// Read verify
    SPINAND_NormalRead(8, 4, (uint8_t *)&c0, 1);
    //printf("c0 [%x)]\n",c0);
    if (c0 != 0xFF)
        return TRUE;

    return FALSE; // not dirty
#else
    return TRUE; // always return dirty
#endif
}

int32_t IsValidBlock(int32_t page)
{
    uint8_t bad_block;
//		uint8_t i;

    //printf("IsValidBlock: [%d]\n",nPBlockAddr);
    bad_block = SPINAND_CheckBadBlock(page);
    //printf("bad_block = %d\n",bad_block);
    if(bad_block == 1)
        return FALSE; // Bad block

#if 0
    //Check if all pages are not dirty
    for (i = 0; i < Pages_per_Block; i++) {
        if (IsDirtyPage(nPBlockAddr, i))
            return FALSE;
    }
#endif

    return TRUE;
}

static void SPINAND_BlockErase(uint8_t PA_H, uint8_t PA_L)
{

    SPI_CS_LOW();
    SPIin(0x06);
    SPI_CS_HIGH();

    SPI_CS_LOW();
    SPIin(0xD8);
    SPIin(0x00); // dummy
    SPIin(PA_H);
    SPIin(PA_L);
    SPI_CS_HIGH();

    SPINAND_ReadyBusyCheck();
}

int32_t EraseBlock(int32_t page)
{
    uint8_t PA_H,PA_L;
    uint8_t status;

    //printf("erase: page %d\n", page);

    PA_H = (page >> 8) & 0xff;
    PA_L = page & 0xff;
    SPINAND_BlockErase(PA_H, PA_L);

    if ((status = SPINAND_CheckProgramEraseFailFlag()) != 0) {
        printf("erase status: %02x\n", status);
        return -1; // Erase failed
    }

    return 0;
}


#define CONFIG_SYS_MAX_SPINAND_DEVICE 1

struct mtd_info *spinand_info[CONFIG_SYS_MAX_SPINAND_DEVICE];

struct nuvoton_spinand_info {
    struct nand_hw_control  controller;
    struct mtd_info         mtd;
    struct nand_chip        chip;
    int                     eBCHAlgo;
    int                     m_i32SMRASize;
};
struct nuvoton_spinand_info g_nuvoton_spinand;
struct nuvoton_spinand_info *nuvoton_spinand;

static struct nand_ecclayout nuvoton_nand_oob;

static void nuvoton_layout_oob_table ( struct nand_ecclayout* pNandOOBTbl, int oobsize , int eccbytes )
{
    pNandOOBTbl->eccbytes = eccbytes;

    pNandOOBTbl->oobavail = oobsize - 4 - eccbytes ;

    pNandOOBTbl->oobfree[0].offset = 4;  // Bad block marker size

    pNandOOBTbl->oobfree[0].length = oobsize - eccbytes - pNandOOBTbl->oobfree[0].offset ;
}

#ifndef CONFIG_SYS_NAND_SELF_INIT
static struct nand_chip spinand_chip[CONFIG_SYS_MAX_NAND_DEVICE];
//static ulong base_address[CONFIG_SYS_MAX_NAND_DEVICE] = CONFIG_SYS_NAND_BASE_LIST;
#endif

static char dev_name[CONFIG_SYS_MAX_SPINAND_DEVICE][8];

static unsigned long total_nand_size; /* in kiB */

/* Register an initialized NAND mtd device with the U-Boot NAND command. */
int spinand_register(int devnum, struct mtd_info *mtd)
{
    if (devnum >= CONFIG_SYS_MAX_SPINAND_DEVICE)
        return -EINVAL;

    spinand_info[devnum] = mtd;

    sprintf(dev_name[devnum], "spinand%d", devnum);
    mtd->name = dev_name[devnum];

#ifdef CONFIG_MTD_DEVICE
    /*
     * Add MTD device so that we can reference it later
     * via the mtdcore infrastructure (e.g. ubi).
     */
    add_mtd_device(mtd);
#endif

    total_nand_size += mtd->size / 1024;

    if (nand_curr_device == -1)
        nand_curr_device = devnum;

    return 0;
}

int board_spinand_init(struct nand_chip *spinand)
{
    struct mtd_info *mtd;

    nuvoton_spinand = &g_nuvoton_spinand;
    memset((void*)nuvoton_spinand,0,sizeof(struct nuvoton_spinand_info));

    if (!nuvoton_spinand)
        return -1;

    mtd=&nuvoton_spinand->mtd;
    nuvoton_spinand->chip.controller = &nuvoton_spinand->controller;

    /* read_buf and write_buf are default */
    /* read_byte and write_byte are default */
    /* hwcontrol always must be implemented */
    spinand->chip_delay = 50;

    spinand->controller = &nuvoton_spinand->controller;

    spinand->ecc.layout    = &nuvoton_nand_oob;
    spinand->ecc.strength  = 8;
    mtd = nand_to_mtd(spinand);

    mtd->priv = spinand;

    /* initial QSPI0 controller */
    outpw(REG_CLK_PCLKEN1, (inpw(REG_CLK_PCLKEN1) | 0x10));

    /* Set GPD2~5 for QSPI0 */
    outpw(REG_SYS_GPD_MFPL, (inpw(REG_SYS_GPD_MFPL) & 0xff0000ff) | 0x00111100);

    PD->MODE = (PD->MODE & 0xFFFF0FFF) | 0x5000; /* Configure PD6 and PD7 as output mode */
    PD6 = 1; /* PD6: SPI0_MOSI1 or SPI flash /WP pin */
    PD7 = 1; /* PD7: SPI0_MISO1 or SPI flash /HOLD pin */
    PD->SMTEN |= 8; /* PD3 clk pin */

    QSPI0->CLKDIV = 4;
    QSPI0->SSCTL = 0; /* AUTOSS=0; low-active; de-select all SS pins. */
    QSPI0->FIFOCTL |= 0x3; /* TX/RX reset */
    while(QSPI0->STATUS & 0x800000);
    QSPI0->CTL = 0x805;
    while (!(QSPI0->STATUS & (1<<15))) {}

    SPINAND_Reset();
    printf("ID = 0x%x\n",SPINAND_ReadID());
    SPINAND_Unprotect();
    /* Detect SPI NAND chips */
    /* first scan to find the device and get the page size */
//    if (nand_scan_ident(mtd, 1, NULL)) {
//        printf("NAND Flash not found !\n");
//        return -1;
//    }

    mtd->writesize = Page_size;
    mtd->oobsize = Spare_size;
    //nuvoton_layout_oob_table ( &nuvoton_nand_oob, mtd->oobsize, g_i32ParityNum[0][nuvoton_spinand->eBCHAlgo] );
    nuvoton_layout_oob_table ( &nuvoton_nand_oob, mtd->oobsize, 32 );

    nuvoton_spinand->m_i32SMRASize  = mtd->oobsize;
    spinand->ecc.bytes = 32; //nuvoton_nand_oob.eccbytes;
    spinand->ecc.size  = mtd->writesize;

    spinand->options = 0;
    spinand->bbt_options = (NAND_BBT_USE_FLASH | NAND_BBT_NO_OOB);


    return 0;
}

//static int spinand_erase(struct mtd_info *info, loff_t off, size_t size)
static int spinand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
    uint32_t off,size,page,blockcnt,i;

    off = instr->addr;
    size = instr->len;
    //printf("[spinand_erase] off 0x%x, size 0x%x\n",off,size);

    blockcnt = size/mtd->erasesize;
    if (size%mtd->erasesize)
        blockcnt++;
    for (i=0; i<blockcnt; i++) {
        page = off/mtd->writesize;
        EraseBlock(page);
        off += mtd->erasesize;
    }

    return 0;
}

static int spinand_read(struct mtd_info *mtd, loff_t from, size_t len,
                        size_t *retlen, u_char *buf)
{
    uint32_t page,pagecnt,i;

    //printf("[spinand_read] from 0x%x, len 0x%x\n",(unsigned int)from,len);

    pagecnt = len/mtd->writesize;
    if (len%mtd->writesize)
        pagecnt++;
    page = from/mtd->writesize;
    for (i=0; i<pagecnt; i++) {
        //printf("page %d\n",page);
        ReadPage(0, page, buf);
        buf += mtd->writesize;
        page++;
    }
    return 0;
}

static int spinand_read_oob(struct mtd_info *mtd, loff_t from,
                            struct mtd_oob_ops *ops)
{
    uint32_t page;

    //printf("spinand_read_oob from 0x%x\n",(unsigned int)from);
    //printf("[spinand_read_oob] databuf 0x%x, oobbuf 0x%x, ooboffs 0x%x, ooblen 0x%x\n",(unsigned int)ops->datbuf,(unsigned int)ops->oobbuf,ops->ooboffs,ops->ooblen);

    page = from/mtd->writesize;
    ReadPage(0, page, ops->datbuf);

    return 0;
}

static int spinand_write_oob(struct mtd_info *mtd, loff_t to,
                             struct mtd_oob_ops *ops)
{
    uint32_t page;

    //printf("spinand_write_oob offset 0x%x\n",(unsigned int)to);
    //printf("[spinand_write_oob] databuf 0x%x, oobbuf 0x%x, ooboffs 0x%x, ooblen 0x%x\n",(unsigned int)ops->datbuf,(unsigned int)ops->oobbuf,ops->ooboffs,ops->ooblen);

    page = to/mtd->writesize;
    WritePage(0, page, (uint8_t*)ops->datbuf);

    return 0;
}

static int spinand_write(struct mtd_info *mtd, loff_t to, size_t len,
                         size_t *retlen, const u_char *buf)
{
    uint32_t page,blockcnt,pagecnt,i;

    //printf("[spinand_write] to 0x%x, len 0x%x\n",(unsigned int)to,len);

    blockcnt = len/mtd->erasesize;
    if (len%mtd->erasesize)
        blockcnt++;
    for (i=0; i<blockcnt; i++) {
        page = to/mtd->writesize;
        EraseBlock(page);
        to += mtd->erasesize;
    }

    pagecnt = len/mtd->writesize;
    if (len%mtd->writesize)
        pagecnt++;
    for (i=0; i<pagecnt; i++) {
        WritePage(0, page, (uint8_t*)buf);
        buf += mtd->writesize;
        page++;
    }

    return 0;
}

static int panic_spinand_write(struct mtd_info *mtd, loff_t to, size_t len,
                               size_t *retlen, const u_char *buf)
{
    //printf("panic_spinand_write\n");

    return 0;
}

static int spinand_block_isbad(struct mtd_info *mtd, loff_t ofs)
{
    //printf("spinand_block_isbad offset 0x%x\n",(unsigned int)ofs);

    return !(IsValidBlock(ofs/mtd->writesize));
}

static int spinand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
    //printf("spinand_block_markbad offset 0x%x\n",(unsigned int)ofs);

    MarkBadBlock(ofs/mtd->writesize);

    return 0;
}

static void spinand_sync(struct mtd_info *mtd)
{
    //printf("spinand_sync\n");
}

static int spinand_block_isreserved(struct mtd_info *mtd, loff_t ofs)
{
    //printf("spinand_block_isreserved ofs 0x%x\n",(unsigned int)ofs);

    return 0;
}

#ifndef CONFIG_SYS_NAND_SELF_INIT
static void spinand_init_chip(int i)
{
    struct nand_chip *spinand = &spinand_chip[i];
    struct mtd_info *mtd = nand_to_mtd(spinand);
    int maxchips = CONFIG_SYS_NAND_MAX_CHIPS;

    if (maxchips < 1)
        maxchips = 1;

    if (board_spinand_init(spinand))
        return;

//	if (nand_scan(mtd, maxchips))
//		return;
    //Below settings should be got from nand_scan()
    spinand->chipsize = Block_count * Pages_per_Block * Page_size;
    mtd->writesize = Page_size;
    mtd->erasesize = Pages_per_Block * (mtd->writesize);
    mtd->oobsize = Spare_size;
    spinand->numchips = i;
    mtd->size = 1 * spinand->chipsize;

    mtd->_erase = spinand_erase;
    mtd->_read = spinand_read;
    mtd->_write = spinand_write;
    mtd->_panic_write = panic_spinand_write;
    mtd->_read_oob = spinand_read_oob;
    mtd->_write_oob = spinand_write_oob;
    mtd->_sync = spinand_sync;
    mtd->_lock = NULL;
    mtd->_unlock = NULL;
    mtd->_block_isreserved = spinand_block_isreserved;
    mtd->_block_isbad = spinand_block_isbad;
    mtd->_block_markbad = spinand_block_markbad;
    mtd->writebufsize = mtd->writesize;

    spinand_register(i, mtd);
}
#endif

#ifdef CONFIG_MTD_CONCAT
static void create_mtd_concat(void)
{
    struct mtd_info *nand_info_list[CONFIG_SYS_MAX_NAND_DEVICE];
    int nand_devices_found = 0;
    int i;

    for (i = 0; i < CONFIG_SYS_MAX_NAND_DEVICE; i++) {
        if (nand_info[i] != NULL) {
            nand_info_list[nand_devices_found] = nand_info[i];
            nand_devices_found++;
        }
    }
    if (nand_devices_found > 1) {
        struct mtd_info *mtd;
        char c_mtd_name[16];

        /*
         * We detected multiple devices. Concatenate them together.
         */
        sprintf(c_mtd_name, "nand%d", nand_devices_found);
        mtd = mtd_concat_create(nand_info_list, nand_devices_found,
                                c_mtd_name);

        if (mtd == NULL)
            return;

        nand_register(nand_devices_found, mtd);
    }

    return;
}
#else
static void create_mtd_concat(void)
{
}
#endif


void spinand_init(void)
{
#ifdef CONFIG_SYS_NAND_SELF_INIT
    board_nand_init();
#else
    int i;

    YAFFS_InitializeMemoryPool();
    for (i = 0; i < CONFIG_SYS_MAX_NAND_DEVICE; i++)
        spinand_init_chip(i);
#endif

    printf("%lu MiB\n", total_nand_size / 1024);

#ifdef CONFIG_SYS_NAND_SELECT_DEVICE
    /*
     * Select the chip in the board/cpu specific driver
     */
    board_nand_select_device(mtd_to_nand(nand_info[nand_curr_device]),
                             nand_curr_device);
#endif

    create_mtd_concat();
}


/// @endcond HIDDEN_SYMBOLS


/*@}*/ /* end of group NAND_EXPORTED_FUNCTIONS */

/*@}*/ /* end of group NAND_Driver */

/*@}*/ /* end of group Standard_Driver */

/*** (C) COPYRIGHT 2019 Nuvoton Technology Corp. ***/
