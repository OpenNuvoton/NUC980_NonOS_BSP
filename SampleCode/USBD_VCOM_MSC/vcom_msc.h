/******************************************************************************
 * @file     massstorage.h
 * @brief    NuMicro ARM9 USB mass storage header file
 * @version  V1.00
 * $Revision: 1 $
 * $Date: 18/08/05 10:06a $
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#ifndef __USBD_MASS_H__
#define __USBD_MASS_H__

/* Define the vendor id and product id */
#define USBD_VID        0x0416
#define USBD_PID        0xDC01

/* Define DMA Maximum Transfer length */
#define USBD_MAX_DMA_LEN    0x1000

/* Define sector size */
#define USBD_SECTOR_SIZE    512

// Define SD Card Maximum Transfer length
// The real buffer space is from g_u32StorageBase to (g_u32StorageBase + USBD_MAX_SD_SECTOR * 512)
#define USBD_MAX_SD_SECTOR  64      // unit is sector, 64 sectors = 32KB
#define USBD_MAX_SD_LEN     (64*512)   // unit is byte, MUST keep USBD_MAX_SD_LEN = USBD_MAX_SD_SECTOR * USBD_SECTOR_SIZE

/* Define EP maximum packet size */
#define CEP_MAX_PKT_SIZE        64
#define CEP_OTHER_MAX_PKT_SIZE  64
#define EPA_MAX_PKT_SIZE        512
#define EPA_OTHER_MAX_PKT_SIZE  64
#define EPB_MAX_PKT_SIZE        512
#define EPB_OTHER_MAX_PKT_SIZE  64
#define EPC_MAX_PKT_SIZE        64
#define EPC_OTHER_MAX_PKT_SIZE  64
#define EPD_MAX_PKT_SIZE        512
#define EPD_OTHER_MAX_PKT_SIZE  64
#define EPE_MAX_PKT_SIZE        512
#define EPE_OTHER_MAX_PKT_SIZE  64

#define CEP_BUF_BASE    0
#define CEP_BUF_LEN     CEP_MAX_PKT_SIZE
#define EPA_BUF_BASE    0x200
#define EPA_BUF_LEN     EPA_MAX_PKT_SIZE
#define EPB_BUF_BASE    0x400
#define EPB_BUF_LEN     EPB_MAX_PKT_SIZE
#define EPC_BUF_BASE    0x800
#define EPC_BUF_LEN     EPC_MAX_PKT_SIZE
#define EPD_BUF_BASE    0xa00
#define EPD_BUF_LEN     EPD_MAX_PKT_SIZE
#define EPE_BUF_BASE    0xc00
#define EPE_BUF_LEN     EPE_MAX_PKT_SIZE

/* Define the interrupt In EP number */
#define BULK_IN_EP_NUM      0x01
#define BULK_OUT_EP_NUM     0x02
#define INT_IN_EP_NUM       0x03
#define BULK_IN_EPD_NUM     0x04
#define BULK_OUT_EPE_NUM    0x05

/* Define Descriptor information */
#define USBD_SELF_POWERED               0
#define USBD_REMOTE_WAKEUP              0
#define USBD_MAX_POWER                  50  /* The unit is in 2mA. ex: 50 * 2mA = 100mA */

/*!<Define Mass Storage Class Specific Request */
#define BULK_ONLY_MASS_STORAGE_RESET    0xFF
#define GET_MAX_LUN                     0xFE

/*!<Define Mass Storage Signature */
#define CBW_SIGNATURE       0x43425355
#define CSW_SIGNATURE       0x53425355

/*!<Define Mass Storage UFI Command */
#define UFI_TEST_UNIT_READY                     0x00
#define UFI_REQUEST_SENSE                       0x03
#define UFI_INQUIRY                             0x12
#define UFI_MODE_SELECT_6                       0x15
#define UFI_MODE_SENSE_6                        0x1A
#define UFI_START_STOP                          0x1B
#define UFI_PREVENT_ALLOW_MEDIUM_REMOVAL        0x1E
#define UFI_READ_FORMAT_CAPACITY                0x23
#define UFI_READ_CAPACITY                       0x25
#define UFI_READ_10                             0x28
#define UFI_WRITE_10                            0x2A
#define UFI_VERIFY_10                           0x2F
#define UFI_MODE_SELECT_10                      0x55
#define UFI_MODE_SENSE_10                       0x5A

/*-----------------------------------------*/
#define BULK_CBW    0x00
#define BULK_IN     0x01
#define BULK_OUT    0x02
#define BULK_CSW    0x04
#define BULK_NORMAL 0xFF

static __inline uint32_t get_be32(uint8_t *buf)
{
    return ((uint32_t) buf[0] << 24) | ((uint32_t) buf[1] << 16) |
           ((uint32_t) buf[2] << 8) | ((uint32_t) buf[3]);
}


/******************************************************************************/
/*                USBD Mass Storage Structure                                 */
/******************************************************************************/

/*!<USB Mass Storage Class - Command Block Wrapper Structure */
struct CBW
{
    uint32_t  dCBWSignature;
    uint32_t  dCBWTag;
    uint32_t  dCBWDataTransferLength;
    uint8_t   bmCBWFlags;
    uint8_t   bCBWLUN;
    uint8_t   bCBWCBLength;
    uint8_t   u8OPCode;
    uint8_t   u8LUN;
    uint8_t   au8Data[14];
};

/*!<USB Mass Storage Class - Command Status Wrapper Structure */
struct CSW
{
    uint32_t  dCSWSignature;
    uint32_t  dCSWTag;
    uint32_t  dCSWDataResidue;
    uint8_t   bCSWStatus;
};


/************************************************/
/*!<Define CDC Class Specific Request */
#define SET_LINE_CODE           0x20
#define GET_LINE_CODE           0x21
#define SET_CONTROL_LINE_STATE  0x22

/* for CDC class */
/* Line coding structure
  0-3 dwDTERate    Data terminal rate (baudrate), in bits per second
  4   bCharFormat  Stop bits: 0 - 1 Stop bit, 1 - 1.5 Stop bits, 2 - 2 Stop bits
  5   bParityType  Parity:    0 - None, 1 - Odd, 2 - Even, 3 - Mark, 4 - Space
  6   bDataBits    Data bits: 5, 6, 7, 8, 16  */

typedef struct
{
    uint32_t  u32DTERate;     /* Baud rate    */
    uint8_t   u8CharFormat;   /* stop bit     */
    uint8_t   u8ParityType;   /* parity       */
    uint8_t   u8DataBits;     /* data bits    */
} STR_VCOM_LINE_CODING;

/*--------------------------------------------------------------------------*/
#define RXBUFSIZE           8192 /* RX buffer size */
#define TXBUFSIZE           8192 /* RX buffer size */
#define TX_FIFO_SIZE        16  /* TX Hardware FIFO size */

/*-------------------------------------------------------------*/
extern volatile int8_t gi8BulkOutReady;
extern STR_VCOM_LINE_CODING gLineCoding;
extern uint16_t gCtrlSignal;
extern volatile uint16_t comRbytes;
extern volatile uint16_t comRhead;
extern volatile uint16_t comRtail;
extern volatile uint16_t comTbytes;
extern volatile uint16_t comThead;
extern volatile uint16_t comTtail;
extern volatile uint32_t usbRbytes;
extern volatile uint32_t usbRhead;
extern volatile uint32_t usbRtail;
extern uint32_t gu32RxSize;
extern uint32_t gu32TxSize;
extern uint8_t gUsbRxBuf[];

/*-------------------------------------------------------------*/
void VCOM_MSC_Init(void);
void VCOM_MSC_InitForHighSpeed(void);
void VCOM_MSC_InitForFullSpeed(void);
void VCOM_MSC_ClassRequest(void);
void VCOM_LineCoding(uint8_t port);
void VCOM_TransferData(void);

/*-------------------------------------------------------------*/
void MSC_RequestSense(void);
void MSC_ReadFormatCapacity(void);
void MSC_ReadCapacity(void);
void MSC_ModeSense10(void);
void MSC_ReceiveCBW(uint32_t u32Buf);
void MSC_ProcessCmd(void);
void MSC_ActiveDMA(uint32_t u32Addr, uint32_t u32Len);
void MSC_BulkOut(uint32_t u32Addr, uint32_t u32Len);
void MSC_BulkIn(uint32_t u32Addr, uint32_t u32Len);
void MSC_AckCmd(uint32_t u32Residue);

void MSC_ReadMedia(uint32_t addr, uint32_t size, uint8_t *buffer);
void MSC_WriteMedia(uint32_t addr, uint32_t size, uint8_t *buffer);

#endif  /* __USBD_MASS_H_ */

/*** (C) COPYRIGHT 2018 Nuvoton Technology Corp. ***/
