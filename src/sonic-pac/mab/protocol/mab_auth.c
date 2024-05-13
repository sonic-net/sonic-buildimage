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

#include "mab_include.h"
#include "sysapi.h"
#include "mab_db.h"
#include "auth_mgr_exports.h"
#include "mab_client.h"
#include "mab_struct.h"
#include "mab_timer.h"

extern mabBlock_t *mabBlock;

/*********************************************************************
 * @purpose Get the authentication method for the user of this port
 *
 * @param   logicalPortInfo  @b{(input))  Logical Port Info node
 * @param   bufHandle  @b{(input)} the handle to the mab PDU
 * @param   authMethod @b{(output)} user's authentication method
 *
 * @returns  SUCCESS
 * @returns  FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
static
RC_t mabSmAuthMethodGet(mabLogicalPortInfo_t *logicalPortInf,
     netBufHandle bufHandle,
     USER_MGR_AUTH_METHOD_t *authMethod)
{
   uchar8 *data;
   enetHeader_t *enetHdr;
   eapolPacket_t *eapolPkt;
   authmgrEapPacket_t *eapPkt;
   eapRrPacket_t *eapRrPkt= NULLPTR;
  RC_t rc =  FAILURE;

  if (bufHandle !=  NULL)
  {
    SYSAPI_NET_MBUF_GET_DATASTART(bufHandle, data);
    enetHdr = ( enetHeader_t *)data;
    eapolPkt = ( eapolPacket_t *)(( uchar8 *)enetHdr +  ENET_HDR_SIZE +  ENET_ENCAPS_HDR_SIZE);
    eapPkt = ( authmgrEapPacket_t *)(( uchar8 *)eapolPkt + sizeof( eapolPacket_t));
    eapRrPkt = ( eapRrPacket_t *)(( uchar8 *)eapPkt + sizeof( authmgrEapPacket_t));
  }

  /* attempt authentication with RADIUS */
  *authMethod =  AUTH_METHOD_RADIUS;

  logicalPortInf->client.authMethod = *authMethod;
  rc =  SUCCESS;

  return rc;
}

/*********************************************************************
 * @purpose  Actions to be performed when sending request to a client
 *
 * @param   logicalPortInfo  @b{(input))  Logical Port Info node
 *
 * @returns  SUCCESS
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabClientRequestAction(mabLogicalPortInfo_t *logicalPortInfo,  netBufHandle bufHandle)
{
  uint32  physPort = 0, lPort = 0, type = 0;
   uchar8 ch, *data =  NULLPTR;
   radiusAttr_t *eapTlv =  NULLPTR;
   BOOL generateNak =  FALSE;

  MAB_IF_NULLPTR_RETURN_LOG(logicalPortInfo);
  MAB_LPORT_KEY_UNPACK(physPort, lPort, type, logicalPortInfo->key.keyNum);

  logicalPortInfo->client.currentIdL = logicalPortInfo->client.attrInfo.idFromServer;

  if ( AUTHMGR_PORT_MAB_AUTH_TYPE_INVALID != logicalPortInfo->client.mabAuthType)
  {
    if(bufHandle !=  NULL) 
    {
      SYSAPI_NET_MBUF_GET_DATASTART(bufHandle, data);
      eapTlv = ( radiusAttr_t *) (data +  ENET_HDR_SIZE +  ENET_ENCAPS_HDR_SIZE + 
          sizeof( eapolPacket_t) + sizeof( authmgrEapPacket_t));
      ch = ( uchar8)EAP_RRMD5;
      if (memcmp(&eapTlv->type,( uchar8 *)&ch,sizeof( uchar8)) !=0)
      {
        /* generate NAK for unsupported EAP types */
        generateNak =  TRUE;
      }
      /* Free bufHandle as another one will be allocated in mabCtlLogicalPortMABGenResp() */
      SYSAPI_NET_MBUF_FREE(bufHandle);
    }
    mabCtlLogicalPortMABGenResp(logicalPortInfo->key.keyNum, generateNak);
  }
  else
  {
    if ( NULL != bufHandle)
    {
      SYSAPI_NET_MBUF_FREE(bufHandle);
    }
  }

  return  SUCCESS;
}

/*********************************************************************
 * @purpose  Actions to be performed when sending response to AAA
 *
 * @param   logicalPortInfo  @b{(input))  Logical Port Info node
 *
 * @returns  SUCCESS
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabClientResponseAction(mabLogicalPortInfo_t *logicalPortInfo,  netBufHandle bufHandle)
{
  mabPortCfg_t * pCfg;
   USER_MGR_AUTH_METHOD_t authMethod;
  uint32  physPort, lPort = 0, type = 0;

  MAB_IF_NULLPTR_RETURN_LOG(logicalPortInfo);
  MAB_LPORT_KEY_UNPACK(physPort, lPort, type, logicalPortInfo->key.keyNum);

  if (mabIntfIsConfigurable(physPort, &pCfg) !=  TRUE)
    return  FAILURE;

  /* start the timer for the Server timeout */
  if ( SUCCESS != mabTimerStart(logicalPortInfo, MAB_SERVER_AWHILE))
  {
    return  FAILURE;
  }

  if (logicalPortInfo->client.suppBufHandle !=  NULL)
  {
    SYSAPI_NET_MBUF_FREE(logicalPortInfo->client.suppBufHandle);
    logicalPortInfo->client.suppBufHandle =  NULL;
  }

  if (mabSmAuthMethodGet(logicalPortInfo, bufHandle, &authMethod) ==  SUCCESS)
  {
    switch (authMethod)
    {
      case  AUTH_METHOD_UNDEFINED:
      case  AUTH_METHOD_REJECT:
        /* reset port from the existing vlans*/
        MAB_EVENT_TRACE(
            "\n%s:Reset vlan Type %s vlan Id %d participation "
            "for linterface = %d as auth method is reject or undefined. \n",
            __FUNCTION__, mabVlanTypeStringGet(logicalPortInfo->client.vlanType),
            logicalPortInfo->client.vlanId, logicalPortInfo->key.keyNum);

        logicalPortInfo->protocol.authFail =  TRUE;

        mabUnAuthenticatedAction(logicalPortInfo);

        break;

      case  AUTH_METHOD_LOCAL:
        /* attempt local authentication */
        (void)mabLocalAuthResponseProcess(logicalPortInfo, bufHandle);
        break;

      case  AUTH_METHOD_RADIUS:
        /* attempt authentication with RADIUS */
        (void)mabRadiusSuppResponseProcess(logicalPortInfo->key.keyNum, bufHandle);
        break;
      case  AUTH_METHOD_NONE:
        /* authorize */
        mabAuthenticatedAction(logicalPortInfo);

        break;

      default:
        /* reset port in any mab assigned vlans*/
        MAB_EVENT_TRACE(
            "\n%s:Reset vlan Type %s vlan Id %d participation "
            "for linterface = %d as auth method is unsupported. \n",
            __FUNCTION__, mabVlanTypeStringGet(logicalPortInfo->client.vlanType),
            logicalPortInfo->client.vlanId, logicalPortInfo->key.keyNum);
        mabUnAuthenticatedAction(logicalPortInfo);

        break;
    }
  }
  else
  {
    /* If we couldn't get an auth method, log an error and fail the authentication */
     LOGF( LOG_SEVERITY_NOTICE,
        "%s: Failed getting auth method, logical port %d."
        " Could not determine the authentication method to be used . "
        "Probably because of  a mis-configuration.", __FUNCTION__, logicalPortInfo->key.keyNum);
    /* reset port in any mab assigned vlans*/
    MAB_EVENT_TRACE(
        "\n%s:Reset vlan Type %s vlan Id %d participation "
        "for linterface = %d as auth method is unsupported. \n",
        __FUNCTION__, mabVlanTypeStringGet(logicalPortInfo->client.vlanType),
        logicalPortInfo->client.vlanId, logicalPortInfo->key.keyNum);

    mabUnAuthenticatedAction(logicalPortInfo);
  }

  return  SUCCESS;
}


