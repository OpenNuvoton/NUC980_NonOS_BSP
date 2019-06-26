/**************************************************************************//**
 * @file     main.c
 * @brief    LwIP httpd sample code with  EMAC interfaces enabled
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "nuc980.h"
#include "sys.h"
#include "etimer.h"
#include "netif/ethernetif.h"
#include "netif/etharp.h"
#include "lwip/init.h"
#include "lwip/tcp.h"
#include "lwip/stats.h"
#include "lwip/memp.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"

//#include "lwip/timers.h"

//#define USE_DHCP
#ifdef USE_DHCP
#include "lwip/dhcp.h"
#endif

/* web page*/
static CHAR idx[] =
{
    0x48, 0x54, 0x54, 0x50, 0x2f, 0x31, 0x2e, 0x30, 0x20, 0x32,
    0x30, 0x30, 0x20, 0x4f, 0x4b, 0xd, 0xa, 0x53, 0x65, 0x72,
    0x76, 0x65, 0x72, 0x3a, 0x20, 0x6c, 0x77, 0x49, 0x50, 0x2f,
    0x70, 0x72, 0x65, 0x2d, 0x30, 0x2e, 0x36, 0x20, 0x28, 0x68,
    0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x77, 0x77, 0x77, 0x2e,
    0x73, 0x69, 0x63, 0x73, 0x2e, 0x73, 0x65, 0x2f, 0x7e, 0x61,
    0x64, 0x61, 0x6d, 0x2f, 0x6c, 0x77, 0x69, 0x70, 0x2f, 0x29,
    0xd, 0xa, 0x43, 0x6f, 0x6e, 0x74, 0x65, 0x6e, 0x74, 0x2d,
    0x74, 0x79, 0x70, 0x65, 0x3a, 0x20, 0x74, 0x65, 0x78, 0x74,
    0x2f, 0x68, 0x74, 0x6d, 0x6c, 0xd, 0xa, 0xd, 0xa, 0x3c,
    0x48, 0x54, 0x4d, 0x4c, 0x3e, 0xd, 0xa, 0x3c, 0x42, 0x4f,
    0x44, 0x59, 0x3e, 0xd, 0xa, 0x4e, 0x55, 0x43, 0x39, 0x38,
    0x30, 0x20, 0x77, 0x65, 0x62, 0x20, 0x73, 0x65, 0x72, 0x76,
    0x65, 0x72, 0x20, 0x64, 0x65, 0x6d, 0x6f, 0x20, 0x62, 0x61,
    0x73, 0x65, 0x64, 0x20, 0x6f, 0x6e, 0x20, 0x6c, 0x77, 0x49,
    0x50, 0xd, 0xa, 0x3c, 0x2f, 0x42, 0x4f, 0x44, 0x59, 0x3e,
    0xd, 0xa, 0x3c, 0x2f, 0x48, 0x54, 0x4d, 0x4c, 0x3e, 0xd,
    0xa
};
static CHAR err404[] =
{
    0x48, 0x54, 0x54, 0x50, 0x2f, 0x31, 0x2e, 0x30, 0x20, 0x34,
    0x30, 0x34, 0x20, 0x46, 0x69, 0x6c, 0x65, 0x20, 0x6e, 0x6f,
    0x74, 0x20, 0x66, 0x6f, 0x75, 0x6e, 0x64, 0xd, 0xa, 0x53,
    0x65, 0x72, 0x76, 0x65, 0x72, 0x3a, 0x20, 0x6c, 0x77, 0x49,
    0x50, 0x2f, 0x70, 0x72, 0x65, 0x2d, 0x30, 0x2e, 0x36, 0x20,
    0x28, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x77, 0x77,
    0x77, 0x2e, 0x73, 0x69, 0x63, 0x73, 0x2e, 0x73, 0x65, 0x2f,
    0x7e, 0x61, 0x64, 0x61, 0x6d, 0x2f, 0x6c, 0x77, 0x69, 0x70,
    0x2f, 0x29, 0xd, 0xa, 0x43, 0x6f, 0x6e, 0x74, 0x65, 0x6e,
    0x74, 0x2d, 0x74, 0x79, 0x70, 0x65, 0x3a, 0x20, 0x74, 0x65,
    0x78, 0x74, 0x2f, 0x68, 0x74, 0x6d, 0x6c, 0xd, 0xa, 0xd,
    0xa, 0x3c, 0x48, 0x54, 0x4d, 0x4c, 0x3e, 0xd, 0xa, 0x3c,
    0x42, 0x4f, 0x44, 0x59, 0x3e, 0xd, 0xa, 0x45, 0x52, 0x52,
    0x4f, 0x52, 0x20, 0x2d, 0x20, 0x46, 0x69, 0x6c, 0x65, 0x20,
    0x6e, 0x6f, 0x74, 0x20, 0x66, 0x6f, 0x75, 0x6e, 0x64, 0xd,
    0xa, 0x3c, 0x2f, 0x42, 0x4f, 0x44, 0x59, 0x3e, 0xd, 0xa,
    0x3c, 0x2f, 0x48, 0x54, 0x4d, 0x4c, 0x3e, 0xd, 0xa
};

unsigned char my_mac_addr0[6] = {0x00, 0x00, 0x00, 0x55, 0x66, 0x77};
unsigned char my_mac_addr1[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

/* recv callback function */
static err_t http_recv(void *arg, struct tcp_pcb *pcb, struct pbuf*p, err_t err)
{
    CHAR *rq;
    /* If we got a NULL pbuf in p, the remote end has closed
    the connection.*/
    if (p != NULL)
    {

        /* The payload pointer in the pbuf contains the data
        in the TCP segment.*/

        rq = p->payload;
        /* Get and response the request file.
           To support other webpage, extent the if statement below
           If the object is larger than single TCP payload, need to send reset
           for the content in main loop and then close the pcb */
        if ((strncmp(rq, "GET /index.htm", 14) == 0)||(strncmp(rq, "GET / ", 6) == 0))
        {
            /* Send the webpage to the remote host. A zero
            in the last argument means that the data should
            not be copied into internal buffers. */
            tcp_recved(pcb, p->tot_len);
            tcp_write(pcb, idx, sizeof(idx) -1, 0);
            tcp_close(pcb);
        }
        else
        {
            // err 404
            tcp_recved(pcb, p->tot_len);
            tcp_write(pcb, err404, sizeof(err404) -1, 0);
            tcp_close(pcb);
        }
        /* Free the pbuf. */
        pbuf_free(p);
    }

    return(ERR_OK);
}

/*accept callback function */
static err_t http_acpt(void *arg,struct tcp_pcb *pcb, err_t err)
{
    /* Setup the function http_recv() to be called when data arrives.*/
    tcp_recv(pcb, http_recv);
    return ERR_OK;
}

/*httpd initialization function.*/
static void httpd_init(void)
{
    struct tcp_pcb *pcb, *pcb_listen;
    /*Create a new TCP PCB.*/
    pcb = tcp_new();
    /*Bind the PCB to TCP port 80.*/
    if (tcp_bind(pcb,NULL,80) != ERR_OK)
        printf("bind error\n");
    /*Change TCP state to LISTEN.*/
    pcb_listen = tcp_listen(pcb);
    /*Setup http_accet() function to be called
    when a new connection arrives.*/
    tcp_accept(pcb_listen, http_acpt);
}


ip4_addr_t gw0, ipaddr0, netmask0;
struct netif netif0;
ip4_addr_t gw1, ipaddr1, netmask1;
struct netif netif1;

uint32_t sysTick = 0;
void ETMR0_IRQHandler(void)
{
    sysTick++;
    // clear timer interrupt flag
    ETIMER_ClearIntFlag(0);
}


/* network initialization function.*/
static void net_init(void)
{

#ifdef USE_DHCP

    IP4_ADDR(&gw0, 0, 0, 0, 0);
    IP4_ADDR(&ipaddr0, 0, 0, 0, 0);
    IP4_ADDR(&netmask0, 0, 0, 0, 0);
    IP4_ADDR(&gw1, 192, 168, 5, 1);
    IP4_ADDR(&ipaddr1, 192, 168, 5, 227);
    IP4_ADDR(&netmask1, 255, 255, 255, 0);
#else

    IP4_ADDR(&gw0, 192, 168, 0, 1);
    IP4_ADDR(&ipaddr0, 192, 168, 0, 227);
    IP4_ADDR(&netmask0, 255, 255, 255, 0);
    IP4_ADDR(&gw1, 192, 168, 5, 1);
    IP4_ADDR(&ipaddr1, 192, 168, 5, 227);
    IP4_ADDR(&netmask1, 255, 255, 255, 0);
#endif

    lwip_init();

    netif_add(&netif0, &ipaddr0, &netmask0, &gw0, NULL, ethernetif_init0, ethernet_input);
    netif_add(&netif1, &ipaddr1, &netmask1, &gw1, NULL, ethernetif_init1, ethernet_input);
    netif_set_default(&netif0);


    netif_set_up(&netif0);
    netif_set_up(&netif1);


#ifdef USE_DHCP
    dhcp_start(&netif0);
#endif

}

void UART_Init(void)
{
    /* enable UART0 clock */
    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | 0x10000);

    /* GPF11, GPF12 */
    outpw(REG_SYS_GPF_MFPH, (inpw(REG_SYS_GPF_MFPH) & 0xfff00fff) | 0x11000); // UART0 multi-function

    /* UART0 line configuration for (115200,n,8,1) */
    outpw(REG_UART0_LCR, inpw(REG_UART0_LCR) | 0x07);
    outpw(REG_UART0_BAUD, 0x30000066); /* 12MHz reference clock input, 115200 */
}

void TIMER_Init(void)
{
    // lwIP needs a timer @ 100Hz. To use another timer source, please modify sys_now() in sys_arch.c as well
    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | (1 << 8));
    // Set timer frequency to 100 Hz
    ETIMER_Open(0, ETIMER_PERIODIC_MODE, 100);
    // Enable timer interrupt
    ETIMER_EnableInt(0);
    sysInstallISR(IRQ_LEVEL_1, IRQ_TIMER0, (PVOID)ETMR0_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_TIMER0);

    // Start Timer 0
    ETIMER_Start(0);
}


extern void chk_link0(void *arg);
extern void chk_link1(void *arg);
int main(void)
{


    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    sysSetLocalInterrupt(ENABLE_IRQ);

    UART_Init();
    printf("lwIP httpd demo\n");
    TIMER_Init();

    net_init();
    httpd_init();
    sys_timeout(2000, chk_link0, NULL);
    sys_timeout(2000, chk_link1, NULL);
    while (1)
        sys_check_timeouts();  // All network traffic is handled in interrupt handler
}

