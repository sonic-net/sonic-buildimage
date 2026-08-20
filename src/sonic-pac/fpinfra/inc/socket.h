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



#ifndef _INCLUDE_SOCKET_
#define _INCLUDE_SOCKET_

#include "pacinfra_common.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netdb.h>

/* From <linux/tcp.h> */
#define TCP_MD5SIG_MAXKEYLEN 80
#define TCP_MD5SIG      14  /* TCP MD5 Signature (RFC2385) */

#define TCP_MD5SIG_FLAG_IFINDEX  0x2  /* ifindex set */

/* If not defined in the kernel headers (unsupported kernels) */
#ifndef TCP_MD5SIG_EXT
#define TCP_MD5SIG_EXT          32      /* TCP MD5 Signature with extensions */
#endif

/* Matches tcp_md5sig structure in linux/tcp.h. Used to set the 
 * TCP_MD5SIG socket option. */
typedef struct  tcp_md5sig 
{
  /* IPv4 address of the remote end of the TCP connection using 
   * a given TCP MD5 password. */
    struct sockaddr_storage tcpm_addr;  

     uchar8   tcpm_flags;             /* extension flags */
     uchar8   tcpm_prefixlen;         /* address prefix */
     ushort16 tcpm_keylen;            /* key length in bytes */
    uint32   tcpm_ifindex;           /* device index for scope (L3 VRF index) */
     uchar8   tcpm_key[TCP_MD5SIG_MAXKEYLEN];     /* key (binary) */
}  tcp_md5sig;

#endif /* _INCLUDE_SOCKET_ */

