/**************************************************************************//**
 * @file     support.c
 * @version  V1.10
 * $Revision: 11 $
 * $Date: 14/10/03 1:54p $
 * @brief  Functions to support USB host driver.
 *
 * @note
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "usb.h"

/// @cond HIDDEN_SYMBOLS


#define  USB_MEMORY_POOL_SIZE   (32*1024)
#define  USB_MEM_BLOCK_SIZE     128

#define  BOUNDARY_WORD          4


static uint32_t  _FreeMemorySize;
uint32_t  _AllocatedMemorySize;


#define USB_MEM_ALLOC_MAGIC     0x19685788    /* magic number in leading block */

typedef struct USB_mhdr
{
    uint32_t  flag;  /* 0:free, 1:allocated, 0x3:first block */
    uint32_t  bcnt;  /* if allocated, the block count of allocated memory block */
    uint32_t  magic;
    uint32_t  reserved;
}  USB_MHDR_T;

uint8_t  _USBMemoryPool[USB_MEMORY_POOL_SIZE] __attribute__((aligned(USB_MEM_BLOCK_SIZE)));


static USB_MHDR_T  *_pCurrent;
uint32_t  *_USB_pCurrent = (uint32_t *)&_pCurrent;

static uint32_t  _MemoryPoolBase, _MemoryPoolEnd;


void  USB_InitializeMemoryPool()
{
    _MemoryPoolBase = (UINT32)&_USBMemoryPool[0] | NON_CACHE_MASK;
    _MemoryPoolEnd = _MemoryPoolBase + USB_MEMORY_POOL_SIZE;
    _FreeMemorySize = _MemoryPoolEnd - _MemoryPoolBase;
    _AllocatedMemorySize = 0;
    _pCurrent = (USB_MHDR_T *)_MemoryPoolBase;
    memset((char *)_MemoryPoolBase, 0, _FreeMemorySize);
}


int  USB_available_memory()
{
    return _FreeMemorySize;
}


int  USB_allocated_memory()
{
    return _AllocatedMemorySize;
}


void  *USB_malloc(INT wanted_size, INT boundary)
{
    USB_MHDR_T  *pPrimitivePos = _pCurrent;
    USB_MHDR_T  *pFound;
    INT   found_size=-1;
    INT   i, block_count;
    INT   wrap = 0;
    int   disable_ohci_irq, disable_ehci_irq;

    if (IS_OHCI_IRQ_ENABLED())
        disable_ohci_irq = 1;
    else
        disable_ohci_irq = 0;

    if (IS_EHCI_IRQ_ENABLED())
        disable_ehci_irq = 1;
    else
        disable_ehci_irq = 0;

    if (disable_ohci_irq)
        DISABLE_OHCI_IRQ();
    if (disable_ehci_irq)
        DISABLE_EHCI_IRQ();

    if (wanted_size >= _FreeMemorySize)
    {
        printf("USB_malloc - want=%d, free=%d\n", wanted_size, _FreeMemorySize);
        if (disable_ohci_irq)
            ENABLE_OHCI_IRQ();
        if (disable_ehci_irq)
            ENABLE_EHCI_IRQ();
        return NULL;
    }

    if ((UINT32)_pCurrent >= _MemoryPoolEnd)
        _pCurrent = (USB_MHDR_T *)_MemoryPoolBase;   /* wrapped */

    do
    {
        if (_pCurrent->flag)          /* is not a free block */
        {
            if (_pCurrent->magic != USB_MEM_ALLOC_MAGIC)
            {
                printf("\nUSB_malloc - incorrect magic number! C:%x F:%x, wanted:%d, Base:0x%x, End:0x%x\n", (UINT32)_pCurrent, _FreeMemorySize, wanted_size, (UINT32)_MemoryPoolBase, (UINT32)_MemoryPoolEnd);
                if (disable_ohci_irq)
                    ENABLE_OHCI_IRQ();
                if (disable_ehci_irq)
                    ENABLE_EHCI_IRQ();
                return NULL;
            }

            if (_pCurrent->flag == 0x3)
                _pCurrent = (USB_MHDR_T *)((UINT32)_pCurrent + _pCurrent->bcnt * USB_MEM_BLOCK_SIZE);
            else
            {
                printf("USB_malloc warning - not the first block!\n");
                _pCurrent = (USB_MHDR_T *)((UINT32)_pCurrent + USB_MEM_BLOCK_SIZE);
            }

            if ((UINT32)_pCurrent > _MemoryPoolEnd)
                printf("USB_malloc - behind limit!!\n");

            if ((UINT32)_pCurrent == _MemoryPoolEnd)
            {
                //printf("USB_alloc - warp!!\n");
                wrap = 1;
                _pCurrent = (USB_MHDR_T *)_MemoryPoolBase;   /* wrapped */
            }

            found_size = -1;          /* reset the accumlator */
        }
        else                         /* is a free block */
        {
            if (found_size == -1)     /* the leading block */
            {
                pFound = _pCurrent;
                block_count = 1;

                if (boundary > BOUNDARY_WORD)
                    found_size = 0;    /* not use the data area of the leading block */
                else
                    found_size = USB_MEM_BLOCK_SIZE - sizeof(USB_MHDR_T);

                /* check boundary -
                 * If boundary > BOUNDARY_WORD, the start of next block should
                 * be the beginning address of allocated memory. Thus, we check
                 * the boundary of the next block. The leading block will be
                 * used as a header only.
                 */
                if ((boundary > BOUNDARY_WORD) &&
                        ((((UINT32)_pCurrent)+USB_MEM_BLOCK_SIZE >= _MemoryPoolEnd) ||
                         ((((UINT32)_pCurrent)+USB_MEM_BLOCK_SIZE) % boundary != 0)))
                    found_size = -1;   /* violate boundary, reset the accumlator */
            }
            else                      /* not the leading block */
            {
                found_size += USB_MEM_BLOCK_SIZE;
                block_count++;
            }

            if (found_size >= wanted_size)
            {
                pFound->bcnt = block_count;
                pFound->magic = USB_MEM_ALLOC_MAGIC;
                _FreeMemorySize -= block_count * USB_MEM_BLOCK_SIZE;
                _AllocatedMemorySize += block_count * USB_MEM_BLOCK_SIZE;
                _pCurrent = pFound;
                for (i=0; i<block_count; i++)
                {
                    _pCurrent->flag = 1;     /* allocate block */
                    _pCurrent = (USB_MHDR_T *)((UINT32)_pCurrent + USB_MEM_BLOCK_SIZE);
                }
                pFound->flag = 0x3;

                if (boundary > BOUNDARY_WORD)
                {
                    if (disable_ohci_irq)
                        ENABLE_OHCI_IRQ();
                    if (disable_ehci_irq)
                        ENABLE_EHCI_IRQ();
                    //printf("- 0x%x, %d\n", (int)pFound, wanted_size);
                    return (void *)((UINT32)pFound + USB_MEM_BLOCK_SIZE);
                }
                else
                {
                    //USB_debug("USB_malloc(%d,%d):%x\tsize:%d, C:0x%x, %d\n", wanted_size, boundary, (UINT32)pFound + sizeof(USB_MHDR_T), block_count * USB_MEM_BLOCK_SIZE, _pCurrent, block_count);
                    if (disable_ohci_irq)
                        ENABLE_OHCI_IRQ();
                    if (disable_ehci_irq)
                        ENABLE_EHCI_IRQ();
                    //printf("- 0x%x, %d\n", (int)pFound, wanted_size);
                    return (void *)((UINT32)pFound + sizeof(USB_MHDR_T));
                }
            }

            /* advance to the next block */
            _pCurrent = (USB_MHDR_T *)((UINT32)_pCurrent + USB_MEM_BLOCK_SIZE);
            if ((UINT32)_pCurrent >= _MemoryPoolEnd)
            {
                wrap = 1;
                _pCurrent = (USB_MHDR_T *)_MemoryPoolBase;   /* wrapped */
                found_size = -1;     /* reset accumlator */
            }
        }
    }
    while ((wrap == 0) || (_pCurrent < pPrimitivePos));

    printf("USB_malloc - No free memory!\n");
    if (disable_ohci_irq)
        ENABLE_OHCI_IRQ();
    if (disable_ehci_irq)
        ENABLE_EHCI_IRQ();
    return NULL;
}


void  USB_free(void *alloc_addr)
{
    USB_MHDR_T  *pMblk;
    UINT32  addr = (UINT32)alloc_addr;
    INT     i, count;
    int     disable_ohci_irq, disable_ehci_irq;

    if (IS_OHCI_IRQ_ENABLED())
        disable_ohci_irq = 1;
    else
        disable_ohci_irq = 0;

    if (IS_EHCI_IRQ_ENABLED())
        disable_ehci_irq = 1;
    else
        disable_ehci_irq = 0;

    //printf("USB_free: 0x%x\n", (int)alloc_addr);

    if ((addr < _MemoryPoolBase) || (addr >= _MemoryPoolEnd))
    {
        if (addr)
            free(alloc_addr);
        return;
    }

    if (disable_ohci_irq)
        DISABLE_OHCI_IRQ();
    if (disable_ehci_irq)
        DISABLE_EHCI_IRQ();

    //printf("USB_free:%x\n", (INT)addr+USB_MEM_BLOCK_SIZE);

    /* get the leading block address */
    if (addr % USB_MEM_BLOCK_SIZE == 0)
        addr -= USB_MEM_BLOCK_SIZE;
    else
        addr -= sizeof(USB_MHDR_T);

    if (addr % USB_MEM_BLOCK_SIZE != 0)
    {
        printf("USB_free fatal error on address: %x!!\n", (UINT32)alloc_addr);
        if (disable_ohci_irq)
            ENABLE_OHCI_IRQ();
        if (disable_ehci_irq)
            ENABLE_EHCI_IRQ();
        return;
    }

    pMblk = (USB_MHDR_T *)addr;
    if (pMblk->flag == 0)
    {
        printf("USB_free(), warning - try to free a free block: %x\n", (UINT32)alloc_addr);
        if (disable_ohci_irq)
            ENABLE_OHCI_IRQ();
        if (disable_ehci_irq)
            ENABLE_EHCI_IRQ();
        return;
    }
    if (pMblk->magic != USB_MEM_ALLOC_MAGIC)
    {
        printf("USB_free(), warning - try to free an unknow block at address:%x.\n", addr);
        if (disable_ohci_irq)
            ENABLE_OHCI_IRQ();
        if (disable_ehci_irq)
            ENABLE_EHCI_IRQ();
        return;
    }

    //_pCurrent = pMblk;

    //printf("+ 0x%x, %d\n", (int)pMblk, pMblk->bcnt);

    count = pMblk->bcnt;
    for (i = 0; i < count; i++)
    {
        pMblk->flag = 0;     /* release block */
        pMblk = (USB_MHDR_T *)((UINT32)pMblk + USB_MEM_BLOCK_SIZE);
    }

    _FreeMemorySize += count * USB_MEM_BLOCK_SIZE;
    _AllocatedMemorySize -= count * USB_MEM_BLOCK_SIZE;
    if (disable_ohci_irq)
        ENABLE_OHCI_IRQ();
    if (disable_ehci_irq)
        ENABLE_EHCI_IRQ();
    return;
}


/// @endcond HIDDEN_SYMBOLS

