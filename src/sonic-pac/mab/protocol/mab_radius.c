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
#include "mab_auth.h"
#include "mab_client.h"
#include "mab_control.h"
#include "mab_timer.h"
#include "utils_api.h"
#include "sysapi.h"
#include "includes.h"
#include "common.h"
#include "eloop.h"
#include "radius.h"
#include "radius_client.h"
#include "mab_radius.h"
#include "osapi_sem.h"

#define RADIUS_STATUS_SUCCESS            1
#define RADIUS_STATUS_CHALLENGED         2
#define RADIUS_STATUS_AUTHEN_FAILURE     3
#define RADIUS_STATUS_REQUEST_TIMED_OUT  4
#define RADIUS_STATUS_COMM_FAILURE       5
#define RADIUS_STATUS_ALL_AUTH_SERVERS_DEAD   6

static  enetMacAddr_t  EAPOL_PDU_MAC_ADDR =
{{0x01, 0x80, 0xC2, 0x00, 0x00, 0x03}};

#include "mab_struct.h"

extern mabBlock_t *mabBlock;

/**************************************************************************
 * @purpose   Handle RADIUS client callbacks
 *
 * @param     status         @b{(input)} status of RADIUS response (accept, reject, challenge, etc)
 * @param     correlator     @b{(input)} correlates responses to requests; for mab, this is ifIndex
 * @param     *attributes    @b{(input)} RADIUS attribute data
 * @param     attributesLen  @b{(input)} length of RADIUS attribute data
 *
 * @returns    SUCCESS
 * @returns    FAILURE
 *
 * @comments
 *
 * @end
 *************************************************************************/
RC_t mabRadiusResponseCallback (void *msg,
                                   unsigned int correlator)
{
  mabAaaMsg_t mabAaaMsg;

  /* Fill in AAA message structure */
  bzero(( uchar8 *)&mabAaaMsg, sizeof(mabAaaMsg_t));
  mabAaaMsg.resp = msg;

  return mabIssueCmd(mabAaaInfoReceived, correlator, &mabAaaMsg);
}

int mab_radius_resp_code_map(unsigned int code, unsigned int *status)
{
  switch (code)
  {
    case RADIUS_CODE_ACCESS_ACCEPT:
         *status = RADIUS_STATUS_SUCCESS;
         break;

    case RADIUS_CODE_ACCESS_REJECT:
         *status = RADIUS_STATUS_AUTHEN_FAILURE;
         break;

    case RADIUS_CODE_ACCESS_CHALLENGE:
         *status = RADIUS_STATUS_CHALLENGED;
         break;
    default:
        *status = RADIUS_STATUS_REQUEST_TIMED_OUT;
  }

 return 0;
}


/**************************************************************************
 * @purpose   Process RADIUS Server responses
 *
 * @param     lIntIfNum       @b{(input)} Logical internal interface number of port being authenticated
 * @param     status         @b{(input)} status of RADIUS response (accept, reject, challenge, etc)
 * @param     *attributes    @b{(input)} RADIUS attribute data
 * @param     attributesLen  @b{(input)} length of RADIUS attribute data
 *
 * @returns    SUCCESS
 * @returns    FAILURE
 *
 * @comments
 *
 * @end
 *************************************************************************/
RC_t mabRadiusResponseProcess(unsigned int lIntIfNum, void *resp) 
{
  mabLogicalPortInfo_t *logicalPortInfo;
  mabPortCfg_t *pCfg;
   RC_t rc =  FAILURE;
   uchar8 ifName[ NIM_IF_ALIAS_SIZE + 1];
  uint32 physPort = 0, lPort = 0, type = 0;
  uint32 status = 0, code = 0;

  if (!(MAB_IS_READY))
    return  SUCCESS;
  logicalPortInfo = mabLogicalPortInfoGet(lIntIfNum);
  if (logicalPortInfo ==  NULLPTR)
  {
    return  FAILURE;
  }

  MAB_LPORT_KEY_UNPACK(physPort, lPort, type, lIntIfNum);

  if (mabIntfIsConfigurable(physPort, &pCfg) !=  TRUE)
    return  FAILURE;
/* get the response code */
  if (-1 == radius_get_resp_code(resp, &code))
    return  FAILURE;

  nimGetIntfName(physPort,  ALIASNAME, ifName);
  MAB_EVENT_TRACE(
      "%s:Received Radius response message on logicalPort:[%s] with status[%d]\n\r",
      __FUNCTION__,ifName, code);

   mab_radius_resp_code_map(code, &status);

  /* Ensure we are expecting a response from the server */
  if (logicalPortInfo->protocol.mabAuthState == MAB_AUTHENTICATING)
  {
    switch (status)
    {
      case RADIUS_STATUS_SUCCESS:
        rc = mabRadiusAcceptProcess(lIntIfNum, resp);

        if (logicalPortInfo->client.attrInfo.serverStateLen != 0)
        {
           memset(logicalPortInfo->client.attrInfo.serverState,0, MAB_SERVER_STATE_LEN);
           logicalPortInfo->client.attrInfo.serverStateLen = 0;
        }

        break;

      case RADIUS_STATUS_CHALLENGED:
        rc = mabRadiusChallengeProcess(lIntIfNum, resp);
        break;

      case RADIUS_STATUS_AUTHEN_FAILURE:

        /* response from radius server received.
           stop the serverwhile timer */
        mabTimerDestroy(mabBlock->mabTimerCB, logicalPortInfo);


        /* Initialize state to NULL as the session has been ended with a failure */ 
        if (logicalPortInfo->client.attrInfo.serverStateLen != 0)
        {
           memset(logicalPortInfo->client.attrInfo.serverState,0, MAB_SERVER_STATE_LEN);
           logicalPortInfo->client.attrInfo.serverStateLen = 0;
        }

          /* Log the information */
           LOGF( LOG_SEVERITY_NOTICE,
              "Radius Authentication failed on physPort:[%s] lIntIfNum:[%d]"
               "Mac Address :[%02x:%02x:%02x:%02x:%02x:%02x].\n\r",
               ifName, lIntIfNum,
               logicalPortInfo->client.suppMacAddr.addr[0],
               logicalPortInfo->client.suppMacAddr.addr[1],
               logicalPortInfo->client.suppMacAddr.addr[2],
               logicalPortInfo->client.suppMacAddr.addr[3],
               logicalPortInfo->client.suppMacAddr.addr[4],
               logicalPortInfo->client.suppMacAddr.addr[5]);

            logicalPortInfo->protocol.authFail =  TRUE;
            mabUnAuthenticatedAction(logicalPortInfo);
        break;

      case RADIUS_STATUS_REQUEST_TIMED_OUT:
      case RADIUS_STATUS_COMM_FAILURE:

        MAB_EVENT_TRACE(
            "Reason:'%s' on physPort:[%d] logical interface:[%s]\n\r",
            (status == RADIUS_STATUS_COMM_FAILURE)?"RADIUS_STATUS_COMM_FAILURE":"RADIUS_STATUS_REQUEST_TIMED_OUT",
            physPort, ifName);

        /* response from radius server received.
           stop the serverwhile timer */
        mabTimerDestroy(mabBlock->mabTimerCB, logicalPortInfo);

        /* Initialize state to NULL as the session has been ended with a failure */
        if (logicalPortInfo->client.attrInfo.serverStateLen != 0)
        {
           memset(logicalPortInfo->client.attrInfo.serverState,0, MAB_SERVER_STATE_LEN);
           logicalPortInfo->client.attrInfo.serverStateLen = 0;
        }

        LOGF(LOG_SEVERITY_NOTICE,
             "Failed to authenticate on logical interface %s.", ifName);

        logicalPortInfo->protocol.authFail =  TRUE;
        mabUnAuthenticatedAction(logicalPortInfo);

        break;
 
      case RADIUS_STATUS_ALL_AUTH_SERVERS_DEAD:

        /* All Radius Auth servers unreachable. Go through Inaccessible Auth Bypass */
         LOGF( LOG_SEVERITY_INFO, "All RADIUS Servers Dead.");

        /* response from radius server received.
           stop the serverwhile timer */
        mabTimerDestroy(mabBlock->mabTimerCB, logicalPortInfo);

        logicalPortInfo->protocol.authFail =  TRUE;
        mabUnAuthenticatedAction(logicalPortInfo);

        break;

      default:
        rc =  FAILURE;
        /* Log the information */
         LOGF( LOG_SEVERITY_NOTICE,
            "Failed to authenticate on interface %s. Received an invalid RADIUS status type.", ifName);
        break;
    }
  }

    radius_msg_free(resp);
  return rc;
}

/**************************************************************************
 * @purpose   Process RADIUS Challenge from server
 *
 * @param     lIntIfNum       @b{(input)} Logical  interface number of port being authenticated
 * @param     *radiusPayload  @b{(input)} payload of RADIUS frame (attributes)
 * @param     payloadLen      @b{(input)} length of RADIUS payload
 *
 * @returns    SUCCESS
 * @returns    FAILURE
 *
 * @comments
 *
 * @end
 *************************************************************************/
RC_t mabRadiusChallengeProcess(uint32 lIntIfNum, void *resp)
{
   netBufHandle bufHandle;
   uchar8 *data, *eapBuf;
   enetHeader_t *enetHdr;
   enet_encaps_t *encap;
   uchar8 srcMac[ MAC_ADDR_LEN];
   eapolPacket_t *eapolPkt;
   authmgrEapPacket_t *eapPkt;
  uint32 length;
  RC_t rc =  SUCCESS;
  mabPortCfg_t *pCfg;
  mabLogicalPortInfo_t *logicalPortInfo;
  uint32 physPort = 0, lPort = 0, type = 0;
   uchar8 ifName[ NIM_IF_ALIAS_SIZE + 1];
  challenge_info_t get_data;

  logicalPortInfo = mabLogicalPortInfoGet(lIntIfNum);
  if ((logicalPortInfo ==  NULLPTR) || ( NULLPTR == resp))
  {
    return  FAILURE;
  }

  MAB_LPORT_KEY_UNPACK(physPort, lPort, type, lIntIfNum);

  if (mabIntfIsConfigurable(physPort, &pCfg) !=  TRUE)
    return  FAILURE;

  /* response from radius server received.
     stop the serverwhile timer */
  mabTimerDestroy(mabBlock->mabTimerCB, logicalPortInfo);

  nimGetIntfName(physPort,  ALIASNAME, ifName);
  MAB_EVENT_TRACE(
      "%s:Received Radius Challenge message for "
      "client - %02X:%02X:%02X:%02X:%02X:%02X on port - %s[%d]\n",
      __FUNCTION__,( uchar8)logicalPortInfo->client.suppMacAddr.addr[0],
      ( uchar8)logicalPortInfo->client.suppMacAddr.addr[1],
      ( uchar8)logicalPortInfo->client.suppMacAddr.addr[2],
      ( uchar8)logicalPortInfo->client.suppMacAddr.addr[3],
      ( uchar8)logicalPortInfo->client.suppMacAddr.addr[4],
      ( uchar8)logicalPortInfo->client.suppMacAddr.addr[5], ifName, lIntIfNum);

   nimGetIntfAddress(physPort,  NULL, srcMac);
  
  SYSAPI_NET_MBUF_GET(bufHandle);
  if (bufHandle ==  NULL)
  {
     LOGF( LOG_SEVERITY_NOTICE,
        "mabRadiusChallengeProcess: Out of system buffers."
        " 802.1X cannot process/transmit message due to lack of internal buffers");
    return  FAILURE;
  }

 SYSAPI_NET_MBUF_GET_DATASTART(bufHandle, data);
  /* Set ethernet header */
  enetHdr = ( enetHeader_t *)(data);
  /* Set dest and source MAC in ethernet header */
  memcpy(enetHdr->dest.addr,  EAPOL_PDU_MAC_ADDR.addr,  MAC_ADDR_LEN);
  memcpy(enetHdr->src.addr, srcMac,  MAC_ADDR_LEN);

  /* Set ethernet type */
  encap = ( enet_encaps_t *)(( uchar8 *)enetHdr +  ENET_HDR_SIZE);
  encap->type = osapiHtons( ETYPE_EAPOL);

  /* Set EAPOL header */
  eapolPkt = ( eapolPacket_t *)(( uchar8 *)encap +  ENET_ENCAPS_HDR_SIZE);
  eapolPkt->protocolVersion =  MAB_PAE_PORT_PROTOCOL_VERSION_1;
  eapolPkt->packetType = EAPOL_EAPPKT;

  eapPkt = ( authmgrEapPacket_t *)(( uchar8 *)eapolPkt + sizeof( eapolPacket_t));
  eapBuf = ( uchar8 *)eapPkt;

  logicalPortInfo->client.attrInfo.rcvdEapAttr =  FALSE;
  if ( NULLPTR != resp)
  {
    memset(&get_data, 0, sizeof(get_data));
    get_data.nas_port = lIntIfNum;
    get_data.challenge = logicalPortInfo->client.mabChallenge;
    get_data.challenge_len = &logicalPortInfo->client.mabChallengelen;
    /*get_data.challenge_len = 32;*/

    get_data.attrInfo = &logicalPortInfo->client.attrInfo;
    get_data.supp_data = eapBuf;


    rc = radiusClientChallengeProcess(resp, &get_data);
    if ( SUCCESS != rc)
    {
      SYSAPI_NET_MBUF_FREE(bufHandle);
      return  FAILURE;
    }

    if (0 == logicalPortInfo->client.attrInfo.accessLevel)
           logicalPortInfo->client.attrInfo.accessLevel = RADIUS_SERVICE_TYPE_LOGIN;


      if ((logicalPortInfo->client.attrInfo.accessLevel != RADIUS_SERVICE_TYPE_ADMIN) &&
           (logicalPortInfo->client.attrInfo.accessLevel != RADIUS_SERVICE_TYPE_LOGIN) &&
           (RADIUS_SERVICE_TYPE_CALL_CHECK != logicalPortInfo->client.attrInfo.accessLevel))
       {
          LOGF( LOG_SEVERITY_NOTICE,
                 "Received an unsupported service-type value (%d) in the "
                 "radius challenge message, sending EAP failure to the "
                 "client %02X:%02X:%02X:%02X:%02X:%02X."
                 "Supported values are 'Login' and 'Admin'.Modify the radius "
                 "server settings with supported service-type",
                 logicalPortInfo->client.attrInfo.accessLevel, ( uchar8)logicalPortInfo->client.suppMacAddr.addr[0],
                 ( uchar8)logicalPortInfo->client.suppMacAddr.addr[1],
                 ( uchar8)logicalPortInfo->client.suppMacAddr.addr[2],
                 ( uchar8)logicalPortInfo->client.suppMacAddr.addr[3],
                 ( uchar8)logicalPortInfo->client.suppMacAddr.addr[4],
                 ( uchar8)logicalPortInfo->client.suppMacAddr.addr[5]);
      SYSAPI_NET_MBUF_FREE(bufHandle);
         return  FAILURE;
       }
  }

  eapolPkt->packetBodyLength = eapPkt->length; /* already in network byte order */

  length =  ENET_HDR_SIZE +  ENET_ENCAPS_HDR_SIZE + sizeof( eapolPacket_t) +
    osapiNtohs(eapolPkt->packetBodyLength);
  SYSAPI_NET_MBUF_SET_DATALENGTH(bufHandle, length);

  /* send request to supplicant */
  rc = mabClientRequestAction(logicalPortInfo, bufHandle);

  return rc;
}

/**************************************************************************
 * @purpose   Build VP list and send Access Request to RADIUS client
 *
 * @param     lIntIfNum       @b{(input)} Logical interface number of port being authenticated
 * @param     *suppEapData  @b{(input)} EAP info received from supplicant
 *
 * @returns    SUCCESS
 * @returns    FAILURE
 *
 * @comments
 *
 * @end
 *************************************************************************/
RC_t mabRadiusAccessRequestSend(uint32 lIntIfNum,  uchar8 *suppEapData)
{
   authmgrEapPacket_t *eapPkt =  NULLPTR;
  RC_t rc;
  mabPortCfg_t *pCfg;
  mabLogicalPortInfo_t *logicalPortInfo;
  uint32 physPort;
  uint32 ifIndex;
  access_req_info_t *req;
  mab_radius_cmd_msg_t cmd_req;
   uchar8 localMac[6];
   uchar8 calledId[AUTHMGR_MAC_ADDR_STR_LEN+1];
   uchar8 callingId[AUTHMGR_MAC_ADDR_STR_LEN+1];
   uchar8 nasPortId[ NIM_IF_ALIAS_SIZE + 1];
   uchar8 md5ChkSum[MAB_MD5_LEN+1], responseData[ PASSWORD_SIZE + MAB_CHALLENGE_LEN + 2];
  uint32 responseDataLen;
   uchar8 password[ PASSWORD_SIZE];
  uint32 challengelen;
   uchar8 chapPassword[MAB_MD5_LEN+1];

  if (suppEapData !=  NULL)
    eapPkt = ( authmgrEapPacket_t *)suppEapData;

  logicalPortInfo = mabLogicalPortInfoGet(lIntIfNum);
  if (logicalPortInfo ==  NULLPTR)
  {
    return  FAILURE;
  }

  MAB_PORT_GET(physPort, lIntIfNum);


  MAB_EVENT_TRACE(
      "%s:Recieved Radius send Access Request message for logical - %d \n",
      __FUNCTION__,lIntIfNum);

  if (mabIntfIsConfigurable(physPort, &pCfg) !=  TRUE)
    return  FAILURE;

  if (nimGetIntfIfIndex(physPort, &ifIndex) !=  SUCCESS)
    return  FAILURE;

  if ( AUTHMGR_PORT_MAB_AUTH_TYPE_CHAP == logicalPortInfo->client.mabAuthType)
  {
    memset(logicalPortInfo->client.mabChallenge,0, MAB_CHALLENGE_LEN);
    mabLocalAuthChallengeGenerate(logicalPortInfo->client.mabChallenge,  MAB_CHAP_CHALLENGE_LEN);
    logicalPortInfo->client.mabChallengelen =  MAB_CHAP_CHALLENGE_LEN;

    /* generate CHAP-Password using MD5 encryption */
    memset(password, 0, sizeof(password));
    osapiStrncpySafe(password, logicalPortInfo->client.mabUserName, strlen(logicalPortInfo->client.mabUserName)+ 1);

    challengelen= logicalPortInfo->client.mabChallengelen;
    responseDataLen = 1 + strlen(( char8 *)password) + challengelen;

    memset(responseData, 0, sizeof(responseData));
    responseData[0] = logicalPortInfo->client.currentIdL;

    memcpy (&responseData[1], password, strlen(( char8 *)password));
    memcpy (&responseData[1+strlen(( char8 *)password)], logicalPortInfo->client.mabChallenge, challengelen);

    mabLocalMd5Calc(responseData, responseDataLen, md5ChkSum);

    chapPassword[0] = logicalPortInfo->client.currentIdL;
    memcpy(&chapPassword[1], md5ChkSum, MAB_MD5_LEN);
  }

  req = (access_req_info_t *)malloc(sizeof(access_req_info_t)); 
  memset(req, 0, sizeof(req));

  /* pack the reqired info to sent the access-req */
  req->user_name = logicalPortInfo->client.mabUserName;
  req->user_name_len = strlen(logicalPortInfo->client.mabUserName);
  req->chap_password = chapPassword;
  req->chap_password_len = MAB_MD5_LEN+1;
  req->challenge = logicalPortInfo->client.mabChallenge;
  req->challenge_len = logicalPortInfo->client.mabChallengelen;
  req->mab_auth_type = logicalPortInfo->client.mabAuthType;
  req->supp_eap_data = suppEapData;

  /* Called-Station-Id */
  bzero(localMac,  MAC_ADDR_LEN);

  if (nimGetIntfAddress(physPort,  NULL, localMac) ==  SUCCESS)
  {
    bzero(calledId, AUTHMGR_MAC_ADDR_STR_LEN+1);
    osapiSnprintf(( char8 *)calledId, sizeof(calledId), "%02X-%02X-%02X-%02X-%02X-%02X",
                  localMac[0], localMac[1], localMac[2], localMac[3], localMac[4], localMac[5]);
   req->calledId = calledId;
   req->calledId_len = strlen(calledId);
  }

  /* Calling-Station-Id */
  bzero(callingId, AUTHMGR_MAC_ADDR_STR_LEN+1);
  osapiSnprintf(( char8 *)callingId, sizeof(callingId), "%02X-%02X-%02X-%02X-%02X-%02X",
      logicalPortInfo->client.suppMacAddr.addr[0],
      logicalPortInfo->client.suppMacAddr.addr[1],
      logicalPortInfo->client.suppMacAddr.addr[2],
      logicalPortInfo->client.suppMacAddr.addr[3],
      logicalPortInfo->client.suppMacAddr.addr[4],
      logicalPortInfo->client.suppMacAddr.addr[5]);

  req->callingId = callingId;
  req->callingId_len = strlen(callingId);

  /* nas-port */
  req->nas_port = ifIndex;


/* nas-port-id */
  bzero(nasPortId, ( NIM_IF_ALIAS_SIZE + 1));
  nimGetIntfName(physPort,  ALIASNAME, nasPortId);
  req->nas_portid = nasPortId;

  if ((AF_INET == mabBlock->nas_ip.af) || ((AF_INET6 == mabBlock->nas_ip.af)))
  {
    req->nas_ip.family = mabBlock->nas_ip.af;
    
    if (AF_INET == mabBlock->nas_ip.af)
    {
      req->nas_ip.addr.ipv4.s_addr = mabBlock->nas_ip.u.v4.s_addr;
    }
    else
    {
      memcpy(&req->nas_ip.addr.ipv6.in6, &mabBlock->nas_ip.u.v6, 16);
    }
  }

  if (strlen(mabBlock->nas_id))
  {
    memcpy(req->nas_id, mabBlock->nas_id, min(sizeof(req->nas_id), sizeof(mabBlock->nas_id)));
  }

req->attrInfo = &logicalPortInfo->client.attrInfo;

memcpy (req->supp_mac, logicalPortInfo->client.suppMacAddr.addr, sizeof(logicalPortInfo->client.suppMacAddr));

req->cxt = mabBlock->rad_cxt;
req->correlator = lIntIfNum;

 rc =  SUCCESS;
 if (0 != radiusAccessRequestSend(req))
 {
  MAB_EVENT_TRACE(
      "%s: radiusAccessRequestSend - failed \n",
      __FUNCTION__);
  rc =  FAILURE;
 }


  if ( SUCCESS != rc) 
  {
     LOGF( LOG_SEVERITY_NOTICE,
        "Failed to send access-req to RADIUS Server.");
     free(req);
    return rc;
  }

      memset(&cmd_req, 0, sizeof(cmd_req));

      strncpy(cmd_req.cmd, "access-req", strlen("access-req")+1);
      cmd_req.data = mabBlock->rad_cxt;
      cmd_req.cmd_data.access_req.req_attr = (void *)req;
      cmd_req.cmd_data.access_req.msg = req->msg_req;
      radius_mab_cmd_req_send(mabBlock->send_fd,(char *)&cmd_req, sizeof(cmd_req)); 
  return rc;
}


/**************************************************************************
 * @purpose   After client disconnected send clear RADIUS messages Request
 *            to RADIUS client
 *
 * @param     suppMacAddr  @b{(input)} Supplicant MAC address
 *
 * @comments
 *
 * @end
 *************************************************************************/
void mabRadiusClearRadiusMsgsSend( enetMacAddr_t suppMacAddr)
{
  mab_radius_cmd_msg_t cmd_req;

  memset(&cmd_req, 0, sizeof(cmd_req));

  strncpy(cmd_req.cmd, "clear-radius-msgs", strlen("clear-radius-msgs")+1);
  cmd_req.data = mabBlock->rad_cxt;
  memcpy(&cmd_req.cmd_data.mab_cli_mac_addr, &suppMacAddr.addr,  ENET_MAC_ADDR_LEN);
  radius_mab_cmd_req_send(mabBlock->send_fd,(char *)&cmd_req, sizeof(cmd_req));
}


/**************************************************************************
 * @purpose   Process EAP Response and Response/Identity frames
 *
 * @param     lIntIfNum   @b{(input)} Logical internal interface number
 * @param     bufHandle  @b{(input)} the handle to the mab PDU
 *
 * @returns    SUCCESS
 * @returns    FAILURE
 *
 * @comments
 *
 * @end
 *************************************************************************/
RC_t mabRadiusSuppResponseProcess(uint32 lIntIfNum,  netBufHandle bufHandle)
{
   uchar8 *data;
   enetHeader_t *enetHdr;
   eapolPacket_t *eapolPkt;
   authmgrEapPacket_t *eapPkt;
   eapRrPacket_t *eapRrPkt =  NULLPTR;
  mabLogicalPortInfo_t *logicalPortInfo;
  uint32 physPort = 0, lPort = 0, type = 0; /*, monitorMode */

  logicalPortInfo = mabLogicalPortInfoGet(lIntIfNum);
  if ((logicalPortInfo ==  NULLPTR) ||
      ( NULL == bufHandle))
  {
    return  FAILURE;
  }
  MAB_LPORT_KEY_UNPACK(physPort, lPort, type, lIntIfNum);

  MAB_EVENT_TRACE("%s: called for port -%d\n",
      __FUNCTION__,lIntIfNum);

  if(bufHandle !=  NULL)
  {
    SYSAPI_NET_MBUF_GET_DATASTART(bufHandle, data);
    enetHdr = ( enetHeader_t *)data;
    eapolPkt = ( eapolPacket_t *)(( uchar8 *)enetHdr +  ENET_HDR_SIZE +  ENET_ENCAPS_HDR_SIZE);
    eapPkt = ( authmgrEapPacket_t *)(( uchar8 *)eapolPkt + sizeof( eapolPacket_t));
    eapRrPkt = ( eapRrPacket_t *)(( uchar8 *)eapPkt + sizeof( authmgrEapPacket_t));
  }

  /* If this is a an EAP/Response-Identity, we need to see if the user has access to this port. */
  if ((eapRrPkt !=  NULLPTR)&&(eapRrPkt->type == EAP_RRIDENTITY))
  {
    /* User name was stored in port info structure when EAP-Response/Identity frame was received.
     * See if this user name matches a locally configured user.  If the user is locally configured
     * but does not have access to this port, generate an error.
     */
#if 0
    if (userMgrLoginIndexGet(( char8 *)logicalPortInfo->client.mabUserName,
          &logicalPortInfo->client.mabUserIndex) ==  SUCCESS)
    {
      /* Make sure user has access to this port */
      if ( (userMgrPortUserAccessGet(physPort, logicalPortInfo->client.mabUserName, &portAllow) ==  SUCCESS) &&
          (portAllow ==  FALSE) )
      {
        logicalPortInfo->protocol.authFail =  TRUE;
        return mabUnAuthenticatedAction(logicalPortInfo);
      }
    }
#endif
  }

  /* Send the Supplicant response to the server */
  return mabSendRespToServer(lIntIfNum, bufHandle);
}

/*********************************************************************
 *
 * @purpose Convert the given vlan string based on the id or name
 *
 * @param    vlanId    @b{(output)} Vlan Id assigned.
 * @param    vlanName  @b{(input)} Vlan string.
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *
 *********************************************************************/
RC_t mabRadiusServerVlanConversionHandle(const  char8 *vlanName, uint32 *vlanId)
{

  /*Check that any vlan has same name with given string*/
  if(/*dot1qVlanIdFromNameGet(vlanName, vlanId) == */ SUCCESS)
  {
    return  SUCCESS;
  } /*check that given string is pure numeric string */
  else if (/*usmDbStringNumericCheck(vlanName) == */ SUCCESS)
  {
    *vlanId = atoi(vlanName);
    return  SUCCESS;
  } 
  else
  { 
    /*Then given string is invalid string */
    *vlanId = 0;
    return  FAILURE;
  }
}

RC_t mabAccessLevelAttrValidate(mabLogicalPortInfo_t *logicalPortInfo)
{
  MAB_IF_NULLPTR_RETURN_LOG(logicalPortInfo);

  /* if the service type from radius server is not 
   **supported by DUT consider the response
   ** as Access-Reject */
  if ((RADIUS_SERVICE_TYPE_ADMIN != logicalPortInfo->client.attrInfo.accessLevel) &&
      (RADIUS_SERVICE_TYPE_LOGIN != logicalPortInfo->client.attrInfo.accessLevel) &&
      (RADIUS_SERVICE_TYPE_CALL_CHECK != logicalPortInfo->client.attrInfo.accessLevel))
  {
     LOGF( LOG_SEVERITY_WARNING,
        "Unable to authenticate as an unsupported service-type value (%d) "
        "received in the radius server access-accept message.Sending EAP "
        "failure to the client %02X:%02X:%02X:%02X:%02X:%02X.Supported "
        "values are 'Login' and 'Admin'.Recommend changing the radius "
        "server settings with supported service-type.",
        logicalPortInfo->client.attrInfo.accessLevel, ( uchar8)logicalPortInfo->client.suppMacAddr.addr[0],
        ( uchar8)logicalPortInfo->client.suppMacAddr.addr[1],
        ( uchar8)logicalPortInfo->client.suppMacAddr.addr[2],
        ( uchar8)logicalPortInfo->client.suppMacAddr.addr[3],
        ( uchar8)logicalPortInfo->client.suppMacAddr.addr[4],
        ( uchar8)logicalPortInfo->client.suppMacAddr.addr[5]); 
    return  FAILURE;
  }
  return  SUCCESS;
}

RC_t mabVlanAttrValidate(mabLogicalPortInfo_t *logicalPortInfo)
{
  uint32 vlanId = 0;

  MAB_IF_NULLPTR_RETURN_LOG(logicalPortInfo);

  /* process the string received and validate the 
     result for presense of VLAN */
  if ( SUCCESS != mabRadiusServerVlanConversionHandle
      (logicalPortInfo->client.attrInfo.vlanString, &vlanId))
  {
    return  FAILURE;
  }

  logicalPortInfo->client.attrInfo.vlanId = vlanId;

  return  SUCCESS;
}

RC_t mabRadiusAcceptPostProcess(mabLogicalPortInfo_t *logicalPortInfo)
{
  uint32 physPort = 0;
   mabPortCfg_t *pCfg =  NULLPTR;

  MAB_IF_NULLPTR_RETURN_LOG(logicalPortInfo);

  MAB_PORT_GET(physPort, logicalPortInfo->key.keyNum);

  /* check access level attribute */
  if ((AUTHMGR_RADIUS_ATTR_TYPE_SERVICE_TYPE & logicalPortInfo->client.attrInfo.attrFlags) &&
     ( SUCCESS != mabAccessLevelAttrValidate(logicalPortInfo)))
  {
    logicalPortInfo->protocol.authFail =  TRUE;
    mabUnAuthenticatedAction(logicalPortInfo);
    return  FAILURE;
  }

   /* check if EAP message is receied.
      If the client is mab aware the eap attribute 
      need to be present in the accept message */

  if (!(AUTHMGR_RADIUS_ATTR_TYPE_EAP_MESSAGE & logicalPortInfo->client.attrInfo.attrFlags))
  {
    if (( TRUE != mabIntfIsConfigurable(physPort, &pCfg)) ||
        ( NULLPTR == pCfg))
    {
      return  FAILURE;
    }

    /* check if the method is MD5 */
    if ( AUTHMGR_PORT_MAB_AUTH_TYPE_EAP_MD5 == pCfg->mabAuthType)
    {
      logicalPortInfo->protocol.authFail =  TRUE;
      mabUnAuthenticatedAction(logicalPortInfo);
      return  FAILURE;
    }
  }

  return  SUCCESS;
}
/**************************************************************************
 * @purpose   Process RADIUS Accept from server
 *
 * @param     lIntIfNum       @b{(input)} Logical internal interface number of port being authenticated
 * @param     *radiusPayload  @b{(input)} payload of RADIUS frame (attributes)
 * @param     payloadLen      @b{(input)} length of RADIUS payload
 *
 * @returns    SUCCESS
 * @returns    FAILURE
 *
 * @comments
 *
 * @end
 *************************************************************************/
RC_t mabRadiusAcceptProcess(uint32 lIntIfNum, void *resp)
{
  uint32 rc =  SUCCESS;
  mabPortCfg_t *pCfg;
  uint32 physPort = 0, lPort = 0, type = 0;
   uchar8 ifName[ NIM_IF_ALIAS_SIZE + 1];
  mabLogicalPortInfo_t *logicalPortInfo =  NULLPTR;

  logicalPortInfo = mabLogicalPortInfoGet(lIntIfNum);
  MAB_IF_NULLPTR_RETURN_LOG (logicalPortInfo);

  MAB_LPORT_KEY_UNPACK(physPort, lPort, type, lIntIfNum);

  if (mabIntfIsConfigurable(physPort, &pCfg) !=  TRUE)
    return  FAILURE;

  nimGetIntfName(physPort,  ALIASNAME, ifName);
  MAB_EVENT_TRACE(
      "%s:Received Radius Accept message for port - %s\n",
      __FUNCTION__,ifName);

  /* response from radius server received.
     stop the serverwhile timer */
  mabTimerDestroy(mabBlock->mabTimerCB, logicalPortInfo);
  
  if ( NULLPTR != resp)
  {
    if (0 != radiusClientAcceptProcess(resp, &logicalPortInfo->client.attrInfo))
    {
      printf("radiusClientAcceptProcess failed \n");
      rc =  FAILURE;
    }

    if ( SUCCESS != rc)
    {
       LOGF( LOG_SEVERITY_INFO, 
              "Could not parse RADIUS attributes.");
      return  FAILURE;
    }
  } /* radiusPayload check  ends here */

  rc = mabRadiusAcceptPostProcess(logicalPortInfo);
  if ( SUCCESS == rc)
  {
    logicalPortInfo->client.authMethod =  AUTH_METHOD_RADIUS;
    mabAuthenticatedAction(logicalPortInfo);
  }

  return rc;
}


/**************************************************************************
* @purpose   Send Response frame to Server
*
* @param     lIntIfNum   @b{(input)} Logical internal interface number
* @param     bufHandle  @b{(input)} the handle to the mab PDU
*
* @returns    SUCCESS
* @returns    FAILURE
*
* @comments
*
* @end
*************************************************************************/
RC_t mabSendRespToServer(uint32 lIntIfNum,  netBufHandle bufHandle)
{
   uchar8 *data;
   authmgrEapPacket_t *eapPkt =  NULLPTR;
  mabLogicalPortInfo_t *logicalPortInfo;

  if ( NULL == bufHandle)
  {
    return  FAILURE;
  }

  MAB_EVENT_TRACE(
                    "%s: called for port -%d\n",__FUNCTION__,lIntIfNum);

  if (bufHandle !=  NULL)
  {
    SYSAPI_NET_MBUF_GET_DATASTART(bufHandle, data);

    eapPkt = ( authmgrEapPacket_t *)(data +  ENET_HDR_SIZE +  ENET_ENCAPS_HDR_SIZE + sizeof( eapolPacket_t));
  }


  if (mabRadiusAccessRequestSend(lIntIfNum, ( uchar8 *)eapPkt) !=  SUCCESS)
  {
     LOGF( LOG_SEVERITY_NOTICE,
        "mabSendRespToServer: mabRadiusAccessRequestSend failed\n"
        " Failed sending message to RADIUS server");
    MAB_EVENT_TRACE(
        "%s: mabRadiusAccessRequestSend failed for port -%d\n",__FUNCTION__,lIntIfNum);

    logicalPortInfo = mabLogicalPortInfoGet(lIntIfNum);
    if (logicalPortInfo !=  NULLPTR)
    {
      logicalPortInfo->protocol.authFail =  TRUE;
      mabUnAuthenticatedAction(logicalPortInfo);
    }
  }

  return  SUCCESS;
}

/**************************************************************************
 * @purpose   Take MAB Radius server lock
 *
 * @param     none
 *
 * @returns    SUCCESS
 * @returns    FAILURE
 *
 * @comments
 *
 * @end
 *************************************************************************/
RC_t mabRadiusServerTaskLockTake(void)
{
  return osapiSemaTake(mabBlock->mabRadiusSrvrTaskSyncSema,  WAIT_FOREVER);
}

/**************************************************************************
 * @purpose   Give MAB Radius server lock
 *
 * @param     none
 *
 * @returns    SUCCESS
 * @returns    FAILURE
 *
 * @comments
 *
 * @end
 *************************************************************************/
RC_t mabRadiusServerTaskLockGive(void)
{
  return osapiSemaGive(mabBlock->mabRadiusSrvrTaskSyncSema);
}
