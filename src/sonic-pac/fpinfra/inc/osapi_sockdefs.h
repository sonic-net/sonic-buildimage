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


#ifndef _OSAPI_SOCKDEFS_H
#define _OSAPI_SOCKDEFS_H

#include "l3_addrdefs.h"

/*NOTE WELL: these values are NOT those of underlying stack.
  Do not confuse  AF_INET and AF_INET,eg. Values differ
  across platforms and we intentionally dont want to include
  a way to resolve them here. (would hose many things). 
*/

/*
 ****************************************************************************
 *                         setsockopt/getsockopt
 ****************************************************************************
 */

/*
 * Temporarily add this #undef to avoid a conflict with
 * a build environment that #defines imr_interface.
 */
#undef imr_interface
typedef struct  ip_mreq_s
{
    struct   l3_in_addr_s   imr_multiaddr;  
    struct   l3_in_addr_s   imr_interface;  
} ip_mreq_t;

typedef struct  ip_mreq6_s
{
    struct   l3_in6_addr_s   imr6_multiaddr;  
    uint32               imr6_intIfNum;  
     uchar8               imr6_ifName[ INTF_NAME_MAX_LEN];
} ip_mreq6_t;


/* IOCTL: note that socket ioctls are all handles through osapi 
   using calls specific to that ioctl. for that reason, ioctl codes
   are not defined here
*/

/* flags for interface type */
#define  IFF_BROADCAST           1
#define  IFF_LOOPBACK            2
#define  IFF_POINTOPOINT         4
#define  IFF_MULTICAST           8




/* for  ICMP6_FILTER sockopt */
typedef struct  icmp6_filter_s
{
    uint32  icmp6_filt[8];  
} icmp6_filter_t;

#define  ICMP6_FILTER_SETPASS(type, filterp) \
    ((((filterp)->icmp6_filt[(type) >> 5]) |= \
        (1 << ((type) & 31))))


#define  ICMP6_FILTER_SETBLOCK(type, filterp) \
    ((((filterp)->icmp6_filt[(type) >> 5]) &= \
        ~(1 << ((type) & 31))))


#define  ICMP6_FILTER_SETPASSALL(filterp) \
    memset((filterp), 0xFF, sizeof(  icmp6_filter_t))

#define  ICMP6_FILTER_SETBLOCKALL(filterp) \
    memset((filterp), 0, sizeof( icmp6_filter_t))



#endif /* _OSAPI_SOCKDEFS_H */

