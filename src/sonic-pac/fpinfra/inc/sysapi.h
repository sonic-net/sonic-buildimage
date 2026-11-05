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

#ifndef SYSAPIPROTO_HEADER
#define SYSAPIPROTO_HEADER
#include "pacinfra_common.h"
#include "cpustats_api.h"
#include "system_exports.h"

#define __FP_FILE__ __FILE__

typedef struct {
  uint32  alloc_rx_norm_alloc_attempts;
  uint32  alloc_rx_high_alloc_attempts;
  uint32  alloc_tx_alloc_attempts;
  uint32  alloc_rx_mid0_alloc_attempts;
  uint32  alloc_rx_mid1_alloc_attempts;
  uint32  alloc_rx_mid2_alloc_attempts;

  uint32  alloc_rx_norm_failures;
  uint32  alloc_rx_high_failures;
  uint32  alloc_tx_failures;
  uint32  alloc_rx_mid0_failures;
  uint32  alloc_rx_mid1_failures;
  uint32  alloc_rx_mid2_failures;

  uint32  alloc_rx_norm;
  uint32  alloc_rx_mid0;
  uint32  alloc_rx_mid1;
  uint32  alloc_rx_mid2;
  uint32  alloc_rx_high;
  uint32  alloc_tx;              /* not used, defined only for symmetry */
} mbuf_stats_t;

extern mbuf_stats_t mbuf_stats;

typedef enum eMbufLocation
{
  MBUF_LOC_FREE = 0,
  MBUF_LOC_ALLOC,            /* Allocated, but no more specific information available */
  MBUF_LOC_IP4_FWD_Q,        /* Holds an IPv4 data packet on the msg q for IP forwarding */
  MBUF_LOC_IP4_FWD,          /* Holds an IPv4 data packet being software forwarded */
  MBUF_LOC_ARP_RX,           /* Holds an incoming ARP packet */
  MBUF_LOC_ARP_PEND_Q,       /* Used to queue an IPv4 data packet pending ARP resolution */
  MBUF_LOC_ARP_RES_CB,       /* On ARP resolution callback message queue */
  MBUF_LOC_PDU_TX,           /* Used to tx a packet in dtlPduTransmit() */
  MBUF_LOC_PDU_RX,           /* Mbuf for incoming packet. Reached dtlPduReceive() */
  MBUF_LOC_VRRP_RX,          /* Incoming VRRP packet */
  MBUF_LOC_DHCP_RELAY_RX,    /* Incoming DHCP relay packet */
  MBUF_LOC_UDP_RELAY_TX,     /* mbuf used by IP helper to forward a UDP message */
  MBUF_LOC_CP_WIO_RX,        /* IP packet intercepted by captive portal, wired interface */
  MBUF_LOC_CP_WIO_ARP_RX,    /* ARP packet intercepted by captive portal, wired interface */
  MBUF_LOC_CP_WIO_TX,        /* Captive portal wired interface, outgoing packet */
  MBUF_LOC_DAI,              /* mbuf allocated by dynamic arp inspection */

  MBUF_LOC_LAST

} eMbufLocation;


/**************************************************************************
*
* @structures SYSAPI_NET_MBUF_HEADER_t
*
* @purpose  net buffer handle
*
* @notes
*
* @end
*************************************************************************/
typedef struct
{
  uint32   applSpecVar;                 /* application specific use */
   uchar8  *bufStart;
  uint32   bufLength;
  void *   taskId;
  uint32   timeStamp;
   BOOL     rxBuffer;  /* set to  TRUE if this buffer was allocated on Rx side */
   BOOL     in_use;    /* Buffer is curently allocated. */
   BOOL     rxUntagged;/* Frame received untagged on ingress port */
  uint32   rxCode;    /* indicates why the pkt was received */
   ushort16 rxMatchRuleNum; /* indicated the rule num set by hardware IFP */
   uchar8   last_file[16]; /* File name that touched the buffer last (debug) or
                                contains the function name specified while registering with
                                sysnet when the buffer is delivered to some other application */
  uint32   last_line;   /* Line number that touched the buffer last (debug) or
                              empty when the last_file conatins the function name to
                              which buffer is delivered */
  eMbufLocation mbufLoc;     /* Indicates where mbuf currently is */
   ushort16 priorityPool;  /* associate priority value from  MBUF_RX_PRIORITY */
   BOOL     tracePacket;   /* Indicates whether the packet is counted by CPU traffic filtering */
   BOOL     dumpPacket;    /* Indicates whether to dump packet. Applicable if trace option is enabled */
   int32    traceIndex;    /* Packet index in CPU traffic trace log. Default is -1 */
  cpuTrafficMatch_t match;   /* Is set on HAPI when packet is matched as a "tracked" */
} SYSAPI_NET_MBUF_HEADER_t;

#define NET_MBUF_START_OFFSET 16

/*
 * This deals with buffer alignment issues on some processors.
 */
typedef enum
{
   MBUF_FRAME_ALIGNED = 0,
   MBUF_IP_ALIGNED,
}  MBUF_ALIGNMENT;

/*
 * Account for necessary alignment. This parm does not effect the ipheader offset and only ensures
 * the data buffer will fall on a natural boundary.
 */
#define  MBUF_ALIGN_BOUND   4

/*
 * This aligns the IP addresses in the IP Header
 */
#define  MBUF_IP_CORRECTION      2

/* Max  Correction required if the buffer is to be
 * (4 byte boundary + IP aligned )
 */
#define SYSAPI_PKT_BUF_ALIGN_LEN   8

/* Macro used to track the Mbuf with the file name and the line num.
 * This can be used by the individual components for further tracking of Mbuf.
 */
#define SYSAPI_NET_MBUF_TRACK(netMbufHandle) sysapiNetMbufTrack(netMbufHandle, __FP_FILE__, __LINE__);

/* L7 NET MBUF MACRO: */
/* Get a NET MBUF - netMbufHandle output */
#define SYSAPI_NET_MBUF_GET(netMbufHandle)                      \
 {                                                              \
   netMbufHandle = sysapiNetMbufGetTrack(__FP_FILE__, __LINE__);   \
   if (netMbufHandle ==  NULL)                                \
   {                                                            \
      LOGF( LOG_SEVERITY_INFO, "Out of system buffers.\n");                       \
   }                                                            \
 }

/* L7 NET MBUF MACRO: */
/* Get a aligned NET MBUF - netMbufHandle output */

#define SYSAPI_BUF_ALIGN(x, align)  (((unsigned long) (x) + ((unsigned long)(align) - 1)) & ~((unsigned long)(align) - 1))

#define SYSAPI_NET_ALIGN_MBUF_GET(netMbufHandle,__ALIGN_TYPE__)         \
{                                                                       \
  netMbufHandle = sysapiNetMbufAlignGet(__FP_FILE__,__LINE__,__ALIGN_TYPE__); \
  if (netMbufHandle ==  NULL)                                         \
  {                                                                     \
     LOGF( LOG_SEVERITY_INFO, "Out of system buffers.\n");                                \
  }                                                                     \
}

/* Align buffer to 4 byte boundary  */
#define SYSAPI_ALIGN_4_BYTE_BOUND_LENGTH   ( MBUF_ALIGN_BOUND -1)

#define SYSAPI_ALIGN_4_BYTE_BOUNDARY(charBuf)  \
  SYSAPI_BUF_ALIGN(charBuf, MBUF_ALIGN_BOUND)

/* L7 NET MBUF MACRO: */
/* Get a NET MBUF (without logging message) - netMbufHandle output */
#define SYSAPI_NET_MBUF_GET_NOLOG(netMbufHandle)                \
 {                                                              \
   (netMbufHandle) = sysapiNetMbufGetTrack(__FILE__, __LINE__); \
 }

/* L7 NET MBUF MACRO: */
/* Free a NET MBUF - netMbufHandle input */
#define SYSAPI_NET_MBUF_FREE(netMbufHandle)                             \
  do                                                                    \
  {                                                                     \
   if ((netMbufHandle) != 0)                                            \
   {                                                                    \
     sysapiNetMbufFreeTrack((netMbufHandle), __FP_FILE__, __LINE__);       \
   }                                                                    \
   else                                                                 \
   {                                                                    \
      sysapiPrintf("SYSAPI_NET_MBUF_FREE ERROR Line : %d Function : %s", \
             __LINE__,__FUNCTION__);                                    \
   }                                                                    \
  } while (0);



/* L7 NET MBUF MACRO: */
/* Get data pointer for a NET MBUF - netMbufHandle input, dataStart output */
#define SYSAPI_NET_MBUF_GET_DATASTART(netMbufHandle, dataStart) \
 dataStart = ((SYSAPI_NET_MBUF_HEADER_t *)netMbufHandle)->bufStart



/* L7 NET MBUF MACRO: */
/* Set data pointer for a NET MBUF - netMbufHandle input, dataStart input */
#define SYSAPI_NET_MBUF_SET_DATASTART(netMbufHandle, dataStart) \
 ((SYSAPI_NET_MBUF_HEADER_t *)netMbufHandle)->bufStart = dataStart



/* L7 NET MBUF MACRO: */
/* Get data length for a NET MBUF - netMbufHandle input, dataLength output */
#define SYSAPI_NET_MBUF_GET_DATALENGTH(netMbufHandle, dataLength) \
 dataLength = ((SYSAPI_NET_MBUF_HEADER_t *)netMbufHandle)->bufLength



/* L7 NET MBUF MACRO: */
/* Set data length for a NET MBUF - netMbufHandle input, dataLength input */
#define SYSAPI_NET_MBUF_SET_DATALENGTH(netMbufHandle, dataLength) \
 ((SYSAPI_NET_MBUF_HEADER_t *)netMbufHandle)->bufLength = dataLength

/* Set MBUF location */
#define SYSAPI_NET_MBUF_SET_LOC(netMbufHandle, loc) \
 ((SYSAPI_NET_MBUF_HEADER_t *)netMbufHandle)->mbufLoc = loc

/* Get MBUF location */
#define SYSAPI_NET_MBUF_GET_LOC(netMbufHandle) \
  ((SYSAPI_NET_MBUF_HEADER_t *)netMbufHandle)->mbufLoc

/* Get rxBuffer */
#define SYSAPI_NET_MBUF_GET_RXBUFFER(netMbufHandle) \
  ((SYSAPI_NET_MBUF_HEADER_t *)netMbufHandle)->rxBuffer

/* Get rxUntagged config */
#define SYSAPI_NET_MBUF_GET_RXUNTAGGED(netMbufHandle) \
  ((SYSAPI_NET_MBUF_HEADER_t *)netMbufHandle)->rxUntagged


/* Reason codes indicating why the packets was received */
#define   MBUF_RX_REASON_NONE  0x0
#define   MBUF_RX_REASON_ICMP_REDIR  0x1 /* IP Packet hit ICMP redirect scenario */
#define   MBUF_RX_REASON_L3_MISS  0x2    /* IP packet failed L3 lookup */
#define   MBUF_RX_SAMPLE_SOURCE  0x4     /* packet ingress sampled to the CPU */
#define   MBUF_RX_SAMPLE_DEST    0x8     /* packet egress sampled to the CPU */
#define   MBUF_RX_REASON_OPENFLOW    0x10   /* packet ingress meant for openflow */
#define   MBUF_RX_REASON_MATCH_RULE_NUM 0x20  /* packet redirected to cpu with an
                                                  explicit rule number. These packets
                                                  are destined for external agents outside of FP.*/

/* Reason codes indicating why VXLAN frames are trapped to CPU */
#define   MBUF_RX_REASON_ACCESS_VLAN_FWD 0x40
#define   MBUF_RX_REASON_ACCESS_PORT_FWD 0x80
#define   MBUF_RX_REASON_TUNNEL_SRC_MISS 0x100
#define   MBUF_RX_REASON_GRE_TUNNEL_SRC_MISS  0x200
#define   MBUF_RX_REASON_GRE_ACCESS_VLAN_FWD  0x400

/**************************************************************************
*
* @purpose  Initialize the sysapi component
*
* @param    none
*
* @returns   SUCCESS
* @returns   ERROR
*
* @notes
*
* @end
*
*************************************************************************/
RC_t sysapiSystemInit(void);

/**************************************************************************
*
* @purpose  Free a network mbuf
*
* @param    Ptr to network mbuf handle
*
* @returns  void
*
* @notes
*
* @end
*
*************************************************************************/
void sysapiNetMbufFree(  netBufHandle netMbufHandle );

/**************************************************************************
*
* @purpose  Free a network mbuf with debug information.
*
* @param    Ptr to network mbuf handle
*
* @returns  void
*
* @notes
*
* @end
*
*************************************************************************/
void sysapiNetMbufFreeTrack(  netBufHandle netMbufHandle, const  char8 *file, uint32 line);

/**************************************************************************
*
* @purpose  Retrieve a network mbuf to the caller (and track the caller)
*
* @param    none.
*
* @returns  A ptr to a network mbuf handle
* @returns  0 if none are available
*
* @notes
*
* @end
*
*************************************************************************/
 netBufHandle sysapiNetMbufGetTrack(const  char8 *file, uint32 line);

/**************************************************************************
*
* @purpose  Retrieve a network mbuf to the caller
*
* @param    none.
*
* @returns  A ptr to a network mbuf handle
* @returns  0 if none are available
*
* @notes
*
* @end
*
*************************************************************************/
 netBufHandle sysapiNetMbufGet( void );

/**************************************************************************
 * @purpose  Retrieve an aligned network mbuf to the caller
 *
 * @param    align   @b{(input)}  Alignment indicator, for IP or frame
 *
 * @returns  A ptr to a network mbuf handle
 * @returns  0 if none are available
 *
 * @note     All mbufs are 4 byte aligned
 *
 * @end
 *************************************************************************/
 netBufHandle sysapiNetMbufAlignGet( uchar8 *file, uint32 line,
     MBUF_ALIGNMENT alignType);

/**************************************************************************
* @purpose  Function used to track the Mbuf with the file name and the line num.
*           This can be used by the individual components for further tracking of Mbuf.
*
* @param    netMbufHandle - Ptr to network mbuf handle
* @param    file - file name
* @param    line - line number
*
* @returns  void
*
* @comments
*
* @end
*************************************************************************/
void sysapiNetMbufTrack( netBufHandle netMbufHandle,  uchar8 *file, uint32 line);

/**********************************************************************
* @pupose   To increment or decrement Mbuf stats per RX priority as in
*            MBUF_RX_PRIORITY
*
* @param    priority @b{(input)}  Mbuf priority to increment/decrement
*                                 corresponding stats
*           operation @b{(input)} Bool value to increment/decrement
*                                  TRUE - to increment
*                                  FALSE - to decrement
*
* @returns  SUCCESS,
*
* @notes
*
* @end
*
*********************************************************************/
RC_t sysapiMbufRxusedStatsUpdate( MBUF_RX_PRIORITY priority,
                                     BOOL operation);

#endif
