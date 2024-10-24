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

#ifndef _INCLUDE_PACKET_
#define _INCLUDE_PACKET_

#define  MAC_ADDR_LEN                 6
#define  ENET_MAC_ADDR_LEN   MAC_ADDR_LEN


#define  ETYPE_IP       0x0800
#define  ETYPE_IPV6     0x86DD
#define  ETYPE_8021Q    0x8100
#define  VLAN_TAG_VLAN_ID_MASK            0x0FFF

#define IP_PROT_IP            4   /* ip in ip encap */
#define IP_PROT_TCP           6

/* Ethernet MAC Address */
typedef struct  mac_addr_s
{
   uchar8  addr[ ENET_MAC_ADDR_LEN];
} enetMacAddr_t;

typedef enum
{
   ENCAP_NONE = 0,
   ENCAP_ETHERNET,
   ENCAP_802

}  ENCAPSULATION_t;

#ifdef  MAC_ENET_BCAST
static  enetMacAddr_t  ENET_BCAST_MAC_ADDR =
{{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
#endif

/* Ethernet Header */
typedef struct  enetHeader_s
{
   enetMacAddr_t    dest;
   enetMacAddr_t    src;
} enetHeader_t;

/* Another Ethernet header, but with an ethertype field. Note that the
 * ethertype field can either be an ethertype (Eth II) or a length (802.3). */
typedef struct  ethHeader_s
{
   enetMacAddr_t  dest;
   enetMacAddr_t  src;
   ushort16  ethertype;   /* type or length */
}  ethHeader_t;

typedef struct  ipHeader_s
{
   uchar8   iph_versLen;      /* version/hdr len */
   uchar8   iph_tos;          /* type of service */
   ushort16 iph_len;          /* pdu len */
   ushort16 iph_ident;        /* identifier */
   ushort16 iph_flags_frag;   /* flags/ fragment offset */
   uchar8   iph_ttl;          /* lifetime */
   uchar8   iph_prot;         /* protocol ID */
   ushort16 iph_csum;         /* checksum */
  uint32   iph_src;          /* src add */
  uint32   iph_dst;          /* dst add */
} ipHeader_t;

/** IPV6 packet header **/
typedef struct  ip6Header_s
{
  uint32    ver_class_flow;
   ushort16  paylen;
   uchar8    next;
   uchar8    hoplim;
   uchar8    src[16];
   uchar8    dst[16];
}  ip6Header_t;
#define  IP6_HEADER_LEN 40

#define TCP_PORT_HTTP     80
#define TCP_PORT_HTTPS    443

typedef struct
{
   ushort16  srcPort;                 /* source port */
   ushort16  destPort;                /* dest port */
  uint32    seqNumber;               /* sequence number */
  uint32    ackNumber;               /* acknowledgement number */
   ushort16  dataOffsetCtrlFlags;     /* offset(4 bits) and control flags(6 bits) */
   ushort16  window;                  /* window size */
   ushort16  checksum;                /* checksum  */
   ushort16  urgentPtr;               /* urgent pointer */
  uint32    options;                 /* TCP options */
}  tcpHeader_t;

/* IPv4 pseudo header for TCP/UDP checksum */
typedef struct  pseudo_header_s
{
  uint32   source_address;
  uint32   dest_address;
   uchar8   place_holder;
   uchar8   protocol;
   ushort16 length;
}  ipv4pseudo_header_t;

/* IPv6 Pseudo header used in the
 * checksum calculations of the upper layer headers.
 * RFC 2460, section 8.1. */
typedef struct  ip6PseudoHdr_s
{
   uchar8     src[16];
   uchar8     dst[16];
  uint32     payloadLength;
   uchar8     zero[3];
   uchar8     nextHeader;
}  ip6PseudoHdr_t;


#endif
