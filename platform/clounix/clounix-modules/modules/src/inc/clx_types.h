/*
 * Copyright 2022 Clounix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 */

/* FILE NAME:   clx_types.h
 * PURPOSE:
 *      Define the commom data type in CLX SDK.
 * NOTES:
 */

#ifndef CLX_TYPES_H
#define CLX_TYPES_H

/* INCLUDE FILE DECLARATIONS
 */

#include <osal/osal_types.h>

/* NAMING CONSTANT DECLARATIONS
 */

#define CLX_BIT_OFF 0
#define CLX_BIT_ON  1

#define CLX_INVALID_ID      (0xFFFFFFFF)
#define CLX_PORT_INVALID    (CLX_INVALID_ID)
#define CLX_SEG_INVALID     (CLX_INVALID_ID)

/* for CPU Rx packet, indicate that the packet
 * is not received from remote switch
 */
#define CLX_PATH_INVALID    (CLX_INVALID_ID)

#define CLX_SEMAPHORE_BINARY           (1)
#define CLX_SEMAPHORE_SYNC             (0)
#define CLX_SEMAPHORE_WAIT_FOREVER     (0xFFFFFFFF)

/* MACRO FUNCTION DECLARATIONS
 */
#if defined(CLX_EN_HOST_32_BIT_BIG_ENDIAN) || defined(CLX_EN_HOST_32_BIT_LITTLE_ENDIAN)
typedef unsigned int            CLX_HUGE_T;
#elif defined(CLX_EN_HOST_64_BIT_BIG_ENDIAN) || defined(CLX_EN_HOST_64_BIT_LITTLE_ENDIAN)
typedef unsigned long long int  CLX_HUGE_T;
#else
#error "The 32bit and 64bit compatible data type are not defined !!"
#endif

#if defined(CLX_EN_HOST_64_BIT_BIG_ENDIAN) || defined(CLX_EN_HOST_64_BIT_LITTLE_ENDIAN) || defined(CLX_EN_64BIT_ADDR)
typedef unsigned long long int  CLX_ADDR_T;
#else
typedef unsigned int            CLX_ADDR_T;
#endif

#if defined(CLX_EN_HOST_64_BIT_BIG_ENDIAN) || defined(CLX_EN_HOST_64_BIT_LITTLE_ENDIAN) || defined(CLX_EN_64BIT_ADDR)
#define CLX_ADDR_PRINT "0x%llx"
#else
#define CLX_ADDR_PRINT "0x%x"
#endif

#if defined(CLX_EN_HOST_64_BIT_BIG_ENDIAN) || defined(CLX_EN_HOST_64_BIT_LITTLE_ENDIAN) || defined(CLX_EN_64BIT_ADDR)
#define CLX_ADDR_64_HI(__addr__)                    ((UI32_T)((__addr__) >> 32))
#define CLX_ADDR_64_LOW(__addr__)                   ((UI32_T)((__addr__) & 0xFFFFFFFF))
#define CLX_ADDR_32_TO_64(__hi32__,__low32__)       (((unsigned long long int)(__low32__)) |             \
                                                     (((unsigned long long int)(__hi32__)) << 32))
#else
#define CLX_ADDR_64_HI(__addr__)                    (0)
#define CLX_ADDR_64_LOW(__addr__)                   (__addr__)
#define CLX_ADDR_32_TO_64(__hi32__,__low32__)       (__low32__)
#endif

#define CLX_BITMAP_SIZE(bit_num)                    ((((bit_num) - 1) / 32) + 1)
#define CLX_IPV4_IS_MULTICAST(addr)                 (0xE0000000 == ((addr) & 0xF0000000))
#define CLX_IPV6_IS_MULTICAST(addr)                 (0xFF == (((UI8_T *)(addr))[0]))
#define CLX_MAC_IS_MULTICAST(mac)                   ((mac[0]) & (0x1))

/* DATA TYPE DECLARATIONS
 */
typedef UI8_T   CLX_BIT_MASK_8_T;
typedef UI16_T  CLX_BIT_MASK_16_T;
typedef UI32_T  CLX_BIT_MASK_32_T;
typedef UI64_T  CLX_BIT_MASK_64_T;

typedef UI8_T   CLX_MAC_T[6];
typedef UI32_T  CLX_IPV4_T;
typedef UI8_T   CLX_IPV6_T[16];

typedef UI64_T  CLX_TIME_T;

/* Bridge Domain id data type. */
typedef UI32_T CLX_BRIDGE_DOMAIN_T;

/* TRILL nickname type. */
typedef UI16_T CLX_TRILL_NICKNAME_T;

typedef union CLX_IP_U
{

    CLX_IPV4_T     ipv4_addr;
    CLX_IPV6_T     ipv6_addr;

}CLX_IP_T;

typedef struct CLX_IP_ADDR_S
{
   CLX_IP_T      ip_addr;
   CLX_IP_T      ip_mask;
   BOOL_T        ipv4 ;
}CLX_IP_ADDR_T;

/* Tunnel type*/
typedef enum
{
    CLX_TUNNEL_TYPE_IPV4INIPV4 = 0,  /* RFC2003, IPv4-in-IPv4 tunnel */
    CLX_TUNNEL_TYPE_IPV4INIPV6,      /* RFC2003, IPv4-in-IPv6 tunnel */
    CLX_TUNNEL_TYPE_IPV6INIPV4,      /* RFC2003, IPv6-in-IPv4 tunnel */
    CLX_TUNNEL_TYPE_IPV6INIPV6,      /* RFC2003, IPv6-in-IPv6 tunnel */
    CLX_TUNNEL_TYPE_GREIPV4INIPV4,   /* RFC2784/RFC2890,GRE IPv4-in-IPv4 tunnel */
    CLX_TUNNEL_TYPE_GREIPV6INIPV4,   /* RFC2784/RFC2890,GRE IPv6-in-IPv4 tunnel */
    CLX_TUNNEL_TYPE_GREIPV4INIPV6,   /* RFC2784/RFC2890,GRE IPv4-in-IPv6 tunnel */
    CLX_TUNNEL_TYPE_GREIPV6INIPV6,   /* RFC2784/RFC2890,GRE IPv6-in-IPv6 tunnel */
    CLX_TUNNEL_TYPE_GRE_NSH,
    CLX_TUNNEL_TYPE_6TO4,            /* RFC3056, 6to4 tunnel*/
    CLX_TUNNEL_TYPE_ISATAP,          /* RFC5214, ISATAP tunnel */
    CLX_TUNNEL_TYPE_NVGRE_L2,
    CLX_TUNNEL_TYPE_NVGRE_V4,
    CLX_TUNNEL_TYPE_NVGRE_V6,
    CLX_TUNNEL_TYPE_NVGRE_NSH,
    CLX_TUNNEL_TYPE_VXLAN,
    CLX_TUNNEL_TYPE_GTP_V4,
    CLX_TUNNEL_TYPE_GTP_V6,
    CLX_TUNNEL_TYPE_MPLSINGRE,
    CLX_TUNNEL_TYPE_VXLANGPE_L2,
    CLX_TUNNEL_TYPE_VXLANGPE_V4,
    CLX_TUNNEL_TYPE_VXLANGPE_V6,
    CLX_TUNNEL_TYPE_VXLANGPE_NSH,
    CLX_TUNNEL_TYPE_FLEX0_L2,
    CLX_TUNNEL_TYPE_FLEX0_V4,
    CLX_TUNNEL_TYPE_FLEX0_V6,
    CLX_TUNNEL_TYPE_FLEX0_NSH,
    CLX_TUNNEL_TYPE_FLEX1_L2,
    CLX_TUNNEL_TYPE_FLEX1_V4,
    CLX_TUNNEL_TYPE_FLEX1_V6,
    CLX_TUNNEL_TYPE_FLEX1_NSH,
    CLX_TUNNEL_TYPE_FLEX2_L2,
    CLX_TUNNEL_TYPE_FLEX2_V4,
    CLX_TUNNEL_TYPE_FLEX2_V6,
    CLX_TUNNEL_TYPE_FLEX2_NSH,
    CLX_TUNNEL_TYPE_FLEX3_L2,
    CLX_TUNNEL_TYPE_FLEX3_V4,
    CLX_TUNNEL_TYPE_FLEX3_V6,
    CLX_TUNNEL_TYPE_FLEX3_NSH,
    CLX_TUNNEL_TYPE_LAST
} CLX_TUNNEL_TYPE_T;

/* tunnel key */
typedef struct CLX_TUNNEL_KEY_S
{
    CLX_IP_ADDR_T       src_ip;           /* key: The outer source IP address used by tunnel encapsulation.*/
    CLX_IP_ADDR_T       dst_ip;           /* key: The outer destination IP address used by tunnel encapsulation.
                                           * For automatic tunnel, this is not required. If not specified,
                                           * its ip address value must be set to 0, but the IP version
                                           * must be same with src_ip.
                                           */
    CLX_TUNNEL_TYPE_T   tunnel_type;      /*key: The tunnel type.*/
}CLX_TUNNEL_KEY_T;

typedef UI16_T CLX_VLAN_T;
typedef UI32_T CLX_PORT_T;

typedef enum{
    CLX_PORT_TYPE_NORMAL = 0,
    CLX_PORT_TYPE_UNIT_PORT,
    CLX_PORT_TYPE_LAG,
    CLX_PORT_TYPE_VM_ETAG,
    CLX_PORT_TYPE_VM_VNTAG,
    CLX_PORT_TYPE_VM_VEPA,
    CLX_PORT_TYPE_FCOE,
    CLX_PORT_TYPE_IP_TUNNEL,
    CLX_PORT_TYPE_TRILL,
    CLX_PORT_TYPE_MPLS,
    CLX_PORT_TYPE_MPLS_PW,
    CLX_PORT_TYPE_CPU_PORT,
    CLX_PORT_TYPE_SFC,
    CLX_PORT_TYPE_LAST
}CLX_PORT_TYPE_T;

/*support Green/Yellow/Red color*/
typedef enum
{
    CLX_COLOR_GREEN = 0,
    CLX_COLOR_YELLOW,
    CLX_COLOR_RED,
    CLX_COLOR_LAST
}CLX_COLOR_T;
typedef enum
{
    CLX_FWD_ACTION_FLOOD = 0,
    CLX_FWD_ACTION_NORMAL,
    CLX_FWD_ACTION_DROP,
    CLX_FWD_ACTION_COPY_TO_CPU,
    CLX_FWD_ACTION_REDIRECT_TO_CPU,
    CLX_FWD_ACTION_FLOOD_COPY_TO_CPU,
    CLX_FWD_ACTION_DROP_COPY_TO_CPU,
    CLX_FWD_ACTION_LAST
} CLX_FWD_ACTION_T;

typedef CLX_HUGE_T  CLX_THREAD_ID_T;
typedef CLX_HUGE_T  CLX_SEMAPHORE_ID_T;
typedef CLX_HUGE_T  CLX_ISRLOCK_ID_T;
typedef CLX_HUGE_T  CLX_IRQ_FLAGS_T;

typedef enum
{
    CLX_DIR_INGRESS  = 0,
    CLX_DIR_EGRESS,
    CLX_DIR_BOTH,
    CLX_DIR_LAST
}CLX_DIR_T;

typedef enum
{
    CLX_VLAN_ACTION_SET,
    CLX_VLAN_ACTION_KEEP,
    CLX_VLAN_ACTION_REMOVE,
    CLX_VLAN_ACTION_LAST
} CLX_VLAN_ACTION_T;

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
    CLX_VLAN_PRECEDENCE_SUBNET_MAC_PROTOCOL_PORT = 1,
    CLX_VLAN_PRECEDENCE_MAC_SUBNET_PROTOCOL_PORT = 4,
    CLX_VLAN_PRECEDENCE_PORT_ONLY                = 7,
    CLX_VLAN_PRECEDENCE_FAVOR_TYPE               = 8,
    CLX_VLAN_PRECEDENCE_FAVOR_ADDR               = 9,
    CLX_VLAN_PRECEDENCE_LAST
} CLX_VLAN_PRECEDENCE_T;

/* VLAN Tag Type */
typedef enum
{
    CLX_VLAN_TAG_NONE = 0,      /* UnTag                                */
    CLX_VLAN_TAG_SINGLE_PRI,    /* Single Customer/Service Priority Tag */
    CLX_VLAN_TAG_SINGLE,        /* Single Customer/Service Tag          */
    CLX_VLAN_TAG_DOUBLE_PRI,    /* Double Tag with any VID=0            */
    CLX_VLAN_TAG_DOUBLE,        /* Double Tag                           */
    CLX_VLAN_TAG_LAST
} CLX_VLAN_TAG_T;

typedef struct CLX_BUM_INFO_S
{
    UI32_T    mcast_id;
    UI32_T    group_label;      /* l2 da group label */
    UI32_T    vid;              /* used when FLAGS_ADD_VID is set */

#define CLX_BUM_INFO_FLAGS_MCAST_VALID    (1U << 0)
#define CLX_BUM_INFO_FLAGS_TO_CPU         (1U << 1)
#define CLX_BUM_INFO_FLAGS_ADD_VID        (1U << 2) /* single tag to double tag (i.e) QinQ */
#define CLX_BUM_INFO_FLAGS_TRILL_ALL_TREE (1U << 3)
    UI32_T    flags;
} CLX_BUM_INFO_T;

typedef enum
{
    CLX_PHY_TYPE_INTERNAL = 0x0,
    CLX_PHY_TYPE_EXTERNAL,
    CLX_PHY_TYPE_LAST
} CLX_PHY_TYPE_T;

typedef enum
{
    CLX_PHY_DEVICE_ADDR_PMA_PMD    = 1,
    CLX_PHY_DEVICE_ADDR_WIS        = 2,
    CLX_PHY_DEVICE_ADDR_PCS        = 3,
    CLX_PHY_DEVICE_ADDR_PHY_XS     = 4,
    CLX_PHY_DEVICE_ADDR_DTE_XS     = 5,
    CLX_PHY_DEVICE_ADDR_TC         = 6,
    CLX_PHY_DEVICE_ADDR_AN         = 7,
    CLX_PHY_DEVICE_ADDR_VENDOR_1   = 30,
    CLX_PHY_DEVICE_ADDR_VENDOR_2   = 31,
    CLX_PHY_DEVICE_ADDR_LAST
} CLX_PHY_DEVICE_ADDR_T;

typedef enum
{
    CLX_BULK_OP_MODE_ERR_STOP = 0,
    CLX_BULK_OP_MODE_ERR_CONTINUE,
    CLX_BULK_OP_MODE_LAST
} CLX_BULK_OP_MODE_T;

typedef struct CLX_RANGE_INFO_S
{
    UI32_T    min_id;
    UI32_T    max_id;
    UI32_T    max_member_cnt;

#define CLX_RANGE_INFO_FLAGS_MAX_MEMBER_CNT    (1U << 0)
    UI32_T    flags;
} CLX_RANGE_INFO_T;

typedef struct CLX_FDL_INFO_S
{
    UI32_T    probability  /* percentage from 0~100 */;
    UI32_T    threshold;   /* range 0 ~ (2^20)-1 */
} CLX_FDL_INFO_T;

/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */

#endif  /* CLX_TYPES_H */
