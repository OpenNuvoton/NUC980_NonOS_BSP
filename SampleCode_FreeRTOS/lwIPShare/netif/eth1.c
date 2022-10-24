/**************************************************************************//**
 * @file     eth1.c
 * @brief    EMAC1 driver source file
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include "nuc980.h"
#include "sys.h"
#include "netif/eth.h"
#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/timeouts.h"

#define ETH1_TRIGGER_RX()    outpw(REG_EMAC1_RSDR, 0)
#define ETH1_TRIGGER_TX()    outpw(REG_EMAC1_TSDR, 0)
#define ETH1_ENABLE_TX()     outpw(REG_EMAC1_MCMDR, inpw(REG_EMAC1_MCMDR) | 0x100)
#define ETH1_ENABLE_RX()     outpw(REG_EMAC1_MCMDR, inpw(REG_EMAC1_MCMDR) | 0x1)
#define ETH1_DISABLE_TX()    outpw(REG_EMAC1_MCMDR, inpw(REG_EMAC1_MCMDR) & ~0x100)
#define ETH1_DISABLE_RX()    outpw(REG_EMAC1_MCMDR, inpw(REG_EMAC1_MCMDR) & ~0x1)


static struct eth_descriptor rx_desc[RX_DESCRIPTOR_NUM] __attribute__ ((aligned(32)));
static struct eth_descriptor tx_desc[TX_DESCRIPTOR_NUM] __attribute__ ((aligned(32)));

static struct eth_descriptor volatile *cur_tx_desc_ptr, *cur_rx_desc_ptr, *fin_tx_desc_ptr;

static u8_t rx_buf[RX_DESCRIPTOR_NUM][PACKET_BUFFER_SIZE];
static u8_t tx_buf[TX_DESCRIPTOR_NUM][PACKET_BUFFER_SIZE];
static int plugged = 0;

extern void ethernetif_input1(u16_t len, u8_t *buf);

/* Write PHY register */
static void mdio_write(u8_t addr, u8_t reg, u16_t val)
{

    outpw(REG_EMAC1_MIID, val);
    outpw(REG_EMAC1_MIIDA, (addr << 8) | reg | 0xB0000);

    while (inpw(REG_EMAC1_MIIDA) & 0x20000);    // wait busy flag clear

}

/* Read PHY register */
static u16_t mdio_read(u8_t addr, u8_t reg)
{
    outpw(REG_EMAC1_MIIDA, (addr << 8) | reg | 0xA0000);
    while (inpw(REG_EMAC1_MIIDA) & 0x20000);    // wait busy flag clear

    return inpw(REG_EMAC1_MIID);
}

/* Reset PHY chip and get auto-negotiation result */
static int reset_phy(void)
{

    u16_t reg;
    u32_t delay;


    mdio_write(CONFIG_PHY_ADDR, MII_BMCR, BMCR_RESET);

    delay = 2000;
    while(delay-- > 0)
    {
        if((mdio_read(CONFIG_PHY_ADDR, MII_BMCR) & BMCR_RESET) == 0)
            break;

    }

    if(delay == 0)
    {
        printf("Reset phy failed\n");
        return(-1);
    }

    mdio_write(CONFIG_PHY_ADDR, MII_ADVERTISE, ADVERTISE_CSMA |
               ADVERTISE_10HALF |
               ADVERTISE_10FULL |
               ADVERTISE_100HALF |
               ADVERTISE_100FULL);

    reg = mdio_read(CONFIG_PHY_ADDR, MII_BMCR);
    mdio_write(CONFIG_PHY_ADDR, MII_BMCR, reg | BMCR_ANRESTART);

    delay = 200000;
    while(delay-- > 0)
    {
        if((mdio_read(CONFIG_PHY_ADDR, MII_BMSR) & (BMSR_ANEGCOMPLETE | BMSR_LSTATUS))
                == (BMSR_ANEGCOMPLETE | BMSR_LSTATUS))
            break;
    }

    if(delay == 0)
    {
        printf("AN failed. Set to 100 FULL\n");
        outpw(REG_EMAC1_MCMDR, inpw(REG_EMAC1_MCMDR) | 0x140000);
        plugged = 0;
        return(-1);
    }
    else
    {
        reg = mdio_read(CONFIG_PHY_ADDR, MII_LPA);

        if(reg & ADVERTISE_100FULL)
        {
            outpw(REG_EMAC1_MCMDR, inpw(REG_EMAC1_MCMDR) | 0x140000);
        }
        else if(reg & ADVERTISE_100HALF)
        {
            outpw(REG_EMAC1_MCMDR, (inpw(REG_EMAC1_MCMDR) & ~0x40000) | 0x100000);
        }
        else if(reg & ADVERTISE_10FULL)
        {
            outpw(REG_EMAC1_MCMDR, (inpw(REG_EMAC1_MCMDR) & ~0x100000) | 0x40000);
        }
        else
        {
            outpw(REG_EMAC1_MCMDR, inpw(REG_EMAC1_MCMDR) & ~0x140000);
        }
    }

    return(0);
}


static void init_tx_desc(void)
{
    u32_t i;


    cur_tx_desc_ptr = fin_tx_desc_ptr = (struct eth_descriptor *)((UINT)(&tx_desc[0]) | 0x80000000);

    for(i = 0; i < TX_DESCRIPTOR_NUM; i++)
    {
        tx_desc[i].status1 = TXFD_PADEN | TXFD_CRCAPP | TXFD_INTEN;
        tx_desc[i].buf = (unsigned char *)((UINT)(&tx_buf[i][0]) | 0x80000000);
        tx_desc[i].status2 = 0;
        tx_desc[i].next = (struct eth_descriptor *)((UINT)(&tx_desc[(i + 1) % TX_DESCRIPTOR_NUM]) | 0x80000000);
    }
    outpw(REG_EMAC1_TXDLSA, (unsigned int)&tx_desc[0] | 0x80000000);
    return;
}

static void init_rx_desc(void)
{
    u32_t i;


    cur_rx_desc_ptr = (struct eth_descriptor *)((UINT)(&rx_desc[0]) | 0x80000000);

    for(i = 0; i < RX_DESCRIPTOR_NUM; i++)
    {
        rx_desc[i].status1 = OWNERSHIP_EMAC;
        rx_desc[i].buf = (unsigned char *)((UINT)(&rx_buf[i][0]) | 0x80000000);
        rx_desc[i].status2 = 0;
        rx_desc[i].next = (struct eth_descriptor *)((UINT)(&rx_desc[(i + 1) % RX_DESCRIPTOR_NUM]) | 0x80000000);
    }
    outpw(REG_EMAC1_RXDLSA, (unsigned int)&rx_desc[0] | 0x80000000);
    return;
}

static void set_mac_addr(u8_t *addr)
{

    outpw(REG_EMAC1_CAMxM_Reg(0), (addr[0] << 24) |
          (addr[1] << 16) |
          (addr[2] << 8) |
          addr[3]);
    outpw(REG_EMAC1_CAMxL_Reg(0), (addr[4] << 24) |
          (addr[5] << 16));
    outpw(REG_EMAC1_CAMCMR, 0x16);
    outpw(REG_EMAC1_CAMEN, 1);    // Enable CAM entry 0

}


void ETH1_halt(void)
{

    outpw(REG_EMAC1_MCMDR, inpw(REG_EMAC1_MCMDR) & ~0x101); // disable tx/rx on

}

void ETH1_RX_IRQHandler(void)
{
    unsigned int status;

    status = inpw(REG_EMAC1_MISTA) & 0xFFFF;
    outpw(REG_EMAC1_MISTA, status);

    if (status & 0x800)
    {
        // Shouldn't goes here, unless descriptor corrupted
    }

    do
    {
        status = cur_rx_desc_ptr->status1;

        if(status & OWNERSHIP_EMAC)
            break;

        if (status & RXFD_RXGD)
        {
            ethernetif_input1(status & 0xFFFF, cur_rx_desc_ptr->buf);
        }

        cur_rx_desc_ptr->status1 = OWNERSHIP_EMAC;
        cur_rx_desc_ptr = cur_rx_desc_ptr->next;

    }
    while (1);

    ETH1_TRIGGER_RX();

}

void ETH1_TX_IRQHandler(void)
{
    unsigned int cur_entry, status;

    status = inpw(REG_EMAC1_MISTA) & 0xFFFF0000;
    outpw(REG_EMAC1_MISTA, status);

    if(status & 0x1000000)
    {
        // Shouldn't goes here, unless descriptor corrupted
        return;
    }

    cur_entry = inpw(REG_EMAC1_CTXDSA);

    while (cur_entry != (u32_t)fin_tx_desc_ptr)
    {
        fin_tx_desc_ptr = fin_tx_desc_ptr->next;
    }

}

/* Check Ethernet link status */
void chk_link1(void *arg)
{
    unsigned int reg;

    LWIP_UNUSED_ARG(arg);
    reg = mdio_read(CONFIG_PHY_ADDR, MII_BMSR);

    if (reg & BMSR_LSTATUS)
    {
        if (!plugged)
        {
            plugged = 1;
            reset_phy();
            outpw(REG_EMAC1_MCMDR, inpw(REG_EMAC1_MCMDR) | 0x101);
        }
    }
    else
    {
        if (plugged)
        {
            plugged = 0;
            outpw(REG_EMAC1_MCMDR, inpw(REG_EMAC1_MCMDR) & ~0x101);
        }
    }
    sys_timeout(2000, chk_link1, NULL);
}

void ETH1_init(u8_t *mac_addr)
{

    outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | (1 << 17));             // EMAC1 clk
    outpw(REG_CLK_DIVCTL8, (inpw(REG_CLK_DIVCTL8) & ~0xFF) | 0xA0);     // MDC clk divider

    // Multi function pin setting
    outpw(REG_SYS_GPF_MFPL, 0x11111111);
    outpw(REG_SYS_GPF_MFPH, (inpw(REG_SYS_GPF_MFPH) & ~0xFF) | 0x11);

    // Reset MAC
    outpw(REG_EMAC1_MCMDR, 0x1000000);

    init_tx_desc();
    init_rx_desc();
    sysFlushCache(D_CACHE);
    set_mac_addr(mac_addr);  // need to reconfigure hardware address 'cos we just RESET emc...
    reset_phy();

    outpw(REG_EMAC1_MCMDR, inpw(REG_EMAC1_MCMDR) | 0x121); // strip CRC, TX on, Rx on
    outpw(REG_EMAC1_MIEN, inpw(REG_EMAC1_MIEN) | 0x01250C11);  // Except tx/rx ok, enable rdu, txabt, tx/rx bus error.

    sysInstallISR(IRQ_LEVEL_1, IRQ_EMC1_TX, (PVOID)ETH1_TX_IRQHandler);
    sysInstallISR(IRQ_LEVEL_1, IRQ_EMC1_RX, (PVOID)ETH1_RX_IRQHandler);
    sysEnableInterrupt(IRQ_EMC1_TX);
    sysEnableInterrupt(IRQ_EMC1_RX);

    ETH1_TRIGGER_RX();

    // check link status every 2 sec
    //sys_timeout(2000, chk_link1, NULL);
}


u8_t *ETH1_get_tx_buf(void)
{
    if(cur_tx_desc_ptr->status1 & OWNERSHIP_EMAC)
        return(NULL);
    else
        return(cur_tx_desc_ptr->buf);
}

void ETH1_trigger_tx(u16_t length, struct pbuf *p)
{
    struct eth_descriptor volatile *desc;
    cur_tx_desc_ptr->status2 = (unsigned int)length;
    desc = cur_tx_desc_ptr->next;    // in case TX is transmitting and overwrite next pointer before we can update cur_tx_desc_ptr
    cur_tx_desc_ptr->status1 |= OWNERSHIP_EMAC;
    cur_tx_desc_ptr = desc;

    ETH1_TRIGGER_TX();

}


