#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// --- OPERATING SYSTEM SETTINGS ---
// We are running without an OS (Bare Metal)
#define NO_SYS 1
// We use the Pico SDK's internal locking, so we need lightweight protection
#define SYS_LIGHTWEIGHT_PROT 1

// --- CRITICAL FIXES FOR YOUR ERROR ---
// These MUST be 0 when NO_SYS is 1.
// You cannot use blocking sockets/netconn in bare metal mode.
#define LWIP_SOCKET 0
#define LWIP_NETCONN 0

// --- MEMORY SETTINGS ---
#define MEM_LIBC_MALLOC 0
#define MEM_ALIGNMENT 4
#define MEM_SIZE 4000
#define MEMP_NUM_TCP_SEG 32
#define MEMP_NUM_ARP_QUEUE 10
#define PBUF_POOL_SIZE 24

// --- PROTOCOL SETTINGS ---
#define LWIP_ARP 1
#define LWIP_ETHERNET 1
#define LWIP_ICMP 1
#define LWIP_RAW 1
#define LWIP_DHCP 1
#define LWIP_IPV4 1
#define LWIP_TCP 1
#define LWIP_UDP 1
#define LWIP_DNS 1

// --- TCP TUNING ---
#define TCP_MSS 1460
#define TCP_WND (8 * TCP_MSS)
#define TCP_SND_BUF (8 * TCP_MSS)

// --- DRIVER SETTINGS ---
#define LWIP_NETIF_STATUS_CALLBACK 1
#define LWIP_NETIF_LINK_CALLBACK 1
#define LWIP_NETIF_HOSTNAME 1
#define LWIP_CHKSUM_ALGORITHM 3
#define LWIP_DHCP_DOES_ACD_CHECK 0

#endif