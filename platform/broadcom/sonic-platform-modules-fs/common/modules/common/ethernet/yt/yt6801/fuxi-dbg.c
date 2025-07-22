// SPDX-License-Identifier: GPL-2.0+
/* Copyright (c) 2021 Motor-comm Corporation.
 * Confidential and Proprietary. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "fuxi-gmac.h"
#include "fuxi-gmac-reg.h"

#ifdef DBG
u32   MPDebugLevel = MP_LOUD;
#endif

#if defined(UEFI)
#include <Library/UefiLib.h>

void DbgPrintAddress(unsigned char* Address)
{
    // If your MAC address has a different size, adjust the printf accordingly.
    //{ASSERT(ETH_LENGTH_OF_ADDRESS == 6); }

    DbgPrintF(MP_LOUD, "%02x-%02x-%02x-%02x-%02x-%02x.", \
        Address[0], Address[1], Address[2], \
        Address[3], Address[4], Address[5]);
}

void fxgmac_dump_buffer(unsigned char *skb, unsigned int len, int tx_rx)
{
#define ZDbgPrintF(Level, ...)    AsciiPrint("fx-pat "  __VA_ARGS__);                                             \

    //unsigned char buffer[128];
    unsigned int i, j;
    char * caption[3] = {	"Tx",
    						"Rx",
    						"Buffer"};

    if(tx_rx > 2) tx_rx = 2;

    ZDbgPrintF(MP_TRACE, " ************** Data dump start ****************\n");
    ZDbgPrintF(MP_TRACE, " %a data of %d bytes\n", caption[tx_rx], len);

    for (i = 0; i < len; i += 32) {
    	unsigned int len_line = min(len - i, 32U);

    	for(j = 0; j < len_line; j++)
    	{
    		AsciiPrint("%02x ", (u8)skb[i + j]);
    	}
    	AsciiPrint("\n");
    }

    ZDbgPrintF(MP_TRACE, " ************** Data dump end ****************\n");
}


#elif defined(KDNET)

#elif defined(_WIN64) || defined(_WIN32)
#include "fuxi-gmac-reg.h"
#include "fuxi-mp.h"
#if DBG

/**
Constants
**/

#define _FILENUMBER     'GBED'

// Bytes to appear on each line of dump output.
//
#define DUMP_BytesPerLine 16

//ULONG               MPDebugLevel = MP_LOUD;
ULONG               MPAllocCount = 0;       // the number of outstanding allocs
NDIS_SPIN_LOCK      MPMemoryLock;           // spinlock for the debug mem list
LIST_ENTRY          MPMemoryList;
BOOLEAN             MPInitDone = FALSE;     // debug mem list init flag 

PVOID 
MPAuditAllocMemTag(
    UINT        Size,
    CHAR*       FileNumber,
    ULONG       LineNumber,
    NDIS_HANDLE MiniportAdapterHandle
    )
{
    PMP_ALLOCATION  pAllocInfo;
    PVOID           Pointer;

    if (!MPInitDone)
    {
        NdisAllocateSpinLock(&MPMemoryLock);
        InitializeListHead(&MPMemoryList);
        MPInitDone = TRUE;
    }

    pAllocInfo = NdisAllocateMemoryWithTagPriority(
                 MiniportAdapterHandle,
                 (UINT)(Size + sizeof(MP_ALLOCATION)), 
                 NIC_TAG,
                 LowPoolPriority);//leileiz

    if (pAllocInfo == (PMP_ALLOCATION)NULL)
    {
        Pointer = NULL;

        DbgPrintF(MP_LOUD,  "%s - file %s, line %d, Size %d failed!", __FUNCTION__, FileNumber, LineNumber, Size);
    }
    else
    {
        Pointer = (PVOID)&(pAllocInfo->UserData);
        MP_MEMSET(Pointer, Size, 0xc);

        pAllocInfo->Signature = 'DOOG';
        pAllocInfo->FileNumber = FileNumber;
        pAllocInfo->LineNumber = LineNumber;
        pAllocInfo->Size = Size;

        NdisAcquireSpinLock(&MPMemoryLock);
        InsertTailList(&MPMemoryList, &pAllocInfo->List);
        MPAllocCount++;
        NdisReleaseSpinLock(&MPMemoryLock);
    }

    DbgPrintF(MP_LOUD,
        "%s - file %s, line %d, %d bytes, [0x"PTR_FORMAT"].", __FUNCTION__,
        FileNumber, LineNumber, Size, Pointer);

    return(Pointer);
}

VOID MPAuditFreeMem(
    IN PVOID  Pointer
    )
{
    PMP_ALLOCATION  pAllocInfo;

    pAllocInfo = CONTAINING_RECORD(Pointer, MP_ALLOCATION, UserData);

    ASSERT(pAllocInfo->Signature == (ULONG)'DOOG');

    NdisAcquireSpinLock(&MPMemoryLock);
    pAllocInfo->Signature = (ULONG)'DEAD';
    RemoveEntryList(&pAllocInfo->List);
    MPAllocCount--;
    NdisReleaseSpinLock(&MPMemoryLock);

    NdisFreeMemory(pAllocInfo, 0, 0);
}

VOID mpDbgPrintUnicodeString(
    IN  PUNICODE_STRING UnicodeString
    )
{
    UCHAR Buffer[256];

    USHORT i;

    for (i = 0; (i < UnicodeString->Length / 2) && (i < 255); i++) 
    {
        Buffer[i] = (UCHAR)UnicodeString->Buffer[i];
    }

#pragma prefast(suppress: __WARNING_POTENTIAL_BUFFER_OVERFLOW, "i is bounded by 255");
    Buffer[i] = '\0';

    DbgPrint("%s", Buffer);
}



// Hex dump 'cb' bytes starting at 'p' grouping 'ulGroup' bytes together.
// For example, with 'ulGroup' of 1, 2, and 4:
//
// 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |................|
// 0000 0000 0000 0000 0000 0000 0000 0000 |................|
// 00000000 00000000 00000000 00000000 |................|
//
// If 'fAddress' is true, the memory address dumped is prepended to each
// line.
//
VOID
Dump(
    __in_bcount(cb) IN CHAR*    p,
    IN ULONG                    cb,
    IN BOOLEAN                  fAddress,
    IN ULONG                    ulGroup 
    )
{
    INT cbLine;

    while (cb)
    {
        cbLine = (cb < DUMP_BytesPerLine) ? cb : DUMP_BytesPerLine;
#pragma prefast(suppress: __WARNING_POTENTIAL_BUFFER_OVERFLOW, "p is bounded by cb bytes");        
        DumpLine( p, cbLine, fAddress, ulGroup );
        cb -= cbLine;
        p += cbLine;
    }
}


VOID
DumpLine(
    __in_bcount(cb) IN CHAR*    p,
    IN ULONG                    cb,  
    IN BOOLEAN                  fAddress,
    IN ULONG                    ulGroup 
    )
{

    CHAR* pszDigits = "0123456789ABCDEF";
    CHAR szHex[ ((2 + 1) * DUMP_BytesPerLine) + 1 ];
    CHAR* pszHex = szHex;
    CHAR szAscii[ DUMP_BytesPerLine + 1 ];
    CHAR* pszAscii = szAscii;
    ULONG ulGrouped = 0;

    if (fAddress) 
    {
        DbgPrint( "RLTK: %p: ", p );
    }
    else 
    {
        DbgPrint( "RLTK: " );
    }

    while (cb)
    {
#pragma prefast(suppress: __WARNING_POTENTIAL_BUFFER_OVERFLOW, "pszHex accessed is always within bounds");    
        *pszHex++ = pszDigits[ ((UCHAR )*p) / 16 ];
        *pszHex++ = pszDigits[ ((UCHAR )*p) % 16 ];

        if (++ulGrouped >= ulGroup)
        {
            *pszHex++ = ' ';
            ulGrouped = 0;
        }

#pragma prefast(suppress: __WARNING_POTENTIAL_BUFFER_OVERFLOW, "pszAscii is bounded by cb bytes");
        *pszAscii++ = (*p >= 32 && *p < 128) ? *p : '.';

        ++p;
        --cb;
    }

    *pszHex = '\0';
    *pszAscii = '\0';

    DbgPrint(
        "%-*s|%-*s|\n",
        (2 * DUMP_BytesPerLine) + (DUMP_BytesPerLine / ulGroup), szHex,
        DUMP_BytesPerLine, szAscii );
}

void fxgmac_dump_buffer(unsigned char *skb, unsigned int len, int tx_rx)
{
    #define ZDbgPrintF(Level, ...) DbgPrint("fx-pat "  ##__VA_ARGS__);  

    unsigned char buffer[128];
    unsigned int i, j;
    char * caption[3] = {	"Tx",
    						"Rx",
    						"Buffer"};

    if(tx_rx > 2) tx_rx = 2;

    ZDbgPrintF(MP_TRACE, " ************** Data dump start ****************\n");
    ZDbgPrintF(MP_TRACE, " %s data of %d bytes\n", caption[tx_rx], len);

    for (i = 0; i < len; i += 32) {
    	unsigned int len_line = min(len - i, 32U);

    	for(j = 0; j < len_line; j++)
    	{
    		sprintf((char *)&buffer[j * 3], " %02x", (u8)skb[i + j]);
    	}
    	buffer[j * 3] = '\0';
    	ZDbgPrintF(MP_TRACE, "  %#06x: %s\n", i, buffer);
    }

    ZDbgPrintF(MP_TRACE, " ************** Data dump end ****************\n");

}

VOID
DbgPrintOidName(
    _In_  NDIS_OID  Oid)
{
    PCHAR oidName = NULL;

    switch (Oid) {

#undef MAKECASE
#define MAKECASE(oidx) case oidx: oidName = NIC_DBG_STRING #oidx "\n"; break;

        /* Operational OIDs */
        MAKECASE(OID_GEN_SUPPORTED_LIST)
            MAKECASE(OID_GEN_HARDWARE_STATUS)
            MAKECASE(OID_GEN_MEDIA_SUPPORTED)
            MAKECASE(OID_GEN_MEDIA_IN_USE)
            MAKECASE(OID_GEN_MAXIMUM_LOOKAHEAD)
            MAKECASE(OID_GEN_MAXIMUM_FRAME_SIZE)
            MAKECASE(OID_GEN_LINK_SPEED)
            MAKECASE(OID_GEN_TRANSMIT_BUFFER_SPACE)
            MAKECASE(OID_GEN_RECEIVE_BUFFER_SPACE)
            MAKECASE(OID_GEN_TRANSMIT_BLOCK_SIZE)
            MAKECASE(OID_GEN_RECEIVE_BLOCK_SIZE)
            MAKECASE(OID_GEN_VENDOR_ID)
            MAKECASE(OID_GEN_VENDOR_DESCRIPTION)
            MAKECASE(OID_GEN_VENDOR_DRIVER_VERSION)
            MAKECASE(OID_GEN_CURRENT_PACKET_FILTER)
            MAKECASE(OID_GEN_CURRENT_LOOKAHEAD)
            MAKECASE(OID_GEN_DRIVER_VERSION)
            MAKECASE(OID_GEN_MAXIMUM_TOTAL_SIZE)
            MAKECASE(OID_GEN_PROTOCOL_OPTIONS)
            MAKECASE(OID_GEN_MAC_OPTIONS)
            MAKECASE(OID_GEN_MEDIA_CONNECT_STATUS)
            MAKECASE(OID_GEN_MAXIMUM_SEND_PACKETS)
            MAKECASE(OID_GEN_SUPPORTED_GUIDS)
            MAKECASE(OID_GEN_NETWORK_LAYER_ADDRESSES)
            MAKECASE(OID_GEN_TRANSPORT_HEADER_OFFSET)
            MAKECASE(OID_GEN_MEDIA_CAPABILITIES)
            MAKECASE(OID_GEN_PHYSICAL_MEDIUM)
            MAKECASE(OID_GEN_MACHINE_NAME)
            MAKECASE(OID_GEN_VLAN_ID)
            MAKECASE(OID_GEN_RECEIVE_SCALE_PARAMETERS)
            MAKECASE(OID_GEN_RECEIVE_SCALE_CAPABILITIES)
            MAKECASE(OID_GEN_RECEIVE_HASH)
#if (NTDDI_VERSION >= NTDDI_WINBLUE) ||(NDIS_SUPPORT_NDIS640)
            MAKECASE(OID_GEN_ISOLATION_PARAMETERS)                 // query only
#endif
            MAKECASE(OID_GEN_RNDIS_CONFIG_PARAMETER)

            /* Operational OIDs for NDIS 6.0 */
            MAKECASE(OID_GEN_MAX_LINK_SPEED)
            MAKECASE(OID_GEN_LINK_STATE)
            MAKECASE(OID_GEN_LINK_PARAMETERS)
            MAKECASE(OID_GEN_MINIPORT_RESTART_ATTRIBUTES)
            MAKECASE(OID_GEN_ENUMERATE_PORTS)
            MAKECASE(OID_GEN_PORT_STATE)
            MAKECASE(OID_GEN_PORT_AUTHENTICATION_PARAMETERS)
            MAKECASE(OID_GEN_INTERRUPT_MODERATION)
            MAKECASE(OID_GEN_PHYSICAL_MEDIUM_EX)

            /* Statistical OIDs */
            MAKECASE(OID_GEN_XMIT_OK)
            MAKECASE(OID_GEN_RCV_OK)
            MAKECASE(OID_GEN_XMIT_ERROR)
            MAKECASE(OID_GEN_RCV_ERROR)
            MAKECASE(OID_GEN_RCV_NO_BUFFER)
            MAKECASE(OID_GEN_DIRECTED_BYTES_XMIT)
            MAKECASE(OID_GEN_DIRECTED_FRAMES_XMIT)
            MAKECASE(OID_GEN_MULTICAST_BYTES_XMIT)
            MAKECASE(OID_GEN_MULTICAST_FRAMES_XMIT)
            MAKECASE(OID_GEN_BROADCAST_BYTES_XMIT)
            MAKECASE(OID_GEN_BROADCAST_FRAMES_XMIT)
            MAKECASE(OID_GEN_DIRECTED_BYTES_RCV)
            MAKECASE(OID_GEN_DIRECTED_FRAMES_RCV)
            MAKECASE(OID_GEN_MULTICAST_BYTES_RCV)
            MAKECASE(OID_GEN_MULTICAST_FRAMES_RCV)
            MAKECASE(OID_GEN_BROADCAST_BYTES_RCV)
            MAKECASE(OID_GEN_BROADCAST_FRAMES_RCV)
            MAKECASE(OID_GEN_RCV_CRC_ERROR)
            MAKECASE(OID_GEN_TRANSMIT_QUEUE_LENGTH)

            /* Statistical OIDs for NDIS 6.0 */
            MAKECASE(OID_GEN_STATISTICS)
            MAKECASE(OID_GEN_BYTES_RCV)
            MAKECASE(OID_GEN_BYTES_XMIT)
            MAKECASE(OID_GEN_RCV_DISCARDS)
            MAKECASE(OID_GEN_XMIT_DISCARDS)

            /* Misc OIDs */
            MAKECASE(OID_GEN_GET_TIME_CAPS)
            MAKECASE(OID_GEN_GET_NETCARD_TIME)
            MAKECASE(OID_GEN_NETCARD_LOAD)
            MAKECASE(OID_GEN_DEVICE_PROFILE)
            MAKECASE(OID_GEN_INIT_TIME_MS)
            MAKECASE(OID_GEN_RESET_COUNTS)
            MAKECASE(OID_GEN_MEDIA_SENSE_COUNTS)

            /* PnP power management operational OIDs */
            MAKECASE(OID_PNP_SET_POWER)
            MAKECASE(OID_PNP_QUERY_POWER)
            MAKECASE(OID_PNP_CAPABILITIES)
            MAKECASE(OID_PNP_ADD_WAKE_UP_PATTERN)
            MAKECASE(OID_PNP_REMOVE_WAKE_UP_PATTERN)
            MAKECASE(OID_PNP_ENABLE_WAKE_UP)
            MAKECASE(OID_PM_HARDWARE_CAPABILITIES)
            MAKECASE(OID_PM_ADD_WOL_PATTERN)
            MAKECASE(OID_PM_REMOVE_WOL_PATTERN)
            MAKECASE(OID_PM_PARAMETERS)
            MAKECASE(OID_PM_WOL_PATTERN_LIST)
            MAKECASE(OID_PM_ADD_PROTOCOL_OFFLOAD)
            MAKECASE(OID_PM_REMOVE_PROTOCOL_OFFLOAD)
            MAKECASE(OID_PM_PROTOCOL_OFFLOAD_LIST)
  
            MAKECASE(OID_PNP_WAKE_UP_PATTERN_LIST)
            /* PnP power management statistical OIDs */
            MAKECASE(OID_PNP_WAKE_UP_ERROR)
            MAKECASE(OID_PNP_WAKE_UP_OK)

            /* Ethernet operational OIDs */
            MAKECASE(OID_802_3_PERMANENT_ADDRESS)
            MAKECASE(OID_802_3_CURRENT_ADDRESS)
            MAKECASE(OID_802_3_MULTICAST_LIST)
            MAKECASE(OID_802_3_MAXIMUM_LIST_SIZE)
            MAKECASE(OID_802_3_MAC_OPTIONS)

            /* Ethernet operational OIDs for NDIS 6.0 */
            MAKECASE(OID_802_3_ADD_MULTICAST_ADDRESS)
            MAKECASE(OID_802_3_DELETE_MULTICAST_ADDRESS)

            /* Ethernet statistical OIDs */
            MAKECASE(OID_802_3_RCV_ERROR_ALIGNMENT)
            MAKECASE(OID_802_3_XMIT_ONE_COLLISION)
            MAKECASE(OID_802_3_XMIT_MORE_COLLISIONS)
            MAKECASE(OID_802_3_XMIT_DEFERRED)
            MAKECASE(OID_802_3_XMIT_MAX_COLLISIONS)
            MAKECASE(OID_802_3_RCV_OVERRUN)
            MAKECASE(OID_802_3_XMIT_UNDERRUN)
            MAKECASE(OID_802_3_XMIT_HEARTBEAT_FAILURE)
            MAKECASE(OID_802_3_XMIT_TIMES_CRS_LOST)
            MAKECASE(OID_802_3_XMIT_LATE_COLLISIONS)

            /*  TCP/IP OIDs */
            MAKECASE(OID_TCP_TASK_OFFLOAD)
            MAKECASE(OID_TCP_TASK_IPSEC_ADD_SA)
            MAKECASE(OID_TCP_TASK_IPSEC_DELETE_SA)
            MAKECASE(OID_TCP_SAN_SUPPORT)
            MAKECASE(OID_TCP_TASK_IPSEC_ADD_UDPESP_SA)
            MAKECASE(OID_TCP_TASK_IPSEC_DELETE_UDPESP_SA)
            MAKECASE(OID_TCP4_OFFLOAD_STATS)
            MAKECASE(OID_TCP6_OFFLOAD_STATS)
            MAKECASE(OID_IP4_OFFLOAD_STATS)
            MAKECASE(OID_IP6_OFFLOAD_STATS)

            /* TCP offload OIDs for NDIS 6 */
            MAKECASE(OID_TCP_OFFLOAD_CURRENT_CONFIG)
            MAKECASE(OID_TCP_OFFLOAD_PARAMETERS)
            MAKECASE(OID_TCP_OFFLOAD_HARDWARE_CAPABILITIES)
            MAKECASE(OID_TCP_CONNECTION_OFFLOAD_CURRENT_CONFIG)
            MAKECASE(OID_TCP_CONNECTION_OFFLOAD_HARDWARE_CAPABILITIES)
            MAKECASE(OID_OFFLOAD_ENCAPSULATION)

#if (NDIS_SUPPORT_NDIS620)
            /* VMQ OIDs for NDIS 6.20 */
            MAKECASE(OID_RECEIVE_FILTER_FREE_QUEUE)
            MAKECASE(OID_RECEIVE_FILTER_CLEAR_FILTER)
            MAKECASE(OID_RECEIVE_FILTER_ALLOCATE_QUEUE)
            MAKECASE(OID_RECEIVE_FILTER_QUEUE_ALLOCATION_COMPLETE)
            MAKECASE(OID_RECEIVE_FILTER_SET_FILTER)
#endif

#if (NDIS_SUPPORT_NDIS630 || NDIS_SUPPORT_NDIS680)
            /* NDIS QoS OIDs for NDIS 6.30 */
            MAKECASE(OID_QOS_PARAMETERS)
#endif

#if (NDIS_SUPPORT_NDIS680)
            /* RSSv2 OIDS*/
            MAKECASE(OID_GEN_RSS_SET_INDIRECTION_TABLE_ENTRIES)
            MAKECASE(OID_GEN_RECEIVE_SCALE_PARAMETERS_V2)
#endif
    }

    if (oidName)
    {
        DbgPrintF(MP_LOUD, "%s", oidName);
    }
    else
    {
        DbgPrintF(MP_LOUD, "<** Unknown OID 0x%08x **>.", Oid);
    }
}

VOID
DbgPrintAddress(
    _In_reads_bytes_(ETH_LENGTH_OF_ADDRESS) PUCHAR Address)
{
    // If your MAC address has a different size, adjust the printf accordingly.
    //{ASSERT(ETH_LENGTH_OF_ADDRESS == 6); }

    DbgPrintF(MP_LOUD, "%02x-%02x-%02x-%02x-%02x-%02x.",
        Address[0], Address[1], Address[2],
        Address[3], Address[4], Address[5]);
}
#endif

#elif defined(LINUX)

#elif defined(UBOOT)

#else

#endif

