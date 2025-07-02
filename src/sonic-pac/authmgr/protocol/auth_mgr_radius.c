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
#include "auth_mgr_client.h"
#include "auth_mgr_timer.h"

#include "auth_mgr_struct.h"
#include "auth_mgr_vlan_db.h"

extern authmgrCB_t *authmgrCB;

/**************************************************************************
 * @purpose   Handle RADIUS client callbacks
 *
 * @param     status         @b{(input)} status of RADIUS response (accept,
 *                                       reject, challenge, etc)
 * @param     correlator     @b{(input)} correlates responses to requests; for
 *                                       authmgr, this is ifIndex
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
RC_t authmgrRadiusResponseCallback (uint32 status, uint32 correlator,
                                        uchar8 * attributes,
                                       uint32 attributesLen)
{
  uint32 lIntIfNum;
  authmgrAaaMsg_t authmgrAaaMsg;

  lIntIfNum = correlator;

  /* Fill in AAA message structure */
  memset (&authmgrAaaMsg, 0, sizeof (authmgrAaaMsg_t));
  authmgrAaaMsg.status = status;
  authmgrAaaMsg.respLen = attributesLen;

  /* Allocate memory for attributes which gets freed by authmgrTask */
  if ( NULLPTR != attributes)
  {
    authmgrAaaMsg.pResponse =
      osapiMalloc ( AUTHMGR_COMPONENT_ID, attributesLen);
    memcpy (authmgrAaaMsg.pResponse, attributes, attributesLen);
  }
  else
  {
    authmgrAaaMsg.respLen =  0;
    authmgrAaaMsg.pResponse =  NULLPTR;
  }
  return authmgrIssueCmd (authmgrAaaInfoReceived, lIntIfNum, &authmgrAaaMsg);
}

/**************************************************************************
 * @purpose   Process RADIUS Server responses
 *
 * @param     lIntIfNum       @b{(input)} Logical internal interface number of
 *                                        port being authenticated
 * @param     status         @b{(input)} status of RADIUS response (accept,
 *                                       reject, challenge, etc)
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
RC_t authmgrRadiusResponseProcess (uint32 lIntIfNum, uint32 status,
                                       uchar8 * attributes,
                                      uint32 attributesLen)
{
  RC_t rc =  FAILURE;
  authmgrPortCfg_t *pCfg;
  authmgrLogicalPortInfo_t *logicalPortInfo;
  uint32 physPort = 0, lPort = 0, type = 0;

  if (authmgrCB->globalInfo->authmgrCfg->adminMode !=  ENABLE)
  {
    return  SUCCESS;
  }
  logicalPortInfo = authmgrLogicalPortInfoGet (lIntIfNum);
  if (logicalPortInfo ==  NULLPTR)
  {
    return  FAILURE;
  }

  AUTHMGR_LPORT_KEY_UNPACK (physPort, lPort, type, lIntIfNum);

  if (authmgrIntfIsConfigurable (physPort, &pCfg) !=  TRUE)
  {
    return  FAILURE;
  }

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_RADIUS, physPort,
                       "%s:Received Radius response message on logicalPort:[%s] with status[%d]\n\r",
                       __FUNCTION__, authmgrIntfIfNameGet(physPort), status);

  /* Ensure we are expecting a response from the server */
  if (logicalPortInfo->protocol.authState == AUTHMGR_AUTHENTICATING)
  {
     return FAILURE;
  }

  /* Need to free the buffer that was passed to us */
  if (attributes !=  NULLPTR)
  {
    osapiFree ( AUTHMGR_COMPONENT_ID, attributes);
  }

  return rc;
}


/*********************************************************************
*
* @purpose Convert the given vlan string based on the id or name
*
* @param    vlanName  @b{(input)} Vlan string.
* @param    vlanId    @b{(output)} Vlan Id assigned.
*
* @returns  SUCCESS
* @returns  FAILURE
*
* @comments
*
* @end
*
*********************************************************************/
RC_t authmgrRadiusServerVlanConversionHandle (const char8 * vlanName,
                                                 uint32 * vlanId)
{
  *vlanId = atoi (vlanName);
  return SUCCESS;
}

/*********************************************************************
* @purpose function to parse the vlan attribute
*
* @param logicalPortInfo -- logical port structure
* @param processInfo -- client info structure
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrVlanAttrValidate (authmgrLogicalPortInfo_t * logicalPortInfo,
                                 authmgrClientInfo_t *processInfo)
{
  uint32 vlanId = 0;
  uint32 physPort;
  RC_t rc1 =  SUCCESS;

  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);
  AUTHMGR_IF_NULLPTR_RETURN_LOG (processInfo);

  AUTHMGR_PORT_GET (physPort, logicalPortInfo->key.keyNum);

  /* process the string received and validate the
     result for presense of VLAN */
  if ( SUCCESS != authmgrRadiusServerVlanConversionHandle
      (authmgrCB->attrInfo.vlanString, &vlanId))
  {
    return  FAILURE;
  }

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, 0,
                       "Validating VLAN %d for PAC client", vlanId);

  rc1 = authmgrVlanCheckStatic(vlanId);

  /* If given vlan is NOT  there */
  if ( NOT_EXIST == rc1)
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, 0,
                         "VLAN %d does not exist in the system", vlanId);
    /* In case of radius assigned trunk mode, 
       dynamic vlan creation is not allowed */
   return FAILURE;    
  }

  if (SUCCESS != authmgrVlanPortParticipationValidate (physPort, vlanId))
  {
    return FAILURE;
  }
  processInfo->vlanId = vlanId;

  return  SUCCESS;
}


/*********************************************************************
* @purpose function to perform the actions after parsing the attributes.
*
* @param logicalPortInfo -- logical port structure
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrRadiusAcceptPostProcess (authmgrLogicalPortInfo_t *
                                        logicalPortInfo, 
                                        authmgrClientInfo_t *processInfo,
                                         AUTHMGR_ATTR_PROCESS_t attrProcess)
{
  uint32 physPort = 0;
 RC_t rc =  SUCCESS;

  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);
  AUTHMGR_IF_NULLPTR_RETURN_LOG (processInfo);

  AUTHMGR_PORT_GET (physPort, logicalPortInfo->key.keyNum);

  /* check vlan attribute */
  if (0 ==  authmgrCB->attrInfo.vlanAttrFlags)
  {
    /* either Vlan attributes aren't received OR
       vlan assignment is not enabled. */

    rc = authmgrPortDefaultVlanGet(physPort, &processInfo->vlanId);
    if ( SUCCESS != rc)
    {
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_RADIUS, physPort,
          "%s:%d:Unable to get the configured default VLAN on port %s", 
          __FUNCTION__, __LINE__, authmgrIntfIfNameGet(physPort));
    }

    if ( SUCCESS != rc)
    {
      logicalPortInfo->protocol.authSuccess =  FALSE;
      logicalPortInfo->protocol.authFail =  TRUE;
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_RADIUS, physPort,
          "%s:%d:Unable to process VLAN attribute", __FUNCTION__, __LINE__);
      rc =  FAILURE;
    }
  }
  else
  {
    if (RADIUS_REQUIRED_TUNNEL_ATTRIBUTES_SPECIFIED != authmgrCB->attrInfo.vlanAttrFlags)
    {
      logicalPortInfo->protocol.authSuccess =  FALSE;
      logicalPortInfo->protocol.authFail =  TRUE;

      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_RADIUS, physPort,
          "%s:%d:Unable to process VLAN attribute", __FUNCTION__, __LINE__);
      rc =  FAILURE;
    }
    else
    {
      rc = authmgrVlanAttrValidate(logicalPortInfo, processInfo);
    }
  }
  return rc;
}
 
