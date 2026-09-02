/*
 * Copyright 2024 Broadcom Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef _L3_ADDRDEFS_H
#define _L3_ADDRDEFS_H

#include "pacinfra_common.h"

/*NOTE WELL: these values are NOT those of underlying stack.
  Do not confuse  AF_INET and AF_INET,eg. Values differ
  across platforms and we intentionally dont want to include
  a way to resolve them here. (would hose many things).
*/

#define INET_IPV4_ALL_MCAST_GROUPS_ADDR   (uint32)0xe0000000
                                                        /* 224.0.0.0 */
#define INET_IPV6_ALL_MCAST_GROUPS_ADDR   "ff00::"

/* Used where a scope ID is required but the IP address has global scope, so
 * there really isn't a scope ID. */
#define  SCOPE_ID_NONE     0


typedef struct  l3_in_addr_s
{
    uint32   s_addr;    /* 32 bit IPv4 address in network byte order */
} l3_in4_addr_t;

/***************************************
 *
 * 128-bit IP6 address.
 ***************************************/
typedef struct  l3_in6_addr_s
{
    union
    {
         uchar8     addr8[16];
         ushort16   addr16[8];
        uint32     addr32[4];
    } in6;
} l3_in6_addr_t;

#define  IP6_LINK_LOCAL_MASKLEN 128

#define  MAX_POSSIBLE_IPV6_PREFIXES 128

/* buf len for printing ipv6 address */
#define IPV6_DISP_ADDR_LEN   46

/* Address display length that's long enough to display IPv4 or IPv6
 * and doesn't sound like it's IPv6-specific. */
#define INET_ADDR_DISP_LEN  IPV6_DISP_ADDR_LEN

#define IPV6_ADDRESS_LEN     sizeof( l3_in6_addr_t)

typedef struct  in6_prefix_s
{
   l3_in6_addr_t in6Addr;
   int32      in6PrefixLen;
}  in6_prefix_t;




/*****************************************************************************
 *                    sockaddr
 ****************************************************************************
 */
typedef struct  sockaddr_s
{
     uchar8             sa_len;
     uchar8             sa_family;
     uchar8             sa_data[14];
} sockaddr_t;


/*
 ****************************************************************************
 *                    sockaddr_in
 ****************************************************************************
 */
typedef struct  sockaddr_in_s
{
     uchar8             sin_len;
     uchar8             sin_family;
     ushort16           sin_port;

     l3_in4_addr_t          sin_addr;

    char                  sin_zero[8];  /* unused */
} sockaddr_in_t;




/*
 ****************************************************************************
 *                    sockaddr_in6
 ****************************************************************************
 */
typedef struct  sockaddr_in6_s
{
     uchar8               sin6_len;
     uchar8               sin6_family;
     ushort16             sin6_port;
    uint32               sin6_flowinfo;

     l3_in6_addr_t           sin6_addr;

    uint32               sin6_scope_id;
} sockaddr_in6_t;


/*
 * union needed to hold sockaddr of unknown type since struct sockaddr is
 * not big enough to hold in6
 */
typedef struct  sockaddr_union_s{
    union{
          sockaddr_t      sa;
          sockaddr_in_t   sa4;
          sockaddr_in6_t  sa6;
    }u;
} sockaddr_union_t;

/*
 ****************************************************************************
 *                    Generic Ip Address Structure
 ****************************************************************************
 */

typedef struct  l3_inet_addr_s
{
   uchar8     family;  /*  AF_INET,  AF_INET6, ... */
  union
  {
    struct  l3_in_addr_s    ipv4;
    struct  l3_in6_addr_s   ipv6;
  } addr;
}  l3_inet_addr_t;

typedef struct  inet_next_hop_s 
{
   l3_inet_addr_t  nhAddr;
   uchar8       flags;
   uchar8       weight; 
  uint32       stkIfIndex;    
}  inet_next_hop_t;


#define  INET_GET_FAMILY(xaddr) \
  (( l3_inet_addr_t *)(xaddr))->family

/* some macros for inet_address */
#define  IP4_IN_MULTICAST(a)      ((((a)) & 0xf0000000) == 0xe0000000)
#define  IP4_IN_CLASS_E(a)        ((((a)) & 0xf0000000) == 0xf0000000)
#define  INET_IS_ADDR_EQUAL(xaddr, yaddr) \
  ( ( AF_INET6 == (( l3_inet_addr_t *)(xaddr))->family) ? \
    ( ((( l3_inet_addr_t *)(xaddr))->addr.ipv6.in6.addr32[0] == \
       (( l3_inet_addr_t *)(yaddr))->addr.ipv6.in6.addr32[0]) && \
      ((( l3_inet_addr_t *)(xaddr))->addr.ipv6.in6.addr32[1] == \
       (( l3_inet_addr_t *)(yaddr))->addr.ipv6.in6.addr32[1]) && \
      ((( l3_inet_addr_t *)(xaddr))->addr.ipv6.in6.addr32[2] == \
       (( l3_inet_addr_t *)(yaddr))->addr.ipv6.in6.addr32[2]) && \
      ((( l3_inet_addr_t *)(xaddr))->addr.ipv6.in6.addr32[3] == \
       (( l3_inet_addr_t *)(yaddr))->addr.ipv6.in6.addr32[3]) ) : \
    ( (( l3_inet_addr_t *)(xaddr))->addr.ipv4.s_addr == \
      (( l3_inet_addr_t *)(yaddr))->addr.ipv4.s_addr ) )

/* this macro and the next one do a memcmp on a 4 byte integer when the address
 * is an IPv4 address. This requires the s_addr field to be in network byte order.
 * An alternative is to use inetAddrCompare(), which doesn't depend on byte order. */
#define  INET_ADDR_COMPARE(xaddr, yaddr) \
    ( ( AF_INET6 == (( l3_inet_addr_t *)(xaddr))->family) ? \
      (memcmp(&((( l3_inet_addr_t *)(xaddr))->addr.ipv6), \
              &((( l3_inet_addr_t *)(yaddr))->addr.ipv6), sizeof( l3_in6_addr_t))) : \
      (memcmp(&((( l3_inet_addr_t *)(xaddr))->addr.ipv4), \
              &((( l3_inet_addr_t *)(yaddr))->addr.ipv4), sizeof( l3_in4_addr_t))) )

#define  INET_FAMILY_ADDR_COMPARE(xaddr, yaddr) \
    ( ((( l3_inet_addr_t *)(xaddr))->family == ((( l3_inet_addr_t *)(yaddr))->family)) ? \
      (( AF_INET6 == (( l3_inet_addr_t *)(xaddr))->family) ? \
      (memcmp(&((( l3_inet_addr_t *)(xaddr))->addr.ipv6), \
              &((( l3_inet_addr_t *)(yaddr))->addr.ipv6), sizeof( l3_in6_addr_t))) : \
      (memcmp(&((( l3_inet_addr_t *)(xaddr))->addr.ipv4), \
              &((( l3_inet_addr_t *)(yaddr))->addr.ipv4), sizeof( l3_in4_addr_t)))) : \
      (memcmp(&((( l3_inet_addr_t *)(xaddr))->family), \
              &((( l3_inet_addr_t *)(yaddr))->family), sizeof( uchar8))) )

#define  INET_ADDR_IS_ZERO(xaddr) \
    ( ( AF_INET6 == (( l3_inet_addr_t *)(xaddr))->family) ? \
    ( ((( l3_inet_addr_t *)(xaddr))->addr.ipv6.in6.addr32[0] == 0) && \
      ((( l3_inet_addr_t *)(xaddr))->addr.ipv6.in6.addr32[1] == 0) && \
      ((( l3_inet_addr_t *)(xaddr))->addr.ipv6.in6.addr32[2] == 0) && \
      ((( l3_inet_addr_t *)(xaddr))->addr.ipv6.in6.addr32[3] == 0)) : \
    ( (( l3_inet_addr_t *)(xaddr))->addr.ipv4.s_addr == 0)) \


#define  INET_IS_ADDR_BROADCAST(xaddr) \
  ( ( AF_INET6 == (( l3_inet_addr_t *)(xaddr))->family) ? \
    ( ((( l3_inet_addr_t *)(xaddr))->addr.ipv6.in6.addr32[0] == \
       0xff) && \
      ((( l3_inet_addr_t *)(xaddr))->addr.ipv6.in6.addr32[1] == \
       0xff) && \
      ((( l3_inet_addr_t *)(xaddr))->addr.ipv6.in6.addr32[2] == \
       0xff) && \
      ((( l3_inet_addr_t *)(xaddr))->addr.ipv6.in6.addr32[3] == \
       0xff) ) : \
    ( (( l3_inet_addr_t *)(xaddr))->addr.ipv4.s_addr == \
      0xffffffff ) )

#define  INET_IS_ADDR_EXPERIMENTAL(xaddr) \
  ( ( AF_INET6 == (( l3_inet_addr_t *)(xaddr))->family) ? 0\
    : ( (( l3_inet_addr_t *)(xaddr))->addr.ipv4.s_addr & 0xf0000000) == 0xf0000000)

/* some macros for inet_address */


#define  IP6_IS_ADDR_UNSPECIFIED(xaddr) \
  ( ((( l3_in6_addr_t *)(xaddr))->in6.addr32[0] == 0) && \
    ((( l3_in6_addr_t *)(xaddr))->in6.addr32[1] == 0) && \
    ((( l3_in6_addr_t *)(xaddr))->in6.addr32[2] == 0) && \
    ((( l3_in6_addr_t *)(xaddr))->in6.addr32[3] == 0) )

#define  IP6_IS_ADDR_LOOPBACK(xaddr) \
  ( ((( l3_in6_addr_t *)(xaddr))->in6.addr32[0] == 0) && \
    ((( l3_in6_addr_t *)(xaddr))->in6.addr32[1] == 0) && \
    ((( l3_in6_addr_t *)(xaddr))->in6.addr32[2] == 0) && \
    ((( l3_in6_addr_t *)(xaddr))->in6.addr32[3] == osapiNtohl(1)) )

#define  IP6_IS_ADDR_EQUAL(xaddr,yaddr) \
  ( ((( l3_in6_addr_t *)(xaddr))->in6.addr32[0] == (( l3_in6_addr_t *)(yaddr))->in6.addr32[0]) && \
    ((( l3_in6_addr_t *)(xaddr))->in6.addr32[1] == (( l3_in6_addr_t *)(yaddr))->in6.addr32[1]) && \
    ((( l3_in6_addr_t *)(xaddr))->in6.addr32[2] == (( l3_in6_addr_t *)(yaddr))->in6.addr32[2]) && \
    ((( l3_in6_addr_t *)(xaddr))->in6.addr32[3] == (( l3_in6_addr_t *)(yaddr))->in6.addr32[3]) )

#define  IP6_IS_ADDR_LINK_LOCAL(xaddr) \
  ((osapiNtohs( (( l3_in6_addr_t *)(xaddr))->in6.addr16[0] ) & 0xffc0) == 0xfe80)

#define  IP6_IS_ADDR_LINK_LOCAL_MANUAL(xaddr) \
  (((osapiNtohs( (( l3_in6_addr_t *)(xaddr))->in6.addr16[0] ) & 0xffff) == 0xfe80) && \
   ((osapiNtohs( (( l3_in6_addr_t *)(xaddr))->in6.addr16[1] ) & 0xffff) == 0x0) && \
   ((osapiNtohs( (( l3_in6_addr_t *)(xaddr))->in6.addr16[2] ) & 0xffff) == 0x0) && \
   ((osapiNtohs( (( l3_in6_addr_t *)(xaddr))->in6.addr16[3] ) & 0xffff) == 0x0))

#define  IP6_IS_ADDR_MULTICAST(xaddr) \
  ( (( l3_in6_addr_t *)(xaddr))->in6.addr8[0] ==0xff )

#define  IP6_IS_ADDR_MULTICAST_LOC_SCOPE(xaddr) \
  (osapiNtohs( (( l3_in6_addr_t *)(xaddr))->in6.addr16[0] ) == 0xff02)

#define  IP6_IS_ADDR_MULTICAST_ZERO_SCOPE(xaddr) \
  ((osapiNtohs( (( l3_in6_addr_t *)(xaddr))->in6.addr16[0] ) & 0x000f) == 0x0)

#define  IP6_IS_ADDR_6TO4(xaddr) \
  (osapiNtohs( (( l3_in6_addr_t *)(xaddr))->in6.addr16[0] ) == 0x2002)

#define  IP6_IS_ADDR_V4COMPAT(xaddr) \
  ( ((( l3_in6_addr_t *)(xaddr))->in6.addr32[0] == 0) && \
    ((( l3_in6_addr_t *)(xaddr))->in6.addr32[1] == 0) && \
    ((( l3_in6_addr_t *)(xaddr))->in6.addr32[2] == 0) && \
    ((( l3_in6_addr_t *)(xaddr))->in6.addr32[3] != osapiNtohl(0)) && \
    ((( l3_in6_addr_t *)(xaddr))->in6.addr32[3] != osapiNtohl(1)) )

#define  IP6_IS_ADDR_V4MAPPED(xaddr) \
  ( ((( l3_in6_addr_t *)(xaddr))->in6.addr32[0] == 0) && \
    ((( l3_in6_addr_t *)(xaddr))->in6.addr32[1] == 0) && \
    ((( l3_in6_addr_t *)(xaddr))->in6.addr32[2] == osapiNtohl(0x0000ffff)) )

#define  IP6_ADDR_V4MAPPED(xaddr) ((( l3_in6_addr_t *)(xaddr))->in6.addr32[3])

#define  IP6_ADDR_LEN 16
#define  IP6_ADDR_COMPARE(xaddr1,xaddr2)  memcmp((xaddr1),(xaddr2), IP6_ADDR_LEN)

/* a handy macro for snmp walks */
#define  IP6_ADDR_INCREMENT(xaddr, xcarry)  {\
   int xx; \
   xcarry =  FALSE; \
   for(xx = 15; xx >= 0 ;xx--) \
   { \
      (xaddr)->in6.addr8[xx]++; \
      if((xaddr)->in6.addr8[xx] != 0) \
      { \
         break; \
      } \
   } \
   if(xx < 0) xcarry =  TRUE; \
}

#define  INET_ADDR_GET_ALL_ONES_MASKLEN(addr)  ((addr)->family ==  AF_INET)?32:128

/*
 Following things are added for mcast support
 */

#define  INET_IN_SSM_RANGE(xaddr) \
    (( AF_INET6 == (( l3_inet_addr_t *)(xaddr))->family) ? \
     (((osapiNtohs( (( l3_inet_addr_t *)(xaddr))->addr.ipv6.in6.addr16[0] ) & 0xfff0) == 0xff30)&&  \
      ((osapiNtohs( (( l3_inet_addr_t *)(xaddr))->addr.ipv6.in6.addr16[1] ) & 0xffff) == 0x0000)&&  \
      ((osapiNtohs( (( l3_inet_addr_t *)(xaddr))->addr.ipv6.in6.addr16[2] ) & 0xffff) == 0x0000)&&  \
      ((osapiNtohs( (( l3_inet_addr_t *)(xaddr))->addr.ipv6.in6.addr16[3] ) & 0xffff) == 0x0000)&&  \
      ((osapiNtohs( (( l3_inet_addr_t *)(xaddr))->addr.ipv6.in6.addr16[4] ) & 0xffff) == 0x0000)&&  \
      ((osapiNtohs( (( l3_inet_addr_t *)(xaddr))->addr.ipv6.in6.addr16[5] ) & 0xffff) == 0x0000)) :\
     ((((( l3_inet_addr_t *)(xaddr))->addr.ipv4.s_addr) & 0xFF000000) == \
      IP_SSM_BASE_ADDR))
     
#define  IP6_IS_ADDR_MC_WELL_KNOWN(xaddr) \
    (osapiNtohs((xaddr)->in6.addr16[0]) == 0xff02)

#define  INET_GET_MAX_MASK_LEN(family) ((family ==  AF_INET)?32:128)

#define  IP6_GET_VER(ip6)      (osapiNtohl((ip6)->ver_class_flow) >> 28)
#define  IP6_SET_VER(ip6,val)  (ip6)->ver_class_flow = osapiHtonl((val & 0x0f) << 28)
#define  IP6_GET_CLASS(ip6)   (( uchar8) ((osapiNtohl((ip6)->ver_class_flow) >> 20) & 0xff))
#define IPFAMILYTYPE(x) ( ((x ==  AF_INET)?"IPv4":((x ==  AF_INET6)?"IPv6":"NONE")) )
#endif /* _L3_ADDRDEFS_H */

