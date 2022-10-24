/**************************************************************************//**
 * @file     ethernetif.c
 * @brief    Ethernet interface header
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__


#include "lwip/err.h"
#include "lwip/netif.h"

err_t ethernetif_init0(struct netif *netif);
err_t ethernetif_init1(struct netif *netif);
void ethernetif_input0(u16_t len, u8_t *buf);
void ethernetif_input1(u16_t len, u8_t *buf);

#endif
