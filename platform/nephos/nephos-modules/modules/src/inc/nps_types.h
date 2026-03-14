/* Copyright (C) 2020  MediaTek, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program.
 */

/* FILE NAME:   nps_types.h
 * PURPOSE:
 *      Define the commom data type in NPS SDK.
 * NOTES:
 */

#ifndef NPS_TYPES_H
#define NPS_TYPES_H

/* INCLUDE FILE DECLARATIONS
 */

#include <osal_types.h>

/* NAMING CONSTANT DECLARATIONS
 */

#define NPS_BIT_OFF 0
#define NPS_BIT_ON  1

#define NPS_PORT_INVALID    (0xFFFFFFFF)
#define NPS_SEG_INVALID     (0xFFFFFFFF)

/* for CPU Rx packet, indicate that the packet
 * is not received from remote switch
 */
#define NPS_PATH_INVALID    (0xFFFFFFFF)


#define NPS_SEMAPHORE_BINARY           (1)
#define NPS_SEMAPHORE_SYNC             (0)
#define NPS_SEMAPHORE_WAIT_FOREVER     (0xFFFFFFFF)

/* MACRO FUNCTION DECLARATIONS
 */
#if defined(NPS_EN_HOST_32_BIT_BIG_ENDIAN) || defined(NPS_EN_HOST_32_BIT_LITTLE_ENDIAN)
typedef unsigned int            NPS_HUGE_T;
#elif defined(NPS_EN_HOST_64_BIT_BIG_ENDIAN) || defined(NPS_EN_HOST_64_BIT_LITTLE_ENDIAN)
typedef unsigned long long int  NPS_HUGE_T;
#else
#error "The 32bit and 64bit compatible data type are not defined !!"
#endif

#if defined(NPS_EN_64BIT_ADDR)
typedef unsigned long long int                      NPS_ADDR_T;
#else
typedef NPS_HUGE_T                                  NPS_ADDR_T;
#endif

#if defined(NPS_EN_HOST_64_BIT_BIG_ENDIAN) || defined(NPS_EN_HOST_64_BIT_LITTLE_ENDIAN) || defined(NPS_EN_64BIT_ADDR)
#define NPS_ADDR_64_HI(__addr__)                    ((__addr__) >> 32)
#define NPS_ADDR_64_LOW(__addr__)                   ((__addr__) & 0xFFFFFFFF)
#define NPS_ADDR_32_TO_64(__hi32__,__low32__)       (((unsigned long long int)(__low32__)) |             \
                                                     (((unsigned long long int)(__hi32__)) << 32))
#else
#define NPS_ADDR_64_HI(__addr__)                    (0)
#define NPS_ADDR_64_LOW(__addr__)                   (__addr__)
#define NPS_ADDR_32_TO_64(__hi32__,__low32__)       (__low32__)
#endif

#define NPS_BITMAP_SIZE(bit_num)                    ((((bit_num) - 1) / 32) + 1)
#define NPS_IPV4_IS_MULTICAST(addr)                 (0xE0000000 == ((addr) & 0xF0000000))
#define NPS_IPV6_IS_MULTICAST(addr)                 (0xFF == (((UI8_T *)(addr))[0]))
#define NPS_MAC_IS_MULTICAST(mac)                   ((mac[0]) & (0x1))

/* DATA TYPE DECLARATIONS
 */
typedef UI8_T   NPS_BIT_MASK_8_T;
typedef UI16_T  NPS_BIT_MASK_16_T;
typedef UI32_T  NPS_BIT_MASK_32_T;
typedef UI64_T  NPS_BIT_MASK_64_T;

typedef UI8_T   NPS_MAC_T[6];
typedef UI32_T  NPS_IPV4_T;
typedef UI8_T   NPS_IPV6_T[16];

typedef UI32_T  NPS_TIME_T;

/* Bridge Domain id data type. */
typedef UI32_T NPS_BRIDGE_DOMAIN_T;

/* TRILL nickname type. */
typedef UI16_T NPS_TRILL_NICKNAME_T;

typedef union NPS_IP_U
{

    NPS_IPV4_T     ipv4_addr;
    NPS_IPV6_T     ipv6_addr;

}NPS_IP_T;

typedef struct NPS_IP_ADDR_S
{
   NPS_IP_T      ip_addr;
   BOOL_T        ipv4 ;
}NPS_IP_ADDR_T;

/* Tunnel type*/
typedef enum
{
    NPS_TUNNEL_TYPE_IPV4INIPV4 = 0,  /* RFC2003, IPv4-in-IPv4 tunnel */
    NPS_TUNNEL_TYPE_IPV4INIPV6,      /* RFC2003, IPv4-in-IPv6 tunnel */
    NPS_TUNNEL_TYPE_IPV6INIPV4,      /* RFC2003, IPv6-in-IPv4 tunnel */
    NPS_TUNNEL_TYPE_IPV6INIPV6,      /* RFC2003, IPv6-in-IPv6 tunnel */
    NPS_TUNNEL_TYPE_GREIPV4INIPV4,   /* RFC2784/RFC2890,GRE IPv4-in-IPv4 tunnel */
    NPS_TUNNEL_TYPE_GREIPV6INIPV4,   /* RFC2784/RFC2890,GRE IPv6-in-IPv4 tunnel */
    NPS_TUNNEL_TYPE_GREIPV4INIPV6,   /* RFC2784/RFC2890,GRE IPv4-in-IPv6 tunnel */
    NPS_TUNNEL_TYPE_GREIPV6INIPV6,   /* RFC2784/RFC2890,GRE IPv6-in-IPv6 tunnel */
    NPS_TUNNEL_TYPE_GRE_NSH,
    NPS_TUNNEL_TYPE_6TO4,            /* RFC3056, 6to4 tunnel*/
    NPS_TUNNEL_TYPE_ISATAP,          /* RFC5214, ISATAP tunnel */
    NPS_TUNNEL_TYPE_NVGRE_L2,
    NPS_TUNNEL_TYPE_NVGRE_V4,
    NPS_TUNNEL_TYPE_NVGRE_V6,
    NPS_TUNNEL_TYPE_NVGRE_NSH,
    NPS_TUNNEL_TYPE_VXLAN,
    NPS_TUNNEL_TYPE_GTP_V4,
    NPS_TUNNEL_TYPE_GTP_V6,
    NPS_TUNNEL_TYPE_MPLSINGRE,
    NPS_TUNNEL_TYPE_VXLANGPE_L2,
    NPS_TUNNEL_TYPE_VXLANGPE_V4,
    NPS_TUNNEL_TYPE_VXLANGPE_V6,
    NPS_TUNNEL_TYPE_VXLANGPE_NSH,
    NPS_TUNNEL_TYPE_FLEX0_L2,
    NPS_TUNNEL_TYPE_FLEX0_V4,
    NPS_TUNNEL_TYPE_FLEX0_V6,
    NPS_TUNNEL_TYPE_FLEX0_NSH,
    NPS_TUNNEL_TYPE_FLEX1_L2,
    NPS_TUNNEL_TYPE_FLEX1_V4,
    NPS_TUNNEL_TYPE_FLEX1_V6,
    NPS_TUNNEL_TYPE_FLEX1_NSH,
    NPS_TUNNEL_TYPE_FLEX2_L2,
    NPS_TUNNEL_TYPE_FLEX2_V4,
    NPS_TUNNEL_TYPE_FLEX2_V6,
    NPS_TUNNEL_TYPE_FLEX2_NSH,
    NPS_TUNNEL_TYPE_FLEX3_L2,
    NPS_TUNNEL_TYPE_FLEX3_V4,
    NPS_TUNNEL_TYPE_FLEX3_V6,
    NPS_TUNNEL_TYPE_FLEX3_NSH,
    NPS_TUNNEL_TYPE_LAST
} NPS_TUNNEL_TYPE_T;

/* tunnel key */
typedef struct NPS_TUNNEL_KEY_S
{
    NPS_IP_ADDR_T       src_ip;           /* key: The outer source IP address used by tunnel encapsulation.*/
    NPS_IP_ADDR_T       dst_ip;           /* key: The outer destination IP address used by tunnel encapsulation.
                                           * For automatic tunnel, this is not required. If not specified,
                                           * its ip address value must be set to 0, but the IP version
                                           * must be same with src_ip.
                                           */
    NPS_TUNNEL_TYPE_T   tunnel_type;      /*key: The tunnel type.*/
}NPS_TUNNEL_KEY_T;

typedef UI16_T NPS_VLAN_T;
typedef UI32_T NPS_PORT_T;

typedef enum{
    NPS_PORT_TYPE_NORMAL = 0,
    NPS_PORT_TYPE_UNIT_PORT,
    NPS_PORT_TYPE_LAG,
    NPS_PORT_TYPE_VM_ETAG,
    NPS_PORT_TYPE_VM_VNTAG,
    NPS_PORT_TYPE_VM_VEPA,
    NPS_PORT_TYPE_FCOE,
    NPS_PORT_TYPE_IP_TUNNEL,
    NPS_PORT_TYPE_TRILL,
    NPS_PORT_TYPE_MPLS,
    NPS_PORT_TYPE_MPLS_PW,
    NPS_PORT_TYPE_CPU_PORT,
    NPS_PORT_TYPE_SFC,
    NPS_PORT_TYPE_LAST
}NPS_PORT_TYPE_T;

/*support Green/Yellow/Red color*/
typedef enum
{
    NPS_COLOR_GREEN = 0,
    NPS_COLOR_YELLOW,
    NPS_COLOR_RED,
    NPS_COLOR_LAST
}NPS_COLOR_T;
typedef enum
{
    NPS_FWD_ACTION_FLOOD = 0,
    NPS_FWD_ACTION_NORMAL,
    NPS_FWD_ACTION_DROP,
    NPS_FWD_ACTION_COPY_TO_CPU,
    NPS_FWD_ACTION_REDIRECT_TO_CPU,
    NPS_FWD_ACTION_FLOOD_COPY_TO_CPU,
    NPS_FWD_ACTION_DROP_COPY_TO_CPU,
    NPS_FWD_ACTION_LAST
} NPS_FWD_ACTION_T;

typedef NPS_HUGE_T  NPS_THREAD_ID_T;
typedef NPS_HUGE_T  NPS_SEMAPHORE_ID_T;
typedef NPS_HUGE_T  NPS_ISRLOCK_ID_T;
typedef NPS_HUGE_T  NPS_IRQ_FLAGS_T;

typedef enum
{
    NPS_DIR_INGRESS  = 0,
    NPS_DIR_EGRESS,
    NPS_DIR_BOTH,
    NPS_DIR_LAST
}NPS_DIR_T;

typedef enum
{
    NPS_VLAN_ACTION_SET,
    NPS_VLAN_ACTION_KEEP,
    NPS_VLAN_ACTION_REMOVE,
    NPS_VLAN_ACTION_LAST
} NPS_VLAN_ACTION_T;

/* VLAN Precedence */
/* 000 = SUBNET_PROTOCOL_MAC_PORT
 * 001 = SUBNET_MAC_PROTOCOL_PORT
 * 010 = PROTOCOL_SUBNET_MAC_PORT
 * 011 = PROTOCOL_MAC_SUBNET_PORT
 * 100 = MAC_SUBNET_PROTOCOL_PORT
 * 101 = MAC_PROTOCOL_SUBNET_PORT
 */
typedef enum
{
    NPS_VLAN_PRECEDENCE_SUBNET_MAC_PROTOCOL_PORT = 1,
    NPS_VLAN_PRECEDENCE_MAC_SUBNET_PROTOCOL_PORT = 4,
    NPS_VLAN_PRECEDENCE_PORT_ONLY                = 7,
    NPS_VLAN_PRECEDENCE_FAVOR_TYPE               = 8,
    NPS_VLAN_PRECEDENCE_FAVOR_ADDR               = 9,
    NPS_VLAN_PRECEDENCE_LAST
} NPS_VLAN_PRECEDENCE_T;

/* VLAN Tag Type */
typedef enum
{
    NPS_VLAN_TAG_NONE = 0,      /* UnTag                                */
    NPS_VLAN_TAG_SINGLE_PRI,    /* Single Customer/Service Priority Tag */
    NPS_VLAN_TAG_SINGLE,        /* Single Customer/Service Tag          */
    NPS_VLAN_TAG_DOUBLE_PRI,    /* Double Tag with any VID=0            */
    NPS_VLAN_TAG_DOUBLE,        /* Double Tag                           */
    NPS_VLAN_TAG_LAST
} NPS_VLAN_TAG_T;

typedef struct NPS_BUM_INFO_S
{
    UI32_T    mcast_id;
    UI32_T    group_label;      /* l2 da group label */
    UI32_T    vid;              /* used when FLAGS_ADD_VID is set */

#define NPS_BUM_INFO_FLAGS_MCAST_VALID    (1 << 0)
#define NPS_BUM_INFO_FLAGS_TO_CPU         (1 << 1)
#define NPS_BUM_INFO_FLAGS_ADD_VID        (1 << 2) /* single tag to double tag (i.e) QinQ */
#define NPS_BUM_INFO_FLAGS_TRILL_ALL_TREE (1 << 3)
    UI32_T    flags;
} NPS_BUM_INFO_T;

typedef enum
{
    NPS_PHY_TYPE_INTERNAL = 0x0,
    NPS_PHY_TYPE_EXTERNAL,
    NPS_PHY_TYPE_LAST
} NPS_PHY_TYPE_T;

typedef enum
{
    NPS_PHY_DEVICE_ADDR_PMA_PMD    = 1,
    NPS_PHY_DEVICE_ADDR_WIS        = 2,
    NPS_PHY_DEVICE_ADDR_PCS        = 3,
    NPS_PHY_DEVICE_ADDR_PHY_XS     = 4,
    NPS_PHY_DEVICE_ADDR_DTE_XS     = 5,
    NPS_PHY_DEVICE_ADDR_TC         = 6,
    NPS_PHY_DEVICE_ADDR_AN         = 7,
    NPS_PHY_DEVICE_ADDR_VENDOR_1   = 30,
    NPS_PHY_DEVICE_ADDR_VENDOR_2   = 31,
    NPS_PHY_DEVICE_ADDR_LAST
} NPS_PHY_DEVICE_ADDR_T;

typedef struct NPS_RANGE_INFO_S
{
    UI32_T    min_id;
    UI32_T    max_id;
    UI32_T    max_member_cnt;

#define NPS_RANGE_INFO_FLAGS_MAX_MEMBER_CNT    (1 << 0)
    UI32_T    flags;
} NPS_RANGE_INFO_T;

/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */

#endif  /* NPS_TYPES_H */
