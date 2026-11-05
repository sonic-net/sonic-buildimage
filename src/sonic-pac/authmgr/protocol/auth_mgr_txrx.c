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

#define  MAC_EAPOL_PDU

#include "auth_mgr_include.h"
#include "auth_mgr_struct.h"
#include "auth_mgr_control.h"
#include <arpa/inet.h>
#include "utils_api.h"
#include "sysapi.h"
#include <net/if.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include "fpSonicUtils.h"


extern authmgrCB_t *authmgrCB;

static  enetMacAddr_t  EAPOL_PDU_MAC_ADDR =
{{0x01, 0x80, 0xC2, 0x00, 0x00, 0x03}};


/**************************************************************************
 * @purpose   Transmit a EAPOL EAP Success to Supplicant
 *
 * @param     lIntIfNum @b{(input)} internal interface number
 * @param     portType @b{(input)} physical or logical interface 
 *
 * @returns   void
 *
 * @comments
 *
 * @end
 *************************************************************************/
void authmgrTxCannedSuccess (uint32 lIntIfNum, authmgrPortType_t portType)
{
   netBufHandle bufHandle =  NULL;
   uchar8 *data;
   enetHeader_t *enetHdr;
   enet_encaps_t *encap;
   uchar8 baseMac[ MAC_ADDR_LEN];
   eapolPacket_t *eapolPkt;
   authmgrEapPacket_t *eapPkt;
  uint32 length;
  authmgrLogicalPortInfo_t *logicalPortInfo;
  uint32 intIfNum = lIntIfNum;

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_API_CALLS, intIfNum,
                       "%s: called for port %d, type %d\n",
                          __FUNCTION__, lIntIfNum, portType);

  if (portType == AUTHMGR_LOGICAL_PORT)
  {
    intIfNum = authmgrPhysPortGet (lIntIfNum);
  }
  SYSAPI_NET_MBUF_GET (bufHandle);
  if (bufHandle ==  NULL)
  {
     LOGF ( LOG_SEVERITY_NOTICE,
             "authmgrTxReqId: Out of System buffers."
             " 802.1X cannot process/transmit message due to lack of internal buffers");
    return;
  }

  SYSAPI_NET_MBUF_GET_DATASTART (bufHandle, data);

  /* Set ethernet header */
  enetHdr = ( enetHeader_t *) (data);

  if (nimGetIntfAddress(intIfNum,  NULL, baseMac) !=  SUCCESS)
  {
     LOGF ( LOG_SEVERITY_NOTICE,
             "802.1X could not send EAP_SUCCESS - Could not get MAC address for interface %d", intIfNum);
    SYSAPI_NET_MBUF_FREE (bufHandle);
    return;
  }

  /* Set dest and source MAC in ethernet header */
  memcpy (enetHdr->dest.addr,  EAPOL_PDU_MAC_ADDR.addr,  MAC_ADDR_LEN);
  memcpy (enetHdr->src.addr, baseMac,  MAC_ADDR_LEN);

  /* Set ethernet type */
  encap = ( enet_encaps_t *) (( uchar8 *) enetHdr +  ENET_HDR_SIZE);
  encap->type = osapiHtons ( ETYPE_EAPOL);

  /* Set EAPOL header */
  eapolPkt =
    ( eapolPacket_t *) (( uchar8 *) encap +  ENET_ENCAPS_HDR_SIZE);
  eapolPkt->protocolVersion =  DOT1X_PAE_PORT_PROTOCOL_VERSION_2;
  eapolPkt->packetType = EAPOL_EAPPKT;
  eapolPkt->packetBodyLength =
    osapiHtons (( ushort16) (sizeof ( authmgrEapPacket_t)));

  /* Set EAP header */
  eapPkt =
    ( authmgrEapPacket_t *) (( uchar8 *) eapolPkt + sizeof ( eapolPacket_t));
  eapPkt->code = EAP_SUCCESS;

  if (portType == AUTHMGR_PHYSICAL_PORT)
  {
    eapPkt->id = authmgrCB->globalInfo->authmgrPortInfo[intIfNum].currentId;
  }
  else
  {
    logicalPortInfo = authmgrLogicalPortInfoGet (lIntIfNum);
    if (logicalPortInfo)
    {
      eapPkt->id = logicalPortInfo->client.currentIdL;
    }
  }

  eapPkt->length = osapiHtons (( ushort16) (sizeof ( authmgrEapPacket_t)));

  length = (uint32) ( ENET_HDR_SIZE +  ENET_ENCAPS_HDR_SIZE +
                        sizeof ( eapolPacket_t) + sizeof ( authmgrEapPacket_t));
  SYSAPI_NET_MBUF_SET_DATALENGTH (bufHandle, length);

  authmgrFrameTransmit (lIntIfNum, bufHandle, portType);
}

/**************************************************************************
 * @purpose   Transmit a EAPOL EAP Failure to Supplicant
 *
 * @param     lIntIfNum @b{(input)} internal interface number
 * @param     portType @b{(input)} physical or logical interface 
 *
 * @returns   void
 *
 * @comments
 *
 * @end
 *************************************************************************/
void authmgrTxCannedFail (uint32 lIntIfNum, authmgrPortType_t portType)
{
   netBufHandle bufHandle =  NULL;
   uchar8 *data;
   enetHeader_t *enetHdr;
   enet_encaps_t *encap;
   uchar8 baseMac[ MAC_ADDR_LEN];
   eapolPacket_t *eapolPkt;
   authmgrEapPacket_t *eapPkt;
  uint32 length;
  authmgrLogicalPortInfo_t *logicalPortInfo;
  uint32 intIfNum = lIntIfNum;

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_API_CALLS, intIfNum,
                       "%s: called for port %d, type %d\n",
                          __FUNCTION__, lIntIfNum, portType);

  if (portType == AUTHMGR_LOGICAL_PORT)
  {
    intIfNum = authmgrPhysPortGet (lIntIfNum);
  }

  SYSAPI_NET_MBUF_GET (bufHandle);
  if (bufHandle ==  NULL)
  {
     LOGF ( LOG_SEVERITY_NOTICE,
             "authmgrTxCannedFail: Out of System buffers."
             " 802.1X cannot process/transmit message due to lack of internal buffers");
    return;
  }

  SYSAPI_NET_MBUF_GET_DATASTART (bufHandle, data);

  /* Set ethernet header */
  enetHdr = ( enetHeader_t *) (data);
  if (nimGetIntfAddress(intIfNum,  NULL, baseMac) !=  SUCCESS)
  {
     LOGF ( LOG_SEVERITY_NOTICE,
             "802.1X could send EAP_FAILURE - Could not get MAC address for interface %d", intIfNum);
    SYSAPI_NET_MBUF_FREE (bufHandle);
    return;
  }

  /* Set dest and source MAC in ethernet header */
  memcpy (enetHdr->dest.addr,  EAPOL_PDU_MAC_ADDR.addr,  MAC_ADDR_LEN);
  memcpy (enetHdr->src.addr, baseMac,  MAC_ADDR_LEN);

  /* Set ethernet type */
  encap = ( enet_encaps_t *) (( uchar8 *) enetHdr +  ENET_HDR_SIZE);
  encap->type = osapiHtons ( ETYPE_EAPOL);

  /* Set EAPOL header */
  eapolPkt =
    ( eapolPacket_t *) (( uchar8 *) encap +  ENET_ENCAPS_HDR_SIZE);
  eapolPkt->protocolVersion =  DOT1X_PAE_PORT_PROTOCOL_VERSION_2;
  eapolPkt->packetType = EAPOL_EAPPKT;
  eapolPkt->packetBodyLength =
    osapiHtons (( ushort16) (sizeof ( authmgrEapPacket_t)));

  /* Set EAP header */
  eapPkt =
    ( authmgrEapPacket_t *) (( uchar8 *) eapolPkt + sizeof ( eapolPacket_t));
  eapPkt->code = EAP_FAILURE;

  if (portType == AUTHMGR_PHYSICAL_PORT)
  {
    eapPkt->id = authmgrCB->globalInfo->authmgrPortInfo[intIfNum].currentId;
  }
  else
  {
    logicalPortInfo = authmgrLogicalPortInfoGet (lIntIfNum);
    if (logicalPortInfo)
    {
      eapPkt->id = logicalPortInfo->client.currentIdL;
    }
  }

  eapPkt->length = osapiHtons (( ushort16) (sizeof ( authmgrEapPacket_t)));

  length = (uint32) ( ENET_HDR_SIZE +  ENET_ENCAPS_HDR_SIZE +
                        sizeof ( eapolPacket_t) + sizeof ( authmgrEapPacket_t));
  SYSAPI_NET_MBUF_SET_DATALENGTH (bufHandle, length);

  authmgrFrameTransmit (lIntIfNum, bufHandle, portType);
}

/*********************************************************************
* @purpose  Transmit a frame
*
* @param    lIntIfNum   @b{(input)} outgoing internal interface number
* @param    bufHandle  @b{(input)} handle to the network buffer where frame is
*                                  stored
 * @param     portType @b{(input)} physical or logical interface 
*
* @returns  void
*
* @comments
*
* @end
*********************************************************************/
void authmgrFrameTransmit (uint32 lIntIfNum,  netBufHandle bufHandle,
                           authmgrPortType_t portType)
{
   uchar8 *data, tmpMac[ ENET_MAC_ADDR_LEN];
  uint32 len = 0;
   uchar8 ifname[ NIM_IF_ALIAS_SIZE + 1] = {0};
  struct sockaddr_ll addr;
   int32 ret;
  uint32 intIfNum = authmgrPhysPortGet (lIntIfNum);

  SYSAPI_NET_MBUF_GET_DATASTART (bufHandle, data);

  if (portType == AUTHMGR_LOGICAL_PORT)
  {
    authmgrLogicalPortInfo_t *logicalPortInfo;
    logicalPortInfo = authmgrLogicalPortInfoGet (lIntIfNum);
    if ( NULLPTR == logicalPortInfo ||  0 == logicalPortInfo->key.keyNum)
    {
      SYSAPI_NET_MBUF_FREE (bufHandle);
      return;
    }
    memset (tmpMac, 0,  ENET_MAC_ADDR_LEN);
    if (memcmp
        (logicalPortInfo->client.suppMacAddr.addr, tmpMac,
          ENET_MAC_ADDR_LEN) != 0)
    {
      memcpy (data, logicalPortInfo->client.suppMacAddr.addr,
           ENET_MAC_ADDR_LEN);
    }


    fpGetHostIntfName(intIfNum, ifname);

    memset(&addr, 0, sizeof(addr));
    addr.sll_family = AF_PACKET;
    addr.sll_ifindex = if_nametoindex((const char *)ifname);

    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_API_CALLS, intIfNum,
                         "%s: Opening raw packet socket for port %d\n", 
                         __FUNCTION__, intIfNum);


    SYSAPI_NET_MBUF_GET_DATALENGTH(bufHandle, len);

    ret = (int) sendto(authmgrCB->globalInfo->eap_socket, data, len, 0,
        (const struct sockaddr*)&addr, sizeof(struct sockaddr_ll));

    if (ret < 0)
    {
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_API_CALLS, intIfNum,
                           "%s: send failed for port %d\n", 
                           __FUNCTION__, intIfNum);
    }

  }

  SYSAPI_NET_MBUF_FREE (bufHandle);
}


