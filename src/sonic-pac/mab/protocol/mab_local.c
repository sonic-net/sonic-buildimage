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

#include <time.h>
#include "mab_include.h"
#include "md5_api.h"
#include "mab_auth.h"
#include "mab_client.h"
#include "mab_control.h"

RC_t mabLocalAuthSendAuthReq(mabLogicalPortInfo_t *logicalPortInfo)
{
  MAB_IF_NULLPTR_RETURN_LOG(logicalPortInfo);
  /* Generate an event that will cause a request (MD5 challenge) to be sent to the supplicant.
   * The bufHandle is  NULLPTR in this case since we didn't actually receive the request
   * from a server.  So, we will need to generate the request locally.
   */
  if (AUTHMGR_CLIENT_UNAWARE == logicalPortInfo->client.clientType)
  {
    /* if MAB client generate and store mab challenge*/
    memset(logicalPortInfo->client.mabChallenge,0,MAB_CHALLENGE_LEN);
    mabLocalAuthChallengeGenerate(logicalPortInfo->client.mabChallenge, MAB_CHALLENGE_LEN);
    logicalPortInfo->client.mabChallengelen = MAB_CHALLENGE_LEN;
  
  }
  return mabClientRequestAction(logicalPortInfo,  NULLPTR);
}

/**************************************************************************
* @purpose   Process EAP Response and Response/Identity frames
*
* @param     logicalPortInfo  @b{(input))  Logical Port Info node
* @param     bufHandle  @b{(input)} the handle to the mab PDU
*
* @returns    SUCCESS
* @returns    FAILURE
*
* @comments
*
* @end
*************************************************************************/
RC_t mabLocalAuthResponseProcess(mabLogicalPortInfo_t *logicalPortInfo,  netBufHandle bufHandle)
{
   uchar8 *data;
   enetHeader_t *enetHdr;
   eapolPacket_t *eapolPkt;
   authmgrEapPacket_t *eapPkt;
   eapRrPacket_t *eapRrPkt;
   BOOL portAllow;
  RC_t rc =  SUCCESS, authRc;
  mabPortCfg_t * pCfg;
  uint32 physPort = 0, type = 0, lPort = 0;
  authmgrFailureReason_t  reason=0;
  
  if ( NULLPTR == logicalPortInfo)
  {
    return  FAILURE;
  }

  MAB_LPORT_KEY_UNPACK(physPort, lPort, type, logicalPortInfo->key.keyNum);

  if ( TRUE != mabIntfIsConfigurable(physPort, &pCfg))
  {
     return  FAILURE;
  }

  enetHdr = ( enetHeader_t *)data;
  eapolPkt = ( eapolPacket_t *)(( uchar8 *)enetHdr +  ENET_HDR_SIZE +  ENET_ENCAPS_HDR_SIZE);
  eapPkt = ( authmgrEapPacket_t *)(( uchar8 *)eapolPkt + sizeof( eapolPacket_t));
  eapRrPkt = ( eapRrPacket_t *)(( uchar8 *)eapPkt + sizeof( authmgrEapPacket_t));

  logicalPortInfo->client.attrInfo.idFromServer = logicalPortInfo->client.currentIdL;

  if (eapRrPkt->type == EAP_RRIDENTITY)
  {
    logicalPortInfo->client.mabUserIndex = MAB_USER_INDEX_INVALID;
    if ( AUTH_METHOD_LOCAL == logicalPortInfo->client.authMethod)
    {

     /* User name was stored in port info structure when EAP-Response/Identity frame was received.
      * See if this user name matches a locally configured user.  If we find a match, generate an
      * event that will cause an MD5 challenge to be sent to the supplicant.  If we don't find a
      * match, generate an authentication failure event.
      */
      if (/*userMgrLoginIndexGet(( char8 *)logicalPortInfo->client.mabUserName,
                             &logicalPortInfo->client.mabUserIndex) != */ SUCCESS)
      {
        logicalPortInfo->client.mabUserIndex = MAB_USER_INDEX_INVALID;
        reason =  AUTHMGR_FAIL_REASON_INVALID_USER;
      }
    }

    if (logicalPortInfo->client.mabUserIndex != MAB_USER_INDEX_INVALID)
    {
      /* Make sure user has access to this port */
      if (/*userMgrPortUserAccessGet(physPort, logicalPortInfo->client.mabUserName, &portAllow) == */ SUCCESS) 
      {
        if(portAllow ==  TRUE) 
        {
          rc = mabLocalAuthSendAuthReq(logicalPortInfo);
        }
      }
      else
      {
        logicalPortInfo->client.mabUserIndex = MAB_USER_INDEX_INVALID;
        reason =  AUTHMGR_FAIL_REASON_INVALID_USER;
      } 
    }
  }
  else if (eapRrPkt->type == EAP_RRMD5)
  {
     uchar8 *response = (( uchar8 *)eapRrPkt) + sizeof( eapRrPacket_t);
    authRc = mabLocalAuthMd5ResponseValidate(logicalPortInfo, response);
    if (authRc ==  SUCCESS)
    {
      /* Log the information */
       LOGF( LOG_SEVERITY_NOTICE,
          "Local MAB Authenticated Successfully.");

      mabAuthenticatedAction(logicalPortInfo);

    }
    else if(authRc ==  NOT_EXIST)
    {
      reason =  AUTHMGR_FAIL_REASON_AUTH_FAILED;
    }
    else 
    {
      /* FAILED Unauthenticated vlan needs to be assigned */
      reason =  AUTHMGR_FAIL_REASON_AUTH_FAILED;
    }
  }

  if(reason == AUTHMGR_FAIL_REASON_INVALID_USER)
  {
    /* Send the challenge to accomodate clients that does not
       accept eap-success/failure for initial eap-identity frames */
    rc = mabLocalAuthSendAuthReq(logicalPortInfo);
  }

  return rc;
}

/**************************************************************************
* @purpose   Validate an MD5 challenge response
*
* @param     logicalPortInfo  @b{(input))  Logical Port Info node
* @param     *response  @b{(input)} pointer to MD5 response data
*
* @returns    SUCCESS if authenticated
* @returns    FAILURE if not authenticated
*
* @comments
*
* @end
*************************************************************************/
RC_t mabLocalAuthMd5ResponseValidate(mabLogicalPortInfo_t *logicalPortInfo,  uchar8 *response)
{
  uint32 suppResponseLen, responseDataLen, challengeLen;
   uchar8 suppAnswer[MAB_MD5_LEN], answer[MAB_MD5_LEN];
   uchar8 responseData[ PASSWORD_SIZE+MAB_CHALLENGE_LEN+1];
   uchar8 userPw[ PASSWORD_SIZE];
  RC_t rc =  FAILURE;

  /* Length is first byte in response field */
  suppResponseLen = (uint32)(*response);
  if (suppResponseLen != MAB_MD5_LEN)
  {
    return  FAILURE;
  }

  if (logicalPortInfo->client.mabUserIndex == MAB_USER_INDEX_INVALID)
  {  
    return  NOT_EXIST;
  }

  memcpy(suppAnswer, &response[1], suppResponseLen);

  memset(userPw, 0x0,  PASSWORD_SIZE);

  challengeLen = logicalPortInfo->client.mabChallengelen;
  responseDataLen = 1 + strlen(( char8 *)userPw) + challengeLen;

  memset(responseData,0,sizeof(responseData));
  responseData[0] = logicalPortInfo->client.currentIdL;
  memcpy(&responseData[1], userPw, strlen(( char8 *)userPw));
  memcpy(&responseData[1+strlen(( char8 *)userPw)], logicalPortInfo->client.mabChallenge, challengeLen);

  mabLocalMd5Calc(responseData, responseDataLen, answer);

  if (memcmp(answer, suppAnswer, MAB_MD5_LEN) == 0)
    rc =  SUCCESS;
  else
    rc =  FAILURE;

  return rc;
}

/**************************************************************************
* @purpose   Generate an random challenge
*
* @param     *challenge     @b{(output)} pointer to buffer to hold challenge
* @param     challengeLen   @b{(input)} length of challenge buffer
*
* @returns   void
*
* @comments
*
* @end
*************************************************************************/
void mabLocalAuthChallengeGenerate( uchar8 *challenge, uint32 challengeLen)
{
  uint32 i, randno, cpLen;
   uchar8 *vector = &challenge[0];

  bzero(( uchar8 *)vector, challengeLen);

	srand((uint32)time(0));
	for (i = 0; i < challengeLen;)
	{
    cpLen = (uint32)sizeof(uint32);

		randno = (uint32)rand();
		memcpy (( uchar8 *) vector, ( uchar8 *) &randno, cpLen);
		vector += cpLen;
		i += cpLen;
	}

  return;
}

/**************************************************************************
* @purpose   Calculate MD5 for given input buffer and length
*
* @param     *inBuf   @b{(input)} pointer to input buffer run MD5 against
* @param     inLen    @b{(input)} length of input buffer
* @param     *outBuf  @b{(output)} pointer to output buffer to hold MD5 result
*
* @returns   void
*
* @comments  *outBuf must be MAB_MD5_LEN (16) bytes
*
* @end
*************************************************************************/
void mabLocalMd5Calc( uchar8 *inBuf, uint32 inLen,  uchar8 *outBuf)
{
   MD5_CTX_t context;

  bzero(outBuf, MAB_MD5_LEN);

  md5_init(&context);
  md5_update(&context, inBuf, inLen);
  md5_final(outBuf, &context);

  return;
}
